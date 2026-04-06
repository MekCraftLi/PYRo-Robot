/**
*******************************************************************************
* @file    application-base.h
* @brief
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
* @date    2026/2/4
* @version 1.0
*******************************************************************************
*/

#pragma once



/* ------- define ----------------------------------------------------------------------------------------------------*/




/* ------- include ---------------------------------------------------------------------------------------------------*/

/* I. OS */
#include "FreeRTOS.h"
#include "event_groups.h"
#include "queue.h"
#include "task.h"

/* II. standard lib */
#include "tools/perf-monitor.h"


#include <cstring>
#include <vector>



/* ------- task strategy ---------------------------------------------------------------------------------------------*/




/* ------- class prototypes-------------------------------------------------------------------------------------------*/

/**
 * @brief 顶层应用接口
 */
class IApplication {
public:
    virtual ~IApplication() = default;

    virtual void init() = 0;
    virtual void run()  = 0;

    // 供内部 Task Entry 调用的主循环
    virtual void taskLoop() = 0;

    [[nodiscard]] virtual const char* getName() const            = 0;
    [[nodiscard]] virtual float getRunTime() const               = 0;
    [[nodiscard]] virtual uint32_t getStackHighWaterMark() const = 0;
    [[nodiscard]] virtual TaskHandle_t getTaskHandle() const     = 0;

    virtual void initEvent() = 0;
    virtual void waitInit()  = 0;
};

/* ===================================================================================================================*/
/* 1. 核心基类 (仅负责纯粹的 OS 任务资源分配与统一启动，无任何通信杂质)                                                 */
/* ===================================================================================================================*/

class StaticAppBase : public IApplication {
  protected:
    class TaskInfo {
      public:
        TaskInfo(bool enable, const char* name, uint16_t stackSize, IApplication* pThread, UBaseType_t priority,
                 TaskHandle_t* pTskHandle, StackType_t* stackBuf, StaticTask_t* pxTask)
            : enable(enable), name(name), stackSize(stackSize), pThread(pThread), priority(priority),
              pTskHandle(pTskHandle), stackBuf(stackBuf), pxTask(pxTask) {}

        bool enable;
        const char* name;
        uint16_t stackSize;
        IApplication* pThread;
        UBaseType_t priority;
        TaskHandle_t* pTskHandle;
        StackType_t* stackBuf;
        StaticTask_t* pxTask;
    };

    inline static std::vector<TaskInfo> _taskInfoRegistry;

    EventGroupHandle_t _initEvent = nullptr;
    StaticEventGroup_t _staticEventGroup{};
    TaskHandle_t _tskHandle = nullptr;
    StaticTask_t _staticTask;
    float _runTime = 0;
    TaskInfo _taskInfo;
    inline static uint8_t _inited = 0;
    // [新增] 为每个线程实例分配独立的性能统计内存块
    PerfCounter::Statistics _perfStats{};

    // [新增] NVI (Non-Virtual Interface) 包转函数，自动完成 RAII 性能统计
    inline void executeRun() {
        PerfCounter pc(_perfStats); // 实例化即开始计时，离开作用域自动结算
        run();                      // 执行用户的业务逻辑
    }

  public:
    StaticAppBase(bool enable, const char* name, uint16_t stackSize, StackType_t* stackBuf, UBaseType_t priority)
        : _taskInfo(enable, name, stackSize, this, priority, &_tskHandle, stackBuf, &_staticTask) {
        _initEvent = xEventGroupCreateStatic(&_staticEventGroup);
        _taskInfoRegistry.push_back(_taskInfo);
    }

    static void startApplications() {
        for (auto& eachApp : _taskInfoRegistry) {
            if (eachApp.enable) {
                *(eachApp.pTskHandle) = xTaskCreateStatic(_taskEntry, eachApp.name, eachApp.stackSize, eachApp.pThread,
                                                          eachApp.priority, eachApp.stackBuf, eachApp.pxTask);
            }
        }
    }

    void initEvent() override { xEventGroupSetBits(_initEvent, 0x01); }
    void waitInit() override { xEventGroupWaitBits(_initEvent, 0x01, pdFALSE, pdFALSE, portMAX_DELAY); }

    [[nodiscard]] const char* getName() const override { return _taskInfo.name; }
    [[nodiscard]] uint32_t getStackHighWaterMark() const override { return uxTaskGetStackHighWaterMark(_tskHandle); }
    [[nodiscard]] TaskHandle_t getTaskHandle() const override { return _tskHandle; }
    // [修改] 利用 FreeRTOS 宏直接将 CPU 周期转化为微秒 (us) 并返回单次执行耗时
    [[nodiscard]] float getRunTime() const override {
        return (float)_perfStats.last_ticks * 1000000.0f / configCPU_CLOCK_HZ;
    }
    // [可选新增] 如果你想获取最大耗时，也可以暴露此接口
    [[nodiscard]] float getMaxRunTime() const {
        return (float)_perfStats.max_ticks * 1000000.0f / configCPU_CLOCK_HZ;
    }
  private:

    [[noreturn]] static void _taskEntry(void* pvParameters) {
        auto* threadObj = static_cast<IApplication*>(pvParameters);
        threadObj->init();
        threadObj->initEvent();
        _inited = 1;

        // 多态调用子类实现的具体调度循环
        threadObj->taskLoop();

        vTaskDelete(nullptr);
        for(;;){}
    }
};

