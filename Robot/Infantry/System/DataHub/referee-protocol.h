/**
 *******************************************************************************
 * @file    referee-protocol.h
 * @brief   RoboMaster 2026 裁判系统全协议数据结构定义
 *******************************************************************************
 * @note
 * 严格遵照 RoboMaster 2026 裁判系统 V1.2.0 协议规范编写。
 * 强制应用 1 字节对齐 (#pragma pack) 配合 DMA 零拷贝解析。
 *******************************************************************************
 */

#ifndef INFANTRY_DATAHUB_REFEREE_PROTOCOL_H
#define INFANTRY_DATAHUB_REFEREE_PROTOCOL_H

#include <cstdint>

#pragma pack(push, 1)

// ============================================================================
// [1] 串口帧基础结构
// ============================================================================

/**
 * @brief 裁判系统数据帧头 [cite: 31, 33, 35]
 */
struct RMFrameHeader {
    uint8_t sof;          // 固定为 0xA5
    uint16_t dataLength;  // 数据段长度
    uint8_t seq;          // 包序号
    uint8_t crc8;         // 帧头 CRC8 校验
};

// ============================================================================
// [2] 常规链路核心数据 (0x000X - 0x020X)
// ============================================================================

// 0x0001: 比赛状态数据 (11 Bytes) [cite: 80]
struct RMGameStatus {
    uint8_t gameType : 4;
    uint8_t gameProgress : 4;
    uint16_t stageRemainTime;
    uint64_t syncTimeStamp;
};

// 0x0002: 比赛结果数据 (1 Byte) [cite: 86]
struct RMGameResult {
    uint8_t winner;
};

// 0x0003: 机器人血量数据 (16 Bytes) [cite: 92]
struct RMRobotHp {
    uint16_t ally1RobotHp;
    uint16_t ally2RobotHp;
    uint16_t ally3RobotHp;
    uint16_t ally4RobotHp;
    uint16_t reserved;
    uint16_t ally7RobotHp;
    uint16_t allyOutpostHp;
    uint16_t allyBaseHp;
};

// 0x0101: 场地事件数据 (4 Bytes) [cite: 106]
struct RMEventData {
    uint32_t eventData; // 包含各种占领状态和能量机关状态的位域拼接
};

// 0x0104: 裁判警告数据 (3 Bytes) [cite: 116]
struct RMRefereeWarning {
    uint8_t level;
    uint8_t offendingRobotId;
    uint8_t count;
};

// 0x0105: 飞镖发射相关数据 (3 Bytes) [cite: 126]
struct RMDartInfo {
    uint8_t dartRemainingTime;
    uint16_t dartInfo;
};

// 0x0201: 机器人性能体系数据 (13 Bytes)
struct RMRobotStatus {
    uint8_t robotId;
    uint8_t robotLevel;
    uint16_t currentHp;
    uint16_t maximumHp;
    uint16_t shooterBarrelCoolingValue;
    uint16_t shooterBarrelHeatLimit;
    uint16_t chassisPowerLimit;
    uint8_t powerManagementGimbalOutput : 1;
    uint8_t powerManagementChassisOutput : 1;
    uint8_t powerManagementShooterOutput : 1;
    uint8_t reserved : 5; // 补齐剩余位域
};

// 0x0202: 实时底盘缓冲能量和射击热量数据 (14 Bytes) [cite: 153, 156]
struct RMPowerHeatData {
    uint16_t reserved1;
    uint16_t reserved2;
    float reserved3;
    uint16_t bufferEnergy;
    uint16_t shooter17mmBarrelHeat;
    uint16_t shooter42mmBarrelHeat;
};

// 0x0203: 机器人位置数据 (16 Bytes) [cite: 167]
struct RMRobotPos {
    float x;
    float y;
    float angle;
};

// 0x0204: 机器人增益数据 (8 Bytes) [cite: 176, 179]
struct RMBuff {
    uint8_t recoveryBuff;
    uint16_t coolingBuff;
    uint8_t defenceBuff;
    uint8_t vulnerabilityBuff;
    uint16_t attackBuff;
    uint8_t remainingEnergy;
};

// 0x0206: 伤害状态数据 (1 Byte) [cite: 182]
struct RMHurtData {
    uint8_t armorId : 4;
    uint8_t hpDeductionReason : 4;
};

