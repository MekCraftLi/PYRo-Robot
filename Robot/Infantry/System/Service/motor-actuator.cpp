/**
 *******************************************************************************
 * @file    motor-actuator.cpp
 * @brief   负责主动更新黑板中的电机反馈并且发送黑板中的数据
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




/* ------- define ----------------------------------------------------------------------------------------------------*/





/* ------- include ---------------------------------------------------------------------------------------------------*/



/* I. header */

#include "motor-actuator.h"
#include "Config/config.h"

/* II. other application */

#include "pyro_dji_motor_drv.h"
#include "pyro_can_drv.h"
#include "dm_motor_compat.h"

#include "../DataHub/blackboard.h"

/* III. standard lib */




/* ------- class prototypes-----------------------------------------------------------------------------------------*/




/* ------- macro -----------------------------------------------------------------------------------------------------*/





/* ------- variables -------------------------------------------------------------------------------------------------*/

[[maybe_unused]] static auto& forceInit = MotActSrvc::instance();


pyro::can_drv_t candrv1(&hfdcan1);
pyro::can_drv_t candrv2(&hfdcan2);
pyro::can_drv_t candrv3(&hfdcan3);

// 改为指针声明




/* ------- application attribute -------------------------------------------------------------------------------------*/

#define APPLICATION_ENABLE     true

#define APPLICATION_NAME       "MotAct"

#define APPLICATION_STACK_SIZE 512

#define APPLICATION_PRIORITY   4

static StackType_t appStack[APPLICATION_STACK_SIZE];



/* ------- message interface attribute -------------------------------------------------------------------------------*/





/* ------- function prototypes ---------------------------------------------------------------------------------------*/





/* ------- function implement ----------------------------------------------------------------------------------------*/


MotActSrvc::MotActSrvc()
    : PeriodicApp(APPLICATION_ENABLE, APPLICATION_NAME, APPLICATION_STACK_SIZE, appStack, APPLICATION_PRIORITY, 1) {}


