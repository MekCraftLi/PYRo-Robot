/**
 *******************************************************************************
 * @file    ui-maker-app.cpp
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




/* ------- define ----------------------------------------------------------------------------------------------------*/





/* ------- include ---------------------------------------------------------------------------------------------------*/



/* I. header */

#include "ui-maker-app.h"

#include "Board-Support-Pack/VideoLink/video-link-remote.h"
#include "System/DataHub/blackboard.h"
#include "System/DataHub/data-def.h"
#include "System/DataHub/referee-data-hub.h"
#include "System/DataHub/referee-protocol.h"
#include "System/DataHub/ui-protocol.h"
#include "System/Input/action.h"
#include "System/Service/ui-renderer-srvc.h"

/* II. other application */


/* III. standard lib */




/* ------- class prototypes-----------------------------------------------------------------------------------------*/




/* ------- macro -----------------------------------------------------------------------------------------------------*/

struct _UiName {
    uint8_t d[3];
    _UiName(uint8_t a, uint8_t b, uint8_t c) : d{a, b, c} {}
    operator uint8_t*() { return d; }
};
#define PYRO_UN(a, b, c) _UiName((uint8_t)(a), (uint8_t)(b), (uint8_t)(c))





/* ------- variables -------------------------------------------------------------------------------------------------*/

[[maybe_unused]] static auto& forceInit = UiMakerApp::instance();

static uint8_t figureIdx;

static constexpr uint16_t positionX                 = 1920 / 2;
static constexpr uint16_t positionY                 = 850;

// 放置在 Flash 中，不占任何 RAM
constexpr UiRelativeElementConfig CUSTOM_UI_TABLE[] = {
    // name, type, color, layer, width, offsetX, offsetY, detailX(endX/a), detailY(endY/b), param1(startAng),
    // param2(endAng)

    // 组 1
    {{'c', '0', '1'}, GraphicType::Arc, UiColor::Main, 0, 3, -14, 20, 20, 20, 130, 210},
    {{'c', '0', '2'}, GraphicType::Arc, UiColor::Main, 0, 3, 14, 20, 20, 20, 155, 225},

    // 组 2
    {{'c', '0', '3'}, GraphicType::Arc, UiColor::Main, 0, 3, 3, -55, 40, 80, 298, 315},
    {{'c', '0', '4'}, GraphicType::Arc, UiColor::Main, 0, 3, -3, -55, 40, 80, 43, 62},

    // 组 3
    {{'c', '0', '5'}, GraphicType::Arc, UiColor::Main, 0, 3, -35, -25, 10, 7, 15, 180},
    {{'c', '0', '6'}, GraphicType::Arc, UiColor::Main, 0, 3, 35, -25, 10, 7, 180, 345},

    // 组 4
    {{'c', '0', '7'}, GraphicType::Arc, UiColor::Main, 0, 3, 7, -33, 40, 40, 230, 270},
    {{'c', '0', '8'}, GraphicType::Arc, UiColor::Main, 0, 3, -7, -33, 40, 40, 90, 130},

    // 组 5
    {{'c', '0', '9'}, GraphicType::Arc, UiColor::Main, 0, 3, 91, 18, 150, 120, 217, 230},
    {{'c', '1', '0'}, GraphicType::Arc, UiColor::Main, 0, 3, -91, 18, 150, 120, 130, 143},

    // // 组 6
    // {{'c','1','1'}, GraphicType::Arc, UiColor::White, 0, 3,   0, -70, 40,  15,  33, 327},
    // {{'c','1','2'}, GraphicType::Arc, UiColor::White, 0, 3,   0, -76, 50,  23,  33, 327},

    // 组 7
    {{'c', '1', '3'}, GraphicType::Line, UiColor::White, 0, 3, -20, -25, -6, -25, 0, 0},
    {{'c', '1', '4'}, GraphicType::Line, UiColor::White, 0, 3, -13, -25, -13, -45, 0, 0},
    {{'c', '1', '5'}, GraphicType::Line, UiColor::White, 0, 3, 0, -23, 0, -40, 0, 0},
    {{'c', '1', '6'}, GraphicType::Arc, UiColor::White, 0, 3, -5, -40, 5, 5, 80, 250},
    {{'c', '1', '7'}, GraphicType::Line, UiColor::White, 0, 3, 6, -23, 6, -40, 0, 0},
    {{'c', '1', '8'}, GraphicType::Line, UiColor::White, 0, 4, 20, -23, 20, -40, 0, 0}, // 线宽为 4
    {{'c', '1', '9'}, GraphicType::Arc, UiColor::White, 0, 3, 13, -40, 7, 5, 80, 280},


};

