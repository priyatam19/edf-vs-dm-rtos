/* C++-only fake globals (Arduino's Serial object). Kept separate from
 * fakes.c so that file can stay plain C, matching the real FreeRTOS kernel
 * source style. */
#include "Arduino.h"

FakeSerial Serial;