// 0x0207: 实时射击数据 (7 Bytes) [cite: 191, 194]
struct RMShootData {
    uint8_t bulletType;
    uint8_t shooterNumber;
    uint8_t launchingFrequency;
    float initialSpeed;
};

// 0x0208: 允许发弹量 (8 Bytes) [cite: 203]
struct RMProjectileAllowance {
    uint16_t projectileAllowance17mm;
    uint16_t projectileAllowance42mm;
    uint16_t remainingGoldCoin;
    uint16_t projectileAllowanceFortress;
};

// 0x0209: 机器人 RFID 模块状态 (5 Bytes) [cite: 212, 219]
struct RMRfidStatus {
    uint32_t rfidStatus;
    uint8_t rfidStatus2;
};

// 0x020A: 飞镖选手端指令数据 (6 Bytes) [cite: 227, 230]
struct RMDartClientCmd {
    uint8_t dartLaunchOpeningStatus;
    uint8_t reserved;
    uint16_t targetChangeTime;
    uint16_t latestLaunchCmdTime;
};

// 0x020B: 地面机器人位置数据 (40 Bytes) [cite: 240]
struct RMGroundRobotPosition {
    float heroX;
    float heroY;
    float engineerX;
    float engineerY;
    float standard3X;
    float standard3Y;
    float standard4X;
    float standard4Y;
    float reserved1;
    float reserved2;
};

// 0x020C: 雷达标记进度数据 (2 Bytes) [cite: 259]
struct RMRadarMarkData {
    uint16_t markProgress;
};

// 0x020D: 哨兵自主决策信息同步 (6 Bytes) [cite: 269, 273]
struct RMSentryInfo {
    uint32_t sentryInfo;
    uint16_t sentryInfo2;
};

// 0x020E: 雷达自主决策信息同步 (1 Byte) [cite: 280]
struct RMRadarInfo {
    uint8_t radarInfo;
};


// ============================================================================
// [3] 交互层数据 (0x030X) 及 UI 绘制结构
// ============================================================================

// 0x0301: 机器人交互数据统一头 (6 Bytes) [cite: 292]
struct RMInteractionHeader {
    uint16_t dataCmdId;  // 子内容 ID
    uint16_t senderId;
    uint16_t receiverId;
};

// 0x0301 (0x0100): 选手端删除图层 (2 Bytes) [cite: 306]
struct RMInteractionLayerDelete {
    uint8_t deleteType;
    uint8_t layer;
};

// UI 基础图形配置结构 (15 Bytes) [cite: 313, 315]
struct RMInteractionFigure {
    uint8_t figureName[3];
    uint32_t operateType : 3;
    uint32_t figureType : 3;
    uint32_t layer : 4;
    uint32_t color : 4;
    uint32_t detailsA : 9;
    uint32_t detailsB : 9;
    uint32_t width : 10;
    uint32_t startX : 11;
    uint32_t startY : 11;
    uint32_t detailsC : 10;
    uint32_t detailsD : 11;
    uint32_t detailsE : 11;
};

// 0x0301 (0x0110): 选手端绘制字符图形 (45 Bytes) [cite: 370]
struct RMExtClientCustomCharacter {
    RMInteractionFigure graphicDataStruct;
    uint8_t data[30];
};

// 0x0301 (0x0120): 哨兵自主决策指令 (4 Bytes) [cite: 377, 380, 384]
struct RMSentryCmd {
    uint32_t sentryCmd;
};

// 0x0301 (0x0121): 雷达自主决策指令 (8 Bytes) [cite: 390]
struct RMRadarCmd {
    uint8_t radarCmd;
    uint8_t passwordCmd;
    uint8_t password[6];
};

// 0x0303: 选手端小地图交互数据 (15 Bytes) [cite: 433]
struct RMMapCommand {
    float targetPositionX;
    float targetPositionY;
    uint8_t cmdKeyboard;
    uint8_t targetRobotId;
    uint16_t cmdSource;
};

// 0x0305: 选手端小地图接收雷达数据 (24 Bytes) [cite: 446, 449]
struct RMMapRobotData {
    uint16_t heroPositionX;
    uint16_t heroPositionY;
    uint16_t engineerPositionX;
    uint16_t engineerPositionY;
    uint16_t infantry3PositionX;
    uint16_t infantry3PositionY;
    uint16_t infantry4PositionX;
    uint16_t infantry4PositionY;
    uint16_t reserved1;
    uint16_t reserved2;
    uint16_t sentryPositionX;
    uint16_t sentryPositionY;
};