// 编译期计算数组长度
constexpr size_t CUSTOM_UI_COUNT = sizeof(CUSTOM_UI_TABLE) / sizeof(CUSTOM_UI_TABLE[0]);

/* ------- application attribute -------------------------------------------------------------------------------------*/

#define APPLICATION_ENABLE     true

#define APPLICATIONPYRO_UNAME       "UiMaker"

#define APPLICATION_STACK_SIZE 512

#define APPLICATION_PRIORITY   4

static StackType_t appStack[APPLICATION_STACK_SIZE];



/* ------- message interface attribute -------------------------------------------------------------------------------*/





/* ------- function prototypes ---------------------------------------------------------------------------------------*/





/* ------- function implement ----------------------------------------------------------------------------------------*/


UiMakerApp::UiMakerApp()
    : PeriodicApp(APPLICATION_ENABLE, APPLICATIONPYRO_UNAME, APPLICATION_STACK_SIZE, appStack, APPLICATION_PRIORITY, 35) {}


void UiMakerApp::init() {
    /* driver object initialize */
    _ui.waitInit();
    _ui.clearGraphic(GraphicDelMode::All);


    // 遍历参数表，利用 Proxy 自动渲染入队

    for (size_t i = 0; i < CUSTOM_UI_COUNT; ++i) {
        const auto& cfg = CUSTOM_UI_TABLE[i];

        // 计算绝对起始坐标
        uint16_t startX = positionX + cfg.offsetX;
        uint16_t startY = positionY + cfg.offsetY;

        // 流式配置通用属性
        auto proxy      = std::move(
            _ui.draw((uint8_t*)cfg.name).layer(cfg.layer).color(cfg.color).width(cfg.width).start(startX, startY));

        // 根据类型装载特异性参数
        if (cfg.type == GraphicType::Arc) {
            proxy.asArc(cfg.param1, cfg.param2, cfg.detailX, cfg.detailY);
        } else if (cfg.type == GraphicType::Line) {
            uint16_t endX = positionX + cfg.detailX;
            uint16_t endY = positionY + cfg.detailY;
            proxy.asLine(endX, endY);
        }
        // 循环单次结束时，proxy 析构，这一个图形自动进入 FreeRTOS 队列
    }

    _ui.draw(PYRO_UN(0, 0, figureIdx++))
        .color(UiColor::Main)
        .width(10)
        .asLine(1920 / 2 - 80, 800)
        .start(1920 / 2 - 110, 835);
    _ui.draw(PYRO_UN(0, 0, figureIdx++))
        .color(UiColor::Main)
        .width(10)
        .asLine(1920 / 2 + 80, 800)
        .start(1920 / 2 + 110, 835);
    _ui.draw(PYRO_UN(0, 0, figureIdx++))
        .color(UiColor::Main)
        .width(10)
        .asLine(1920 / 2 - 130, 800)
        .start(1920 / 2 - 160, 835);
    _ui.draw(PYRO_UN(0, 0, figureIdx++))
        .color(UiColor::Main)
        .width(10)
        .asLine(1920 / 2 + 130, 800)
        .start(1920 / 2 + 160, 835);
    _ui.draw(PYRO_UN(0, 0, figureIdx++))
        .color(UiColor::Main)
        .width(10)
        .asLine(1920 / 2 - 180, 800)
        .start(1920 / 2 - 210, 835);
    _ui.draw(PYRO_UN(0, 0, figureIdx++))
        .color(UiColor::Main)
        .width(10)
        .asLine(1920 / 2 + 180, 800)
        .start(1920 / 2 + 210, 835);


    _ui.draw(PYRO_UN(0, 0, figureIdx++)).color(UiColor::White).width(10).asArc(0, 80, 400, 120).start(450, 0);
    _ui.draw(PYRO_UN(0, 0, figureIdx++)).color(UiColor::White).width(10).asArc(0, 80, 400, 130).start(450, 0);
    _ui.draw(PYRO_UN(0, 0, figureIdx++)).color(UiColor::White).width(10).asArc(0, 80, 400, 140).start(450, 0);
    _ui.draw(PYRO_UN(0, 0, figureIdx++)).color(UiColor::White).width(10).asArc(0, 80, 400, 150).start(450, 0);
    _ui.draw(PYRO_UN(3, 0, 0)).color(UiColor::Orange).width(10).asArc(0, 80, 400, 150).start(450, 0);


    _ui.draw(PYRO_UN(0, 0, figureIdx++))
        .color(UiColor::White)
        .width(6)
        .asArc(185, 203, 1000, 210)
        .start(1920 / 2, 950);


    _ui.draw(PYRO_UN(0, 0, figureIdx++))
        .color(UiColor::White)
        .width(6)
        .asArc(196, 220, 1000, 150)
        .start(1920 / 2, 970);


    _ui.draw(PYRO_UN(0, 0, figureIdx++))
        .color(UiColor::White)
        .width(6)
        .asArc(140, 164, 1000, 150)
        .start(1920 / 2, 970);


    _ui.draw(PYRO_UN(4, 0, 0)).color(UiColor::Cyan).width(17).asArc(185, 203, 1000, 210).start(1920 / 2, 944);
    _ui.draw(PYRO_UN(5, 0, 0)).color(UiColor::Cyan).width(17).asArc(157, 175, 1000, 210).start(1920 / 2, 944);

    _ui.draw(PYRO_UN(1, 0, 0)).color(UiColor::White).width(2).asFloat(0.0).start(1920 / 2 - 20, 750);
    _ui.draw(PYRO_UN(2, 0, 0)).color(UiColor::Orange).width(2).asFloat(0.0).start(600, 50);

    _ui.draw(PYRO_UN(6, 0, 0)).color(UiColor::Pink).width(4).asCircle(30).start(1920 / 2, 1080 / 2);



    resetUI.bind(&resetUISingal, &resetUITrig);
}

