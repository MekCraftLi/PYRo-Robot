/**
 *******************************************************************************
 * @file    referee-data-hub.h
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
 * @date    2026/3/4
 * @version 1.0
 *******************************************************************************
 */


/* Define to prevent recursive inclusion -----------------------------------------------------------------------------*/

#ifndef INFANTRY_REFEREE_DATA_HUB_H
#define INFANTRY_REFEREE_DATA_HUB_H




/*-------- 1. includes and imports -----------------------------------------------------------------------------------*/

#include "System/crtp.h"
#include "referee-protocol.h"
#include "tools/seq-variable.h"


/*-------- 2. enum and define ----------------------------------------------------------------------------------------*/




/*-------- 3. interface ----------------------------------------------------------------------------------------------*/



class RefereeDataHub : public Singleton<RefereeDataHub> {
public:
 // =========================================================
    // [1] 宏观比赛与场地状态域 (0x000X & 0x010X)
    // =========================================================
    SeqVariable<RMGameStatus>          gameStatus;           // 0x0001: 比赛阶段与剩余时间
    SeqVariable<RMGameResult>          gameResult;           // 0x0002: 比赛结果
    SeqVariable<RMRobotHp>             allRobotHp;           // 0x0003: 全场机器人与建筑血量
    SeqVariable<RMEventData>           eventData;            // 0x0101: 场地事件(占领/能量机关等)
    SeqVariable<RMRefereeWarning>      warningData;          // 0x0104: 裁判警告/黄红牌
    SeqVariable<RMDartInfo>            dartInfo;             // 0x0105: 飞镖发射状态与目标

    // =========================================================
    // [2] 机器人本体核心状态域 (0x0201 - 0x0205, 0x0208 - 0x020E)
    // =========================================================
    SeqVariable<RMRobotStatus>         robotStatus;          // 0x0201: 本机状态、等级、物理上限
    SeqVariable<RMPowerHeatData>       powerHeat;            // 0x0202: 实时功率、热量、飞容能量
    SeqVariable<RMRobotPos>            robotPos;             // 0x0203: 本机坐标与朝向
    SeqVariable<RMBuff>                buffState;            // 0x0204: 自身增益状态(攻防/冷却)
    SeqVariable<RMProjectileAllowance> projectileAllowance;  // 0x0208: 允许发弹量与金币
    SeqVariable<RMRfidStatus>          rfidStatus;           // 0x0209: RFID 模块刷卡状态
    SeqVariable<RMDartClientCmd>       dartClientCmd;        // 0x020A: 飞镖选手端指令
    SeqVariable<RMGroundRobotPosition> groundRobotPos;       // 0x020B: 己方地面机器人坐标矩阵
    SeqVariable<RMRadarMarkData>       radarMarkData;        // 0x020C: 雷达标记易伤进度
    SeqVariable<RMSentryInfo>          sentryInfo;           // 0x020D: 哨兵自主决策信息反馈
    SeqVariable<RMRadarInfo>           radarInfo;            // 0x020E: 雷达自主决策状态与密钥

    // =========================================================
    // [3] 射击与受击反馈域 (0x0206 - 0x0207)
    // =========================================================
    SeqVariable<RMHurtData>            hurtData;             // 0x0206: 受击装甲板 ID 与扣血原因
    SeqVariable<RMShootData>           shootData;            // 0x0207: 实时射击类型、频率与初速度

    // =========================================================
    // [4] 小地图、图传与自定义交互域 (0x030X)
    // =========================================================
    SeqVariable<RMMapCommand>          mapCommand;           // 0x0303: 小地图云台手点击坐标
    SeqVariable<RMMapRobotData>        mapRobotData;         // 0x0305: 小地图接收雷达坐标
    SeqVariable<RMCustomClientData>    customClientData;     // 0x0306: 图传链路键鼠控制数据
    SeqVariable<RMMapData>             mapPathData;          // 0x0307: 哨兵路径规划坐标点
    SeqVariable<RMCustomInfo>          customInfo;           // 0x0308: 选手端自定义提示文本

    // 图传链路长数据流
    SeqVariable<RMCustomData30>        customDataToRobot;    // 0x0302/0x0311: 客户端发往机器人的数据
    SeqVariable<RMCustomData30>        customDataToClient;   // 0x0309: 机器人发往客户端的数据
    SeqVariable<RMCustomData300>       customByteBlock;      // 0x0310: 机器人发往客户端的大数据流

    // =========================================================
    // [5] 雷达无线链路 (敌方预警) 域 (0x0A01 - 0x0A06)
    // =========================================================
    SeqVariable<RMRadarEnemyPos>       enemyPos;             // 0x0A01: 敌方全队坐标矩阵
    SeqVariable<RMRadarEnemyHp>        enemyHp;              // 0x0A02: 敌方全队血量
    SeqVariable<RMRadarEnemyAmmo>      enemyAmmo;            // 0x0A03: 敌方全队剩余弹量
    SeqVariable<RMRadarEnemyStatus>    enemyStatus;          // 0x0A04: 敌方金币与占领宏观状态
    SeqVariable<RMRadarEnemyBuff>      enemyBuff;            // 0x0A05: 敌方全队增益情况
    SeqVariable<RMRadarEnemyPassword>  enemyPassword;        // 0x0A06: 敌方干扰波密钥

private:


};


/*-------- 4. decorator ----------------------------------------------------------------------------------------------*/




/*-------- 5. factories ----------------------------------------------------------------------------------------------*/





#endif /*INFANTRY_REFEREE_DATA_HUB_H*/
