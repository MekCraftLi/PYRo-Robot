/**
 *******************************************************************************
 * @file    blackboard.h
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

#ifndef INFANTRY_CHASSIS_BLACKBOARD_H
#define INFANTRY_CHASSIS_BLACKBOARD_H



/*-------- 1. includes and imports -----------------------------------------------------------------------------------*/

#include "../../tools/seq-variable.h"
#include "data-def.h"
#include "vision-protocol.h"

#include "Component/Motor/pyro_dji_motor_drv.h"
#include "tools/crtp.h"



/*-------- 2. enum and define ----------------------------------------------------------------------------------------*/




/*-------- 3. interface ----------------------------------------------------------------------------------------------*/


#ifdef CHASSIS
class Blackboard: public Singleton<Blackboard> {
public:

    Blackboard() = default;

    SeqVariable<GimbalToChassisComm>  rComm;
    SeqVariable<ChassisToGimbalComm> tComm;
    // ----------------------------------------
    // [意图区] (主要由 DR16 任务 / ROS 通信任务 写入)
    // ----------------------------------------
    SeqVariable<RcRawData>    rc_raw;
    SeqVariable<RcRawData>    rc_other; // 其他输入源（如第二遥控器、上位机、图传链路等）


    SeqVariable<ChassisCmd>   chassisCmd;
    SeqVariable<GimbalCmd>    gimbal_cmd;

    // ----------------------------------------
    // [状态区] (主要由 CAN 接收任务 / SPI 中断 写入)
    // ----------------------------------------
    SeqVariable<ImuState>     imuState;
    SeqVariable<ChassisState> chassisState;
    SeqVariable<ChassisTelemetry> chassisTelem;
    SeqVariable<SuperCapState> capState;

    // ----------------------------------------
    // [输出区] (主要由 核心控制算法任务 写入)
    // ----------------------------------------
    SeqVariable<ChassisOutput> chassisOut;
    SeqVariable<GimbalOutput>  gimbalOut;
    SeqVariable<SuperCapOutput>  capOut;

    // ----------------------------------------
    // [中间区] (主要由 核心控制算法任务 同步写入)
    // ----------------------------------------


  private:

};


#elifdef GIMBAL
class Blackboard: public Singleton<Blackboard> {
public:

    Blackboard() = default;

    // ----------------------------------------
    // [意图区] (主要由 DR16 任务 / ROS 通信任务 写入)
    // ----------------------------------------
    SeqVariable<RcRawData>    rc_raw;
    SeqVariable<RcRawData>    rc_other; // 其他输入源（如第二遥控器、上位机、图传链路等）


    SeqVariable<ChassisCmd>   chassisCmd;
    SeqVariable<ShootCmd>     shootCmd;
    SeqVariable<GimbalCmd>    gimbalCmd;

    // ----------------------------------------
    // [状态区] (主要由 CAN 接收任务 / SPI 中断 写入)
    // ----------------------------------------
    SeqVariable<ImuState>     imuState;
    SeqVariable<GimbalState>  gimbalState;

    SeqVariable<BoosterState>  boosterState;


    // ----------------------------------------
    // [输出区] (主要由 核心控制算法任务 写入)
    // ----------------------------------------
    SeqVariable<GimbalOutput>  gimbalOut;
    SeqVariable<BoosterOutput> boosterOut;
    SeqVariable<GimbalToChassisComm> g2cOutput;
    // [新增] 云台接收到底盘发来的数据区
    SeqVariable<ChassisToGimbalComm> c2gComm;
    // ----------------------------------------
    // [中间区] (主要由 核心控制算法任务 同步写入)
    // ----------------------------------------
    SeqVariable<GimbalTelemetry> gimbalTelem;
    SeqVariable<VisionTelemetry> visionTelem;
    // =========================================================
    // 视觉上位机通信域
    // =========================================================
    SeqVariable<VisionCommand>   visionCmd;    // 接收到的视觉控制指令

private:

};


#endif

/*-------- 4. decorator ----------------------------------------------------------------------------------------------*/




/*-------- 5. factories ----------------------------------------------------------------------------------------------*/





#endif /*INFANTRY_CHASSIS_BLACKBOARD_H*/
