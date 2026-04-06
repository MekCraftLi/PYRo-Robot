/**
 *******************************************************************************
 * @file    crc.h
 * @brief   RoboMaster 通信协议 CRC8/CRC16 校验工具库
 *******************************************************************************
 * @attention
 *
 * none
 *
 *******************************************************************************
 * @note
 *
 * 基于 RoboMaster 官方算法，重构为 C++ 风格接口。
 * 用于图传链路、裁判系统等协议帧的解析与封装。
 *
 *******************************************************************************
 * @author  Gemini
 * @date    2026/3/3
 * @version 1.0
 *******************************************************************************
 */

#ifndef INFANTRY_TOOLS_CRC_H
#define INFANTRY_TOOLS_CRC_H

/*-------- 1. includes and imports -----------------------------------------------------------------------------------*/

#include <cstdint>

/*-------- 2. enum and define ----------------------------------------------------------------------------------------*/

/*-------- 3. interface ----------------------------------------------------------------------------------------------*/

namespace Crc {

    // 官方协议指定的 CRC 初始值
    constexpr uint8_t  CRC8_INIT_VAL  = 0xFF;
    constexpr uint16_t CRC16_INIT_VAL = 0xFFFF;

    // =========================================================
    // CRC8 接口 (用于帧头校验)
    // =========================================================

    /**
     * @brief 计算 CRC8 校验和
     * @param message 数据指针
     * @param length  数据长度
     * @param initCrc 初始值，默认 0xFF
     * @return uint8_t 计算出的 CRC8 值
     */
    uint8_t calculateCrc8(const uint8_t* message, uint32_t length, uint8_t initCrc = CRC8_INIT_VAL);

    /**
     * @brief 验证数据帧的 CRC8 校验和 (假定 CRC 在 message 的最后 1 字节)
     * @param message 包含 CRC 校验码的完整数据指针
     * @param length  完整数据的长度 (包含 CRC)
     * @return bool   校验成功返回 true
     */
    bool verifyCrc8(const uint8_t* message, uint32_t length);

    /**
     * @brief 在数据帧尾部追加 CRC8 校验和
     * @param message 数据指针 (数组长度必须至少为 length，最后 1 字节将被覆盖为 CRC)
     * @param length  完整数据的长度 (包含将要写入的 CRC)
     */
    void appendCrc8(uint8_t* message, uint32_t length);


    // =========================================================
    // CRC16 接口 (用于整包数据校验)
    // =========================================================

    /**
     * @brief 计算 CRC16 校验和
     * @param message 数据指针
     * @param length  数据长度
     * @param initCrc 初始值，默认 0xFFFF
     * @return uint16_t 计算出的 CRC16 值
     */
    uint16_t calculateCrc16(const uint8_t* message, uint32_t length, uint16_t initCrc = CRC16_INIT_VAL);

    /**
     * @brief 验证数据帧的 CRC16 校验和 (假定 CRC 在 message 的最后 2 字节，小端序)
     * @param message 包含 CRC 校验码的完整数据指针
     * @param length  完整数据的长度 (包含 CRC)
     * @return bool   校验成功返回 true
     */
    bool verifyCrc16(const uint8_t* message, uint32_t length);

    /**
     * @brief 在数据帧尾部追加 CRC16 校验和 (小端序写入)
     * @param message 数据指针 (数组长度必须至少为 length，最后 2 字节将被覆盖为 CRC)
     * @param length  完整数据的长度 (包含将要写入的 CRC)
     */
    void appendCrc16(uint8_t* message, uint32_t length);

} // namespace Crc

#endif /*INFANTRY_TOOLS_CRC_H*/