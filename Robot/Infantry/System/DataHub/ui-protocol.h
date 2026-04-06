/**
*******************************************************************************
* @file    ui-protocol.h
* @brief   裁判系统 UI 绘制底层协议结构定义 (Semantic Ver.)
*******************************************************************************
*/

#ifndef INFANTRY_UI_PROTOCOL_H
#define INFANTRY_UI_PROTOCOL_H

#include <cstdint>

enum class RMSubCmdId : uint16_t {
    DeleteLayer = 0x0100,
    RenderOne   = 0x0101,
    RenderTwo   = 0x0102,
    RenderFive  = 0x0103,
    RenderSeven = 0x0104,
    RenderChar  = 0x0110,
};



// 1. 图形类型 (Shape Type)
enum class GraphicType : uint8_t {
    Line      = 0,
    Rectangle = 1,
    Circle    = 2,
    Ellipse   = 3,
    Arc       = 4,
    Float     = 5,
    Int       = 6,
    String    = 7
};

// 2. 删除模式 (Delete Mode)
enum class GraphicDelMode : uint8_t { Null = 0, Layer = 1, All = 2, Name = 3 };

// 3. 渲染操作 (Render Action)
enum class GraphicOption : uint8_t { Null = 0, Add = 1, Update = 2, Delete = 3 };

// 4. UI 颜色预设
enum class UiColor : uint8_t {
    Main   = 0,
    Yellow = 1,
    Green  = 2,
    Orange = 3,
    Purple = 4,
    Pink   = 5,
    Cyan   = 6,
    Black  = 7,
    White  = 8,
    Del    = 9
};

#pragma pack(push, 1)

// 5. RM 官方底层图形数据载荷 (Graphic Data Payload)
struct RMInteractionFigurePayload {
    uint8_t graphicName[3]; // 图形索引名
    uint32_t opt    : 3;    // 对应 GraphicAction
    uint32_t type   : 3;    // 对应 GraphicType
    uint32_t layer  : 4;    // 图层号 (0-9)
    uint32_t color  : 4;    // 颜色
    uint32_t param1 : 9;    // 起始角度 / 字体大小
    uint32_t param2 : 9;    // 终止角度
    uint32_t width  : 10;   // 线宽
    uint32_t startX : 11;   // 起点 X / 圆心 X
    uint32_t startY : 11;   // 起点 Y / 圆心 Y
    uint32_t param3 : 10;   // 半径 X / 浮点小数部分
    uint32_t endX   : 11;   // 终点 X / 半径 Y
    uint32_t endY   : 11;   // 终点 Y / 浮点整数部分
};

struct RMInteractionLayerDeletePayload {
    GraphicDelMode delMode;
    uint8_t layer;
};


// 1. 裁判系统标准通信帧头 (5 Byte)
struct RmFrameHeader {
    uint8_t sof;         // 起始字节 (固定为 0xA5)
    uint16_t dataLength; // 数据段长度
    uint8_t seq;         // 包序号
    uint8_t crc8;        // 帧头 CRC8 校验
};

// 2. UI 交互数据专有段头 (6 Byte)
struct RmInteractiveHeader {
    uint16_t subCmdId;   // 交互子命令码 (决定是 1/2/5/7 个图形)
    uint16_t senderId;   // 发送方 ID (本机器人 ID)
    uint16_t receiverId; // 接收方 ID (对应的操作手客户端 ID)
};


#pragma pack(pop)

// 6. 业务层图形配置属性 (Graphic Properties)
// 用于向下传参，避免每次都写重复的图层、颜色等信息
struct GraphicProperties {
    uint8_t name[3];
    GraphicOption action;
    uint8_t layer;
    UiColor color;
    uint16_t lineWidth;
    uint16_t startX;
    uint16_t startY;
};

#endif /* INFANTRY_UI_PROTOCOL_H */