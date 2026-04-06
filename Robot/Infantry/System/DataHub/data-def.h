/**
 *******************************************************************************
 * @file    data-def.h
 * @brief   简要描述
 *******************************************************************************
 * @attention
 *
 * none
 *
 *******************************************************************************
 * @note
 *
 * none
 *
 *******************************************************************************
 * @author  MekLi
 * @date    2026/2/27
 * @version 1.0
 *******************************************************************************
 */


/* Define to prevent recursive inclusion -----------------------------------------------------------------------------*/

#ifndef INFANTRY_CHASSIS_DATA_DEF_H
#define INFANTRY_CHASSIS_DATA_DEF_H



/*-------- 1. includes and imports -----------------------------------------------------------------------------------*/


#include "Component/Motor/pyro_dji_motor_drv.h"


/*-------- 2. enum and define ----------------------------------------------------------------------------------------*/

// ==========================================
// 1. 意图区数据 (外界输入)
// ==========================================

// 原始遥控器数据 (独立于具体协议)
struct RcRawData {
    float right_x, right_y; // 右摇杆 (-1.0 ~ 1.0)
    float left_x, left_y;   // 左摇杆 (-1.0 ~ 1.0)
    float dial;             // 拨轮   (-1.0 ~ 1.0)
    uint8_t sw_left;        // 左开关 (1:上, 2:中, 3:下)
    uint8_t sw_right;       // 右开关 (1:上, 2:中, 3:下)
    uint32_t timestamp;
};

enum class ControlSource : uint8_t {
    SAFE_STOP = 0, // 急停/无力 (最高优先级)
    REMOTE    = 1, // 遥控器主导
    VISION    = 2, // 视觉主导
    KEYMOUSE  = 3  // 键鼠主导
};

// ==========================================
// [新增] 模式枚举定义
// ==========================================
enum ChassisMode : uint8_t {
    CHASSIS_RELAX  = 0, // 无力/急停模式
    CHASSIS_NORMAL = 1, // 遥控器手动控制模式 (速度环)
    CHASSIS_SPIN   = 2, // 自动/视觉/状态保留模式
};

enum GimbalMode : uint8_t {
    GIMBAL_RELAX  = 0, // 无力模式
    GIMBAL_NORMAL = 1, // 遥控器控制模式 (速度环)
    GIMBAL_AUTO   = 2, // 视觉自瞄/绝对角度模式 (位置-速度串级环)
};

// 在 Solution/System/DataHub/data-def.h 中

enum class ShootEvent {
    NONE = 0,
    EMERGENCY_STOP,
    FRIC_TOGGLE, // 摩擦轮开启/关闭 切换事件
    SINGLE_FIRE, // 单发事件
    BURST_START, // 连发开始事件
    BURST_STOP   // 连发结束事件
};

struct ShootCmd {
    ShootEvent event = ShootEvent::NONE;
    struct {
        uint32_t burstShot : 1 = 0;
    } state;
};


// 底盘与云台的宏观期望指令
struct ChassisCmd {
    float vx, vy, vw; // 期望速度 (m/s, rad/s)
    uint8_t mode;     // 模式控制
    uint32_t timestamp;
};

struct GimbalCmd {
    // 【修改】将绝对角度改为期望角速度 (rad/s)
    float yawVel;
    float pitchVel;

    // --- 新增：视觉模式使用的高阶期望 ---
    float targetYaw;
    float targetPitch;
    float targetYawSpeed;        // 目标角速度 (用于前馈)
    float targetYawAcceleration; // 目标角加速度 (用于高级动力学前馈)
    // ------------------------------------


    uint8_t mode;
    uint32_t timestamp;
};

// ==========================================
// 2. 状态区数据 (层级物理反馈)
// ==========================================

struct ImuState {
    float accel[3], gyro[3];
    float roll, pitch, yaw;
    float temperature;
    uint32_t timestamp;
};

struct MotorState {
    float pos;    // 角度 (rad)
    float vel;    // 角速度 (rad/s)
    float torque; // 真实反馈力矩 (N.m) 或 电流 (A)
    int8_t temp;  // 温度 (°C)
    bool online;
};

