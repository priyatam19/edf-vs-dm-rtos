/* Test-control API for the host/fakes/ FreeRTOS stand-ins. Lets a test set
 * the fake clock, inspect what vTaskPrioritySet()/vTaskSuspend() etc. were
 * last called with for a given handle, and reset all fake state between
 * tests. */
#pragma once

#include "Arduino_FreeRTOS.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Resets every fake task slot, the fake clock, and the assert-fired flag.
 * Call at the start of every test. */
void schedtest_Reset( void );

/* Sets the value xTaskGetTickCount()/xTaskGetTickCountFromISR() return. */
void schedtest_SetTick( TickType_t xTick );

/* Returns the priority last set via vTaskPrioritySet() for this handle (or
 * the priority it was created with, if never changed). */
UBaseType_t schedtest_GetPriority( TaskHandle_t xTask );

/* Returns pdTRUE if vTaskSuspend()/vTaskResume() left this handle suspended. */
BaseType_t schedtest_IsSuspended( TaskHandle_t xTask );

/* Returns pdTRUE if vTaskDelete() was called on this handle. */
BaseType_t schedtest_IsDeleted( TaskHandle_t xTask );

/* Number of live (created, not deleted) fake task slots. */
int schedtest_LiveTaskCount( void );

/* pdTRUE if configASSERT() fired (via vApplicationAssertHook()) since the
 * last schedtest_Reset(). */
BaseType_t schedtest_AssertFired( void );

#ifdef __cplusplus
}
#endif
