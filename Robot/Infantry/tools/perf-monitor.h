/**
 *******************************************************************************
 * @file    perf-monitor.h
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
 * @date    2026/3/1
 * @version 1.0
 *******************************************************************************
 */


/* Define to prevent recursive inclusion -----------------------------------------------------------------------------*/

#ifndef INFANTRY_PERF_MONITOR_H
#define INFANTRY_PERF_MONITOR_H



/*-------- 1. includes and imports -----------------------------------------------------------------------------------*/

#include "stm32h7xx.h"


#include <cstdint>



/*-------- 2. enum and define ----------------------------------------------------------------------------------------*/




/*-------- 3. interface ----------------------------------------------------------------------------------------------*/


/**
 * @brief 性能计时助手 (RAII)
 * 只要在函数开头实例化，生命周期结束时自动统计并更新最大/平均耗时
 */
class PerfCounter {
public:
    struct Statistics {
        uint32_t last_ticks;
        uint32_t max_ticks;
        uint64_t total_ticks;
        uint32_t count;
    };

    // 传入该任务对应的统计结构体引用
    PerfCounter(Statistics& stats) : _stats(stats) {
        _start_tick =  DWT->CYCCNT; // 记录起始时钟周期
    }

    ~PerfCounter() {
        uint32_t end_tick = DWT->CYCCNT;
        uint32_t elapsed = end_tick - _start_tick;

        _stats.last_ticks = elapsed;
        if (elapsed > _stats.max_ticks) _stats.max_ticks = elapsed;
        _stats.total_ticks += elapsed;
        _stats.count++;
    }

private:
    Statistics& _stats;
    uint32_t _start_tick;
};


/*-------- 4. decorator ----------------------------------------------------------------------------------------------*/




/*-------- 5. factories ----------------------------------------------------------------------------------------------*/





#endif /*INFANTRY_PERF_MONITOR_H*/
