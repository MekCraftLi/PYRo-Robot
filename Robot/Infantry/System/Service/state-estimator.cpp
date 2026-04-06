/**
 *******************************************************************************
 * @file    state-estimator.cpp
 * @brief   简要描述
 *******************************************************************************
 * @attention
 *
 * none
 *
 *******************************************************************************
 * @note
 *
 * 进行数学解算的时候严格遵循右手系
 * 坐标轴正方向：
 * X->前
 * Y->左
 * Z->上
 * 旋转正方向
 * Roll - x - 向右
 * Pitch - y - 向下
 * Yaw - z - 向左
 *
 * 开发板BMI088获取的加速度，角速度正方向以及顺序：
 * x -> 向后
 * y -> 向右
 * z -> 向上
 * roll -> 向左
 * pitch -> 向上
 * yaw -> 向左
 *
 *******************************************************************************
 * @author  MekLi
 * @date    2026/2/5
 * @version 1.0
 *******************************************************************************
 */




/* ------- define ----------------------------------------------------------------------------------------------------*/

#define IMU_SPI_HANDLE     hspi2


#define ACCEL_CS_GPIO_Port GPIOC
#define ACCEL_CS_Pin       GPIO_PIN_0
#define GYRO_CS_GPIO_Port  GPIOC
#define GYRO_CS_Pin        GPIO_PIN_3


/* ------- include ---------------------------------------------------------------------------------------------------*/



/* I. header */

#include "state-estimator.h"


/* II. other application */



#include "System/DataHub/blackboard.h"
#include "System/DataHub/data-def.h"


/* III. standard lib */
#include "Algorithm/EKF/QuaternionEKF.h"
#include "Config/Gimbal/algo-config.h"
#include "pyro_dwt_drv.h"



/* ------- class prototypes-----------------------------------------------------------------------------------------*/




/* ------- macro -----------------------------------------------------------------------------------------------------*/





/* ------- variables -------------------------------------------------------------------------------------------------*/

static __attribute__((section(".dma_pool"))) uint8_t rxBuf[64];
static __attribute__((section(".dma_pool"))) uint8_t txBuf[64];


uint32_t exeTimeUs;

[[maybe_unused]] static auto& app = ImuApp::instance();




/* ------- application attribute -------------------------------------------------------------------------------------*/

#define APPLICATION_ENABLE     true

#define APPLICATION_NAME       "Imu"

#define APPLICATION_STACK_SIZE 1024

#define APPLICATION_PRIORITY   48

#define APPLICATION_PERIOD_MS  0

static StackType_t appStack[APPLICATION_STACK_SIZE];




/* ------- message interface attribute -------------------------------------------------------------------------------*/





/* ------- function prototypes ---------------------------------------------------------------------------------------*/





/* ------- function implement ----------------------------------------------------------------------------------------*/


ImuApp::ImuApp()
    : NotifyApp(APPLICATION_ENABLE, APPLICATION_NAME, APPLICATION_STACK_SIZE, appStack, APPLICATION_PRIORITY),
      _bmi088(&IMU_SPI_HANDLE, ACCEL_CS_GPIO_Port, GYRO_CS_GPIO_Port, ACCEL_CS_Pin, GYRO_CS_Pin, txBuf, rxBuf) {}


void ImuApp::init() {
    HAL_TIM_Base_Start(&htim23);
    HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_4);

    _bmi088.init(Bmi088AccRange::RANGE_3G, Bmi088AccODR::ODR_200_HZ, Bmi088AccWidth::OSR2,
                 Bmi088GyroRange::RANGE_500_DPS, Bmi088GyroWidth::ODR_1000HZ_BW_116HZ);

    IMU_QuaternionEKF_Init(10.0f, 0.001f, 10000000.0f, 0.9996f, 1 / (2 * M_PI));
}