void MotActSrvc::init() {
    /* driver object initialize */

    /* 1. driver object initialize */
    candrv1.init();
    candrv1.start();
    candrv2.init();
    candrv2.start();
    candrv3.init();
    candrv3.start();

    /* 2. 【关键】先将底层硬件注册进 hub */
    pyro::can_hub_t::get_instance()->hub_register_can_obj(&hfdcan1, &candrv1);
    pyro::can_hub_t::get_instance()->hub_register_can_obj(&hfdcan2, &candrv2);
    pyro::can_hub_t::get_instance()->hub_register_can_obj(&hfdcan3, &candrv3);


#ifdef CHASSIS
    /* 3. 再实例化电机对象，此时它们就能从 hub 中成功获取 _can_drv 并注册反馈邮箱了 */
    for (uint8_t i = 0; i < 4; i++) {
        new (&drive[i]) pyro::dji_m3508_motor_drv_t((pyro::dji_motor_tx_frame_t::register_id_t)i,
                                                    Config::Hardware::MotorTopo::DRIVE_MOTOR_CANS[i]);
        new (&steer[i]) pyro::dji_gm_6020_motor_drv_t((pyro::dji_motor_tx_frame_t::register_id_t)i,
                                                      Config::Hardware::MotorTopo::STEER_MOTOR_CANS[i]);
    }
    new (&trigger)
        pyro::dji_m2006_motor_drv_t(Config::Hardware::MotorTopo::TRIGGER_ID, Config::Hardware::MotorTopo::COMM_CAN);
    new (&yaw) pyro::dji_gm_6020_motor_drv_t(Config::Hardware::MotorTopo::YAW_ID, Config::Hardware::MotorTopo::COMM_CAN);
#elifdef GIMBAL

    new (&fric[0]) pyro::dji_m3508_motor_drv_t(Config::Hardware::MotorTopo::FRIC_LEFT_ID,
                                               Config::Hardware::MotorTopo::FRIC_LEFT_CAN);
    new (&fric[1]) pyro::dji_m3508_motor_drv_t(Config::Hardware::MotorTopo::FRIC_RIGHT_ID,
                                               Config::Hardware::MotorTopo::FRIC_RIGHT_CAN);
    new (&trigger)
        pyro::dji_m2006_motor_drv_t(Config::Hardware::MotorTopo::TRIGGER_ID, Config::Hardware::MotorTopo::TRIGGER_CAN);
    new (&yaw) pyro::dji_gm_6020_motor_drv_t(Config::Hardware::MotorTopo::YAW_ID, Config::Hardware::MotorTopo::YAW_CAN);
    new (&pitch) compat::DmMotorCompat(0x01, 0x00, Config::Hardware::MotorTopo::PITCH_CAN);

    // ==========================================
    // 【补充】达妙电机 MIT 模式核心配置参数对齐
    // 注意：这里的范围必须与你用“达妙调参助手”烧录进电机内部的 P_MAX, V_MAX, T_MAX 绝对一致！
    // 否则数据解包出来的浮点数全都是错的！
    // ==========================================
    pitch.set_position_range(-Config::Algorithm::Chassis::DM_MOTOR_PMAX,
                             Config::Algorithm::Chassis::DM_MOTOR_PMAX); // 设定位置限位 (rad)
    pitch.set_rotate_range(-Config::Algorithm::Chassis::DM_MOTOR_VMAX,
                           Config::Algorithm::Chassis::DM_MOTOR_VMAX); // 设定速度限位 (rad/s)
    pitch.set_torque_range(-Config::Algorithm::Chassis::DM_MOTOR_TMAX,
                           Config::Algorithm::Chassis::DM_MOTOR_TMAX); // 设定扭矩限位 (N.m)

    // 设置 MIT 模式下的阻抗参数 (若使用串级PID输出扭矩，Kp和Kd必须设为0)
    pitch.set_runtime_kp(Config::Algorithm::Gimbal::DM_MOT_PITCH_KP);
    pitch.set_runtime_kd(Config::Algorithm::Gimbal::DM_MOT_PITCH_KD);

    // 发送使能指令 (0xFC)
    pitch.enable();

    fric[0].enable();
    fric[1].enable();
    trigger.enable();

#endif
}

#ifdef CHASSIS
void MotActSrvc::run() {
    ChassisState state{.timestamp = xTaskGetTickCount()};
    ChassisOutput chasOut{};
    Blackboard::instance().chassisOut.read(chasOut);

    // 更新数据与发送数据
    yaw.update_feedback();
    // yaw向左为正
    state.yaw.pos = -compat::apply_ecd_offset(yaw.get_current_position(), Config::Hardware::MotorTopo::YAW_OFFSET);
    state.yaw.torque = -yaw.get_current_torque();
    state.yaw.vel    = -yaw.get_current_rotate();
    state.yaw.temp   = yaw.get_temperature();




    for (uint8_t i = 0; i < 4; i++) {
        drive[i].update_feedback();
        steer[i].update_feedback();
        state.modules[i].drive.pos    = drive[i].get_current_position();
        state.modules[i].drive.temp   = drive[i].get_temperature();
        state.modules[i].drive.torque = drive[i].get_current_torque();
        state.modules[i].drive.vel    = drive[i].get_current_rotate();
        state.modules[i].drive.online = drive[i].is_online();

        state.modules[i].steer.pos    = compat::apply_ecd_offset(steer[i].get_current_position(), Config::Hardware::MotorTopo::STEER_ECD_OFFSET[i]);
        state.modules[i].steer.temp   = steer[i].get_temperature();
        state.modules[i].steer.torque = steer[i].get_current_torque();
        state.modules[i].steer.vel    = steer[i].get_current_rotate();
        state.modules[i].steer.vel    = steer[i].is_online();


        steer[i].send_torque(chasOut.steerVoltage[i]);

        drive[i].send_torque(chasOut.driveCurrent[i]);
    }



    Blackboard::instance().chassisState.write(state);
}
#elifdef GIMBAL

