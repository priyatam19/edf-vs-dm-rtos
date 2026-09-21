# Flashing this onto real hardware (Arduino IDE)

Target: Arduino Mega 2560 (ATmega2560). Library: **feilipu/Arduino_FreeRTOS_Library, pinned
at tag `10.4.3-8`** (matches this project's FreeRTOS Kernel V10.4.3) — install this exact
version, not whatever the Library Manager offers as latest; a newer kernel version may not
expose the same trace-macro hooks the same way (see below).

## One-time setup

1. Install the Arduino IDE and the `Arduino_FreeRTOS_Library` at tag `10.4.3-8`
   (Library Manager, or clone `https://github.com/feilipu/Arduino_FreeRTOS_Library` and
   `git checkout 10.4.3-8` into your `libraries/` folder).
2. Copy **`FreeRTOSConfig.h`, `schedPolicy.h`, `scheduler.h`, `scheduler-final.cpp`** from
   this `Code/` directory into that library's `src/` directory, overwriting the library's own
   default `FreeRTOSConfig.h`. This is required, not optional: the library's kernel source
   (`tasks.c`, `queue.c`, ...) includes `"FreeRTOSConfig.h"` with quotes, which resolves
   relative to the *library's own directory* before any Arduino IDE include path — so unless
   these files physically live next to the kernel source, the library silently falls back to
   its own bundled default config instead of this project's.
   - `schedPolicy.h` is a new file (not part of the original two-file copy this README used to
     describe): `FreeRTOSConfig.h` itself now includes it, for the same reason.
3. Open `Code/main/main.ino` as your sketch (the `main/main.ino` naming is required — the
   Arduino IDE only recognizes a sketch folder whose name matches its main `.ino` file).

## Choosing a configuration

Set `schedSCHEDULING_POLICY` in `schedPolicy.h` (not `scheduler.h` — this moved so the same
value can size `configMAX_PRIORITIES` in `FreeRTOSConfig.h` too):

```c
#define schedSCHEDULING_POLICY schedSCHEDULING_POLICY_EDF   // or _RMS, or _DMS
```

For EDF, also choose exactly one implementation:

```c
#define schedEDF_EFFICIENT 1   // or 0 for naive EDF
```

You can only run naive or efficient EDF at a time, never both.

To run the constrained-deadline task set instead of the default one (see the root README for
why you'd want to — it's the one that actually shows RM and DM disagree), set in the same file:

```c
#define SCHED_TASKSET 1
```

## Verifying it builds before flashing

This project also has a PlatformIO setup (`platformio.ini` at the repo root) that
cross-compiles all four configurations for this exact board without needing hardware —
useful for catching config mistakes before a flash-and-wait cycle:

```
pio run -e rms          # or dms, edf_naive, edf_efficient
```

It builds from this same `Code/` tree unmodified (see `tools/pio_overlay_freertos_config.py`
for the mechanics) — it does not replace the manual library-copy step above for actually
flashing hardware, only for compile verification and CI.

## Verifying the efficient-EDF hooks are wired up

Efficient EDF depends on FreeRTOS trace macros (defined in `FreeRTOSConfig.h`, only when
`schedEDF_EFFICIENT==1`) actually firing at the right kernel events. `pio run -e edf_efficient`
only proves this *compiles* — it does not prove the hooks fire correctly at runtime. To check
that on real hardware: build with `SCHED_VERBOSE_DEMO=1`, flash, and watch the serial monitor —
you should see tasks visibly preempting each other (a low-priority task's prints interrupted by
a higher-priority one) rather than running strictly in creation order. If every task simply
runs to completion before the next starts, the trace macros aren't actually reaching the
scheduler and something upstream of `FreeRTOSConfig.h` has changed.

See the root `README.md` for hardware/architecture/attribution and the measurement pipeline.
