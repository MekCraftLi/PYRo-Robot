/**
 *******************************************************************************
 * @file    vision-comm.cpp
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
 * @date    2026/3/6
 * @version 1.0
 *******************************************************************************
 */




/* ------- define ----------------------------------------------------------------------------------------------------*/





/* ------- include ---------------------------------------------------------------------------------------------------*/



/* I. header */

#include "vision-comm.h"
#include <cstdio>

#include "Config/Gimbal/hw-config.h"
#include "System/DataHub/blackboard.h"
#include "class/cdc/cdc_device.h"
#include "pyro_dwt_drv.h"
#include "referee-data-hub.h"
#include "referee-protocol.h"
#include "tools/crc.h"


/* II. other application */


/* III. standard lib */




/* ------- class prototypes-----------------------------------------------------------------------------------------*/




/* ------- macro -----------------------------------------------------------------------------------------------------*/





/* ------- variables -------------------------------------------------------------------------------------------------*/

[[maybe_unused]] static auto& forceInit = VisionCommSrvc::instance();

// 强制将缓冲区放入 DMA 可访问且非 Cache 区域
[[maybe_unused]] __attribute__((section(".dma_pool"))) static uint8_t dmaRxBuf[VisionCommSrvc::RX_BUFFER_SIZE];
uint8_t dmaTxBuf[0x100];

/* ------- application attribute -------------------------------------------------------------------------------------*/

#define APPLICATION_ENABLE     true

#define APPLICATION_NAME       "VisionComm"

#define APPLICATION_STACK_SIZE 512

#define APPLICATION_PRIORITY   4

static StackType_t appStack[APPLICATION_STACK_SIZE];



/* ------- message interface attribute -------------------------------------------------------------------------------*/





/* ------- function prototypes ---------------------------------------------------------------------------------------*/





/* ------- function implement ----------------------------------------------------------------------------------------*/


VisionCommSrvc::VisionCommSrvc()
    : PeriodicApp(APPLICATION_ENABLE, APPLICATION_NAME, APPLICATION_STACK_SIZE, appStack, APPLICATION_PRIORITY, 2) {}


void VisionCommSrvc::init() {
    /* driver object initialize */
    // 开启 DMA 环形连续接收模式
    // HAL_UART_Receive_DMA(&Config::Hardware::Comms::VISION_UART, dmaRxBuf, RX_BUFFER_SIZE);
}


void VisionCommSrvc::run() {
    // 1. 处理接收流 (解析上位机发来的 Command)
    processRxStream();

    // 2. 打包并发送 (向上位机发送 Telemetry)
    sendTxFrame();
}

void VisionCommSrvc::processRxStream() {
    // 动态获取 DMA 写入的当前位置

    uint32_t available = tud_cdc_available();

    if (available == 0) {
        return;
    }

    // 2. 将数据一次性从底层 FIFO 拉取到局部缓冲区 (提高效率)
    // 设定一个稍微大于单帧长度的局部 Buffer，防止一次发来多帧
    uint8_t rxBuf[128];

    // 限制单次最大读取量，防止局部数组越界
    uint32_t readBytes = tud_cdc_read(rxBuf, sizeof(rxBuf));

    // 3. 将拉取到的字节逐个喂入你原有的状态机
    for (uint32_t i = 0; i < readBytes; ++i) {
        uint8_t byte = rxBuf[i];

        switch (_rxState) {
            case RxState::WAIT_SOF:
                if (byte == VISION_SOF) {
                    _unpackBuf[0] = byte;
                    _unpackSize   = 1;
                    _rxState      = RxState::WAIT_PAYLOAD;
                }
                break;

            case RxState::WAIT_PAYLOAD:
                _unpackBuf[_unpackSize++] = byte;

                // 如果接收完了一整帧
                if (_unpackSize == sizeof(VisionRxFrame)) {
                    // 校验整包 CRC16
                    if (Crc::verifyCrc16(_unpackBuf, sizeof(VisionRxFrame))) {
                        auto* frame = reinterpret_cast<VisionRxFrame*>(_unpackBuf);
                        // 校验通过，写入黑板，供云台控制任务使用
                        Blackboard::instance().visionCmd.write(frame->payload);
                    }
                    // 重新等待下一帧的 SOF
                    _rxState = RxState::WAIT_SOF;
                }
                break;
        }
    }
}
void VisionCommSrvc::sendTxFrame() {
    // 确保上一次 DMA 传输已经完成，防止踩内存

    // 1. 从黑板读取最新的云台、底盘状态
    VisionTelemetry telem{};
    ImuState state{};
    GimbalOutput out{};
    ChassisToGimbalComm comm{};
    VisionTxFrame dmaTxFrame{};


    Blackboard::instance().imuState.read(state);
    Blackboard::instance().c2gComm.read(comm);
    Blackboard::instance().gimbalOut.read(out);




    telem.currentPitch = -state.pitch;
    telem.currentYaw   = state.yaw;
    telem.autoAimMode  = 1;
    telem.enemyColor   = comm.msg.robotId > 100;
    telem.initialSpeed = static_cast<float>(comm.msg.initialSpeedX100) / 100.0f;




    // 2. 组装数据帧
    dmaTxFrame.sof     = VISION_SOF;
    dmaTxFrame.payload = telem;

    // 3. 计算并在末尾追加 CRC16 (使用我们之前封装好的 CRC 工具)
    Crc::appendCrc16(reinterpret_cast<uint8_t*>(&dmaTxFrame), sizeof(VisionTxFrame));

    memcpy(dmaTxBuf, &dmaTxFrame, sizeof(VisionTxFrame));
    dmaTxBuf[sizeof(VisionTxFrame)] = '\n';

    if (tud_cdc_write_available() < sizeof(VisionTxFrame) + 1) {
        tud_cdc_write_clear();
    }

    RMShootData shootData{};

    RefereeDataHub::instance().shootData.read(shootData);

    uint16_t len = snprintf(reinterpret_cast<char*>(dmaTxBuf), sizeof(dmaTxBuf), "从裁判系统中获得初速:%dmm/s\n",
                            shootData.initialSpeed * 1000);
    tud_cdc_write(dmaTxBuf, len);
    tud_cdc_write_flush();

    //
    // // 4. 触发 DMA 发送 (非阻塞，CPU 立刻返回)
    // HAL_UART_Transmit_DMA(&Config::Hardware::Comms::VISION_UART, reinterpret_cast<uint8_t*>(&dmaTxBuf),
    //                       sizeof(VisionTxFrame) + 1);
}