/**
 *******************************************************************************
 * @file    class-cdc.h
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

#ifndef INFANTRY_CLASS_CDC_H
#define INFANTRY_CLASS_CDC_H
#include "System/crtp.h"
#include "common/tusb_types.h"

#include <cstdint>



/*-------- 1. includes and imports -----------------------------------------------------------------------------------*/





/*-------- 2. enum and define ----------------------------------------------------------------------------------------*/




/*-------- 3. interface ----------------------------------------------------------------------------------------------*/

class UsbCdc : public Singleton<UsbCdc> {

  public:
    UsbCdc() {}
    static void init();
    static void reset(uint8_t);
    static uint16_t open(uint8_t, tusb_desc_interface_t const* itf_desc, uint16_t);
    static bool controlXferCb(uint8_t, uint8_t, tusb_control_request_t const*);
    static bool xferCb(uint8_t, uint8_t, xfer_result_t, uint32_t);
};



/*-------- 4. decorator ----------------------------------------------------------------------------------------------*/




/*-------- 5. factories ----------------------------------------------------------------------------------------------*/





#endif /*INFANTRY_CLASS_CDC_H*/