// 0x0306: 自定义控制器与选手端交互数据 (8 Bytes) [cite: 516, 519]
struct RMCustomClientData {
    uint16_t keyValue;
    uint16_t xPosition : 12;
    uint16_t mouseLeft : 4;
    uint16_t yPosition : 12;
    uint16_t mouseRight : 4;
    uint16_t reserved;
};

// 0x0307: 选手端小地图接收路径数据 (105 Bytes) [cite: 457]
struct RMMapData {
    uint8_t intention;
    uint16_t startPositionX;
    uint16_t startPositionY;
    int8_t deltaX[49];
    int8_t deltaY[49];
    uint16_t senderId;
};

// 0x0308: 选手端小地图接收自定义数据 (34 Bytes) [cite: 471]
struct RMCustomInfo {
    uint16_t senderId;
    uint16_t receiverId;
    uint8_t userData[30];
};

// 0x0302/0x0309/0x0311: 图传链路 30 字节交互结构体 [cite: 484, 491, 507]
struct RMCustomData30 {
    uint8_t data[30];
};

// 0x0310: 机器人发送给自定义客户端的数据 (300 Bytes) [cite: 500]
struct RMCustomData300 {
    uint8_t data[300];
};


// ============================================================================
// [4] 雷达无线链路专用数据 (0x0A01 - 0x0A06)
// ============================================================================

// 0x0A01: 对方机器人的位置坐标 (24 Bytes) [cite: 532, 535]
struct RMRadarEnemyPos {
    uint16_t heroX;
    uint16_t heroY;
    uint16_t engineerX;
    uint16_t engineerY;
    uint16_t standard3X;
    uint16_t standard3Y;
    uint16_t standard4X;
    uint16_t standard4Y;
    uint16_t aerialX;
    uint16_t aerialY;
    uint16_t sentryX;
    uint16_t sentryY;
};

// 0x0A02: 对方机器人的血量信息 (12 Bytes) [cite: 538]
struct RMRadarEnemyHp {
    uint16_t heroHp;
    uint16_t engineerHp;
    uint16_t standard3Hp;
    uint16_t standard4Hp;
    uint16_t reserved;
    uint16_t sentryHp;
};

// 0x0A03: 对方机器人的剩余发弹量信息 (10 Bytes) [cite: 540, 544]
struct RMRadarEnemyAmmo {
    uint16_t heroAmmo;
    uint16_t standard3Ammo;
    uint16_t standard4Ammo;
    uint16_t aerialAmmo;
    uint16_t sentryAmmo;
};

// 0x0A04: 对方队伍的宏观状态信息 (8 Bytes) [cite: 546, 550]
struct RMRadarEnemyStatus {
    uint16_t remainingGold;
    uint16_t totalGold;
    uint32_t statusFlags; // 各种占领状态位域
};

// 0x0A05: 对方各机器人当前增益效果 (36 Bytes) [cite: 552, 556]
struct RMRadarEnemyBuff {
    uint8_t heroRecovery;
    uint16_t heroCooling;
    uint8_t heroDefence;
    uint8_t heroVulnerability;
    uint16_t heroAttack;

    uint8_t engineerRecovery;
    uint16_t engineerCooling;
    uint8_t engineerDefence;
    uint8_t engineerVulnerability;
    uint16_t engineerAttack;

    uint8_t standard3Recovery;
    uint16_t standard3Cooling;
    uint8_t standard3Defence;
    uint8_t standard3Vulnerability;
    uint16_t standard3Attack;

    uint8_t standard4Recovery;
    uint16_t standard4Cooling;
    uint8_t standard4Defence;
    uint8_t standard4Vulnerability;
    uint16_t standard4Attack;

    uint8_t sentryRecovery;
    uint16_t sentryCooling;
    uint8_t sentryDefence;
    uint8_t sentryVulnerability;
    uint16_t sentryAttack;

    uint8_t sentryPosture;
};

// 0x0A06: 对方干扰波密钥 (6 Bytes) [cite: 558]
struct RMRadarEnemyPassword {
    uint8_t password[6];
};

#pragma pack(pop)

#endif /*INFANTRY_DATAHUB_REFEREE_PROTOCOL_H*/