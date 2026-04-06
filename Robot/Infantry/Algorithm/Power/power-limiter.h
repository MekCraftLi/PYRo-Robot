/**
 *******************************************************************************
 * @file    power-limiter.h
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


/* Define to prevent recursive inclusion -----------------------------------------------------------------------------*/

#ifndef INFANTRY_POWER_LIMITER_H
#define INFANTRY_POWER_LIMITER_H



/*-------- 1. includes and imports -----------------------------------------------------------------------------------*/

#include "../../tools/crtp.h"
#include <cstdint>


/*-------- 2. enum and define ----------------------------------------------------------------------------------------*/




/*-------- 3. interface ----------------------------------------------------------------------------------------------*/


class PowerLimiter : public Singleton<PowerLimiter> {
public:
    struct ModelParam {
        float kw; // 速度平方项系数 (摩擦损耗)
        float k1; // 交叉项系数 (机械做功)
        float kr; // 电流平方项系数 (热损耗)
        float pa; // 静态功耗
    };

    PowerLimiter();

    /**
     * @brief 获取包含 Buffer 余量的动态功率天花板
     */
    static float getDynamicPowerLimit(uint16_t limitPower, float refBuffer) ;

    /**
     * @brief 第一层：宏观速度诱导 (MPVS)。反解允许的最大速度缩放比例
     * @param targetOmegas 4个轮子的期望角速度
     * @param realTorques  4个轮子的真实反馈电流 (建议低通滤波)
     * @param powerLimit   动态功率上限
     * @return float       速度缩放系数 Kv [0.0 ~ 1.0]
     */
    float calculateVelocityScale(const float targetOmegas[4],
                                 const float realTorques[4],
                                 float powerLimit) const;

    /**
     * @brief 第二层：微观电流钳位。验算并反解允许的最大电流缩放比例
     * @param targetTorques PID 算出的预期下发电流
     * @param realOmegas    4个轮子的真实转速
     * @param powerLimit    动态功率上限
     * @return float        电流缩放系数 Ki [0.0 ~ 1.0]
     */
    float calculateCurrentScale(const float targetTorques[4],
                                const float realOmegas[4],
                                float powerLimit) const;

private:
    ModelParam _param{};
};


/*-------- 4. decorator ----------------------------------------------------------------------------------------------*/




/*-------- 5. factories ----------------------------------------------------------------------------------------------*/





#endif /*INFANTRY_POWER_LIMITER_H*/
