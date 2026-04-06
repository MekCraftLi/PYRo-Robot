/**
 *******************************************************************************
 * @file    ui-renderer-srvc.cpp
 * @brief   裁判系统 UI 渲染服务实现 (UI Renderer Service)
 *******************************************************************************
 * @attention
 * 实现了无锁的图形渲染指令入队，以及自适应 1/2/5/7 批量封包发送机制。
 *******************************************************************************
 * @author  MekLi
 * @date    2026/3/19
 * @version 1.0
 *******************************************************************************
 */

/* ------- include -----------------------------------------------------------*/
#include "ui-renderer-srvc.h"

#include "Config/Chassis/hw-config.h"
#include "System/DataHub/referee-data-hub.h" // 假设这里包含底层的发包接口
#include "tools/crc.h"

#include <cstring>

/* ------- variables ---------------------------------------------------------*/

[[maybe_unused]] static auto& forceInit = UiRendererSrvc::instance();
// 定义 DMA 专用的发送缓冲区，放置在不可 Cache 的内存段中防 DMA 脏数据
__attribute__((section(".dma_pool"))) static uint8_t uiTxBuffer[256];


#define APPLICATION_ENABLE     true
#define APPLICATION_NAME       "UiRenderer"
#define APPLICATION_STACK_SIZE 1024
#define APPLICATION_PRIORITY   4 // 渲染服务需要较高的优先级，保证UI不卡顿

static StackType_t appStack[APPLICATION_STACK_SIZE];

/* ------- function implement ------------------------------------------------*/

// 构造函数：10ms 周期 (100Hz)，高频消费队列，防止指令积压
UiRendererSrvc::UiRendererSrvc()
    : PeriodicApp(APPLICATION_ENABLE, APPLICATION_NAME, APPLICATION_STACK_SIZE, appStack, APPLICATION_PRIORITY, 50) {
}

void UiRendererSrvc::init() {
    // 创建深度为 35 的渲染管线队列 (足够缓存 5 帧 7图形满载包)
    _renderQueue = xQueueCreate(35, sizeof(RMInteractionFigurePayload));
}

void UiRendererSrvc::run() {
    if (_renderQueue == nullptr) return;

    UBaseType_t waitingCount = uxQueueMessagesWaiting(_renderQueue);
    if (waitingCount == 0) return;

    // --- 新增：1. 屏障拦截 (偷窥队首指令) ---
    RMInteractionFigurePayload peekPayload;
    xQueuePeek(_renderQueue, &peekPayload, 0);

    // 检查是否是我们在 clearAllFast() 中设置的“魔法全清指令”
    if (peekPayload.graphicName[0] == 0xFF && peekPayload.graphicName[1] == 0xFF) {

        // 确认是全清指令，将其正式从队列取出
        xQueueReceive(_renderQueue, &peekPayload, 0);

        // 构造 0x0100 删除操作的 2 字节数据段
        static struct {
            uint8_t deleteType;
            uint8_t layer;
        } __attribute__((packed)) delData = {2, 0}; // 2: 删除所有

        // 复用你的协议封装逻辑发包
        _sendCustomPacket(0x0100, reinterpret_cast<uint8_t*>(&delData), sizeof(delData));

        return; // 全清包发送完毕，直接结束当前周期
    }

    // --- 2. 原有的大疆自适应批量打包算法 (1/2/5/7) ---
    uint8_t batchSize = 0;
    uint16_t subCmdId = 0;

    if (waitingCount >= 7) {
        batchSize = 7; subCmdId = 0x0104;
    } else if (waitingCount >= 5) {
        batchSize = 5; subCmdId = 0x0103;
    } else if (waitingCount >= 2) {
        batchSize = 2; subCmdId = 0x0102;
    } else {
        batchSize = 1; subCmdId = 0x0101;
    }

    // 从队列中取出图形
    RMInteractionFigurePayload payloadBatch[7];
    for (uint8_t i = 0; i < batchSize; ++i) {
        // 如果在批量取出的过程中，不慎遇到了全清魔法指令怎么办？
        // 因为我们在 clearAllFast 中使用了 xQueueReset，所以遇到全清时它绝对是队首！
        // 也就是一定会在上面的 Peek 阶段被拦截，不会混入普通的 1/2/5/7 批处理中，非常安全。
        xQueueReceive(_renderQueue, &payloadBatch[i], 0);
    }

    _sendCustomPacket(subCmdId, payloadBatch, sizeof(RMInteractionFigurePayload) * batchSize);

}



