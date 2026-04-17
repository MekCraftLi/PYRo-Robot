/**
 *******************************************************************************
 * @file    hw-config.h
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
 * @date    2026/3/1
 * @version 1.0
 *******************************************************************************
 */


/* Define to prevent recursive inclusion -----------------------------------------------------------------------------*/

#ifndef INFANTRY_HW_CONFIG_H
#define INFANTRY_HW_CONFIG_H




/*-------- 1. includes and imports -----------------------------------------------------------------------------------*/

#include "main.h"
#include "pyro_can_drv.h"
#include "pyro_dji_motor_drv.h"
#include "usart.h"



/*-------- 2. enum and define ----------------------------------------------------------------------------------------*/

namespace Config::Hardware {

constexpr uint32_t SYSTEM_CLOCK_HZ = 480000000;


// ========================================
// 1. 底盘机械参数 (单位: 米/弧度)
// ========================================

// ========================================
// 2. 电机 CAN 总线拓扑映射
// ========================================
namespace MotorTopo {
constexpr pyro::can_hub_t::which_can FRIC_LEFT_CAN                = pyro::can_hub_t::can2;
constexpr pyro::can_hub_t::which_can FRIC_RIGHT_CAN               = pyro::can_hub_t::can2;
constexpr pyro::can_hub_t::which_can PITCH_CAN                    = pyro::can_hub_t::can1;
constexpr pyro::can_hub_t::which_can YAW_CAN                      = pyro::can_hub_t::can1;
constexpr pyro::can_hub_t::which_can TRIGGER_CAN                  = pyro::can_hub_t::can2;

constexpr pyro::dji_motor_tx_frame_t::register_id_t FRIC_LEFT_ID  = pyro::dji_motor_tx_frame_t::id_1;
constexpr pyro::dji_motor_tx_frame_t::register_id_t FRIC_RIGHT_ID = pyro::dji_motor_tx_frame_t::id_2;
constexpr pyro::dji_motor_tx_frame_t::register_id_t YAW_ID        = pyro::dji_motor_tx_frame_t::id_5;
constexpr pyro::dji_motor_tx_frame_t::register_id_t TRIGGER_ID    = pyro::dji_motor_tx_frame_t::id_3;
constexpr pyro::dji_motor_tx_frame_t::register_id_t PITCH         = pyro::dji_motor_tx_frame_t::id_5;


constexpr uint16_t YAW_OFFSET                                     = 1526;

// 弹丸初速度
constexpr float PROJECTILE_TARGET_MUZZLE_VELOCITY                 = 23.5f;

// 弹速调整系数
constexpr float FRIC_ADJUST_K                                     = 0.85f;
// 摩擦轮半径
constexpr float FRIC_RADIUS                                       = 0.03f;

// 发射速度 (发/秒)
constexpr float SHOOT_SPEED                                       = 15.0f;

// 拨弹盘速度
constexpr float TRIGGER_SPEED                                     = SHOOT_SPEED / 8 * 2 * M_PI * 36;

constexpr float FRIC_TARGET_SPEED = PROJECTILE_TARGET_MUZZLE_VELOCITY / FRIC_RADIUS * FRIC_ADJUST_K;

} // namespace MotorTopo


// ========================================
// 3. 核心外设映射
// ========================================
namespace Comms {
// 遥控器 DR16 接收串口
#if REMOTE_DEVICE == REMOTE_DR16
inline constexpr UART_HandleTypeDef& REMOTE_UART = huart5;
#elif REMOTE_DEVICE == REMOTE_VIDEO_LINK
inline constexpr UART_HandleTypeDef& REMOTE_UART = huart1;
#elif REMOTE_DEVICE == REMOTE_GAMEPAD
inline constexpr UART_HandleTypeDef& REMOTE_UART = huart7;
#endif
inline constexpr UART_HandleTypeDef& VISION_UART     = huart7;


inline constexpr FDCAN_HandleTypeDef& BOARD_COMM_CAN = hfdcan1;

} // namespace Comms
} // namespace Config::Hardware


/*-------- 3. interface ----------------------------------------------------------------------------------------------*/




/*-------- 4. decorator ----------------------------------------------------------------------------------------------*/




/*-------- 5. factories ----------------------------------------------------------------------------------------------*/





#endif /*INFANTRY_HW_CONFIG_H*/
