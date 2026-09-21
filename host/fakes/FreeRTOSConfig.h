/*
 * Host-side FreeRTOSConfig.h for the native unit tests (host/test/). Mirrors
 * Code/FreeRTOSConfig.h's policy-driven sizing (via schedPolicy.h) so the
 * same configMAX_PRIORITIES logic is exercised, but doesn't need the
 * efficient-EDF trace-macro wiring: the native suite targets the
 * fixed-priority (RMS/DMS) TCB_ARRAY path -- see host/fakes/README.md for
 * what is and isn't covered on the host.
 */
#ifndef FREERTOS_CONFIG_H
#define FREERTOS_CONFIG_H

#include "schedPolicy.h"

#define configTICK_RATE_HZ 1000 /* 1 tick == 1 ms, for readable test values */
#define configUSE_16_BIT_TICKS 1

#if( schedSCHEDULING_POLICY == schedSCHEDULING_POLICY_EDF && schedEDF_EFFICIENT == 1 )
    #define configMAX_PRIORITIES ( 3 )
#else
    #define configMAX_PRIORITIES ( schedACTIVE_NUMBER_OF_PERIODIC_TASKS + 1 )
#endif

#endif /* FREERTOS_CONFIG_H */
