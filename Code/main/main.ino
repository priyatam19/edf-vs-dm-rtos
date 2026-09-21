#include <Arduino_FreeRTOS.h>

#include "scheduler.h"
#include "task.h"

//#include "PerformanceMetrics.h"

unsigned long startTime;
const unsigned long timeoutPeriod = 60000; // 1 minute in milliseconds


TaskHandle_t xHandle1 = NULL;
TaskHandle_t xHandle2 = NULL;
TaskHandle_t xHandle3 = NULL;
TaskHandle_t xHandle4 = NULL;


// Function prototypes
void printSchedulerOverhead(void);

// ---- Calibrated CPU-bound workload -----------------------------------
// Replaces the old Serial.println-loop "work": printing at 9600 baud made
// UART buffering/transmission part of the measured job, and looping a fixed
// number of times (5 or 11) doesn't correspond to any particular amount of
// CPU time. This does a fixed amount of integer computation and retains the
// result in a volatile global so the compiler can't optimize the loop away,
// then reports how much CPU time each task actually consumed relative to
// its budget (schedACTIVE_NUMBER_OF_PERIODIC_TASKS's xMaxExecTime).
static volatile uint32_t gWorkloadSink = 0;

static void schedBusyWork( uint32_t iterations )
{
  uint32_t acc = gWorkloadSink;
  for( uint32_t i = 0; i < iterations; i++ )
  {
    acc = acc * 2654435761UL + i; // cheap, non-optimizable mixing step
  }
  gWorkloadSink = acc;
}

// Iterations-per-millisecond, measured once at boot via micros() so a
// requested execution budget (ms, as passed to vSchedulerPeriodicTaskCreate)
// and the iteration count that actually consumes that much CPU time are in
// the same units on this specific board/clock speed -- passing "100 ms" to
// task creation only sets the budget the scheduler enforces, it does not by
// itself make a task consume 100 ms of CPU time.
static uint32_t gIterationsPerMs = 1;

static void calibrateWorkload( void )
{
  const uint32_t calibrationIterations = 20000UL;
  unsigned long start = micros();
  schedBusyWork( calibrationIterations );
  unsigned long elapsedUs = micros() - start;
  if( elapsedUs == 0 )
  {
    elapsedUs = 1;
  }
  gIterationsPerMs = ( uint32_t ) ( ( calibrationIterations * 1000ULL ) / elapsedUs );
  if( gIterationsPerMs == 0 )
  {
    gIterationsPerMs = 1;
  }
  #if( SCHED_VERBOSE_DEMO == 1 )
    Serial.print("Workload calibration: "); Serial.print(gIterationsPerMs); Serial.println(" iterations/ms");
  #endif
}

static uint32_t schedMsToIterations( uint32_t ms )
{
  return ms * gIterationsPerMs;
}

// the loop function runs over and over again forever
void loop() {
      // Check if the current time minus the start time is greater than the timeout period
    if (millis() - startTime > timeoutPeriod) {
        // Print the performance metrics just before ending

        // End the scheduler
        vTaskEndScheduler();

        printMetrics();
        // Halt the system
        while (true) {
            // Optional: Add a small delay to ensure the output buffer is emptied
            delay(1000);
        }

    }

    //Optional: Include a small delay here if the loop runs too fast
    delay(10);
}

#if( SCHED_TASKSET == 1 )
  // ---- Constrained-deadline task set (SCHED_TASKSET=1) -----------------
  // deadline < period, chosen so RM's period-order and DM's deadline-order
  // disagree -- see tasksets/constrained_deadline.json for the numbers and
  // why. The original task set below (SCHED_TASKSET=0, the default) can't
  // demonstrate this: every task there has deadline == period.
  static void taskConstrainedA( void *pvParameters )
  {
    (void) pvParameters;
    schedBusyWork( schedMsToIterations( 24 ) ); // ~60% of its 40 ms budget
  }

  static void taskConstrainedB( void *pvParameters )
  {
    (void) pvParameters;
    schedBusyWork( schedMsToIterations( 24 ) ); // ~60% of its 40 ms budget
  }

  static void taskConstrainedC( void *pvParameters )
  {
    (void) pvParameters;
    schedBusyWork( schedMsToIterations( 30 ) ); // ~60% of its 50 ms budget
  }
