/**
 *******************************************************************************
 * @file    tusb_config.h
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
 * @date    2026/2/6
 * @version 1.0
 *******************************************************************************
 */


/* Define to prevent recursive inclusion -----------------------------------------------------------------------------*/

#ifndef INFANTRY_CHASSIS_TUSB_CONFIG_H
#define INFANTRY_CHASSIS_TUSB_CONFIG_H

#ifdef __cplusplus
extern "C" {
#endif


/*-------- 1. includes and imports -----------------------------------------------------------------------------------*/




/*-------- 2. enum and define ----------------------------------------------------------------------------------------*/


#define CFG_TUSB_MCU              OPT_MCU_STM32H7
#define CFG_TUSB_OS               OPT_OS_FREERTOS

// ★ 关键：设置 Debug 级别为 2，TinyUSB 会通过 printf 打印出所有的控制传输请求！
// 请确保你的工程已经重定向了 printf 到串口
#define CFG_TUSB_DEBUG            0
// 启用 TinyUSB Device
#define CFG_TUD_ENABLED           1

// 配置 RHPORT 1 为设备模式，且使用内部全速 PHY
#define CFG_TUSB_RHPORT1_MODE     (OPT_MODE_DEVICE | OPT_MODE_FULL_SPEED)
#define CFG_TUD_DWC2_SLAVE_ENABLE 1
// 你的端点0大小
#define CFG_TUD_ENDPOINT0_SIZE    64


// ★ 关键：关闭所有的标准 USB 类
#define CFG_TUD_CDC               1
#define CFG_TUD_MSC               0
#define CFG_TUD_HID               0
#define CFG_TUD_MIDI              0
#define CFG_TUD_VENDOR            0 // 即使我们用Vendor类，这里也设为0，因为我们连Vendor的端点都不想处理

#define BOARD_TUD_RHPORT          0
#define BOARD_TUD_MAX_SPEED       OPT_MODE_DEFAULT_SPEED

/* -------- CDC --------------*/
// 3. 配置 CDC 相关的端点 FIFO 缓冲区大小 (全速模式下通常为 64)
#define CFG_TUD_CDC_RX_BUFSIZE    64
#define CFG_TUD_CDC_TX_BUFSIZE    64
#define CFG_TUD_CDC_EP_BUFSIZE    64

/*-------- 3. interface ----------------------------------------------------------------------------------------------*/




/*-------- 4. decorator ----------------------------------------------------------------------------------------------*/




/*-------- 5. factories ----------------------------------------------------------------------------------------------*/



#ifdef __cplusplus
}
#endif

#endif /*INFANTRY_CHASSIS_TUSB_CONFIG_H*/
