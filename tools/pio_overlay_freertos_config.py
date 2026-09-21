"""
PlatformIO pre-build hook: overlays Code/FreeRTOSConfig.h and
Code/schedPolicy.h onto the installed Arduino_FreeRTOS_Library's own src/.

Why this is needed: Arduino_FreeRTOS.h includes "FreeRTOSConfig.h" with
quotes, and a quote-include always resolves relative to the including file's
own directory before any -I search path is consulted -- that's true
regardless of include_dir/-I ordering, and PlatformIO's per-library dependency
scoping means the project's own include_dir is not added to a *library's*
compile flags (only to the project's own sources). The library ships its own
default FreeRTOSConfig.h in its src/, so without this overlay that bundled
default silently wins over Code/FreeRTOSConfig.h; and since our
FreeRTOSConfig.h itself quote-includes "schedPolicy.h", that needs to sit
alongside it too once overlaid. This is exactly what Code/README.md's
Arduino IDE instructions mean by "replace the files in src of FreeRTOS
library" -- this script does the same thing, automatically, per PlatformIO
environment, so CI builds the configuration this project actually ships.

This script also stages a build-time-only copy of Code/scheduler-final.cpp
into Code/main/ (git-ignored). That's needed because PlatformIO's .ino
discovery (platformio.builder.tools.pioino.FindInoNodes) globs *.ino
non-recursively in src_dir's own root only -- it will never see
Code/main/main.ino if src_dir=Code, so src_dir is set to Code/main instead
(see platformio.ini). scheduler-final.cpp lives one directory up from there,
so it's staged alongside main.ino at build time rather than moved/duplicated
in the tracked tree, keeping the Arduino IDE layout (Code/main/ contains only
main.ino) exactly as documented in Code/README.md.
"""
Import("env")

import shutil
from pathlib import Path

project_dir = Path(env.subst("$PROJECT_DIR"))
libdeps_dir = Path(env.subst("$PROJECT_LIBDEPS_DIR")) / env.subst("$PIOENV") / "FreeRTOS" / "src"

overlay_files = ["FreeRTOSConfig.h", "schedPolicy.h"]

if libdeps_dir.is_dir():
    for name in overlay_files:
        shutil.copyfile(project_dir / "Code" / name, libdeps_dir / name)
    print("[pio_overlay_freertos_config] overlaid {} into {}".format(overlay_files, libdeps_dir))
else:
    print("[pio_overlay_freertos_config] skipped: {} does not exist yet".format(libdeps_dir))

staged_scheduler = project_dir / "Code" / "main" / "scheduler-final.cpp"
shutil.copyfile(project_dir / "Code" / "scheduler-final.cpp", staged_scheduler)
print("[pio_overlay_freertos_config] staged {} for build".format(staged_scheduler))
