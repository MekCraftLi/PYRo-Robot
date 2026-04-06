/**
 *******************************************************************************
 * @file    usb-descriptors.c
 * @brief   USB描述符
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


/* ------- define ----------------------------------------------------------------------------------------------------*/





/* ------- include ---------------------------------------------------------------------------------------------------*/

#include "device/usbd.h"
#include "tusb.h"




/* ------- class prototypes-------------------------------------------------------------------------------------------*/





/* ------- macro -----------------------------------------------------------------------------------------------------*/





/* ------- variables -------------------------------------------------------------------------------------------------*/
// 1. 设备描述符 (标准虚拟串口参数)
tusb_desc_device_t const desc_device = {
    .bLength            = sizeof(tusb_desc_device_t),
    .bDescriptorType    = TUSB_DESC_DEVICE,
    .bcdUSB             = 0x0200,
    .bDeviceClass       = TUSB_CLASS_MISC, // 复合设备
    .bDeviceSubClass    = MISC_SUBCLASS_COMMON,
    .bDeviceProtocol    = MISC_PROTOCOL_IAD,
    .bMaxPacketSize0    = CFG_TUD_ENDPOINT0_SIZE,
    .idVendor           = 0x0483, // ST 默认 VID
    .idProduct          = 0x5740, // 虚拟串口默认 PID
    .bcdDevice          = 0x0100,
    .iManufacturer      = 0x01,
    .iProduct           = 0x02,
    .iSerialNumber      = 0x00,
    .bNumConfigurations = 0x01
};

// 2. 配置描述符 (利用宏一键生成！)
#define EPNUM_CDC_NOTIF   0x81
#define EPNUM_CDC_OUT     0x02
#define EPNUM_CDC_IN      0x82

// 总长度 = 9字节配置头 + 58字节的CDC接口集合
#define CONFIG_TOTAL_LEN  (TUD_CONFIG_DESC_LEN + TUD_CDC_DESC_LEN)

uint8_t const desc_configuration[] = {
    // 包含 2 个接口 (Interface 0 和 1)
    TUD_CONFIG_DESCRIPTOR(1, 2, 0, CONFIG_TOTAL_LEN, 0x00, 100),

    // 一行代码搞定 CDC 的所有端点和功能描述符配置！
    // 参数：接口号(0), 字符串索引(0), 通知端点(0x81), 通知包长(8), 数据OUT端点(0x02), 数据IN端点(0x82), 数据最大包长(64)
    TUD_CDC_DESCRIPTOR(0, 0, EPNUM_CDC_NOTIF, 8, EPNUM_CDC_OUT, EPNUM_CDC_IN, 64)
};

// 3. 回调函数暴露给 C 核心
#ifdef __cplusplus
extern "C" {
#endif

    uint8_t const * tud_descriptor_device_cb(void) { return (uint8_t const *) &desc_device; }
    uint8_t const * tud_descriptor_configuration_cb(uint8_t index) { return desc_configuration; }
    uint16_t const* tud_descriptor_string_cb(uint8_t index, uint16_t langid) { return NULL; } // 偷懒不回传字符串

#ifdef __cplusplus
}
#endif