void UiRendererSrvc::_sendCustomPacket(uint16_t subCmdId, void* payloadData, uint16_t payloadLen) {
    static uint8_t seqCounter = 0;
    uint16_t dataSegmentLen = sizeof(RmInteractiveHeader) + payloadLen;

    RMRobotStatus robotStatus{};
    RefereeDataHub::instance().robotStatus.read(robotStatus);
    uint16_t senderId   = 3; // 你的原有逻辑
    uint16_t receiverId = 3 + 0x0100;

    uint32_t offset = 0;

    // 组装帧头
    RmFrameHeader header{};
    header.sof = 0xA5;
    header.dataLength = dataSegmentLen;
    header.seq = seqCounter++;
    std::memcpy(&uiTxBuffer[offset], &header, 4);
    Crc::appendCrc8(uiTxBuffer, 5);
    offset += sizeof(RmFrameHeader);

    // 组装命令码
    uint16_t cmdId = 0x0301;
    std::memcpy(&uiTxBuffer[offset], &cmdId, sizeof(cmdId));
    offset += sizeof(cmdId);

    // 组装交互头
    RmInteractiveHeader interactHeader{};
    interactHeader.subCmdId   = subCmdId;
    interactHeader.senderId   = senderId;
    interactHeader.receiverId = receiverId;
    std::memcpy(&uiTxBuffer[offset], &interactHeader, sizeof(RmInteractiveHeader));
    offset += sizeof(RmInteractiveHeader);

    // 组装载荷数据 (2字节 或 N*15字节)
    std::memcpy(&uiTxBuffer[offset], payloadData, payloadLen);
    offset += payloadLen;

    // 组装 CRC16
    uint16_t frameTotalLength = offset + 2;
    Crc::appendCrc16(uiTxBuffer, frameTotalLength);

    // 触发 DMA
    HAL_UART_Transmit_DMA(&Config::Hardware::Comms::REFEREE_SYSTEM_UART, uiTxBuffer, frameTotalLength);
}



// -----------------------------------------------------------------------------
// 内部渲染管线工具
// -----------------------------------------------------------------------------

void UiRendererSrvc::_applyProperties(RMInteractionFigurePayload& payload, const GraphicProperties& props, GraphicType type) {
    std::memcpy(payload.graphicName, props.name, 3);
    payload.opt = static_cast<uint32_t>(props.action);
    payload.type   = static_cast<uint32_t>(type);
    payload.layer  = props.layer;
    payload.color  = static_cast<uint32_t>(props.color);
    payload.width = props.lineWidth;
    payload.startX = props.startX;
    payload.startY = props.startY;

    // 初始化其余位域，防止脏数据
    payload.param1 = 0; payload.param2 = 0;
    payload.param3 = 0; payload.endX   = 0; payload.endY = 0;
}

void UiRendererSrvc::_submitToPipeline(const RMInteractionFigurePayload& payload) {
    if (_renderQueue != nullptr) {
        xQueueSend(_renderQueue, &payload, portMAX_DELAY /* 阻塞直到有空间 */);
    }
}

// -----------------------------------------------------------------------------
// UI 图形绘制 API 实现
// -----------------------------------------------------------------------------

void UiRendererSrvc::drawLine(const GraphicProperties& props, uint16_t endX, uint16_t endY) {
    RMInteractionFigurePayload payload{};
    _applyProperties(payload, props, GraphicType::Line);
    payload.endX = endX;
    payload.endY = endY;
    _submitToPipeline(payload);
}

void UiRendererSrvc::drawRectangle(const GraphicProperties& props, uint16_t endX, uint16_t endY) {
    RMInteractionFigurePayload payload{};
    _applyProperties(payload, props, GraphicType::Rectangle);
    payload.endX = endX;
    payload.endY = endY;
    _submitToPipeline(payload);
}

