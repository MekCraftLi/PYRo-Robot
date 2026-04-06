/**
 *******************************************************************************
 * @file    seq-variable.h
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

#ifndef INFANTRY_CHASSIS_SEQ_VARIABLE_H
#define INFANTRY_CHASSIS_SEQ_VARIABLE_H



/*-------- 1. includes and imports -----------------------------------------------------------------------------------*/

#include "FreeRTOS.h"
#include "task.h"



/*-------- 2. enum and define ----------------------------------------------------------------------------------------*/




/*-------- 3. interface ----------------------------------------------------------------------------------------------*/

// 内存屏障，防止编译器重排指令 (ARM Cortex-M 适用)
#define MEMORY_BARRIER() __asm volatile ("dmb" ::: "memory")

/**
 * @brief 顺序锁包装器 (Seqlock)
 * @tparam T 必须是可通过赋值拷贝的平凡数据结构 (POD)
 */
template <typename T>
class SeqVariable {
public:
    SeqVariable() : m_seq(0) {}

    /**
     * @brief 写者接口 (生产者如 ISR 或解析任务调用)
     * @note 会短暂关闭中断以保证结构体拷贝时不被更高优先级抢占
     */
    void write(const T& new_data) {
        // 屏蔽操作系统的任务调度和普通中断，耗时仅几十纳秒
        taskENTER_CRITICAL();

        m_seq = m_seq + 1; // 奇数：标记“正在写入脏数据”
        MEMORY_BARRIER();

        m_data = new_data; // 核心：多变量整体内存拷贝

        MEMORY_BARRIER();
        m_seq = m_seq + 1; // 偶数：标记“写入完成，数据干净”

        taskEXIT_CRITICAL(); // 恢复中断
    }
    void writeFromISR(const T& data) {
        // 中断级关中断（保存当前中断状态掩码）
        uint32_t ulSavedInterruptStatus = taskENTER_CRITICAL_FROM_ISR();
        m_seq = m_seq + 1; // 奇数：标记“正在写入脏数据”
        MEMORY_BARRIER();
        m_data = data; // 执行数据拷贝
        MEMORY_BARRIER();
        m_seq = m_seq + 1; // 偶数：标记“写入完成，数据干净”

        // 恢复中断状态掩码
        taskEXIT_CRITICAL_FROM_ISR(ulSavedInterruptStatus);
    }
    /**
     * @brief 读者接口 (消费者如控制任务调用)
     * @note 绝对无锁，绝对不阻塞，通过自旋重试保证数据完整
     */
    void read(T& out_data) const {
        uint32_t seq_before;
        do {
            seq_before = m_seq;
            MEMORY_BARRIER();

            // 如果 seq 是奇数，说明此时写者正在操作，自旋等待 (在单核中其实极少发生，除非同优先级)
            if (seq_before & 1) {
                continue;
            }

            out_data = m_data; // 拷贝走整个结构体

            MEMORY_BARRIER();

            // 关键逻辑：如果拷贝完发现版本号变了，说明刚才拷贝进行到一半时，
            // 被高优先级的写者抢占并覆盖了数据，这组数据作废，重新执行拷贝！
        } while (seq_before != m_seq);
    }

private:
    volatile uint32_t m_seq; // 必须 volatile，强制每次从内存读取，防止编译器优化掉 while 循环
    T m_data;
};


/*-------- 4. decorator ----------------------------------------------------------------------------------------------*/




/*-------- 5. factories ----------------------------------------------------------------------------------------------*/





#endif /*INFANTRY_CHASSIS_SEQ_VARIABLE_H*/
