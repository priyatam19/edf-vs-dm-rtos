# host/fakes

Minimal, host-native stand-ins for the FreeRTOS headers `Code/scheduler.h`
includes, so `Code/scheduler-final.cpp` can be compiled and unit tested on
the machine running the tests -- no ATmega2560 board required. See
`host/test/test_scheduler_native/test_main.cpp` for how it's used.

## Why this works without a real scheduler

`xTaskCreate`, `vTaskPrioritySet`, `xTaskDelayUntil`, etc. here are simple
state-recording fakes (`fakes.c`), not a working FreeRTOS port -- there is no
real preemption or context switching. That's enough because every test
targets the *synchronous* bookkeeping functions in `scheduler-final.cpp`
(priority assignment, deadline/overrun checking, TCB array management), not
`prvPeriodicTaskCode`'s task loop itself, which would need a real scheduler
to drive.

`list.h`/`list.c` are the exception: they're the **real, unmodified** kernel
source, vendored at the same tag this project's firmware builds against
(`feilipu/Arduino_FreeRTOS_Library@10.4.3-8`), because the EDF paths' sorted-
list bookkeeping is worth testing with full fidelity, not a reimplementation.

`TickType_t` is hardcoded to 16-bit here, matching `configUSE_16_BIT_TICKS==1`
in the real `Code/FreeRTOSConfig.h`, so a tick-wraparound test on the host
means the same thing it would on real hardware.

## Scope

Covered on the host (see `host/test/`): fixed-priority (RMS/DMS) priority
assignment, deadline-miss detection (including 16-bit tick wraparound),
execution-budget overrun and recovery, TCB array bookkeeping (creation,
deletion, capacity, slot reuse).

Not covered on the host, and left as an explicit checklist item for the next
board session: the EDF sorted-list paths (naive and efficient), the
`prvPeriodicTaskCode` task loop itself (including the response-time
measurement added to it), the efficient-EDF trace-macro wiring actually
firing, and anything about real timing. All of those need a real running
kernel; the compile-time verification in `platformio.ini`'s `edf_naive`/
`edf_efficient` envs confirms they build, not that they behave correctly at
runtime.
