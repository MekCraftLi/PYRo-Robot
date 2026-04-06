/**
 *******************************************************************************
 * @file    fire-ctrl-app.h
 * @brief   拨弹摩擦火控 FSM 应用
 *
 * 状态机总览:
 *
 *   Passive ──[FRIC_TOGGLE]──> SpinUp ──[friction wheels at speed]──> Ready
 *       ^                                                                    |
 *       |                    +---[FRIC_TOGGLE / EMERGENCY_STOP]-------------+
 *       |                    |
 *       |              Ready +--[!calibrated + SINGLE_FIRE]--> CaliReverse
 *       |              Ready +--[calibrated + SINGLE_FIRE]---> SingleFire --> Ready
 *       |              Ready +--[burstShot]--> BurstFire --> Ready
 *       |              Ready +--[burstShot + heat warn]--> SafeBurst --> Ready
 *       |                    |
 *       |             CaliReverse ──[hard stop]──> CaliForward ──[zero]──> Ready/...
 *       |                    ^
 *       |                    +---[jam from SingleFire/BurstFire/SafeBurst]
 *       |
 *       +----[from any state: FRIC_TOGGLE / EMERGENCY_STOP]
 *
 * 数据流:
 *   Commander -> Blackboard(shootCmd) -> FireCtrlApp -> FSM -> PID -> Blackboard(boosterOut)
 *                                               ^                     |
 *                                         Blackboard(boosterState) ---+
 *******************************************************************************
 */

#ifndef INFANTRY_FIRE_CTRL_APP_H
#define INFANTRY_FIRE_CTRL_APP_H

/*-------- includes ---------------------------------------------------------------------------------------------------*/

#ifdef __cplusplus

#include "FreeRTOS.h"
#include "task.h"

#include "./System/Thread/application-base.h"
#include "Algorithm/Shoot/speed-compensater.h"
#include "Algorithm/Shoot/heat-controller.h"
#include "System/DataHub/data-def.h"
#include "./tools/crtp.h"
#include "pyro_algo_pid.h"
#include "pyro_core_fsm.h"

/*-------- class ------------------------------------------------------------------------------------------------------*/

struct debug_trigger_t{
    int32_t currentTrigger;
    int32_t targetTrigger;
    uint8_t state;
    uint32_t offset;
};

class FireCtrlApp final : public PeriodicApp, public Singleton<FireCtrlApp> {
  public:
    FireCtrlApp();

    // -------------------------------------------
    // 状态枚举
    // -------------------------------------------
    enum class FireState {
        Passive,       // 休眠: 摩擦轮停, 拨弹锁位
        SpinUp,        // 摩擦轮启动中
        Ready,         // 就绪: 摩擦轮已达速, 等待开火指令
        CaliReverse,   // 校准: 反转寻找机械死区
        CaliForward,   // 校准: 正转回到零点
        SingleFire,    // 单发: 拨弹盘推进一发
        BurstFire,     // 连发: 速度环全速连发
        JamClear,      // (废弃) 堵转清除
        SafeBurst,     // 安全连发: 位置环逐发受控连发
    };

    // -------------------------------------------
    // FSM 上下文 (黑板数据缓存)
    // -------------------------------------------
    struct FireCtrlCtx {
        // --- 输入指令 (来自 Commander / Blackboard) ---
        ShootCmd cmd;                       // 最新射击指令
        ShootEvent transientEvent;          // 边沿检测后的瞬态事件 (仅 1 tick)

        // --- 算法组件 ---
        SpeedCompensator speedCompensator;  // 弹速闭环补偿器
        HeatController heatController;      // 热量管理控制器

        // --- 电机反馈 (来自 MotActuator) ---
        BoosterState fdb;                   // 电机实时状态

        // --- 状态标识 ---
        FireState state;                    // 当前 FSM 状态 (供外部遥测)

        // --- FSM 输出目标 (供 calculateCurrents PID 运算) ---
        float targetFricSpeed;              // 摩擦轮目标转速 (rad/s)
        int32_t targetTriggerEcd;           // 拨弹盘目标编码器位置 (用于位置环)
        uint32_t triggerOffset;             // 编码器零点偏移 (校准后确定)
        float targetTriggerSpeed;           // 拨弹盘目标转速 (用于速度环)
        bool useTriggerSpeedLoopOnly;       // true=绕过位置环, 仅速度环 (连发/校准)

