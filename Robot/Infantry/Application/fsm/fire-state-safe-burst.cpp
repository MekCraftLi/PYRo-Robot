/**
 * SafeBurst State
 *
 * Role: 安全连发态 — 位置环逐发推进, 每发前检查热量余量, 适合热量紧张时的受控连发
 *
 * Entry actions:
 *   - 清除校准标志 (下次单发前需重新校准)
 *   - 切回位置环模式
 *   - 清零堵转计时器
 *   - 将目标锁定到前方最近的槽位作为首发目标
 *
 * Exit condition (transitions OUT):
 *   - FRIC_TOGGLE / EMERGENCY_STOP -> Passive
 *   - BURST_STOP / burstShot==0 -> Ready
 *   - 热量不足 -> Ready
 *   - 堵转超时 2000 ms -> CaliReverse (记录来源为 SafeBurst)
 *
 * Context modifications:
 *   - Writes: isCalibrated, useTriggerSpeedLoopOnly, blockStartTick,
 *             targetTriggerEcd, jamSourceState, state
 */

#include "fire-ctrl-app.h"

void FireCtrlApp::StateSafeBurst::enter(FireCtrlCtx* ctx) {
    // --- 初始化 ---
    ctx->isCalibrated            = false;
    ctx->useTriggerSpeedLoopOnly = false;
    ctx->blockStartTick          = 0;

    // --- 锁定前方最近槽位为首发目标 ---
    int32_t currentEcd   = ctx->fdb.triggerEcd + ctx->fdb.triggerRound * 8192 - ctx->triggerOffset;
    int32_t ecdPerBullet = 8192 * 36 / 8;
    ctx->targetTriggerEcd = ((currentEcd + ecdPerBullet - 1) / ecdPerBullet) * ecdPerBullet;

    ctx->state = FireState::SafeBurst;
}

void FireCtrlApp::StateSafeBurst::execute(FireCtrlCtx* ctx) {
    // --- 紧急退出 ---
    if (ctx->transientEvent == ShootEvent::FRIC_TOGGLE ||
        ctx->transientEvent == ShootEvent::EMERGENCY_STOP) {
        request_switch(&instance()._statePassive);
        return;
    }

    // --- 停止条件 ---
    if (ctx->transientEvent == ShootEvent::BURST_STOP || ctx->cmd.state.burstShot == 0) {
        request_switch(&instance()._stateReady);
        return;
    }

    // --- 续杯逻辑: 接近目标槽位时, 检查热量决定是否继续 ---
    int32_t currentEcd   = ctx->fdb.triggerEcd + ctx->fdb.triggerRound * 8192 - ctx->triggerOffset;
    int32_t ecdPerBullet = 8192 * 36 / 8;

    if (ctx->targetTriggerEcd - currentEcd < ecdPerBullet / 2) {
        if (ctx->heatController.canShootSingle()) {
            ctx->targetTriggerEcd += ecdPerBullet;
        } else {
            request_switch(&instance()._stateReady);
            return;
        }
    }

    // --- 堵转检测 ---
    float targetAngle = (float)(ctx->targetTriggerEcd) / (float)(8192 * 36) * 2.0f * (float)M_PI;
    float realAngle   = (float)(currentEcd) / (float)(8192 * 36) * 2.0f * (float)M_PI;
    float err         = targetAngle - realAngle;
    while (err >  (float)M_PI)  err -= 2.0f * (float)M_PI;
    while (err < -(float)M_PI)  err += 2.0f * (float)M_PI;

    if (std::abs(err) > (float)M_PI / 16.0f && std::abs(ctx->fdb.trigger.vel) < 10.0f) {
        if (ctx->blockStartTick == 0)
            ctx->blockStartTick = xTaskGetTickCount();
        else if (xTaskGetTickCount() - ctx->blockStartTick >= pdMS_TO_TICKS(2000)) {
            ctx->jamSourceState = FireState::SafeBurst;
            request_switch(&instance()._stateCaliReverse);
            return;
        }
    } else {
        ctx->blockStartTick = 0;
    }
}
