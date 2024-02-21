/*
 *  ======== ti_freertos_config.c ========
 *  Configured FreeRTOS module definitions
 *
 *  DO NOT EDIT - This file is generated
 *  by the SysConfig tool.
 */

#include <stddef.h>
#include <stdint.h>

#include "FreeRTOSConfig.h"

/* C files contributed by /freertos/FreeRTOS */
#include <../../Source/list.c>
#include <../../Source/queue.c>
#include <../../Source/tasks.c>
#include <../../Source/timers.c>
#include <../../Source/croutine.c>
#include <../../Source/event_groups.c>
#include <../../Source/stream_buffer.c>
#include <../../Source/portable/MemMang/heap_4.c>

/* C files contributed by /freertos/dpl/Settings */
#include <dpl/AppHooks_freertos.c>
#include <dpl/DebugP_freertos.c>
#include <dpl/MutexP_freertos.c>
#include <dpl/QueueP_freertos.c>
#include <dpl/SemaphoreP_freertos.c>
#include <dpl/StaticAllocs_freertos.c>
#include <dpl/SwiP_freertos.c>
#include <dpl/SystemP_freertos.c>
#include <dpl/TaskP_freertos.c>
#include <dpl/ClockP_freertos.c>
#include <dpl/HwiPCC32XX_freertos.c>
#include <dpl/PowerCC32XX_freertos.c>
#include <dpl/TimestampPCC32XX_freertos.c>
#include <startup/startup_cc32xx_iar.c>

/* C files contributed by /ti/posix/freertos/Settings */
#define TI_POSIX_FREERTOS_MEMORY_ENABLEADV

#include <ti/posix/freertos/clock.c>
#include <ti/posix/freertos/memory.c>
#include <ti/posix/freertos/mqueue.c>
#include <ti/posix/freertos/pthread_barrier.c>
#include <ti/posix/freertos/pthread.c>
#include <ti/posix/freertos/pthread_cond.c>
#include <ti/posix/freertos/pthread_mutex.c>
#include <ti/posix/freertos/pthread_rwlock.c>
#include <ti/posix/freertos/sched.c>
#include <ti/posix/freertos/semaphore.c>
#include <ti/posix/freertos/sleep.c>
#include <ti/posix/freertos/timer.c>
#include <ti/posix/freertos/Mtx.c>


/* Wrapper functions for using the queue registry regardless of whether it is enabled or disabled */
void vQueueAddToRegistryWrapper(QueueHandle_t xQueue, const char * pcQueueName)
{
    /* This function is intentionally left empty as the Queue Registry is disabled */
}

void vQueueUnregisterQueueWrapper(QueueHandle_t xQueue)
{
    /* This function is intentionally left empty as the Queue Registry is disabled */
}