/* ===================================================================================================================*/
/* 2. 调度特化类 (定义了不同任务的行为，并暴露出相应的外部接口)                                                          */
/* ===================================================================================================================*/

/**
 * @brief 周期任务 (无对外暴露接口)
 */
class PeriodicApp : public StaticAppBase {
  protected:
    uint32_t _periodMs;

    void taskLoop() override {
        TickType_t xLastExecutionTime = xTaskGetTickCount();
        const TickType_t kPeriodTicks = pdMS_TO_TICKS(_periodMs);
        for (;;) {
            executeRun();
            vTaskDelayUntil(&xLastExecutionTime, kPeriodTicks);
        }
    }

  public:
    PeriodicApp(bool enable, const char* name, uint16_t stackSize, StackType_t* stackBuf, UBaseType_t priority, uint32_t periodMs)
        : StaticAppBase(enable, name, stackSize, stackBuf, priority), _periodMs(periodMs) {}
};

/**
 * @brief 连续任务 (无对外暴露接口)
 */
class ContinuousApp : public StaticAppBase {
  protected:
    void taskLoop() override {
        for (;;) {
            executeRun();
        }
    }

  public:
    using StaticAppBase::StaticAppBase; // 继承构造函数
};

/**
 * @brief 通知任务 (专有接口：notify, notifyFromISR)
 */
class NotifyApp : public StaticAppBase {
  protected:
    TickType_t _timeoutTicks;

    void taskLoop() override {
        for (;;) {
            ulTaskNotifyTake(pdTRUE, _timeoutTicks);
            executeRun();
        }
    }

  public:
    NotifyApp(bool enable, const char* name, uint16_t stackSize, StackType_t* stackBuf, UBaseType_t priority, TickType_t timeoutTicks = portMAX_DELAY)
        : StaticAppBase(enable, name, stackSize, stackBuf, priority), _timeoutTicks(timeoutTicks) {}

    // --- 专有接口：仅通知任务拥有 ---
    void notify() {
        if (_tskHandle) xTaskNotifyGive(_tskHandle);
    }

    void notifyFromISR(BaseType_t* pxHigherPriorityTaskWoken) {
        if (_tskHandle) vTaskNotifyGiveFromISR(_tskHandle, pxHigherPriorityTaskWoken);
    }
};

/**
 * @brief 队列任务 (专有接口：sendMsg, sendMsgFromISR，并独占队列资源)
 */
class QueueApp : public StaticAppBase {
  public:
    class IPCMsg {
      public:
        IPCMsg() : pMsg(nullptr), msgLen(0) {}
        IPCMsg(void* pMsg, const uint16_t msgLen) : pMsg(pMsg), msgLen(msgLen) {}
        void* pMsg;
        uint16_t msgLen;
    };

  protected:
    QueueHandle_t _commQueue = nullptr;
    StaticQueue_t _staticQueue{};
    TickType_t _timeoutTicks;
    IPCMsg _currentMsg; // 存放当前收到的消息供 run() 读取

    void taskLoop() override {
        for (;;) {
            if (xQueueReceive(_commQueue, &_currentMsg, _timeoutTicks) == pdTRUE) {
                executeRun();
            } else {
                _currentMsg.pMsg = nullptr;
                _currentMsg.msgLen = 0;
                executeRun(); // 超时也会触发 run()，业务层通过 pMsg 是否为空判断
            }
        }
    }

  public:
    QueueApp(bool enable, const char* name, uint16_t stackSize, StackType_t* stackBuf, UBaseType_t priority,
             uint16_t msgQueueSize, uint8_t* queueBuf, TickType_t timeoutTicks = portMAX_DELAY)
        : StaticAppBase(enable, name, stackSize, stackBuf, priority), _timeoutTicks(timeoutTicks) {
        // 只有队列任务才会在构造时申请队列内存
        if (queueBuf != nullptr && msgQueueSize != 0) {
            _commQueue = xQueueCreateStatic(msgQueueSize, sizeof(IPCMsg), queueBuf, &_staticQueue);
        }
    }

    // --- 专有接口：仅队列任务拥有 ---
    BaseType_t sendMsg(void* pMsg, uint16_t msgLen, TickType_t ticksToWait = 0) {
        if (!_commQueue) return pdFAIL;
        IPCMsg msg(pMsg, msgLen);
        return xQueueSend(_commQueue, &msg, ticksToWait);
    }

    BaseType_t sendMsgFromISR(void* pMsg, uint16_t msgLen, BaseType_t* pxHigherPriorityTaskWoken) {
        if (!_commQueue) return pdFAIL;
        IPCMsg msg(pMsg, msgLen);
        return xQueueSendFromISR(_commQueue, &msg, pxHigherPriorityTaskWoken);
    }

    // 获取当前消息的便捷方法
    const IPCMsg& getCurrentMsg() const { return _currentMsg; }
};
/* ------- macro -----------------------------------------------------------------------------------------------------*/




/* ------- variables -------------------------------------------------------------------------------------------------*/





/* ------- function implement ----------------------------------------------------------------------------------------*/
