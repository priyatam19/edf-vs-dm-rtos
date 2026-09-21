#ifndef SCHEDULER_H_
#define SCHEDULER_H_

#include <Arduino_FreeRTOS.h>
#include <task.h>
#include <projdefs.h>
#include <timers.h>
#include <list.h>
#include <croutine.h>
#include <portable.h>
#include <stack_macros.h>
#include <mpu_wrappers.h>
#include <FreeRTOSVariant.h>
#include <message_buffer.h>
#include <semphr.h>
#include <FreeRTOSConfig.h>
#include <stream_buffer.h>
#include <portmacro.h>
#include <event_groups.h>
#include <queue.h>
#include <Arduino.h>
// #include <PerformanceMetrics.h>
#include "schedPolicy.h"


#ifdef __cplusplus
extern "C" {
#endif

/* Scheduling policy selection (schedSCHEDULING_POLICY_{MANUAL,RMS,DMS,EDF},
 * schedEDF_NAIVE/schedEDF_EFFICIENT) lives in schedPolicy.h so that
 * FreeRTOSConfig.h can size configMAX_PRIORITIES and the efficient-EDF trace
 * macros from the same definitions -- see that file for the full rationale.
 * Override schedSCHEDULING_POLICY there or via a build flag; don't redefine
 * it here. */

extern uint8_t schedSchedulingPolicy;      /* Currently active scheduling policy */

/* Maximum number of periodic tasks that can be created. (Scheduler task is
 * not included) Must be >= schedACTIVE_NUMBER_OF_PERIODIC_TASKS (schedPolicy.h). */
#define schedMAX_NUMBER_OF_PERIODIC_TASKS 5

#if( schedACTIVE_NUMBER_OF_PERIODIC_TASKS > schedMAX_NUMBER_OF_PERIODIC_TASKS )
    #error "schedACTIVE_NUMBER_OF_PERIODIC_TASKS exceeds schedMAX_NUMBER_OF_PERIODIC_TASKS"
#endif

#if( schedSCHEDULING_POLICY == schedSCHEDULING_POLICY_EDF && schedEDF_EFFICIENT == 1 )
    /* Functions that must be wired to trace macros for efficient EDF to
     * work -- defined in scheduler-final.cpp, wired in FreeRTOSConfig.h.
     * See FreeRTOSConfig.h for which trace macro drives which function. */
    void vSchedulerBlockTrace( void );
    void vSchedulerSuspendTrace( TaskHandle_t xTaskHandle );
    void vSchedulerReadyTrace( TaskHandle_t xTaskHandle );
#endif /* efficient EDF */

/* Set this define to 1 to enable Timing-Error-Detection for detecting tasks
 * that have missed their deadlines. Tasks that have missed their deadlines
 * will be deleted, recreated and restarted during next period. */
#define schedUSE_TIMING_ERROR_DETECTION_DEADLINE 1

/* Set this define to 1 to enable Timing-Error-Detection for detecting tasks
 * that have exceeded their worst-case execution time. Tasks that have exceeded
 * their worst-case execution time will be preempted until next period. */
#define schedUSE_TIMING_ERROR_DETECTION_EXECUTION_TIME 1

/* Set this define to 1 to enable the scheduler task. This define must be set to 1
* when using following features:
* EDF scheduling policy, Timing-Error-Detection of execution time,
* Timing-Error-Detection of deadline, Polling Server. */
#define schedUSE_SCHEDULER_TASK 1


#if( schedUSE_SCHEDULER_TASK == 1 )
	/* Priority of the scheduler task. */
	#define schedSCHEDULER_PRIORITY ( configMAX_PRIORITIES - 1 )
	/* Stack size of the scheduler task. */
	#define schedSCHEDULER_TASK_STACK_SIZE 200 
	/* The period of the scheduler task in software ticks. */
	#define schedSCHEDULER_TASK_PERIOD pdMS_TO_TICKS( 100 )	
#endif /* schedUSE_SCHEDULER_TASK */

/* This function must be called before any other function call from scheduler.h. */
void vSchedulerInit( void );

/* Sets the scheduling policy. */
void vSchedulerSetPolicy(uint8_t policy);

/* Creates a periodic task.
 *
 * pvTaskCode: The task function.
 * pcName: Name of the task.
 * usStackDepth: Stack size of the task in words, not bytes.
 * pvParameters: Parameters to the task function.
 * uxPriority: Priority of the task. (Only used when scheduling policy is set to manual)
 * pxCreatedTask: Pointer to the task handle.
 * xPhaseTick: Phase given in software ticks. Counted from when vSchedulerStart is called.
 * xPeriodTick: Period given in software ticks.
 * xMaxExecTimeTick: Worst-case execution time given in software ticks.
 * xDeadlineTick: Relative deadline given in software ticks.
 * */
void vSchedulerPeriodicTaskCreate( TaskFunction_t pvTaskCode, const char *pcName, UBaseType_t uxStackDepth, void *pvParameters, UBaseType_t uxPriority,
		TaskHandle_t *pxCreatedTask, TickType_t xPhaseTick, TickType_t xPeriodTick, TickType_t xMaxExecTimeTick, TickType_t xDeadlineTick );

/* Deletes a periodic task associated with the given task handle. */
void vSchedulerPeriodicTaskDelete( TaskHandle_t xTaskHandle );

/* Starts scheduling tasks. */
void vSchedulerStart( void );


void initializePerformanceMetrics();
void printMetrics();

/* Dumps the buffered trace-event log (job release/completion/deadline-miss/
 * overrun events) as CSV -- also called once at the end of printMetrics(). */
void dumpTraceCSV();

#ifdef __cplusplus
}
#endif


#endif /* SCHEDULER_H_ */
