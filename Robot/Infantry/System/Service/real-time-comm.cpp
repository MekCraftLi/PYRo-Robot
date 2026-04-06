/**
 *******************************************************************************
 * @file    real-time-comm.cpp
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
 * @date    2026/3/3
 * @version 1.0
 *******************************************************************************
 */




/* ------- define ----------------------------------------------------------------------------------------------------*/





/* ------- include ---------------------------------------------------------------------------------------------------*/



/* I. header */

#include "real-time-comm.h"

#include "Application/fire-ctrl-app.h"
#include "Config/Chassis/hw-config.h"
#include "System/DataHub/blackboard.h"
#include "System/DataHub/data-def.h"
#include "System/DataHub/referee-data-hub.h"
#include "System/DataHub/referee-protocol.h"
#include "motor-actuator.h"

/* II. other application */


/* III. standard lib */




/* ------- class prototypes-----------------------------------------------------------------------------------------*/




/* ------- macro -----------------------------------------------------------------------------------------------------*/





/* ------- variables -------------------------------------------------------------------------------------------------*/

[[maybe_unused]] static auto& forceInit = RealTimeCommApp::instance();



/* ------- application attribute -------------------------------------------------------------------------------------*/

#define APPLICATION_ENABLE     true

#define APPLICATION_NAME       "RealTimeComm"

#define APPLICATION_STACK_SIZE 512

#define APPLICATION_PRIORITY   4

static StackType_t appStack[APPLICATION_STACK_SIZE];



/* ------- message interface attribute -------------------------------------------------------------------------------*/





/* ------- function prototypes ---------------------------------------------------------------------------------------*/





/* ------- function implement ----------------------------------------------------------------------------------------*/


RealTimeCommApp::RealTimeCommApp()
    : PeriodicApp(APPLICATION_ENABLE, APPLICATION_NAME, APPLICATION_STACK_SIZE, appStack, APPLICATION_PRIORITY, 10) {}


void RealTimeCommApp::init() {
    /* driver object initialize */
    MotActSrvc::instance().waitInit();
}


void RealTimeCommApp::run() {
#ifdef GIMBAL
    static GimbalToChassisComm output;
#elifdef CHASSIS
#endif

    [[maybe_unused]] static FDCAN_TxHeaderTypeDef txHeader = {
#ifdef GIMBAL
        .Identifier = 0x100,
#elifdef CHASSIS
        .Identifier = 0x101,
#endif
        .IdType              = FDCAN_STANDARD_ID,
        .TxFrameType         = FDCAN_DATA_FRAME,
        .DataLength          = FDCAN_DLC_BYTES_8,
        .ErrorStateIndicator = FDCAN_ESI_ACTIVE,
        .BitRateSwitch       = FDCAN_BRS_OFF,
        .FDFormat            = FDCAN_CLASSIC_CAN,
        .TxEventFifoControl  = FDCAN_NO_TX_EVENTS,
        .MessageMarker       = 0,
    };


#ifdef GIMBAL
    Blackboard::instance().g2cOutput.read(output);
    output.msg.shootEn = static_cast<uint8_t>(FireCtrlApp::instance().getFireState()) > 0;


    HAL_FDCAN_AddMessageToTxFifoQ(&hfdcan1, &txHeader, output.buffer);

#elifdef CHASSIS
    // 1. 获取裁判系统中的各项数据
    RMShootData shootData{};
    RMPowerHeatData powerHeatData{};
    RMRobotStatus robotStatus{};
    ChassisToGimbalComm tComm{};
    ImuState imuState{};

    RefereeDataHub::instance().shootData.read(shootData);
    RefereeDataHub::instance().powerHeat.read(powerHeatData);
    RefereeDataHub::instance().robotStatus.read(robotStatus);
    Blackboard::instance().imuState.read(imuState);

    // 2. 填充到底盘发往云台的结构体中
    tComm.msg.initialSpeedX100      = shootData.initialSpeed * 100;
    tComm.msg.shooter17mmBarrelHeat = powerHeatData.shooter17mmBarrelHeat;
    tComm.msg.coolingRate           = robotStatus.shooterBarrelCoolingValue;
    tComm.msg.heatLimit             = robotStatus.shooterBarrelHeatLimit;
    tComm.msg.robotId               = robotStatus.robotId;
    tComm.msg.chassisYawSpeed       = (int8_t)(imuState.gyro[2] * 10);

    // 4. 将 buffer 发送至 CAN 邮箱
    HAL_FDCAN_AddMessageToTxFifoQ(&Config::Hardware::Comms::BOARD_COMM_CAN, &txHeader, tComm.buffer);
#endif
}

#ifdef CHASSIS
extern "C" void getBoardCommFromISR(uint8_t* pData) {

    static GimbalToChassisComm comm{};
    memcpy(comm.buffer, pData, 8);
    Blackboard::instance().rComm.writeFromISR(comm);
}
#elifdef GIMBAL

extern "C" void getBoardCommFromISR(uint8_t* pData) {

    // [新增] 云台板在此接收来自底盘的 0x101 CAN 数据
    static ChassisToGimbalComm comm{};
    memcpy(comm.buffer, pData, 8);
    Blackboard::instance().c2gComm.writeFromISR(comm);
}
#endif