// 在合适位置添加业务层结构体
struct SuperCapState {
    float voltage;      // 转换为真实电压 (V)
    float capPower;     // 充放电功率 (W)
    float chassisPower; // 底盘功率 (W)
    bool isCapLow;      // 没电标志
    bool isError;       // 错误标志
    bool isOnline;      // 离线检测标志
};

struct SuperCapOutput {
    float refereePower;  // 当前消耗功率
    uint8_t powerLimit;  // 功率上限
    uint8_t powerBuffer; // 缓冲能量 (J)
    bool enableCap;      // 是否允许超级电容放电
};
// 舵轮模块组合状态
struct SwerveModuleState {
    MotorState drive; // 动力轮
    MotorState steer; // 航向舵
};

struct ChassisState {
    SwerveModuleState modules[4]; // 4个舵轮
    MotorState yaw;
    uint32_t timestamp;
};

struct GimbalState {
    MotorState yaw;
    MotorState pitch;
    uint32_t timestamp;
};

struct BoosterState {
    MotorState fric[2];
    MotorState trigger;
    int32_t triggerEcd;
    int32_t triggerRound;
    uint32_t timestamp;
};

// ==========================================
// 3. 输出区数据 (执行器控制量)
// ==========================================

struct ChassisOutput {
    float driveCurrent[4]; // 4个动力轮目标电流 (A)
    float steerVoltage[4]; // 4个航向舵目标电压 (V)
};

struct GimbalOutput {
    float yawVoltage;
    float targetPitchPos;
    float targetPitchSpeed;
    float pitchFeedforwardTorque;
    bool pitchEn;
};

struct BoosterOutput {
    float fricLeftCurrent;
    float fricRightCurrent;
    float triggerCurrent;
};

union GimbalToChassisComm {

    __attribute__((packed)) struct {
        int32_t vx    : 6; //  正方向： 向前
        int32_t vy    : 6; // 正方向： 向左
        uint32_t mode : 4;
        uint32_t shootEn: 1;
        uint32_t resetUI:1;
    } msg;

    uint8_t buffer[8];
};
// ==========================================
// [新增] 底盘向云台发送的通信联合体
// ==========================================
union ChassisToGimbalComm {
    __attribute__((packed)) struct {
        // 将 float (4字节) 压缩为 uint16_t (2字节) 传初速度，乘以 100 发送，云台除以 100
        uint32_t initialSpeedX100      : 7; // 弹丸初速度 * 100 (2 Bytes)
        uint32_t shooter17mmBarrelHeat : 9; // 17mm 枪口当前热量 (2 Bytes)
        uint32_t heatLimit             : 9; // 热量上限 (如 150, 240, 360)
        uint32_t coolingRate           : 7; // 冷却速率 (如 40, 60, 80)
        uint8_t robotId;                    // 机器人 ID (1 Byte)
        int8_t chassisYawSpeed;
    } msg;
    uint8_t buffer[8];
};
// ==========================================
// 4. 遥测/中间区数据 (控制过程可视化)
// ==========================================
// 这部分极其关键，记录了从 "Cmd" 到 "Output" 之间的数学变换细节

struct ChassisTelemetry {
    // 运动学正解 (Forward Kinematics)：根据轮速推算出的底盘真实质心速度
    float real_vx, real_vy, real_vw;

    // 运动学逆解 (Inverse Kinematics)：算出的 4个轮子预期打角和转速
    float targetSteerAngle[4];
    float targetSteerVelocity[4];
    float targetDriveSpd[4];

    // 底盘功率观测器
    float estimated_power_w; // 预估底盘总消耗功率 (Watt)

    uint32_t timestamp;
};

struct GimbalTelemetry {
    float targetYawRad;
    float targetYawRotate;
    float targetPitchRad;
    float targetPitchRotate;
};

/*-------- 3. interface ----------------------------------------------------------------------------------------------*/




/*-------- 4. decorator ----------------------------------------------------------------------------------------------*/




/*-------- 5. factories ----------------------------------------------------------------------------------------------*/





#endif /*INFANTRY_CHASSIS_DATA_DEF_H*/
