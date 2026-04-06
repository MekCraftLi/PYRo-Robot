/**
 *******************************************************************************
 * @file    super-cap-comm.cpp
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
 * @date    2026/3/18
 * @version 1.0
 *******************************************************************************
 */




/* ------- define ----------------------------------------------------------------------------------------------------*/





/* ------- include ---------------------------------------------------------------------------------------------------*/



/* I. header */

#include "super-cap-comm.h"

#include "System/DataHub/blackboard.h"
#include "System/DataHub/referee-data-hub.h"
#include "tools/crc.h"

/* II. other application */


/* III. standard lib */




/* ------- class prototypes-----------------------------------------------------------------------------------------*/




/* ------- macro -----------------------------------------------------------------------------------------------------*/





/* ------- variables -------------------------------------------------------------------------------------------------*/

[[maybe_unused]] static auto& forceInit = SuperCapCommSrvc::instance();

// 定义专用的 DMA 内存池缓存区
__attribute__((section(".dma_pool"))) static uint8_t capRxBuf[256] = {0};
__attribute__((section(".dma_pool"))) static CapRxFrame _rxFrame {};
__attribute__((section(".dma_pool"))) static CapTxFrame _txFrame {};
__attribute__((section(".dma_pool"))) static uint8_t capTxBuf[32] = {0};



/* ------- application attribute -------------------------------------------------------------------------------------*/

#define APPLICATION_ENABLE     true

#define APPLICATION_NAME       "superCapComm"

#define APPLICATION_STACK_SIZE 512

#define APPLICATION_PRIORITY   4

static StackType_t appStack[APPLICATION_STACK_SIZE];



/* ------- message interface attribute -------------------------------------------------------------------------------*/






/* ------- function prototypes ---------------------------------------------------------------------------------------*/





/* ------- function implement ----------------------------------------------------------------------------------------*/


SuperCapCommSrvc::SuperCapCommSrvc()
    : PeriodicApp(APPLICATION_ENABLE, APPLICATION_NAME, APPLICATION_STACK_SIZE,  appStack, APPLICATION_PRIORITY, 2){
}


void SuperCapCommSrvc::init() {
    /* driver object initialize */
    vTaskDelay(1000);
    // 开启空闲中断 DMA 接收
    HAL_UARTEx_ReceiveToIdle_DMA(&Config::Hardware::Comms::SUPER_CAP_UART, (uint8_t*)&_rxFrame, sizeof(_rxFrame));
}


void SuperCapCommSrvc::run() {
    // 1. 从黑板获取最新业务指令
    SuperCapOutput scOut{};
    RMPowerHeatData powerHeatData{};

    RMRobotStatus robotStatus{};

    RefereeDataHub::instance().powerHeat.read(powerHeatData);
    RefereeDataHub::instance().robotStatus.read(robotStatus);



    _txFrame.header.sof = 0x55;

    Crc::appendCrc8((uint8_t*)&_txFrame, 1); // 计算帧头 CRC8，假设仅涵盖 SOF 字节

    // 2. 将业务数据打包至底层协议结构体
    _txFrame.data.power_referee = 0; // 假设扩大100倍发送，视电容方代码而定
    _txFrame.data.power_limit_referee = robotStatus.chassisPowerLimit;
    _txFrame.data.power_buffer_referee = powerHeatData.bufferEnergy;
    _txFrame.data.power_buffer_limit_referee = 60; // 根据比赛规则写死或动态传入
    _txFrame.data.use_cap = 1;
    _txFrame.data.kill_chassis_user = 0;
    _txFrame.data.speed_up_user_now = 0;

    // 3. 计算 CRC 校验
    // （注意：具体的 CRC 函数名请替换为您 tools/crc.h 中的真实函数）
    // 计算 CRC8 (仅涵盖 SOF 字节)
    Crc::appendCrc8((uint8_t*)&_txFrame, 2); // 假设 CRC8 位于帧头的第二个字节

    // 计算 CRC16 (涵盖 Header + Data)
    Crc::appendCrc16((uint8_t*)&_txFrame, sizeof(CapTxFrame)); // 假设 CRC16 位于帧尾，占 2 字节
    // 4. 数据拷贝到 DMA 内存池并触发发送
    memcpy(capTxBuf, &_txFrame, sizeof(CapTxFrame));

    HAL_UART_Transmit_DMA(&Config::Hardware::Comms::SUPER_CAP_UART, capTxBuf, sizeof(CapTxFrame));
}

void SuperCapCommSrvc::onUartRxEventCallback(size_t size) {

    static SuperCapState capState{};
    capState.capPower =(float) _rxFrame.data.cap_power_cap / 100.0f - 250.0f;
    capState.chassisPower = (float) _rxFrame.data.chassis_power_cap / 100.0f;
    capState.isCapLow = _rxFrame.data.cap_low_flag != 0;
    capState.isError = _rxFrame.data.error_flag != 0;
    capState.voltage = (float)_rxFrame.data.vot_cap / 100.0f;
    Blackboard::instance().capState.writeFromISR(capState);
    HAL_UARTEx_ReceiveToIdle_DMA(&Config::Hardware::Comms::SUPER_CAP_UART, (uint8_t*)&_rxFrame, sizeof(_rxFrame));

}

void SuperCapCommSrvc::onUartErrCallback() {

    HAL_UARTEx_ReceiveToIdle_DMA(&Config::Hardware::Comms::SUPER_CAP_UART, (uint8_t*)&_rxFrame, sizeof(_rxFrame));

}