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

#include "pyro_can_drv.h"
#include "pyro_dji_motor_drv.h"
#include "usart.h"




/*-------- 2. enum and define ----------------------------------------------------------------------------------------*/

namespace Config::Hardware {

constexpr uint32_t SYSTEM_CLOCK_HZ = 480000000;


// ========================================
// 1. 底盘机械参数 (单位: 米/弧度)
// ========================================
namespace Chassis {
constexpr float WHEEL_BASE       = 0.36f;          // 前后轴距
constexpr float TRACK_WIDTH      = 0.33f;          // 左右轮距
constexpr float WHEEL_RADIUS     = 0.06f;          // 轮子半径
constexpr float DRIVE_GEAR_RATIO = 268.0f / 17.0f; // 动力轮减速比 (M3508)
} // namespace Chassis

// ========================================
// 2. 电机 CAN 总线拓扑映射
// ========================================
namespace MotorTopo {
// 定义四个舵轮底盘的电机 ID [右前, 左前, 左后, 右后]
// 注意：底层驱动通常从 0 开始算，所以如果是大疆电调上的 1~4 号，这里可以写 0~3
constexpr uint8_t DRIVE_MOTOR_IDS[4]                           = {1, 3, 2, 4}; // 对应大疆的 3, 2, 4, 1

// 动力轮挂载的 CAN 总线
constexpr pyro::can_hub_t::which_can DRIVE_MOTOR_CANS[4]       = {pyro::can_hub_t::can2, pyro::can_hub_t::can1,
                                                                  pyro::can_hub_t::can2, pyro::can_hub_t::can1};

// 航向舵挂载的 CAN 总线 (假设与动力轮一致)
constexpr pyro::can_hub_t::which_can STEER_MOTOR_CANS[4]       = {pyro::can_hub_t::can2, pyro::can_hub_t::can1,
                                                                  pyro::can_hub_t::can2, pyro::can_hub_t::can1};

constexpr pyro::can_hub_t::which_can COMM_CAN                  = pyro::can_hub_t::can3;

constexpr pyro::dji_motor_tx_frame_t::register_id_t YAW_ID     = pyro::dji_motor_tx_frame_t::id_5;
constexpr pyro::dji_motor_tx_frame_t::register_id_t TRIGGER_ID = pyro::dji_motor_tx_frame_t::id_3;

constexpr uint16_t STEER_ECD_OFFSET[4]                         = {1122, 7202, 4052, 2474+4096};
constexpr uint16_t YAW_OFFSET                                  = 0x051b
;

} // namespace MotorTopo

// ========================================
// 3. 核心外设映射
// ========================================
namespace Comms {
// 遥控器 DR16 接收串口
inline constexpr UART_HandleTypeDef& REMOTE_UART  = huart5;
inline constexpr UART_HandleTypeDef& REFEREE_SYSTEM_UART = huart1;
inline constexpr FDCAN_HandleTypeDef& BOARD_COMM_CAN = hfdcan3;
inline constexpr UART_HandleTypeDef& SUPER_CAP_UART = huart7;
} // namespace Comms


} // namespace Config::Hardware


/*-------- 3. interface ----------------------------------------------------------------------------------------------*/




/*-------- 4. decorator ----------------------------------------------------------------------------------------------*/




/*-------- 5. factories ----------------------------------------------------------------------------------------------*/





#endif /*INFANTRY_HW_CONFIG_H*/
