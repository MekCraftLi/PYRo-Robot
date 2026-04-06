/**
 *******************************************************************************
 * @file    power-limiter.cpp
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
 * @date    2026/3/5
 * @version 1.0
 *******************************************************************************
 */


/* ------- define ----------------------------------------------------------------------------------------------------*/





/* ------- include ---------------------------------------------------------------------------------------------------*/

#include "power-limiter.h"
#include <cmath>
#include <algorithm>



/* ------- class prototypes-------------------------------------------------------------------------------------------*/





/* ------- macro -----------------------------------------------------------------------------------------------------*/





/* ------- variables -------------------------------------------------------------------------------------------------*/





/* ------- function implement ----------------------------------------------------------------------------------------*/

PowerLimiter::PowerLimiter() {
    // 摩擦损耗项系数 (对应 rad/s 的平方) -> 由原 3.0e-7 换算而来
    _param.kw = 2.735e-5f;

    // 机械做功系数 (电流 * 角速度) -> 物理意义为 M3508 的扭矩常数 Kt (N.m/A)
    _param.k1 = 0.02184f;

    // 热损耗项系数 (对应 安培A 的平方) -> 物理意义为 M3508 等效内阻 (欧姆)
    _param.kr = 0.1308f;

    // 静态功耗 (W) -> 不变
    _param.pa = 2.2f;
}

float PowerLimiter::getDynamicPowerLimit(uint16_t limitPower, float refBuffer) {
    auto limit = (float)limitPower;

    // 离线容错保护：裁判系统断线或为0时，强行默认为 80W
    if (limit < 10.0f) {
        limit = 80.0f;
        refBuffer = 60.0f; // 假装满血
    }

    if (refBuffer > 40.0f) {
        return limit * 2.0f; // 满血爆发 (如允许 160W)
    } else if (refBuffer > 15.0f) {
        return limit + (refBuffer - 15.0f) * (limit / 25.0f); // 线性过渡
    } else {
        return limit * 0.8f; // 濒危保命
    }
}

float PowerLimiter::calculateVelocityScale(const float targetOmegas[4],
                                           const float realTorques[4],
                                           float powerLimit) const
{
    float A = 0.0f, B = 0.0f, C = 0.0f;

    for (int i = 0; i < 4; i++) {
        A += _param.kw * targetOmegas[i] * targetOmegas[i];
        B += _param.k1 * realTorques[i] * targetOmegas[i];
        C += _param.kr * realTorques[i] * realTorques[i] + _param.pa;
    }
    C -= powerLimit;

    if (C > 0.0f) return 0.0f; // 热损耗已超标，必须刹车
    if (A < 1e-6f) return 1.0f; // 无速度需求

    float delta = B * B - 4.0f * A * C;
    if (delta < 0.0f) return 0.0f;

    float kv = (-B + std::sqrt(delta)) / (2.0f * A);
    return std::clamp(kv, 0.0f, 1.0f);
}

float PowerLimiter::calculateCurrentScale(const float targetTorques[4],
                                          const float realOmegas[4],
                                          float powerLimit) const
{
    float A = 0.0f, B = 0.0f, C = 0.0f;

    for (int i = 0; i < 4; i++) {
        A += _param.kr * targetTorques[i] * targetTorques[i]; // 电流变为未知数，二次项是 Kr
        B += _param.k1 * targetTorques[i] * realOmegas[i];    // 一次项是机械功率
        C += _param.kw * realOmegas[i] * realOmegas[i] + _param.pa;
    }
    C -= powerLimit;

    if (A < 1e-6f) return 1.0f; // 没有下发电流

    float delta = B * B - 4.0f * A * C;
    if (delta < 0.0f) return 0.0f;

    float ki = (-B + std::sqrt(delta)) / (2.0f * A);
    return std::clamp(ki, 0.0f, 1.0f);
}