/* Minimal host-side stand-in for Arduino.h: just enough for
 * Code/scheduler-final.cpp / main.ino to compile natively. Serial is a
 * no-op sink (tests care about scheduler state, not console output). */
#pragma once

#include <stdint.h>
#include <stdio.h>
#include <string.h> /* real Arduino.h pulls this in transitively too; scheduler-final.cpp uses memset() */

#ifdef __cplusplus
class FakeSerial
{
public:
    operator bool() const { return true; }
    void begin( unsigned long ) {}
    template <typename T> void print( T v ) { }
    template <typename T> void println( T v ) { }
    void println() {}
};

extern FakeSerial Serial;
#endif

static inline unsigned long millis( void ) { return 0; }
static inline unsigned long micros( void ) { return 0; }
static inline void delay( unsigned long ) {}
