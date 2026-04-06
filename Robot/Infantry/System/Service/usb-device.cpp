/**
 *******************************************************************************
 * @file    usb-device.cpp
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



/* I. header */

#include "usb-device.h"

#include "Board-Support-Pack/USB/dev-usb.h"
#include "device/usbd.h"
#include "stm32h723xx.h"

/* II. other application */


/* III. standard lib */




/* ------- class prototypes-----------------------------------------------------------------------------------------*/




/* ------- macro -----------------------------------------------------------------------------------------------------*/





/* ------- variables -------------------------------------------------------------------------------------------------*/

[[maybe_unused]] static auto& forceInit = UsbDeviceApp::instance();



/* ------- application attribute -------------------------------------------------------------------------------------*/

#define APPLICATION_ENABLE     false

#define APPLICATION_NAME       "UsbDevice"

#define APPLICATION_STACK_SIZE 2048

#define APPLICATION_PRIORITY   4

static StackType_t appStack[APPLICATION_STACK_SIZE];



/* ------- message interface attribute -------------------------------------------------------------------------------*/





/* ------- function prototypes ---------------------------------------------------------------------------------------*/





/* ------- function implement ----------------------------------------------------------------------------------------*/


UsbDeviceApp::UsbDeviceApp()
    : ContinuousApp(APPLICATION_ENABLE, APPLICATION_NAME, APPLICATION_STACK_SIZE, appStack, APPLICATION_PRIORITY) {}


void UsbDeviceApp::init() {
    /* driver object initialize */

    if (tusb_init() == false) {
        while (0)
            ;
    }
    // USB_OTG_HS->GINTMSK |= (USB_OTG_GINTMSK_IEPINT | USB_OTG_GINTMSK_OEPINT | USB_OTG_GINTMSK_RXFLVLM);
}


void UsbDeviceApp::run() { // 1. 必须一直调用 tud_task() 来处理底层的枚举和端点事件

    tud_task();
    vTaskDelay(1);
}