uint32_t exeTime = 0;
void ImuApp::run() {
    static uint32_t dwtCnt;

    float dt = pyro::dwt_drv_t::get_delta_t(&dwtCnt);


    _bmi088.getImuData(data);

    // -------------------------------------------------------------
    // 【极其关键：坐标系对齐】
    // 在这里，你必须根据你的板子安装方向，手动凑出“右手直角坐标系” (前X, 左Y, 上Z)。
    // 比如，如果你的板子 Y 轴是向后的，你得传 -gy 和 -ay。
    // -------------------------------------------------------------
    float input_gx = -data.rate.x, input_gy = -data.rate.y, input_gz = data.rate.z;
    float input_ax = -data.accel.x, input_ay = -data.accel.y, input_az = data.accel.z;

// =========================================================================
    // 1. BMI088 恒温控制 (1Hz 极低频 PI + 死区控制 + 单向抗饱和)
    // =========================================================================
    static uint16_t temp_ctrl_divider = 0;
    // 【核心改进 1】：1KHz / 1000 = 1Hz。完美匹配 BMI088 1.28秒的硬件刷新率，彻底消除盲区积分！
    if (++temp_ctrl_divider >= 1000) {
        temp_ctrl_divider = 0;

        static float temp_integral = 0.0f;
        static float current_pwm_limit = 0.0f;
        float pwm_out = 0.0f;
        // 获取满量程和爬升步长
        float max_arr = (float)__HAL_TIM_GET_AUTORELOAD(&htim3);
        float absolute_max_pwm = max_arr * Config::Algorithm::Imu::MAX_HEATER_POWER_RATIO;
        float ramp_step = max_arr * 0.01f; // 限制爬升率 1% 每秒，更加轻柔


        if (data.temperature < Config::Algorithm::Imu::TEMP_MIN_SAFE ||
            data.temperature > Config::Algorithm::Imu::TEMP_MAX_SAFE) {
            pwm_out = 0.0f;
            temp_integral = 0.0f;
            current_pwm_limit = 0.0f;
        } else {
            if (current_pwm_limit < absolute_max_pwm) {
                current_pwm_limit += ramp_step;
            } else if (current_pwm_limit > absolute_max_pwm) {
                current_pwm_limit = absolute_max_pwm;
            }

            // 计算原始误差
            float raw_error = Config::Algorithm::Imu::TARGET_TEMPERATURE - data.temperature;
            float temp_error = 0.0f;

            // 【核心改进 2】：引入平移死区 (Deadband)
            // 如果误差在 ±0.15℃ 以内，强行将误差清零，防止 0.125℃ 的硬件跳变引起 P 项抖动
            if (std::abs(raw_error) > Config::Algorithm::Imu::TEMP_DEADBAND) {
                // 平移误差，保证切入/切出死区时输出是连续平滑的
                if (raw_error > 0) temp_error = raw_error - Config::Algorithm::Imu::TEMP_DEADBAND;
                else temp_error = raw_error + Config::Algorithm::Imu::TEMP_DEADBAND;
            } else {
                temp_error = 0.0f; // 处于 39.875 时，误差视作 0
            }

            // 【核心修改 2】：采用极弱的比例项和慢速积分项
            // Kp=15: 即使误差 1℃，也只敢给出 1.5% 的功率，绝不喧宾夺主
            const float Kp = 15.0f * (max_arr / 1000.0f);
            // Ki=0.5: 极慢的积分，像调点滴一样慢慢寻找稳态维持点
            const float Ki = 0.5f * (max_arr / 1000.0f);

            // 积分分离与“冻结挂起” (Suspend) 逻辑
            if (std::abs(raw_error) < 1.0f) {
                // 只有在距离目标 1℃ 范围内，才允许精细积分
                temp_integral += temp_error * 1.0f;
            } else {
                // 【关键修复】：如果被外界干扰吹离了 1℃，不准清零积分！
                // 而是将积分项“冻结”在当前值，保留系统对环境稳态散热功率的“记忆”
                // 仅靠此时较大的 P 项将温度拉回 1℃ 范围内
            }

            // 动态抗饱和，允许 I 项主导整个维持功率
            float max_i_out = current_pwm_limit;
            if (temp_integral > (max_i_out / Ki)) temp_integral = max_i_out / Ki;

            // 依然绝不允许 I 项为负，避免制冷逻辑干扰
            if (temp_integral < 0.0f) temp_integral = 0.0f;

            pwm_out = Kp * temp_error + Ki * temp_integral;

            if (pwm_out > current_pwm_limit) pwm_out = current_pwm_limit;
            if (pwm_out < 0.0f) pwm_out = 0.0f;
        }

        __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_4, (uint32_t)pwm_out);
    }
    // =========================================================================
    // 2. 宏控制的 60s 静态严苛校准程序 (带连续温度守护)
    // =========================================================================
