/* Minimal stand-in for FreeRTOS's projdefs.h -- only pdMS_TO_TICKS is
 * actually used by Code/scheduler-final.cpp / main.ino. */
#pragma once

#define pdMS_TO_TICKS( xTimeInMs ) \
    ( ( TickType_t ) ( ( ( TickType_t ) ( xTimeInMs ) * ( TickType_t ) configTICK_RATE_HZ ) / ( TickType_t ) 1000 ) )
