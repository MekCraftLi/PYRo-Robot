/**
 *******************************************************************************
 * @file    referee.cpp
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
 * @date    2026/3/4
 * @version 1.0
 *******************************************************************************
 */




/* ------- define ----------------------------------------------------------------------------------------------------*/





/* ------- include ---------------------------------------------------------------------------------------------------*/



/* I. header */

#include "referee.h"

#include "Config/Chassis/hw-config.h"
#include "System/DataHub/referee-data-hub.h"
#include "System/DataHub/referee-protocol.h"
#include "commander.h"
#include "pyro_bsp_uart.h"
#include "tools/crc.h"

/* II. other application */


/* III. standard lib */




/* ------- class prototypes-----------------------------------------------------------------------------------------*/




/* ------- macro -----------------------------------------------------------------------------------------------------*/

// =========================================================================
// 架构师魔法：利用宏与 Lambda 自动生成解包代码，彻底消除重复的 if(length == ...)
// =========================================================================
#define REGISTER_SIMPLE_HANDLER(CMD_ID, STRUCT_TYPE, HUB_TARGET)                                                       \
    {CMD_ID, [](const uint8_t* d, uint16_t l) {                                                                        \
         if (l == sizeof(STRUCT_TYPE)) {                                                                               \
             RefereeDataHub::instance().HUB_TARGET.write(*reinterpret_cast<const STRUCT_TYPE*>(d));                    \
         }                                                                                                             \
     }}



/* ------- variables -------------------------------------------------------------------------------------------------*/

[[maybe_unused]] static auto& forceInit = RefereeSrvc::instance();

__attribute__((section(".dma_pool"))) static uint8_t usart1RxBuf[RefereeSrvc::RX_BUFFER_SIZE];


// =========================================================================
// 裁判系统指令分发映射表 (Registry Map)
// 【警告】：这里的元素必须严格按照 CMD_ID 从小到大升序排列，以支持二分查找！
// =========================================================================
const RefereeSrvc::CmdMapItem RefereeSrvc::_handlerRegistry[] = {
    REGISTER_SIMPLE_HANDLER(0x0001, RMGameStatus, gameStatus),
    REGISTER_SIMPLE_HANDLER(0x0101, RMEventData, eventData),
    REGISTER_SIMPLE_HANDLER(0x0104, RMRefereeWarning, warningData),
    REGISTER_SIMPLE_HANDLER(0x0201, RMRobotStatus, robotStatus),
    REGISTER_SIMPLE_HANDLER(0x0202, RMPowerHeatData, powerHeat),
    REGISTER_SIMPLE_HANDLER(0x0206, RMHurtData, hurtData),
    REGISTER_SIMPLE_HANDLER(0x0207, RMShootData, shootData),
    REGISTER_SIMPLE_HANDLER(0x0208, RMProjectileAllowance, projectileAllowance),

    // 对于复杂的自定义交互包，指向专门的静态处理函数
    {0x0301, RefereeSrvc::handleInteractionData}};

// 编译期自动计算表的大小
const size_t RefereeSrvc::_registrySize = sizeof(_handlerRegistry) / sizeof(_handlerRegistry[0]);



/* ------- application attribute -------------------------------------------------------------------------------------*/

#define APPLICATION_ENABLE     true

#define APPLICATION_NAME       "Referee"

#define APPLICATION_STACK_SIZE 512

#define APPLICATION_PRIORITY   4

static StackType_t appStack[APPLICATION_STACK_SIZE];



/* ------- message interface attribute -------------------------------------------------------------------------------*/





/* ------- function prototypes ---------------------------------------------------------------------------------------*/





/* ------- function implement ----------------------------------------------------------------------------------------*/


RefereeSrvc::RefereeSrvc()
    : NotifyApp(APPLICATION_ENABLE, APPLICATION_NAME, APPLICATION_STACK_SIZE, appStack, APPLICATION_PRIORITY) {}


void RefereeSrvc::init() {
    /* driver object initialize */
    pyro::bsp_uart::get_uart1().add_rx_event_callback(
        [this](uint8_t* p, uint16_t size, BaseType_t&) -> bool {
            onUartRxEventCallback(size);
            return true;
        },
        reinterpret_cast<uint32_t>(this));
    CommanderSrvc::instance().waitInit();

    memset(usart1RxBuf, 0, sizeof(usart1RxBuf));
    HAL_UARTEx_ReceiveToIdle_DMA(&Config::Hardware::Comms::REFEREE_SYSTEM_UART, usart1RxBuf, sizeof(usart1RxBuf));
}