void UiMakerApp::run() {
    GimbalToChassisComm comm{};
    SuperCapState capState{};
    ImuState imuState{};
    Blackboard::instance().rComm.read(comm);
    Blackboard::instance().capState.read(capState);
    Blackboard::instance().imuState.read(imuState);



    resetUISingal.updateRaw(comm.msg.resetUI);

    resetUI.update(0.035f);

    if (resetUI.isTriggered()) {
        init();
    } else {

        if (comm.msg.shootEn) {
            _ui.draw(PYRO_UN(6, 0, 0), GraphicOption::Update)
                .color(UiColor::Green)
                .width(4)
                .asCircle(30)
                .start(1920 / 2, 1080 / 2);
        } else {
            _ui.draw(PYRO_UN(6, 0, 0), GraphicOption::Update)
                .color(UiColor::Pink)
                .width(4)
                .asCircle(30)
                .start(1920 / 2, 1080 / 2);
        }

        float ratioCap    = capState.voltage / 28.0f;
        float ratioSpin = abs(imuState.gyro[2]) / (2 * M_PI) / 3;

        if (ratioCap > 1.0f) {
            ratioCap = 1.0f;
        }
        if (ratioSpin > 1.0f) {
            ratioSpin = 1.0f;
        }

        uint16_t angleCap = ratioCap * 18;
        uint16_t angleSpin = ratioSpin * 80;




        _ui.draw(PYRO_UN(4, 0, 0), GraphicOption::Update).color(UiColor::Cyan).width(17).asArc(185, 185 + angleCap, 1000, 210).start(1920 / 2, 944);
        _ui.draw(PYRO_UN(5, 0, 0), GraphicOption::Update).color(UiColor::Cyan).width(17).asArc(176 - angleCap, 175, 1000, 210).start(1920 / 2, 944);
        _ui.draw(PYRO_UN(1, 0, 0), GraphicOption::Update).color(UiColor::White).width(2).asFloat(capState.voltage).start(1920 / 2 - 20, 750);

        _ui.draw(PYRO_UN(3, 0, 0), GraphicOption::Update).color(UiColor::Orange).width(10).asArc(79-angleSpin, 80, 400, 150).start(450, 0);
        _ui.draw(PYRO_UN(2, 0, 0), GraphicOption::Update).color(UiColor::Orange).width(2).asFloat(imuState.gyro[2] / (2 * M_PI)).start(600, 50);




    }
}

// -------------------------------------------------------------
// 抽离出的纯动态逻辑（不关心当前是 Add 还是 Update）
// -------------------------------------------------------------
void UiMakerApp::drawDynamicGraphics() {}

void UiMakerApp::drawStaticGraphics() {}