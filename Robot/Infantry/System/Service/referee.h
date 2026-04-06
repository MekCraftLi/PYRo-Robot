/**
 *******************************************************************************
 * @file    referee.h
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
 * @date    2026/3/4
 * @version 1.0
 *******************************************************************************
 */


/* Define to prevent recursive inclusion -----------------------------------------------------------------------------*/

#ifndef INFANTRY_REFEREE_H
#define INFANTRY_REFEREE_H




/*-------- 1. includes and imports -----------------------------------------------------------------------------------*/

#ifdef __cplusplus

/* I. interface */

#include "System/Thread/application-base.h"
#include "tools/crtp.h"

/* II. OS */


/* III. middlewares */


/* IV. drivers */
#include "usart.h"


/* V. standard lib */





/*-------- 2. enum ---------------------------------------------------------------------------------------------------*/





/*-------- 3. interface ---------------------------------------------------------------------------------------------*/

class RefereeSrvc final : public NotifyApp, public Singleton<RefereeSrvc> {
  public:
    RefereeSrvc();

    void init() override;

    void run() override;

    void onUartRxEventCallback(size_t size);
    void onUartErrCallback();

    /************ setter & getter ***********/
    
    // 解析状态机定义
    enum class ParseState {
        WAIT_SOF,
        WAIT_HEADER,
        WAIT_PAYLOAD
    };

    // 内存配置：DMA 接收环形缓冲区
    static constexpr uint16_t RX_BUFFER_SIZE = 1024;
    // 内存配置：协议最大单包大小 (包含自定义数据 300 字节 + 帧头/尾) [cite: 1518, 1475]
    static constexpr uint16_t MAX_FRAME_SIZE = 512;

    // 环形缓冲读写指针
    uint16_t _readPtr = 0;

    // 状态机上下文
    ParseState _state = ParseState::WAIT_SOF;
    uint8_t _unpackBuf[MAX_FRAME_SIZE]; // 线性重组缓冲区
    uint16_t _unpackSize = 0;           // 当前已缓存的字节数
    uint16_t _expectedSize = 0;         // 当前帧期望接收的总长度

    // 内部处理函数
    void parseFrame(const uint8_t* frame, uint16_t length);
    void dispatchCommand(uint16_t cmdId, const uint8_t* data, uint16_t length);

  private:

    // 1. 定义处理函数的函数指针 (使用裸指针代替 std::function 杜绝动态内存)
    using CmdHandler = void (*)(const uint8_t* data, uint16_t length);

    // 2. 映射表条目结构
    struct CmdMapItem {
        uint16_t cmdId;
        CmdHandler handler;
    };

    // 3. 静态只读映射表 (将存储在 Flash/RODATA 区域)
    static const CmdMapItem _handlerRegistry[];
    static const size_t _registrySize;

    // 4. 对于无法“无脑解包”的复杂数据，单独提供静态解析函数
    static void handleInteractionData(const uint8_t* data, uint16_t length);


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