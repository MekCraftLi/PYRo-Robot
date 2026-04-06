/**
 *******************************************************************************
 * @file    vision-comm.h
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

#ifndef INFANTRY_VISION_COMM_H
#define INFANTRY_VISION_COMM_H




/*-------- 1. includes and imports -----------------------------------------------------------------------------------*/

#ifdef __cplusplus

/* I. interface */

#include "System/DataHub/vision-protocol.h"
#include "System/Thread/application-base.h"
#include "tools/crtp.h"

/* II. OS */


/* III. middlewares */


/* IV. drivers */


/* V. standard lib */





/*-------- 2. enum ---------------------------------------------------------------------------------------------------*/




/*-------- 3. interface ---------------------------------------------------------------------------------------------*/

class VisionCommSrvc final : public PeriodicApp, public Singleton<VisionCommSrvc> {
  public:
    VisionCommSrvc();

    // 状态机枚举
    enum class RxState {
        WAIT_SOF,
        WAIT_PAYLOAD
    };

    // 内存配置
    static constexpr uint16_t RX_BUFFER_SIZE = 256;
    void init() override;

    void run() override;

    /************ setter & getter ***********/



  private:
    friend class Singleton<VisionCommSrvc>;


    // 解析上下文
    RxState  _rxState = RxState::WAIT_SOF;
    uint16_t _readPtr = 0;
    uint16_t _unpackSize = 0;
    uint8_t  _unpackBuf[sizeof(VisionRxFrame)];

    // 内部方法
    void processRxStream();
    void sendTxFrame();


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