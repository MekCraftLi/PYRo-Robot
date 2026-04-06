/**
 *******************************************************************************
 * @file    ui-maker-app.h
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
 * @date    2026/3/20
 * @version 1.0
 *******************************************************************************
 */


/* Define to prevent recursive inclusion -----------------------------------------------------------------------------*/

#ifndef INFANTRY_UI_MAKER_APP_H
#define INFANTRY_UI_MAKER_APP_H




/*-------- 1. includes and imports -----------------------------------------------------------------------------------*/

#ifdef __cplusplus

/* I. interface */

#include "System/DataHub/ui-protocol.h"
#include "System/Input/ControlImpl/control-impl-switch.h"
#include "System/Input/TriggerImpl/trigger-impl-hold.h"
#include "System/Input/action.h"
#include "System/Service/ui-renderer-srvc.h"
#include "System/Thread/application-base.h"
#include "tools/crtp.h"

/* II. OS */


/* III. middlewares */


/* IV. drivers */


/* V. standard lib */





/*-------- 2. enum ---------------------------------------------------------------------------------------------------*/




/*-------- 3. interface ---------------------------------------------------------------------------------------------*/

struct UiRelativeElementConfig {
    uint8_t name[3];           // 3 字节协议映射
    GraphicType type;          // 图形类型
    UiColor color;             // 颜色
    uint8_t layer;             // 图层
    uint16_t width;            // 线宽

    // 相对偏移量 (使用有符号整型 int16_t 支持负数偏移)
    int16_t offsetX;
    int16_t offsetY;

    // 细节参数：对于直线是 endOffsetX/Y，对于圆弧是 半轴 a/b
    int16_t detailX;
    int16_t detailY;

    // 附加参数：如圆弧的起始/终止角度
    uint16_t param1;
    uint16_t param2;
};


class UiMakerApp final : public PeriodicApp, public Singleton<UiMakerApp> {
  public:
    UiMakerApp();

    void init() override;


    void run() override;

    /************ setter & getter ***********/



  private:
    UiRendererSrvc& _ui = UiRendererSrvc::instance();

    InputAction resetUI;
    TriggerHold resetUITrig = TriggerHold(0.5f, 0.001, true);

    ControlSwitch resetUISingal = ControlSwitch({{0, 0}, {1, 1}}); // 二档开关，0->-1.0, 1->1.0
    // 抽离动态绘制函数
    void drawDynamicGraphics();
    void drawStaticGraphics();
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