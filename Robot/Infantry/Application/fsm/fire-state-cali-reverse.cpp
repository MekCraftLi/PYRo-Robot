/**
 * CaliReverse State
 *
 * Role: 校准反转态 — 拨弹盘高速反转, 直到碰到机械死区 (堵转) 后切换到 CaliForward
 *
 * Entry actions:
 *   - 清零堵转计时器
 *   - 切换到纯速度环模式
 *
 * Exit actions:
 *   - 清空速度环积分项 (避免残留积分驱动电机)
 *   - 记录当前编码器位置为零点偏移 (triggerOffset)
 *
 * Exit condition (transitions OUT):
 *   - FRIC_TOGGLE / EMERGENCY_STOP -> Passive
 *   - 堵转检测超时 700 ms (速度误差 > 50%) -> 根据来源决定目标状态:
 *       SingleFire 来源 -> Ready (回到就绪)
 *       BurstFire 来源 -> BurstFire / SafeBurst / Ready (根据热量)
 *       SafeBurst 来源 -> SafeBurst / Ready (根据热量)
 *       默认 (首次校准) -> Ready
 *     决定后进入 CaliForward 正转回到零点
 *
 * Context modifications:
 *   - Writes: blockStartTick, useTriggerSpeedLoopOnly, targetTriggerSpeed,
 *             triggerOffset, targetStateAfterCali, state
 *   - Clears: _triggerSpdPid (in exit)
 */

#include "fire-ctrl-app.h"
#include "Config/Gimbal/hw-config.h"

static void decideTargetStateAfterCali(FireCtrlApp::FireCtrlCtx* ctx) {
    switch (ctx->jamSourceState) {
        case FireCtrlApp::FireState::SingleFire:
            // 单发堵转 → 回到 Ready 等待下次指令
            ctx->targetStateAfterCali = FireCtrlApp::FireState::CaliForward;
            break;

        case FireCtrlApp::FireState::BurstFire:
            // 连发堵转 → 根据热量恢复
            if (ctx->heatController.isApproachingHeatLimit()) {
                if (ctx->heatController.canShootSingle()) {
                    ctx->targetStateAfterCali = FireCtrlApp::FireState::SafeBurst;
                } else {
                    ctx->targetStateAfterCali = FireCtrlApp::FireState::CaliForward;
                }
            } else {
                ctx->targetStateAfterCali = FireCtrlApp::FireState::BurstFire;
            }
            break;

        case FireCtrlApp::FireState::SafeBurst:
            // 安全连发堵转 → 根据热量恢复
            if (ctx->heatController.canShootSingle()) {
                ctx->targetStateAfterCali = FireCtrlApp::FireState::SafeBurst;
            } else {
                ctx->targetStateAfterCali = FireCtrlApp::FireState::CaliForward;
            }
            break;

        default:
            // 首次校准 → 回到就绪
            ctx->targetStateAfterCali = FireCtrlApp::FireState::CaliForward;
            break;
    }
}

void FireCtrlApp::StateCaliReverse::enter(FireCtrlCtx* ctx) {
    // --- 初始化 ---
    ctx->blockStartTick          = 0;
    ctx->useTriggerSpeedLoopOnly = true;
    ctx->state                   = FireState::CaliReverse;
}

void FireCtrlApp::StateCaliReverse::execute(FireCtrlCtx* ctx) {
    // --- 高速反转寻找机械死区 ---
    ctx->targetTriggerSpeed = -Config::Hardware::MotorTopo::TRIGGER_SPEED;

    // --- 紧急退出 ---
    if (ctx->transientEvent == ShootEvent::FRIC_TOGGLE ||
        ctx->transientEvent == ShootEvent::EMERGENCY_STOP) {
        request_switch(&instance()._statePassive);
        return;
    }

    // --- 堵转检测: 实际速度远小于目标 → 碰到机械限位 ---
    if (std::abs(ctx->fdb.trigger.vel - ctx->targetTriggerSpeed) > ctx->targetTriggerSpeed * 0.5f) {
        if (ctx->blockStartTick == 0)
            ctx->blockStartTick = xTaskGetTickCount();
        else if (xTaskGetTickCount() - ctx->blockStartTick >= pdMS_TO_TICKS(700)) {
            decideTargetStateAfterCali(ctx);
            request_switch(&instance()._stateCaliForward);
        }
    } else {
        ctx->blockStartTick = 0;
    }
}

void FireCtrlApp::StateCaliReverse::exit(FireCtrlCtx* ctx) {
    // --- 清空速度环积分, 防止残留积分继续驱动电机 ---
    instance()._triggerSpdPid.clear();

    // --- 以当前编码器位置作为新的零点 ---
    ctx->triggerOffset = ctx->rawTriggerEcd;
}
