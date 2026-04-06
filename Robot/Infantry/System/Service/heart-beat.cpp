/**
 *******************************************************************************
 * @file    heart-beat.cpp
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
 * @date    2026/2/28
 * @version 1.0
 *******************************************************************************
 */




/* ------- define ----------------------------------------------------------------------------------------------------*/





/* ------- include ---------------------------------------------------------------------------------------------------*/



/* I. header */

#include "heart-beat.h"

#include "pyro_dwt_drv.h"

/* II. other application */
#include "../DataHub/blackboard.h"
#include "commander.h"
#include "motor-actuator.h"

/* III. standard lib */




/* ------- class prototypes-----------------------------------------------------------------------------------------*/




/* ------- macro -----------------------------------------------------------------------------------------------------*/





/* ------- variables -------------------------------------------------------------------------------------------------*/



[[maybe_unused]] static auto& forceInit = HeartBeatApp::instance();



/* ------- application attribute -------------------------------------------------------------------------------------*/

#define APPLICATION_ENABLE     true

#define APPLICATION_NAME       "HeartBeat"

#define APPLICATION_STACK_SIZE 512

#define APPLICATION_PRIORITY   4

static StackType_t appStack[APPLICATION_STACK_SIZE];



/* ------- message interface attribute -------------------------------------------------------------------------------*/






/* ------- function prototypes ---------------------------------------------------------------------------------------*/



Blackboard* bb = &Blackboard::instance();

/* ------- function implement ----------------------------------------------------------------------------------------*/


HeartBeatApp::HeartBeatApp()
    : PeriodicApp(APPLICATION_ENABLE, APPLICATION_NAME, APPLICATION_STACK_SIZE,  appStack, APPLICATION_PRIORITY, 1000){
}


void HeartBeatApp::init() {
    /* driver object initialize */
    pyro::dwt_drv_t::init(480);
}

struct {
    float commander;
    float motorActuator;
    float realTimeComm;
    float stateEstimator;
    float usbDevice;
    float visionComm;
    float fireCtrl;
    float moveCtrl;
}runTime;


void HeartBeatApp::run() {
 pyro::dwt_drv_t::get_timeline();

#ifdef GIMBAL
    GimbalOutput out{};
    Blackboard::instance().gimbalOut.read(out);
    if (out.pitchEn) {
        MotActSrvc::instance().pitch.enable();
    } else {
        MotActSrvc::instance().pitch.disable();
    }
#endif

    // ========================================================================
    // 【终极防御】：主动轮询 FDCAN 硬件状态，无视任何中断配置
    // FDCAN_CCCR_INIT 位为 1 代表硬件处于初始化/休眠状态（被 Bus-Off 强杀）
    // ========================================================================
    if (hfdcan1.Instance->PSR & FDCAN_PSR_EP) {
        HAL_FDCAN_Stop(&hfdcan1);
        HAL_FDCAN_Start(&hfdcan1);
        // 重启后必须重新激活 RX 中断，否则彻底收不到数据
        HAL_FDCAN_ActivateNotification(&hfdcan1, FDCAN_IT_RX_FIFO0_NEW_MESSAGE | FDCAN_IT_ERROR_PASSIVE, 0);
    }

    if (hfdcan2.Instance->PSR & FDCAN_PSR_EP) {
        HAL_FDCAN_Stop(&hfdcan2);
        HAL_FDCAN_Start(&hfdcan2);
        HAL_FDCAN_ActivateNotification(&hfdcan2, FDCAN_IT_RX_FIFO0_NEW_MESSAGE | FDCAN_IT_ERROR_PASSIVE, 0);
    }

    if (hfdcan3.Instance->PSR & FDCAN_PSR_EP) {
        HAL_FDCAN_Stop(&hfdcan3);
        HAL_FDCAN_Start(&hfdcan3);
        HAL_FDCAN_ActivateNotification(&hfdcan3, FDCAN_IT_RX_FIFO0_NEW_MESSAGE | FDCAN_IT_ERROR_PASSIVE, 0);
    }
    // ========================================================================
    runTime.commander = CommanderSrvc::instance().getRunTime();
    runTime.motorActuator = CommanderSrvc::instance().getRunTime();
    runTime.realTimeComm = CommanderSrvc::instance().getRunTime();
    runTime.stateEstimator = CommanderSrvc::instance().getRunTime();
    runTime.usbDevice = CommanderSrvc::instance().getRunTime();
    runTime.visionComm = CommanderSrvc::instance().getRunTime();
    runTime.fireCtrl = CommanderSrvc::instance().getRunTime();
    runTime.moveCtrl = CommanderSrvc::instance().getRunTime();


}