#if IMU_CALIBRATION_MODE
    static uint32_t calib_count = 0;
    static float gyro_bias[3]  = {0};
    static float accel_bias[3] = {0};

    const float CALIB_TOTAL_SAMPLES = 60000.0f;
    const float INV_SAMPLES = 1.0f / CALIB_TOTAL_SAMPLES;

    if (calib_count < CALIB_TOTAL_SAMPLES) {
        // 计算当前温度与目标温度的偏差绝对值
        float temp_diff = std::abs(data.temperature - Config::Algorithm::Imu::TARGET_TEMPERATURE);

        // 【核心逻辑】：时刻判断温度是否在允许的容差范围内
        if (temp_diff <= Config::Algorithm::Imu::CALIB_TEMP_TOLERANCE) {
            // 温度达标，累加份额
            gyro_bias[0] += input_gx * INV_SAMPLES;
            gyro_bias[1] += input_gy * INV_SAMPLES;
            gyro_bias[2] += input_gz * INV_SAMPLES;

            accel_bias[0] += input_ax * INV_SAMPLES;
            accel_bias[1] += input_ay * INV_SAMPLES;
            accel_bias[2] += (input_az - 9.80665f) * INV_SAMPLES;

            calib_count++;

            // 可选：在这里控制一个 LED 快闪，表示“正在进行有效的数据收集”
        } else {
            // 【一票否决】：只要温度一跑偏，立刻销毁所有已收集的数据，从头再来！
            calib_count = 0;

            gyro_bias[0] = 0.0f; gyro_bias[1] = 0.0f; gyro_bias[2] = 0.0f;
            accel_bias[0] = 0.0f; accel_bias[1] = 0.0f; accel_bias[2] = 0.0f;

            // 可选：在这里控制 LED 慢闪或常亮，表示“温度不达标，正在等待升温”
        }

        // 强行返回，拦截底层的 EKF 解算和控制律
        return;
    } else {
        // 校准完美完成（证明温度连续 60 秒都锁死在 50℃±0.5℃ 且机身静止）
        while(1) {
            // 可选：在这里控制 LED 长亮或蜂鸣器长鸣，提示操作手可以连上 Debug 抄写数据了
            // 此时 gyro_bias 和 accel_bias 中就是完美的实战温度零偏
        }
    }
#else
    // =========================================================================
    // [执行] 3. 正常运行模式：扣除漂移误差
    // =========================================================================
    input_gx -= Config::Algorithm::Imu::GYRO_BIAS_X;
    input_gy -= Config::Algorithm::Imu::GYRO_BIAS_Y;
    input_gz -= Config::Algorithm::Imu::GYRO_BIAS_Z;

    input_ax -= Config::Algorithm::Imu::ACCEL_BIAS_X;
    input_ay -= Config::Algorithm::Imu::ACCEL_BIAS_Y;
    input_az -= Config::Algorithm::Imu::ACCEL_BIAS_Z;
#endif


    // 3. 执行 EKF 更新 (纯数学运算，几微秒跑完)
    uint32_t startTime = pyro::dwt_drv_t::get_current_ticks();
    IMU_QuaternionEKF_Update(input_gx, input_gy, input_gz,
                             input_ax, input_ay, input_az, dt);


    // ==========================================
    // 【新增防线 2】：浮点异常拦截与 EKF 软重启
    // ==========================================
    /* 判断运算结果是否合法（拦截NaN） */
    if (std::isnan(QEKF_INS.Pitch) || std::isinf(QEKF_INS.Pitch) ||
        std::isnan(QEKF_INS.Roll)  || std::isinf(QEKF_INS.Roll)  ||
        std::isnan(QEKF_INS.Yaw)   || std::isinf(QEKF_INS.Yaw))
    {
        NVIC_SystemReset();
        // 3. 拦截生效，直接 return 结束本次解算，绝对不将脏数据写入 Blackboard！
        return;
    }

    // 4. 从全局的 QEKF_INS 结构体中提取解算好的欧拉角
    // (QEKF_INS 是 QuaternionEKF.h 中定义好的全局结构体，算完后自动更新)
    ImuState state{};


    state.roll  = QEKF_INS.Roll;  // 内部已转为 Rad 或 Degree，请查阅 QuaternionEKF.h 确认
    state.pitch = QEKF_INS.Pitch;
    state.yaw   = QEKF_INS.Yaw;

    // 保留陀螺仪原始数据给云台内环
    state.gyro[0] = input_gx;
    state.gyro[1] = input_gy;
    state.gyro[2] = input_gz;

    state.accel[0] = input_ax;
    state.accel[1] = input_ay;
    state.accel[2] = input_az;

    state.temperature = data.temperature;

    state.timestamp = xTaskGetTickCount();

    /* 判断运算结果是否合法（拦截NaN） */

    // 5. 写入你自己的黑板
    Blackboard::instance().imuState.write(state);

}


void HAL_SPI_RxCpltCallback(SPI_HandleTypeDef* hspi) {
    if (hspi == &hspi2) {
        GPIOC->BSRR                         = (ACCEL_CS_Pin | GYRO_CS_Pin);
        BaseType_t xHigherPriorityTaskWoken = pdFALSE;
        xSemaphoreGiveFromISR(app._waitForReceive, &xHigherPriorityTaskWoken);
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    }
}


void HAL_SPI_TxCpltCallback(SPI_HandleTypeDef* hspi) {
    if (hspi == &hspi2) {
        app._bmi088.onTxComplete();
    }
}


void HAL_SPI_TxRxCpltCallback(SPI_HandleTypeDef* hspi) {
    if (hspi == &hspi2) {
        app._bmi088.onTransferComplete();
    }
}

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin) {
    if (GPIO_Pin == GPIO_PIN_12 && app._inited) {
        BaseType_t pxHigherPriorityTaskWoken;
        app._bmi088.onExti();
        ImuApp::instance().notifyFromISR(&pxHigherPriorityTaskWoken);
    }
}
