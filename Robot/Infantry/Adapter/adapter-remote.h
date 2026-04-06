/**
 *******************************************************************************
 * @file    adapter-remote.h
 * @brief   定义统一的遥控器接口类型
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
 * @date    2026/2/9
 * @version 1.0
 *******************************************************************************
 */


/* Define to prevent recursive inclusion -----------------------------------------------------------------------------*/

#ifndef INFANTRY_CHASSIS_ADAPTER_REMOTE_H
#define INFANTRY_CHASSIS_ADAPTER_REMOTE_H



/*-------- 1. includes and imports -----------------------------------------------------------------------------------*/

#include "../System/Input/Control.h"
#include <string>
#include <vector>



/*-------- 2. enum and define ----------------------------------------------------------------------------------------*/

/**
 * @brief 语义轴 ID (Semantic Axis IDs)
 * 定义机器人通用的控制轴，不依赖具体硬件通道名
 * 参考文档: "定义机器人支持的所有语义功能" [cite: 30]
 */
enum class AxisID {
    MoveX,     // 底盘左右平移
    MoveY,     // 底盘前后平移
    ViewYaw,   // 云台左右旋转
    ViewPitch, // 云台俯仰
    Aux1,      // 辅助轴 (如拨轮)
    MAX_AXIS
};

/**
 * @brief 语义按键 ID (Semantic Button IDs)
 * 定义通用的开关/按键功能
 */
enum class ButtonID {
    ModeSwitch, // 模式切换开关
    FuncSwitch, // 功能触发开关 (如发射/摩擦轮)
    KeyA,       // 通用按键 A (备用)
    KeyB,       // 通用按键 B (备用)
    MAX_BUTTON
};



/*-------- 3. interface ----------------------------------------------------------------------------------------------*/



/**
 * @brief 逻辑遥控器基类 (Abstract Base Class)
 * 职责:
 * 1. 屏蔽硬件差异 (Adapter Pattern)
 * 2. 提供统一的查询接口
 * 3. 所有的具体遥控器 (Remote_DR16, Remote_Xbox) 必须继承此类
 */
class RemoteBase {
  public:
    virtual ~RemoteBase()                         = default;

    // =============================================================
    // 核心接口：获取原子控件 (Accessors)
    // 业务层通过这些接口获取 Control 对象，然后绑定到 Action
    // =============================================================

    /**
     * @brief 获取指定语义的轴控件
     * @return IInputControl* (可能为空，需检查)
     */
    virtual IInputControl* getAxis(AxisID id)     = 0;

    /**
     * @brief 获取指定语义的按键/开关控件
     * @return IInputControl*
     */
    virtual IInputControl* getButton(ButtonID id) = 0;

    // =============================================================
    // 可选：全局状态查询 (方便调试或快速访问)
    // =============================================================

    // 是否连接 (看门狗状态)
    virtual bool isConnected() const              = 0;
    virtual void onDataReceived()                 = 0;
};


/*-------- 4. decorator ----------------------------------------------------------------------------------------------*/




/*-------- 5. factories ----------------------------------------------------------------------------------------------*/





#endif /*INFANTRY_CHASSIS_ADAPTER_REMOTE_H*/
