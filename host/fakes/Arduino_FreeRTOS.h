/*
 * Minimal host-side stand-in for Arduino_FreeRTOS.h, used only to compile
 * and unit test Code/scheduler-final.cpp natively (PlatformIO `native` env,
 * see platformio.ini's [env:native] and host/test/). Not a FreeRTOS port:
 * there is no real scheduler here, so xTaskCreate/xTaskDelayUntil/etc. are
 * simple state-recording fakes (see task.h + fakes.c), not working
 * implementations. This is enough because the functions under test
 * (priority assignment, deadline/overrun checking, TCB array bookkeeping)
 * are plain synchronous functions, not the task loop itself -- see
 * host/fakes/README.md.
 *
 * Core types/macros mirror the real Arduino_FreeRTOS.h closely enough that
 * the vendored, unmodified list.h/list.c (pinned to the same kernel tag
 * this project builds against, 10.4.3-8) compile and behave identically to
 * hardware, including TickType_t being 16-bit -- matching
 * configUSE_16_BIT_TICKS==1 in the real FreeRTOSConfig.h -- so a tick
 * wraparound test here means the same thing it would on the ATmega2560.
 */
#pragma once

#define INC_ARDUINO_FREERTOS_H /* required by the real, vendored list.h/task.h */

#include <stdint.h>
#include <stddef.h>

#include "FreeRTOSConfig.h"

typedef int32_t BaseType_t;
typedef uint32_t UBaseType_t;
typedef uint16_t TickType_t; /* configUSE_16_BIT_TICKS == 1, matching the real config */

#define pdFALSE ( ( BaseType_t ) 0 )
#define pdTRUE  ( ( BaseType_t ) 1 )
#define pdPASS  ( pdTRUE )
#define pdFAIL  ( pdFALSE )

#define portMAX_DELAY ( ( TickType_t ) 0xffff )
#define tskIDLE_PRIORITY ( ( UBaseType_t ) 0U )

#define configLIST_VOLATILE
#define mtCOVERAGE_TEST_MARKER()
#define mtCOVERAGE_TEST_DELAY()
#define PRIVILEGED_FUNCTION /* MPU wrappers not used; real portable.h defines this via FreeRTOSConfig.h's configENABLE_MPU */

#ifdef __cplusplus
extern "C" {
#endif
void vApplicationAssertHook( void );
#ifdef __cplusplus
}
#endif

#define configASSERT( x ) if( ( x ) == 0 ) { vApplicationAssertHook(); }

#include "task.h"
#include "list.h"