void RefereeSrvc::run() {
    // 1. 获取当前 DMA 的写指针位置
    // 在 STM32 中，DMA 计数器 NDTR 递减，计算出实际写入的偏移量
    uint16_t writePtr = RX_BUFFER_SIZE - __HAL_DMA_GET_COUNTER(Config::Hardware::Comms::REFEREE_SYSTEM_UART.hdmarx);

    // 2. 只要读指针没追上写指针，就说明有新数据
    while (_readPtr != writePtr) {
        // 逐字节读取 (或者按块拷贝，这里逐字节读取进状态机是最稳妥的)
        uint8_t byte = usart1RxBuf[_readPtr];
        _readPtr     = (_readPtr + 1) % RX_BUFFER_SIZE;

        // --- 状态机流式解包 ---
        switch (_state) {
            case ParseState::WAIT_SOF:
                if (byte == 0xA5) { // 协议起始字节 [cite: 1479]
                    _unpackBuf[0] = byte;
                    _unpackSize   = 1;
                    _state        = ParseState::WAIT_HEADER;
                }
                break;

            case ParseState::WAIT_HEADER:
                _unpackBuf[_unpackSize++] = byte;
                if (_unpackSize == 5) { // 帧头长度为 5 字节
                    // 校验 CRC8`

                    if (Crc::verifyCrc8(_unpackBuf, 5)) {
                        auto* header  = reinterpret_cast<RMFrameHeader*>(_unpackBuf);
                        // 计算总长度 = 帧头(5) + cmd_id(2) + data_length + CRC16(2)
                        _expectedSize = 5 + 2 + header->dataLength + 2;

                        if (_expectedSize > MAX_FRAME_SIZE) { // 保护：长度异常
                            _state = ParseState::WAIT_SOF;
                        } else {
                            _state = ParseState::WAIT_PAYLOAD;
                        }
                    } else {
                        // 校验失败，重新寻找 SOF
                        _state = ParseState::WAIT_SOF;
                    }
                }
                break;

            case ParseState::WAIT_PAYLOAD:
                _unpackBuf[_unpackSize++] = byte;
                if (_unpackSize == _expectedSize) {
                    // 整包接收完毕，校验 CRC16 [cite: 1475, 2700]
                    if (Crc::verifyCrc16(_unpackBuf, _expectedSize)) {
                        parseFrame(_unpackBuf, _expectedSize);
                    }
                    _state = ParseState::WAIT_SOF; // 准备接收下一帧
                }
                break;
        }
    }
}

void RefereeSrvc::parseFrame(const uint8_t* frame, uint16_t length) {
    // 提取 cmd_id
    uint16_t cmdId      = (frame[6] << 8) | frame[5]; // 小端序

    // 指向数据段的指针
    const uint8_t* data = frame + 7;
    auto* header        = reinterpret_cast<const RMFrameHeader*>(frame);
    uint16_t dataLen    = header->dataLength;

    // 路由分发
    dispatchCommand(cmdId, data, dataLen);
}

// =========================================================================
// 核心分发器：基于二分查找的 O(log N) 极速分发
// =========================================================================
void RefereeSrvc::dispatchCommand(uint16_t cmdId, const uint8_t* data, uint16_t length) {
    if (data == nullptr || length == 0)
        return;

    // 标准二分查找算法 (取代了底层的 O(N) switch-case 跳表)
    int left  = 0;
    int right = _registrySize - 1;

    while (left <= right) {
        int mid = left + (right - left) / 2;

        if (_handlerRegistry[mid].cmdId == cmdId) {
            // 🎯 命中！直接通过函数指针调用对应的 Lambda 或静态函数
            _handlerRegistry[mid].handler(data, length);
            return;
        }

        if (_handlerRegistry[mid].cmdId < cmdId) {
            left = mid + 1;
        } else {
            right = mid - 1;
        }
    }

    // 未知或不需要解析的 cmdId，直接静默丢弃
}

// =========================================================================
// 复杂协议的专属处理逻辑
// =========================================================================
void RefereeSrvc::handleInteractionData(const uint8_t* data, uint16_t length) {
    if (length < sizeof(RMInteractionHeader))
        return;

    auto* header           = reinterpret_cast<const RMInteractionHeader*>(data);
    const uint8_t* payload = data + sizeof(RMInteractionHeader);

    switch (header->dataCmdId) {
        case 0x0121:
            // 处理雷达交互或键鼠...
            break;
        default:
            break;
    }
}

void RefereeSrvc::onUartRxEventCallback(size_t size) {
    BaseType_t xHigherPriorityTaskWoken;
    notifyFromISR(&xHigherPriorityTaskWoken);
}
void RefereeSrvc::onUartErrCallback() {}