        // --- 校准 & 堵转 ---
        int32_t currentTriggerEcd;
        uint32_t rawTriggerEcd;
        bool isCalibrated      = false;        // 是否已完成拨弹盘校准
        TickType_t stateStartTick = 0;         // 状态进入时刻 (FreeRTOS tick)
        TickType_t blockStartTick = 0;         // 堵转检测起始时刻 (0=未堵转)
        FireState jamSourceState = FireState::Passive; // 堵转来源状态 (校准后恢复)
        FireState targetStateAfterCali = FireState::Ready; // 校准完成后目标状态 (由 CaliReverse 决定)
    };

    // -------------------------------------------
    // FSM 状态类声明
    // -------------------------------------------
    struct StatePassive : public pyro::state_t<FireCtrlCtx> {
        void enter(FireCtrlCtx* ctx) override;
        void execute(FireCtrlCtx* ctx) override;
        void exit(FireCtrlCtx* ctx) override {}
    };
    struct StateSpinUp : public pyro::state_t<FireCtrlCtx> {
        void enter(FireCtrlCtx* ctx) override;
        void execute(FireCtrlCtx* ctx) override;
        void exit(FireCtrlCtx* ctx) override {}
    };
    struct StateReady : public pyro::state_t<FireCtrlCtx> {
        void enter(FireCtrlCtx* ctx) override;
        void execute(FireCtrlCtx* ctx) override;
        void exit(FireCtrlCtx* ctx) override {}
    };
    struct StateCaliReverse : public pyro::state_t<FireCtrlCtx> {
        void enter(FireCtrlCtx* ctx) override;
        void execute(FireCtrlCtx* ctx) override;
        void exit(FireCtrlCtx* ctx) override;
    };
    struct StateCaliForward : public pyro::state_t<FireCtrlCtx> {
        void enter(FireCtrlCtx* ctx) override;
        void execute(FireCtrlCtx* ctx) override;
        void exit(FireCtrlCtx* ctx) override {}
    };
    struct StateSingleFire : public pyro::state_t<FireCtrlCtx> {
        void enter(FireCtrlCtx* ctx) override;
        void execute(FireCtrlCtx* ctx) override;
        void exit(FireCtrlCtx* ctx) override {}
    };
    struct StateBurstFire : public pyro::state_t<FireCtrlCtx> {
        void enter(FireCtrlCtx* ctx) override;
        void execute(FireCtrlCtx* ctx) override;
        void exit(FireCtrlCtx* ctx) override;
    };
    class StateSafeBurst : public pyro::state_t<FireCtrlCtx> {
    public:
        void enter(FireCtrlCtx* ctx) override;
        void execute(FireCtrlCtx* ctx) override;
        void exit(FireCtrlCtx* ctx) override {}
    };
    // DEPRECATED: JamClear is never entered; CaliReverse/CaliForward replaced it.
    struct StateJamClear : public pyro::state_t<FireCtrlCtx> {
        void enter(FireCtrlCtx* ctx) override;
        void execute(FireCtrlCtx* ctx) override;
        void exit(FireCtrlCtx* ctx) override;
    };

    // -------------------------------------------
    // 生命周期
    // -------------------------------------------
    void init() override;
    void run() override;

    // -------------------------------------------
    // 访问器
    // -------------------------------------------
    FireState getFireState();

  private:
    void updateTransientEvent();
    void calculateCurrents(BoosterOutput& out);

    // --- FSM ---
    pyro::fsm_t<FireCtrlCtx> _fsm;
    FireCtrlCtx _ctx;

    // --- 状态单例 ---
    StatePassive   _statePassive;
    StateSpinUp    _stateSpinUp;
    StateReady     _stateReady;
    StateCaliReverse _stateCaliReverse;
    StateCaliForward _stateCaliForward;
    StateSingleFire _stateSingleFire;
    StateBurstFire _stateBurstFire;
    StateSafeBurst _stateSafeBurst;
    StateJamClear  _stateJamClear;

    // --- PID 控制器 ---
    pyro::pid_t _fricLeftSpdPid  = pyro::pid_t(0.22f, 0.0f, 0.0f, 0.0f, 20.0f);
    pyro::pid_t _fricRightSpdPid = pyro::pid_t(0.22f, 0.0f, 0.0f, 0.0f, 20.0f);
    pyro::pid_t _triggerPosPid   = pyro::pid_t(1000.0f, 0.0f, 0.0f, 100.0f, 1000.0f);
    pyro::pid_t _triggerSpdPid   = pyro::pid_t(0.05f, 0.02f, 0.0f, 5.0f, 20.0f);

    ShootEvent _lastEvent = ShootEvent::NONE;
};
#endif

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __cplusplus
}
#endif

#endif
