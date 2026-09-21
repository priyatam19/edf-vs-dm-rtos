/* Implementation of the host/fakes/ FreeRTOS task API stand-ins -- see
 * task.h and fakes.h for the rationale. No real scheduling happens here:
 * xTaskDelayUntil() just advances *pxPreviousWakeTime and returns
 * immediately, since the code under test in host/test/ never runs the
 * (blocking, infinite-loop) task functions directly, only the synchronous
 * bookkeeping functions around them. */
#include "Arduino_FreeRTOS.h"
#include "fakes.h"

#include <string.h>
#include <stdlib.h>

#define SCHEDTEST_MAX_TASKS 16

struct TaskControlBlock_t
{
    int inUse;
    UBaseType_t priority;
    BaseType_t suspended;
    BaseType_t deleted;
    void *tls0;
};

static struct TaskControlBlock_t xTasks[ SCHEDTEST_MAX_TASKS ];
static TickType_t xFakeTick = 0;
static BaseType_t xAssertFired = pdFALSE;

void schedtest_Reset( void )
{
    memset( xTasks, 0, sizeof( xTasks ) );
    xFakeTick = 0;
    xAssertFired = pdFALSE;
}

void schedtest_SetTick( TickType_t xTick )
{
    xFakeTick = xTick;
}

UBaseType_t schedtest_GetPriority( TaskHandle_t xTask )
{
    return ( (struct TaskControlBlock_t *) xTask )->priority;
}

BaseType_t schedtest_IsSuspended( TaskHandle_t xTask )
{
    return ( (struct TaskControlBlock_t *) xTask )->suspended;
}

BaseType_t schedtest_IsDeleted( TaskHandle_t xTask )
{
    return ( (struct TaskControlBlock_t *) xTask )->deleted;
}

int schedtest_LiveTaskCount( void )
{
    int count = 0;
    for( int i = 0; i < SCHEDTEST_MAX_TASKS; i++ )
    {
        if( xTasks[ i ].inUse && !xTasks[ i ].deleted )
        {
            count++;
        }
    }
    return count;
}

BaseType_t schedtest_AssertFired( void )
{
    return xAssertFired;
}

void vApplicationAssertHook( void )
{
    xAssertFired = pdTRUE;
}

BaseType_t xTaskCreate( TaskFunction_t pxTaskCode, const char * const pcName, const uint16_t usStackDepth,
                         void * const pvParameters, UBaseType_t uxPriority, TaskHandle_t * const pxCreatedTask )
{
    (void) pxTaskCode;
    (void) pcName;
    (void) usStackDepth;
    (void) pvParameters;

    for( int i = 0; i < SCHEDTEST_MAX_TASKS; i++ )
    {
        if( !xTasks[ i ].inUse )
        {
            xTasks[ i ].inUse = 1;
            xTasks[ i ].priority = uxPriority;
            xTasks[ i ].suspended = pdFALSE;
            xTasks[ i ].deleted = pdFALSE;
            xTasks[ i ].tls0 = NULL;
            if( pxCreatedTask != NULL )
            {
                *pxCreatedTask = &xTasks[ i ];
            }
            return pdPASS;
        }
    }
    return pdFAIL;
}

void vTaskDelete( TaskHandle_t xTaskToDelete )
{
    if( xTaskToDelete != NULL )
    {
        ( (struct TaskControlBlock_t *) xTaskToDelete )->deleted = pdTRUE;
    }
}

void vTaskPrioritySet( TaskHandle_t xTask, UBaseType_t uxNewPriority )
{
    ( (struct TaskControlBlock_t *) xTask )->priority = uxNewPriority;
}

UBaseType_t uxTaskPriorityGet( TaskHandle_t xTask )
{
    return ( (struct TaskControlBlock_t *) xTask )->priority;
}

void vTaskSuspend( TaskHandle_t xTaskToSuspend )
{
    ( (struct TaskControlBlock_t *) xTaskToSuspend )->suspended = pdTRUE;
}

void vTaskResume( TaskHandle_t xTaskToResume )
{
    ( (struct TaskControlBlock_t *) xTaskToResume )->suspended = pdFALSE;
}

TickType_t xTaskGetTickCount( void )
{
    return xFakeTick;
}

TickType_t xTaskGetTickCountFromISR( void )
{
    return xFakeTick;
}

BaseType_t xTaskDelayUntil( TickType_t * const pxPreviousWakeTime, const TickType_t xTimeIncrement )
{
    *pxPreviousWakeTime = ( TickType_t ) ( *pxPreviousWakeTime + xTimeIncrement );
    return pdTRUE;
}

TaskHandle_t xTaskGetCurrentTaskHandle( void )
{
    return NULL;
}

TaskHandle_t xTaskGetIdleTaskHandle( void )
{
    return NULL;
}

BaseType_t xTaskGetSchedulerState( void )
{
    return taskSCHEDULER_RUNNING;
}

void vTaskSetThreadLocalStoragePointer( TaskHandle_t xTaskToSet, BaseType_t xIndex, void * pvValue )
{
    (void) xIndex; /* only slot 0 is used by scheduler-final.cpp */
    ( (struct TaskControlBlock_t *) xTaskToSet )->tls0 = pvValue;
}

void * pvTaskGetThreadLocalStoragePointer( TaskHandle_t xTaskToQuery, BaseType_t xIndex )
{
    (void) xIndex;
    if( xTaskToQuery == NULL )
    {
        return NULL;
    }
    return ( (struct TaskControlBlock_t *) xTaskToQuery )->tls0;
}

void vTaskNotifyGiveFromISR( TaskHandle_t xTaskToNotify, BaseType_t * pxHigherPriorityTaskWoken )
{
    (void) xTaskToNotify;
    if( pxHigherPriorityTaskWoken != NULL )
    {
        *pxHigherPriorityTaskWoken = pdFALSE;
    }
}

uint32_t ulTaskNotifyTake( BaseType_t xClearCountOnExit, TickType_t xTicksToWait )
{
    (void) xClearCountOnExit;
    (void) xTicksToWait;
    return 0;
}

void vTaskStartScheduler( void )
{
}

void vTaskEndScheduler( void )
{
}

void * pvPortMalloc( size_t xSize )
{
    return malloc( xSize );
}

void vPortFree( void * pv )
{
    free( pv );
}

TaskHandle_t xTimerGetTimerDaemonTaskHandle( void )
{
    return NULL;
}
