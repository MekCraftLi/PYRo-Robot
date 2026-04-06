/**
 *******************************************************************************
 * @file    motor-actuator.h
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

#ifndef INFANTRY_CAN_PARSE_H
#define INFANTRY_CAN_PARSE_H




/*-------- 1. includes and imports -----------------------------------------------------------------------------------*/

#ifdef __cplusplus

/* I. interface */

#include "../../tools/crtp.h"
#include "../Thread/application-base.h"
#include "pyro_dji_motor_drv.h"
#include "pyro_dm_motor_drv.h"
#include "dm_motor_compat.h"

/* II. OS */


/* III. middlewares */


/* IV. drivers */


/* V. standard lib */





/*-------- 2. enum ---------------------------------------------------------------------------------------------------*/




/*-------- 3. interface ---------------------------------------------------------------------------------------------*/

class MotActSrvc final : public PeriodicApp, public Singleton<MotActSrvc> {
  public:
    MotActSrvc();

    void init() override;

    void run() override;

    /************ setter & getter ***********/


    // 1. 静态分配足够大、且字节对齐的原始内存（不会触发任何电机初始化逻辑）


    // 2. 建立一个全局引用，将其重解释为电机数组。这满足了你不想用指针的需求！
#ifdef CHASSIS
    pyro::dji_m3508_motor_drv_t (&drive)[4]   = reinterpret_cast<pyro::dji_m3508_motor_drv_t (&)[4]>(_driveMem);
    pyro::dji_gm_6020_motor_drv_t (&steer)[4] = reinterpret_cast<pyro::dji_gm_6020_motor_drv_t (&)[4]>(_steerMem);
#elifdef GIMBAL
    pyro::dji_m3508_motor_drv_t (&fric)[2] = reinterpret_cast<pyro::dji_m3508_motor_drv_t (&)[2]>(_fircMem);
    compat::DmMotorCompat& pitch          = reinterpret_cast<compat::DmMotorCompat&>(_pitchMem);
#endif
    pyro::dji_gm_6020_motor_drv_t& yaw = reinterpret_cast<pyro::dji_gm_6020_motor_drv_t(&)>(_yawMem);
    [[maybe_unused]] pyro::dji_m2006_motor_drv_t& trigger =
        reinterpret_cast<pyro::dji_m2006_motor_drv_t(&)>(_triggerMem);

  private:
    /* message interface */

    // 1. message queue

    // 2. mutex

    // 3. semphr

    // 4. notify

    // 5. stream or message

    // 6. event group
#ifdef CHASSIS
    alignas(pyro::dji_m3508_motor_drv_t) uint8_t _driveMem[sizeof(pyro::dji_m3508_motor_drv_t) * 4];
    alignas(pyro::dji_gm_6020_motor_drv_t) uint8_t _steerMem[sizeof(pyro::dji_gm_6020_motor_drv_t) * 4];
#elifdef GIMBAL
    alignas(compat::DmMotorCompat) uint8_t _pitchMem[sizeof(compat::DmMotorCompat)];
    alignas(pyro::dji_m3508_motor_drv_t) uint8_t _fircMem[sizeof(pyro::dji_m3508_motor_drv_t) * 2];
#endif
    alignas(pyro::dji_gm_6020_motor_drv_t) uint8_t _yawMem[sizeof(pyro::dji_gm_6020_motor_drv_t)];
    alignas(pyro::dji_m2006_motor_drv_t) uint8_t _triggerMem[sizeof(pyro::dji_m2006_motor_drv_t)];
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