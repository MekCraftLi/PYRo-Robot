/**
 *******************************************************************************
 * @file    super-cap-protocol.h
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


/* Define to prevent recursive inclusion -----------------------------------------------------------------------------*/

#ifndef INFANTRY_SUPER_CAP_PROTOCOL_H
#define INFANTRY_SUPER_CAP_PROTOCOL_H



/*-------- 1. includes and imports -----------------------------------------------------------------------------------*/

#include <cstdint>



/*-------- 2. enum and define ----------------------------------------------------------------------------------------*/




/*-------- 3. interface ----------------------------------------------------------------------------------------------*/

#pragma pack(push, 1) // 强制 1 字节对齐，防止编译器插入 Padding

// 帧头
struct CapFrameHeader {
    uint8_t sof;  // 帧头起始字节 (通常为 0xA5 等)
    uint8_t crc8; // CRC8 校验码
};

// 帧尾
struct CapFrameTail {
    uint16_t crc16; // CRC16 校验码
};

// MCU 接收的数据 (对应您代码中的 tx_data_t)
struct CapToMcuData {
    uint16_t cap_power_cap;     // 电容充放电功率
    uint16_t chassis_power_cap; // ADC测得底盘功率
    uint16_t vot_cap;           // 电容电压
    uint8_t error_flag;         // 错误标志
    uint8_t cap_low_flag;       // 电容没电标志
    uint8_t over_normal_c_l;    // 电流到达额定值标志
};

// MCU 发送的数据 (对应您代码中的 rx_data_t)
struct McuToCapData {
    uint16_t power_referee;             // 裁判系统底盘功率
    uint8_t power_limit_referee;        // 底盘功率上限
    uint8_t power_buffer_referee;       // 底盘缓冲功率
    uint8_t power_buffer_limit_referee; // 底盘缓冲功率上限
    uint8_t use_cap;                    // 是否使用电容 (1 开启, 0 关闭)
    uint8_t kill_chassis_user;          // 自杀标志
    uint8_t speed_up_user_now;          // 飞坡/加速保留位
};

// 完整接收帧 (电容 -> MCU)
struct CapRxFrame {
    CapFrameHeader header;
    CapToMcuData data;
    CapFrameTail tail;
};

// 完整发送帧 (MCU -> 电容)
struct CapTxFrame {
    CapFrameHeader header;
    McuToCapData data;
    CapFrameTail tail;
};



#pragma pack(pop) // 恢复默认内存对齐


/*-------- 4. decorator ----------------------------------------------------------------------------------------------*/




/*-------- 5. factories ----------------------------------------------------------------------------------------------*/





#endif /*INFANTRY_SUPER_CAP_PROTOCOL_H*/
