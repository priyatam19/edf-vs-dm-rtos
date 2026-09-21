/*
 * FreeRTOS Kernel V10.4.3
 * Copyright (C) 2020 Amazon.com, Inc. or its affiliates.  All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * https://www.FreeRTOS.org
 * https://github.com/FreeRTOS
 *
 */

#ifndef FREERTOS_CONFIG_H
#define FREERTOS_CONFIG_H

#include <avr/io.h>
#include "schedPolicy.h"

/*-----------------------------------------------------------
 * Application specific definitions.
 *
 * These definitions should be adjusted for your particular hardware and
 * application requirements.
 *
 * THESE PARAMETERS ARE DESCRIBED WITHIN THE 'CONFIGURATION' SECTION OF THE
 * FreeRTOS API DOCUMENTATION AVAILABLE ON THE FreeRTOS.org WEB SITE.
 *
 * See https://www.freertos.org/a00110.html.
 *----------------------------------------------------------*/

// And on to the things the same no matter the AVR type...
#define configUSE_PREEMPTION                1

// Define configUSE_IDLE_HOOK
#ifndef configUSE_IDLE_HOOK
    #define configUSE_IDLE_HOOK             1
#endif

#define configUSE_TICK_HOOK                 1
#define configCPU_CLOCK_HZ                  ( ( uint32_t ) F_CPU )          // This F_CPU variable set by the environment

/* Number of priority levels, sized from the active scheduling policy and
 * task count (schedPolicy.h) per upstream ESFree's own configuration rule
 * (thesis sec. 4.2.2): periodic tasks + 1 for RMS/DMS/naive EDF so every
 * task can get a distinct fixed priority below the scheduler task; a fixed
 * 3 for efficient EDF, which only ever uses "running"/"not running"/idle.
 * This is what fixes the priority underflow (tasks assigned priority -1)
 * that a fixed configMAX_PRIORITIES==4 caused with 4 periodic tasks. */
#if( schedSCHEDULING_POLICY == schedSCHEDULING_POLICY_EDF && schedEDF_EFFICIENT == 1 )
    #define configMAX_PRIORITIES ( 3 )
#else
    #define configMAX_PRIORITIES ( schedACTIVE_NUMBER_OF_PERIODIC_TASKS + 1 )
#endif

#define configIDLE_SHOULD_YIELD             1
#define configMINIMAL_STACK_SIZE            ( 192 )
#define configMAX_TASK_NAME_LEN             ( 8 )

#define configQUEUE_REGISTRY_SIZE           0
#define configCHECK_FOR_STACK_OVERFLOW      1

#define configUSE_TRACE_FACILITY            1
#define configUSE_16_BIT_TICKS              1

#define configUSE_MUTEXES                   1
#define configUSE_RECURSIVE_MUTEXES         1
#define configUSE_COUNTING_SEMAPHORES       1
#define configUSE_TIME_SLICING              1
#define configUSE_QUEUE_SETS                0
#define configUSE_MALLOC_FAILED_HOOK        1

#define configSUPPORT_DYNAMIC_ALLOCATION    1
#define configSUPPORT_STATIC_ALLOCATION     0

/* Timer definitions. */
#define configUSE_TIMERS                    1
#define configTIMER_TASK_PRIORITY           ( ( UBaseType_t ) 3 )
#define configTIMER_QUEUE_LENGTH            ( ( UBaseType_t ) 10 )
#define configTIMER_TASK_STACK_DEPTH        ( 85 )

/* Co-routine definitions. */
#define configUSE_CO_ROUTINES               0
#define configMAX_CO_ROUTINE_PRIORITIES     ( (UBaseType_t ) 2 )

/* Set the stack depth type to be uint16_t. */
#define configSTACK_DEPTH_TYPE              uint16_t

/* Set the stack pointer type to be uint16_t, otherwise it defaults to unsigned long */
#define portPOINTER_SIZE_TYPE               uint16_t

/* ESFree stores a pointer to its own per-task bookkeeping struct (SchedTCB_t)
 * in TLS slot schedTHREAD_LOCAL_STORAGE_POINTER_INDEX (0, see
 * scheduler-final.cpp). Without this define the TCB's TLS array is sized 0
 * (see Arduino_FreeRTOS.h's own fallback default), so writing to slot 0
 * corrupts adjacent TCB memory instead of erroring -- this is required, not
 * optional, for every scheduling policy, not just efficient EDF. */
#define configNUM_THREAD_LOCAL_STORAGE_POINTERS 1

/* Set the following definitions to 1 to include the API function, or zero
to exclude the API function. */

