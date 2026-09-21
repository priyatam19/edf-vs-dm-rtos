/*
 * Minimal host-side stand-in for FreeRTOS's task.h -- declares only the
 * types/functions Code/scheduler-final.cpp actually calls (see the grep
 * this was scoped from, in the PR/commit that added host/). Implementations
 * are simple state-recording fakes in fakes.c: there is no real scheduler
 * on the host, so xTaskDelayUntil et al. never block. That's fine because
 * the functions under test in host/test/ are the synchronous bookkeeping
 * functions (priority assignment, deadline/overrun checks, TCB array
 * management), not the task loop itself.
 */
#ifndef INC_TASK_H
#define INC_TASK_H

#ifndef INC_ARDUINO_FREERTOS_H
    #error "Arduino_FreeRTOS.h must be included before task.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

struct TaskControlBlock_t;
typedef struct TaskControlBlock_t * TaskHandle_t;
typedef void (* TaskFunction_t)( void * );

typedef enum
{
    eRunning = 0,
    eReady,
    eBlocked,
    eSuspended,
    eDeleted,
    eInvalid
} eTaskState;

#define taskSCHEDULER_SUSPENDED   ( ( BaseType_t ) 0 )
#define taskSCHEDULER_NOT_STARTED ( ( BaseType_t ) 1 )
#define taskSCHEDULER_RUNNING     ( ( BaseType_t ) 2 )

BaseType_t xTaskCreate( TaskFunction_t pxTaskCode, const char * const pcName, const uint16_t usStackDepth,
                         void * const pvParameters, UBaseType_t uxPriority, TaskHandle_t * const pxCreatedTask );
void vTaskDelete( TaskHandle_t xTaskToDelete );
void vTaskPrioritySet( TaskHandle_t xTask, UBaseType_t uxNewPriority );
UBaseType_t uxTaskPriorityGet( TaskHandle_t xTask );
void vTaskSuspend( TaskHandle_t xTaskToSuspend );
void vTaskResume( TaskHandle_t xTaskToResume );

TickType_t xTaskGetTickCount( void );
TickType_t xTaskGetTickCountFromISR( void );
BaseType_t xTaskDelayUntil( TickType_t * const pxPreviousWakeTime, const TickType_t xTimeIncrement );

TaskHandle_t xTaskGetCurrentTaskHandle( void );
TaskHandle_t xTaskGetIdleTaskHandle( void );
BaseType_t xTaskGetSchedulerState( void );

void vTaskSetThreadLocalStoragePointer( TaskHandle_t xTaskToSet, BaseType_t xIndex, void * pvValue );
void * pvTaskGetThreadLocalStoragePointer( TaskHandle_t xTaskToQuery, BaseType_t xIndex );

void vTaskNotifyGiveFromISR( TaskHandle_t xTaskToNotify, BaseType_t * pxHigherPriorityTaskWoken );
uint32_t ulTaskNotifyTake( BaseType_t xClearCountOnExit, TickType_t xTicksToWait );

void vTaskStartScheduler( void );
void vTaskEndScheduler( void );

void * pvPortMalloc( size_t xSize );
void vPortFree( void * pv );

#define taskENTER_CRITICAL() do {} while( 0 )
#define taskEXIT_CRITICAL()  do {} while( 0 )

/* Timer daemon handle, used by efficient EDF only (not exercised by the
 * native test suite -- see host/fakes/README.md); provided so the symbol
 * still links if it's ever referenced. */
TaskHandle_t xTimerGetTimerDaemonTaskHandle( void );

#ifdef __cplusplus
}
#endif

#endif /* INC_TASK_H */
