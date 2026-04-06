/**
 *******************************************************************************
 * @file    super-cap-comm.h
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

#ifndef INFANTRY_SUPER_CAP_COMM_H
#define INFANTRY_SUPER_CAP_COMM_H




/*-------- 1. includes and imports -----------------------------------------------------------------------------------*/

#ifdef __cplusplus

/* I. interface */

#include "System/DataHub/super-cap-protocol.h"
#include "System/Thread/application-base.h"
#include "tools/crtp.h"

/* II. OS */


/* III. middlewares */


/* IV. drivers */
#include "Config/config.h"


/* V. standard lib */





/*-------- 2. enum ---------------------------------------------------------------------------------------------------*/




/*-------- 3. interface ---------------------------------------------------------------------------------------------*/

class SuperCapCommSrvc final : public PeriodicApp, public Singleton<SuperCapCommSrvc> {
  public:
    SuperCapCommSrvc();

    void init() override;
    void run() override;

    // 暴露给 USART 中断的回调接口
    void onUartRxEventCallback(size_t size);
    void onUartErrCallback();
    /************ setter & getter ***********/
    


  private:

    uint16_t _rxReadPtr;
    static constexpr uint8_t SOF_MAGIC = 0xA5; // 假设您的帧头起始字节为 0xA5

    /* message interface */
    
    // 1. message queue
    
    // 2. mutex
    
    // 3. semphr
    
    // 4. notify
    
    // 5. stream or message
    
    // 6. event group

};
#endif


#ifdef __cplusplus
extern "C" {
#endif

    /* C Interface */

#ifdef __cplusplus
}
#endif




/*-------- 4. decorator ----------------------------------------------------------------------------------------------*/




/*-------- 5. factories ----------------------------------------------------------------------------------------------*/

#endif