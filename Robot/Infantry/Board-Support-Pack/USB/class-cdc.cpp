/**
 *******************************************************************************
 * @file    class-cdc.cpp
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


/* ------- define ----------------------------------------------------------------------------------------------------*/





/* ------- include ---------------------------------------------------------------------------------------------------*/

#include "class-cdc.h"

#include "device/usbd.h"
#include "device/usbd_pvt.h"



/* ------- class prototypes-------------------------------------------------------------------------------------------*/





/* ------- macro -----------------------------------------------------------------------------------------------------*/





/* ------- variables -------------------------------------------------------------------------------------------------*/





/* ------- function implement ----------------------------------------------------------------------------------------*/

void UsbCdc::init() {}

void UsbCdc::reset(uint8_t rhport) {
    // 收到 USB Bus Reset的回调
}

uint16_t UsbCdc::open(uint8_t rhport, tusb_desc_interface_t const * itf_desc, uint16_t maxLen) {
    // 这个位置TinyUSB将配置描述符传进来
    // 在这里调用usbd_edpt_open以打开物理端点(0x81, 0x02, 0x82)

    // 这里的返回值返回的是取走的描述符字节数
    // 75Byte的描述符，减去9Byte的配置头，其余全部取走
    // 1. 安全检查：如果传进来的不是 CDC 控制接口 (0x02)，直接忽略
    if (itf_desc->bInterfaceClass != 0x02) return 0;

    // 2. 唤醒 STM32 底层硬件端点！(极其重要)
    // 必须和你在描述符里写的地址、类型、大小一模一样

    // 打开 0x81 中断通知端点 (Notif EP)
    tusb_desc_endpoint_t ep_notif;
    ep_notif.bLength = 7;
    ep_notif.bDescriptorType = TUSB_DESC_ENDPOINT;
    ep_notif.bEndpointAddress = 0x81;
    ep_notif.bmAttributes.xfer = TUSB_XFER_INTERRUPT;
    ep_notif.wMaxPacketSize = 8;
    ep_notif.bInterval = 16;
    usbd_edpt_open(rhport, &ep_notif);

    // 打开 0x02 批量接收端点 (OUT EP)
    tusb_desc_endpoint_t ep_out;
    ep_out.bLength = 7;
    ep_out.bDescriptorType = TUSB_DESC_ENDPOINT;
    ep_out.bEndpointAddress = 0x02;
    ep_out.bmAttributes.xfer = TUSB_XFER_BULK;
    ep_out.wMaxPacketSize = 64;
    ep_out.bInterval = 0;
    usbd_edpt_open(rhport, &ep_out);

    // 打开 0x82 批量发送端点 (IN EP)
    tusb_desc_endpoint_t ep_in;
    ep_in.bLength = 7;
    ep_in.bDescriptorType = TUSB_DESC_ENDPOINT;
    ep_in.bEndpointAddress = 0x82;
    ep_in.bmAttributes.xfer = TUSB_XFER_BULK;
    ep_in.wMaxPacketSize = 64;
    ep_in.bInterval = 0;
    usbd_edpt_open(rhport, &ep_in);

    // 3. 返回精确消耗的字节数：
    // Interface0(9) + FuncDesc(19) + EP_NOTIF(7) + Interface1(9) + EP_OUT(7) + EP_IN(7) = 58 字节
    return 58;
}


// setup控制包拦截
bool UsbCdc::controlXferCb(uint8_t rhport, uint8_t stage, tusb_control_request_t const * request) {
    // 发送的设置波特率(0x20)、打开串口(0x22)
    // 拦截面向 CDC 类 (Class) 的特殊请求
    if (request->bmRequestType_bit.type == TUSB_REQ_TYPE_CLASS) {

        // 动作 A: 电脑上的串口助手点击了“打开串口”
        if (request->bRequest == 0x22) { // SET_CONTROL_LINE_STATE
            if (stage == CONTROL_STAGE_SETUP) {
                // 向电脑回复一个长度为 0 的 ACK 状态包，电脑就会显示“已连接”
                tud_control_status(rhport, request);
            }
            return true;
        }

        // 动作 B: 电脑下发波特率 (115200 等)
        if (request->bRequest == 0x20) { // SET_LINE_CODING
            if (stage == CONTROL_STAGE_SETUP) {
                // 我们这儿偷个懒，不解析波特率了（因为虚拟串口跑的是 USB 物理极速，波特率没意义）
                // 直接告诉电脑：“好的，请求放行”
                tud_control_status(rhport, request);
            }
            return true;
        }
    }

    // 不认识的请求直接返回 false，TinyUSB 会自动回复 STALL (拒绝)
    return false;

}

bool UsbCdc::xferCb(uint8_t rhport, uint8_t ep_addr, xfer_result_t result, uint32_t xferred_bytes) {

    // 接收到数据帧或者发送完数据之后的回调函数
    return true;
}

// 打包成驱动结构体
usbd_class_driver_t const cdcDriver = {
.init = UsbCdc::init,
    .reset = UsbCdc::reset,
    .open = UsbCdc::open,
    .control_xfer_cb = UsbCdc::controlXferCb,
    .xfer_cb = UsbCdc::xferCb,
    .sof = nullptr
};

usbd_class_driver_t const* usbd_app_driver_get_cb(uint8_t* driver_count) {
    // driver_count 驱动数量
    *driver_count = 1;

    // 返回驱动指针
    return &cdcDriver;
}


