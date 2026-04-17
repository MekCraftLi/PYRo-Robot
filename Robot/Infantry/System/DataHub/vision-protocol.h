/**
 *******************************************************************************
 * @file    vision-protocol.h
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


/* Define to prevent recursive inclusion -----------------------------------------------------------------------------*/

#ifndef INFANTRY_VISION_PROTOCOL_H
#define INFANTRY_VISION_PROTOCOL_H



/*-------- 1. includes and imports -----------------------------------------------------------------------------------*/

#include <cstdint>



/*-------- 2. enum and define ----------------------------------------------------------------------------------------*/


#pragma pack(push, 1)

// ============================================================================
// 1. 核心业务数据体
// ============================================================================

/**
 * @brief STM32 发送给视觉上位机的遥测状态数据 (原 InputData)
 */
struct VisionTelemetry {
    float   currentYaw;
    float   currentPitch;
    float   selfVelocityMagnitude;
    float   selfVelocityAngle;
    float initialSpeed;
    uint8_t shootDelay;


    // 位域保持原有的内存布局分布
    uint8_t robotState  : 5;
    uint8_t stopRecord  : 1;
    uint8_t autoAimMode : 1;
    uint8_t enemyColor  : 1;
};

/**
 * @brief 视觉上位机发送给 STM32 的控制指令数据 (原 OutputData)
 */
struct VisionCommand {
    float   targetYaw;
    float   targetYawSpeed;
    float   targetYawAcceleration;
    float   targetPitch;
    float   targetPitchSpeed;
    float   targetPitchAcceleration;

    // 位域保持原有的内存布局分布
    uint8_t fireCommand  : 1;
    uint8_t isSingleShot : 1;
    uint8_t targetId     : 6;

    uint8_t aimState;
};

// ============================================================================
// 2. 物理层数据帧包装 (防止粘包与错位)
// ============================================================================

constexpr uint8_t VISION_SOF = 0xA5; // 帧头起始符

struct VisionTxFrame {
    uint8_t         sof = VISION_SOF;
    VisionTelemetry payload;
    uint16_t        crc16;
};

struct VisionRxFrame {
    uint8_t         sof;
    VisionCommand   payload;
    uint16_t        crc16;
};

struct shootDataDebug {
    float initialSpeed;
    float fircTgt;
    float fircFdb;
    float trigSpd;
    float trigtrq;
};

#pragma pack(pop)


/*-------- 3. interface ----------------------------------------------------------------------------------------------*/




/*-------- 4. decorator ----------------------------------------------------------------------------------------------*/




/*-------- 5. factories ----------------------------------------------------------------------------------------------*/





#endif /*INFANTRY_VISION_PROTOCOL_H*/