float kp = 0.1f;
float kd;
void MotActSrvc::run() {

    GimbalState gstate{.timestamp = xTaskGetTickCount()};
    BoosterState bstate{.timestamp = gstate.timestamp};
    GimbalOutput gout{};
    BoosterOutput bout{};

    Blackboard::instance().boosterState.read(bstate);

    yaw.update_feedback();
    pitch.update_feedback();
    fric[0].update_feedback();
    fric[1].update_feedback();
    trigger.update_feedback();

    gstate.yaw.pos      = compat::apply_ecd_offset(yaw.get_current_position(), Config::Hardware::MotorTopo::YAW_OFFSET);
    gstate.yaw.temp     = yaw.get_temperature();
    gstate.yaw.torque   = yaw.get_current_torque();
    gstate.yaw.vel      = yaw.get_current_rotate();
    gstate.yaw.online   = yaw.is_online();

    gstate.pitch.pos    = pitch.get_current_position();
    gstate.pitch.temp   = pitch.get_temperature();
    gstate.pitch.torque = pitch.get_current_torque();
    gstate.pitch.vel    = pitch.get_current_rotate();
    gstate.pitch.online = pitch.is_online();

    for (uint8_t i = 0; i < 2; i++) {
        bstate.fric[i].pos    = fric[i].get_current_position();
        bstate.fric[i].temp   = fric[i].get_temperature();
        bstate.fric[i].torque = fric[i].get_current_torque();
        bstate.fric[i].vel    = fric[i].get_current_rotate();
        bstate.fric[i].online = fric[i].is_online();
    }

    bstate.trigger.pos    = trigger.get_current_position();
    bstate.trigger.temp   = trigger.get_temperature();
    bstate.trigger.torque = trigger.get_current_torque();
    bstate.trigger.vel    = trigger.get_current_rotate();
    bstate.triggerEcd     = compat::position_to_raw_ecd(trigger.get_current_position());
    bstate.trigger.online = trigger.is_online();

    static int32_t lastEcd;
    int32_t deltaEcd = bstate.triggerEcd - lastEcd;
    if (deltaEcd < -4096) {
        // 原始值突变变小，说明正向转过了零点 (例如 8190 -> 10)
        bstate.triggerRound++;
        if (bstate.triggerRound >= 36) {
            bstate.triggerRound = 0; // 满36圈，输出轴刚好转满一圈，圈数归零
        }
    } else if (deltaEcd > 4096) {
        // 原始值突变变大，说明反向转过了零点 (例如 10 -> 8190)
        bstate.triggerRound--;
        if (bstate.triggerRound < 0) {
            bstate.triggerRound = 35; // 退回上一圈
        }
    }
    lastEcd = bstate.triggerEcd;


    Blackboard::instance().gimbalState.write(gstate);
    Blackboard::instance().boosterState.write(bstate);

    Blackboard::instance().boosterOut.read(bout);
    Blackboard::instance().gimbalOut.read(gout);

    yaw.send_torque(gout.yawVoltage);
    trigger.send_torque(bout.triggerCurrent);
    fric[Config::Hardware::MotorTopo::FRIC_LEFT_ID].send_torque(bout.fricLeftCurrent);
    fric[Config::Hardware::MotorTopo::FRIC_RIGHT_ID].send_torque(bout.fricRightCurrent);
    // 【补充】达妙电机 MIT 控制输出
    // 此处使用了 data-def.h 中定义的 pitchCurrent (实为 Torque 扭矩量)
    // 结合我们在 init() 中设置的 Kp=0, Kd=0，这就是标准的力矩透传控制


    // 提示：如果你上一轮修改了 dm_motor_drv_t 并增加了 send_mit_ctrl(pos, vel, t_ff)
    // 并且希望使用满血的电机内部阻抗控制，这里可以改为：
    if (gout.pitchEn) {
        pitch.send_mit_ctrl(gout.targetPitchPos, gout.targetPitchSpeed, gout.pitchFeedforwardTorque);
    }
}


#endif