void UiRendererSrvc::drawCircle(const GraphicProperties& props, uint16_t radius) {
    RMInteractionFigurePayload payload{};
    _applyProperties(payload, props, GraphicType::Circle);
    // RM协议中，画圆的半径存在 detailsD (重构后的 endX) 中
    payload.endX = radius;
    _submitToPipeline(payload);
}

void UiRendererSrvc::drawEllipse(const GraphicProperties& props, uint16_t radiusX, uint16_t radiusY) {
    RMInteractionFigurePayload payload{};
    _applyProperties(payload, props, GraphicType::Ellipse);
    payload.endX = radiusX; // 原 detailsD
    payload.endY = radiusY; // 原 detailsE
    _submitToPipeline(payload);
}

void UiRendererSrvc::drawArc(const GraphicProperties& props, uint16_t radiusX, uint16_t radiusY, uint16_t startAngle, uint16_t endAngle) {
    RMInteractionFigurePayload payload{};
    _applyProperties(payload, props, GraphicType::Arc);
    payload.param1 = startAngle; // 原 detailsA
    payload.param2 = endAngle;   // 原 detailsB
    payload.endX   = radiusX;    // 原 detailsD
    payload.endY   = radiusY;    // 原 detailsE
    _submitToPipeline(payload);
}

// -----------------------------------------------------------------------------
// 数值与文本渲染 API 实现
// -----------------------------------------------------------------------------

void UiRendererSrvc::drawFloat(const GraphicProperties& props, uint16_t fontSize, float value) {
    RMInteractionFigurePayload payload{};
    _applyProperties(payload, props, GraphicType::Float);
    payload.param1 = fontSize; // 原 detailsA

    // 浮点数拆解：乘 1000 后转整型，拆分到 3 个字段中
    uint32_t valTemp = static_cast<uint32_t>(value * 1000.0f);
    payload.param3 = valTemp & 0x3FF;             // 原 detailsC (10 bit)
    payload.endX   = (valTemp >> 10) & 0x7FF;     // 原 detailsD (11 bit)
    payload.endY   = (valTemp >> 21) & 0x7FF;     // 原 detailsE (11 bit)

    _submitToPipeline(payload);
}

void UiRendererSrvc::drawInt(const GraphicProperties& props, uint16_t fontSize, int32_t value) {
    RMInteractionFigurePayload payload{};
    _applyProperties(payload, props, GraphicType::Int);
    payload.param1 = fontSize; // 原 detailsA

    // 整数拆解
    payload.param3 = value & 0x3FF;               // 原 detailsC (10 bit)
    payload.endX   = (value >> 10) & 0x7FF;       // 原 detailsD (11 bit)
    payload.endY   = (value >> 21) & 0x7FF;       // 原 detailsE (11 bit)

    _submitToPipeline(payload);
}

// -----------------------------------------------------------------------------
// 控制 API 实现
// -----------------------------------------------------------------------------

void UiRendererSrvc::clearGraphic(GraphicDelMode mode, const uint8_t* graphicName) {
    RMInteractionFigurePayload payload{};
    payload.opt = static_cast<uint32_t>(GraphicOption::Delete);

    switch (mode) {
        case GraphicDelMode::Name:
            if (graphicName) { std::memcpy(payload.graphicName, graphicName, 3); }
            break;

        case GraphicDelMode::Layer:
            payload.color = static_cast<uint32_t>(UiColor::Del);
            payload.layer = graphicName ? graphicName[0] : 0; // 借用 graphicName[0] 传图层号
            break;

        case GraphicDelMode::All:
            if (_renderQueue == nullptr) return;

            // 【核心魔法】空间震优化：O(1) 瞬间清空所有积压的无用绘制指令
            xQueueReset(_renderQueue);

            // 使用非法的 ASCII 字符 0xFF 作为全清的魔法标识
            payload.graphicName[0] = 0xFF;
            payload.graphicName[1] = 0xFF;
            payload.graphicName[2] = 0xFF;

            // 复用 action 字段，但实际上后台任务检测到 0xFF 就会拦截
            payload.opt = static_cast<uint32_t>(GraphicOption::Delete);

            // 将全清指令压入队列，由于刚刚 Reset 过，它一定是队列的第一个
            _submitToPipeline(payload);

        default:
            break;
    }

    _submitToPipeline(payload);
}