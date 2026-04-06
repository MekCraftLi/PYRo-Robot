/**
 *******************************************************************************
 * @file    movtion-ctrl-app.h
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
 * @date    2026/2/27
 * @version 1.0
 *******************************************************************************
 */


/* Define to prevent recursive inclusion -----------------------------------------------------------------------------*/

#ifndef INFANTRY_CHASSIS_MOVTION_CTRL_APP_H
#define INFANTRY_CHASSIS_MOVTION_CTRL_APP_H




/*-------- 1. includes and imports -----------------------------------------------------------------------------------*/

#ifdef __cplusplus

/* I. interface */

#include "../System/Thread/application-base.h"
#include "../tools/crtp.h"
#include "Config/config.h"
#include "pyro_algo_pid.h"

/* II. OS */


/* III. middlewares */

#include "Algorithm/Motion/s-curve-planner.h"
#include "Algorithm/Power/power-limiter.h"


/* IV. drivers */


/* V. standard lib */





/*-------- 2. enum ---------------------------------------------------------------------------------------------------*/

struct PIDParam {
    float kp;
    float ki;
    float kd;
    float integralLimit;
    float outputLimit;
};



/*-------- 3. interface ---------------------------------------------------------------------------------------------*/

class MovtionCtrlApp final : public PeriodicApp, public Singleton<MovtionCtrlApp> {
public:
    MovtionCtrlApp();

    void init() override;

    void run() override;

    /************ setter & getter ***********/


#ifdef CHASSIS

    // [新增] 实例化三个轴的 S 曲线规划器
    // 这里的参数基于 RM 步兵常见调车经验，你可以通过 algo-config.h 去宏定义
    // 1. 前后平移 (Vx)：通常底盘较长，抗前倾能力较强，可以略微激进
    // 最大速度 3m/s，最大加速度 4.5m/s²(约0.45G)，加加速度 25m/s³
    SCurveVelocityPlanner vxPlanner{3.0f, 4.5f, 500.0f};

    // 2. 左右平移 (Vy)：麦轮左右平移效率低，且轮距通常较短，最容易侧翻！必须保守！
    // 最大速度 2.5m/s，最大加速度 3.0m/s²(约0.3G)，加加速度 15m/s³ (强制柔和起步)
    SCurveVelocityPlanner vyPlanner{2.5f, 4.5f, 500.0f};

    // 3. 自旋 (Vw)：“小陀螺”模式。自旋通常不会翻车，但会引起极大的离心力和功率尖峰。
    // 最大速度 6.28rad/s(约1圈/秒)，角加速度 15rad/s²，角加加速度 80rad/s³
    SCurveVelocityPlanner vwPlanner{6.28f, 15.0f, 80.0f};

    pyro::pid_t driveSpdPid[4] = {
        pyro::pid_t(0.2f, 0.000f, 0.0f, 1.0f, 20.0f),
        pyro::pid_t(0.2f, 0.000f, 0.0f, 1.0f, 20.0f),
        pyro::pid_t(0.2f, 0.000f, 0.0f, 1.0f, 20.0f),
        pyro::pid_t(0.2f, 0.000f, 0.0f, 1.0f, 20.0f),
    };
    pyro::pid_t steerPosPid[4] = {
        pyro::pid_t(58.0f,  5.0f, 0.0f, 10.0f, 50.0f),
        pyro::pid_t(58.0f,  5.0f, 0.0f, 10.0f, 50.0f),
        pyro::pid_t(58.0f,  5.0f, 0.0f, 10.0f, 50.0f),
        pyro::pid_t(58.0f,  5.0f, 0.0f, 10.0f, 50.0f),
    };
    pyro::pid_t steerSpdPid[4] = {
        pyro::pid_t(0.87f,  0.0f, 0.0f, 1.0f, 24.0f),
        pyro::pid_t(0.87f,  0.0f, 0.0f, 1.0f, 24.0f),
        pyro::pid_t(0.87f,  0.0f, 0.0f, 1.0f, 24.0f),
        pyro::pid_t(0.87f,  0.0f, 0.0f, 1.0f, 24.0f)
    };

    pyro::pid_t yawPosPid = pyro::pid_t(11.0f, 0.1f, 0.1f, 1.0f, 100.0f);

    uint8_t motorIdx[4] = {0};



#elif defined(GIMBAL)

    pyro::pid_t yawPosPid = pyro::pid_t(20.0f, 0.8f, 0.0f, 10.0f, 500.0f);
    pyro::pid_t yawSpdPid = pyro::pid_t(18.0f, 0.0f, 0.0f, 0.0f, 24.0f);
    pyro::pid_t pitchPosPid = pyro::pid_t(0.0f, Config::Algorithm::Gimbal::DM_MOT_PITCH_KI, 0.0f, 12.0f, 12.0f);

#endif



  private:
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