#else
  // ---- Implicit-deadline task set (SCHED_TASKSET=0, default) -----------
  // deadline == period for every task -- see tasksets/implicit_deadline.json.
  static void testFunc1( void *pvParameters )
  {
    (void) pvParameters;
    #if( SCHED_VERBOSE_DEMO == 1 )
      Serial.println("task 1 running");
    #endif
    schedBusyWork( schedMsToIterations( 60 ) ); // ~60% of its 100 ms budget
  }

  static void testFunc2( void *pvParameters )
  {
    (void) pvParameters;
    #if( SCHED_VERBOSE_DEMO == 1 )
      Serial.println("task 2 running");
    #endif
    schedBusyWork( schedMsToIterations( 120 ) ); // ~60% of its 200 ms budget
  }

  static void testFunc3( void *pvParameters )
  {
    (void) pvParameters;
    #if( SCHED_VERBOSE_DEMO == 1 )
      Serial.println("task 3 running");
    #endif
    schedBusyWork( schedMsToIterations( 120 ) ); // ~60% of its 200 ms budget
  }

  static void testFunc4( void *pvParameters )
  {
    (void) pvParameters;
    #if( SCHED_VERBOSE_DEMO == 1 )
      Serial.println("task 4 running");
    #endif
    schedBusyWork( schedMsToIterations( 60 ) ); // ~60% of its 100 ms budget
  }
#endif /* SCHED_TASKSET */


void setup()
{
  Serial.begin(9600);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB, on LEONARDO, MICRO, YUN, and other 32u4 based boards.
  }
	char c1 = 'a';
	char c2 = 'b';
  char c3 = 'c';
  char c4 = 'd';
  startTime = millis();
  vSchedulerInit();
  calibrateWorkload();

  // vSchedulerSetPolicy(schedSCHEDULING_POLICY_EDF); // Set the scheduling policy to EDF explicitly

  initializePerformanceMetrics();
  Serial.println("System Initialized.");

  #if( SCHED_TASKSET == 1 )
    // Priority arguments below are only used as a starting value; RMS/DMS/
    // naive-EDF all recompute and overwrite it in vSchedulerStart(). Kept
    // in-range ([0, schedACTIVE_NUMBER_OF_PERIODIC_TASKS-1]) regardless.
    vSchedulerPeriodicTaskCreate(taskConstrainedA, "cA", configMINIMAL_STACK_SIZE, &c1, 0, &xHandle1, pdMS_TO_TICKS(0), pdMS_TO_TICKS(300), pdMS_TO_TICKS(40), pdMS_TO_TICKS(100));
    vSchedulerPeriodicTaskCreate(taskConstrainedB, "cB", configMINIMAL_STACK_SIZE, &c2, 1, &xHandle2, pdMS_TO_TICKS(0), pdMS_TO_TICKS(150), pdMS_TO_TICKS(40), pdMS_TO_TICKS(140));
    vSchedulerPeriodicTaskCreate(taskConstrainedC, "cC", configMINIMAL_STACK_SIZE, &c3, 2, &xHandle3, pdMS_TO_TICKS(0), pdMS_TO_TICKS(250), pdMS_TO_TICKS(50), pdMS_TO_TICKS(200));
  #else
    vSchedulerPeriodicTaskCreate(testFunc1, "t1", configMINIMAL_STACK_SIZE, &c1, 3, &xHandle1, pdMS_TO_TICKS(0), pdMS_TO_TICKS(800), pdMS_TO_TICKS(100), pdMS_TO_TICKS(800));
    vSchedulerPeriodicTaskCreate(testFunc2, "t2", configMINIMAL_STACK_SIZE, &c2, 3, &xHandle2, pdMS_TO_TICKS(0), pdMS_TO_TICKS(1000), pdMS_TO_TICKS(200), pdMS_TO_TICKS(1000));
    vSchedulerPeriodicTaskCreate(testFunc3, "t3", configMINIMAL_STACK_SIZE, &c3, 1, &xHandle3, pdMS_TO_TICKS(0), pdMS_TO_TICKS(1500), pdMS_TO_TICKS(200), pdMS_TO_TICKS(1500));
    vSchedulerPeriodicTaskCreate(testFunc4, "t4", configMINIMAL_STACK_SIZE, &c4, 2, &xHandle4, pdMS_TO_TICKS(0), pdMS_TO_TICKS(2000), pdMS_TO_TICKS(100), pdMS_TO_TICKS(2000));
  #endif /* SCHED_TASKSET */

  vSchedulerStart();

}