#define INCLUDE_vTaskPrioritySet                1
#define INCLUDE_uxTaskPriorityGet               1
#define INCLUDE_vTaskDelete                     1
#define INCLUDE_vTaskCleanUpResources           1
#define INCLUDE_vTaskSuspend                    1
#define INCLUDE_vResumeFromISR                  1
#define INCLUDE_xTaskDelayUntil                 1
#define INCLUDE_vTaskDelay                      1
/* Efficient EDF's vSchedulerReadyTrace() calls xTaskGetSchedulerState().
 * configUSE_TIMERS==1 already pulls that function in regardless of this
 * define (see task.h), but efficient EDF depends on it directly, so it's
 * set explicitly rather than relying on that as a side effect. */
#define INCLUDE_xTaskGetSchedulerState          1
#define INCLUDE_xTaskGetIdleTaskHandle          1 // create an idle task handle.
#define INCLUDE_xTaskGetCurrentTaskHandle       1
#define INCLUDE_uxTaskGetStackHighWaterMark     1
/* Efficient EDF's vSchedulerReadyTrace() also calls
 * xTimerGetTimerDaemonTaskHandle() (see timers.c); documented as required
 * by upstream ESFree's own configuration guide. */
#define INCLUDE_xTimerGetTimerDaemonTaskHandle  1

#if( schedSCHEDULING_POLICY == schedSCHEDULING_POLICY_EDF && schedEDF_EFFICIENT == 1 )
    /*
     * Efficient EDF keeps every non-running ready task at schedPRIORITY_NOT_RUNNING
     * and only raises the head of the ready list to schedPRIORITY_RUNNING, so it
     * depends on the kernel telling it about state transitions via trace macros
     * instead of doing bookkeeping on every tick. Wiring per upstream ESFree's
     * documented configuration (github.com/RobinK2/ESFree master's thesis,
     * "FreeRTOS Configurations for Efficient EDF"), verified against this
     * project's pinned kernel (feilipu/Arduino_FreeRTOS_Library @ 10.4.3-8,
     * src/tasks.c and src/queue.c) for exact macro arity.
     *
     * The three vScheduler*Trace hook functions are defined in
     * scheduler-final.cpp and declared in scheduler.h. They're also
     * forward-declared here because FreeRTOSConfig.h is processed (from
     * Arduino_FreeRTOS.h) before task.h defines TaskHandle_t, and these
     * macros are expanded from inside the kernel's own .c files, which never
     * see scheduler.h at all. The forward declaration below spells
     * TaskHandle_t exactly as task.h itself does
     * (`typedef struct TaskControlBlock_t * TaskHandle_t;`), so when task.h
     * is included later it's a redundant identical typedef, not a conflict
     * -- and scheduler.h's own declarations (seen together with these in
     * scheduler-final.cpp/main.ino) match type-for-type instead of colliding
     * over TaskHandle_t vs. void*.
     */
    #ifdef __cplusplus
    extern "C" {
    #endif
    struct TaskControlBlock_t;
    typedef struct TaskControlBlock_t * TaskHandle_t;
    void vSchedulerBlockTrace( void );
    void vSchedulerSuspendTrace( TaskHandle_t xTaskHandle );
    void vSchedulerReadyTrace( TaskHandle_t xTaskHandle );
    #ifdef __cplusplus
    }
    #endif

    #define traceBLOCKING_ON_QUEUE_RECEIVE( pxQueue )      vSchedulerBlockTrace()
    #define traceBLOCKING_ON_QUEUE_SEND( pxQueue )         vSchedulerBlockTrace()
    #define traceTASK_DELAY_UNTIL( xTimeToWake )           vSchedulerBlockTrace()
    #define traceTASK_SUSPEND( pxTaskToSuspend )           vSchedulerSuspendTrace( pxTaskToSuspend )
    #define traceMOVED_TASK_TO_READY_STATE( pxTCB )        vSchedulerReadyTrace( pxTCB )
#endif /* efficient EDF */

#define configMAX(a,b)  ({ __typeof__ (a) _a = (a); __typeof__ (b) _b = (b); _a > _b ? _a : _b; })
#define configMIN(a,b)  ({ __typeof__ (a) _a = (a); __typeof__ (b) _b = (b); _a < _b ? _a : _b; })

/**
 * configASSERT macro: https://www.freertos.org/a00110.html#configASSERT
 */
#ifndef configASSERT
    #define configDEFAULT_ASSERT 0
#else
    /**
     * Enable configASSERT macro if it is defined.
     */
    #ifndef configDEFAULT_ASSERT
        #define configDEFAULT_ASSERT 1
    #endif

    /**
     * Define a hook method for configASSERT macro if configASSERT is enabled.
     */
    #if configDEFAULT_ASSERT == 1
        extern void vApplicationAssertHook();
        #define configASSERT( x ) if (( x ) == 0) { vApplicationAssertHook(); }
    #endif
#endif


#endif /* FREERTOS_CONFIG_H */
