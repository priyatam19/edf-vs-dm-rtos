/*
 * Native host tests for Code/scheduler-final.cpp, run without any board
 * (PlatformIO `native` platform: `pio test -e native`). This file #includes
 * scheduler-final.cpp directly so tests reach its static (file-local)
 * functions and drive the *actual* compiled logic -- not a reimplementation
 * of it -- see host/fakes/README.md for the fakes this links against and
 * what's out of scope on the host (the EDF sorted-list paths, and anything
 * needing a real running scheduler).
 *
 * Built with schedSCHEDULING_POLICY=schedSCHEDULING_POLICY_DMS (see
 * platformio.ini's [env:native]), so this exercises the fixed-priority
 * TCB_ARRAY path -- the same struct/array bookkeeping RMS uses.
 *
 * Note: vSchedulerPeriodicTaskCreate() only populates the TCB array; the
 * real TaskHandle_t values (and each TCB's thread-local-storage pointer
 * back to itself) aren't set until prvCreateAllTasks() actually calls
 * xTaskCreate(), same as in the real vSchedulerStart(). Tests that need a
 * populated handle call prvCreateAllTasks() explicitly after creating their
 * tasks (and after prvSetFixedPriorities(), when priority order matters --
 * that's the real call order in vSchedulerStart() too).
 */
#include <unity.h>
#include "fakes.h"

#include "../../../Code/scheduler-final.cpp"

static void dummyTask( void * pvParameters ) { (void) pvParameters; }

void setUp( void )
{
    schedtest_Reset();
    xTaskCounter = 0;
    prvInitTCBArray();
}

void tearDown( void )
{
}

// ---- 1. Single task, then multiple tasks with known execution order ----

void test_SingleTask_GetsHighestAvailablePriority( void )
{
    TaskHandle_t h;
    vSchedulerPeriodicTaskCreate( dummyTask, "A", 100, NULL, 0, &h,
                                   0, pdMS_TO_TICKS( 500 ), pdMS_TO_TICKS( 50 ), pdMS_TO_TICKS( 500 ) );
    prvSetFixedPriorities();
    prvCreateAllTasks();

    // configMAX_PRIORITIES = schedACTIVE_NUMBER_OF_PERIODIC_TASKS + 1 (see
    // FreeRTOSConfig.h); scheduler task takes the top slot, so a single
    // periodic task gets the next one down.
    TEST_ASSERT_EQUAL_UINT32( configMAX_PRIORITIES - 2, schedtest_GetPriority( h ) );
    TEST_ASSERT_FALSE( schedtest_AssertFired() );
}

void test_MultipleTasks_KnownDeadlineOrder( void )
{
    // DMS: shortest relative deadline gets the highest priority. Deadlines
    // deliberately out of creation order to prove it's sorting, not just
    // echoing creation order.
    TaskHandle_t hLong, hShort, hMid;
    vSchedulerPeriodicTaskCreate( dummyTask, "long",  100, NULL, 0, &hLong,
                                   0, pdMS_TO_TICKS( 1000 ), pdMS_TO_TICKS( 50 ), pdMS_TO_TICKS( 1000 ) );
    vSchedulerPeriodicTaskCreate( dummyTask, "short", 100, NULL, 0, &hShort,
                                   0, pdMS_TO_TICKS( 200 ),  pdMS_TO_TICKS( 50 ), pdMS_TO_TICKS( 200 ) );
    vSchedulerPeriodicTaskCreate( dummyTask, "mid",   100, NULL, 0, &hMid,
                                   0, pdMS_TO_TICKS( 500 ),  pdMS_TO_TICKS( 50 ), pdMS_TO_TICKS( 500 ) );
    prvSetFixedPriorities();
    prvCreateAllTasks();

    UBaseType_t pShort = schedtest_GetPriority( hShort );
    UBaseType_t pMid   = schedtest_GetPriority( hMid );
    UBaseType_t pLong  = schedtest_GetPriority( hLong );

    TEST_ASSERT_TRUE_MESSAGE( pShort > pMid, "shortest deadline must outrank mid deadline" );
    TEST_ASSERT_TRUE_MESSAGE( pMid > pLong, "mid deadline must outrank longest deadline" );
    TEST_ASSERT_TRUE_MESSAGE( pLong < ( UBaseType_t ) configMAX_PRIORITIES, "no priority reaches the scheduler's own level" );
}

void test_EqualDeadlines_DifferentPhases_GetSamePriority( void )
{
    // Two tasks with the same relative deadline but different phases: DMS
    // sorts by deadline only, so they must land on the same priority level.
    TaskHandle_t hA, hB;
    vSchedulerPeriodicTaskCreate( dummyTask, "A", 100, NULL, 0, &hA,
                                   pdMS_TO_TICKS( 0 ),   pdMS_TO_TICKS( 500 ), pdMS_TO_TICKS( 50 ), pdMS_TO_TICKS( 500 ) );
    vSchedulerPeriodicTaskCreate( dummyTask, "B", 100, NULL, 0, &hB,
                                   pdMS_TO_TICKS( 100 ), pdMS_TO_TICKS( 500 ), pdMS_TO_TICKS( 50 ), pdMS_TO_TICKS( 500 ) );
    prvSetFixedPriorities();
    prvCreateAllTasks();

    TEST_ASSERT_EQUAL_UINT32( schedtest_GetPriority( hA ), schedtest_GetPriority( hB ) );
}

// ---- 2. A job that misses its deadline before receiving CPU time -------

void test_JobMissesDeadlineBeforeCpuTime( void )
{
    TaskHandle_t h;
    vSchedulerPeriodicTaskCreate( dummyTask, "A", 100, NULL, 0, &h,
                                   0, pdMS_TO_TICKS( 100 ), pdMS_TO_TICKS( 20 ), pdMS_TO_TICKS( 100 ) );
    prvCreateAllTasks();
    SchedTCB_t *pxTCB = ( SchedTCB_t * ) pvTaskGetThreadLocalStoragePointer( h, schedTHREAD_LOCAL_STORAGE_POINTER_INDEX );
    TEST_ASSERT_NOT_NULL( pxTCB );

    // Simulate: task has run once before, this period's job never got the
    // CPU (xWorkIsDone stays pdFALSE), and we're now well past its deadline.
    pxTCB->xExecutedOnce = pdTRUE;
    pxTCB->xWorkIsDone = pdFALSE;
    pxTCB->xLastWakeTime = 1000;

    TEST_ASSERT_EQUAL_UINT32( 0, pxTCB->xDeadlineMisses );
    prvCheckDeadline( pxTCB, ( TickType_t ) ( 1000 + 100 + 1 ) ); // 1 tick past the 100-tick deadline
    TEST_ASSERT_EQUAL_UINT32( 1, pxTCB->xDeadlineMisses );
}

void test_DeadlineMiss_CountedExactlyOnce( void )
{
    // Regression for: deadline misses used to be countable from two places
    // (the real timing-error handler, and dead switch-out instrumentation
    // that duplicated the check). The one live path must add exactly one
    // miss per call.
    TaskHandle_t h;
    vSchedulerPeriodicTaskCreate( dummyTask, "A", 100, NULL, 0, &h,
                                   0, pdMS_TO_TICKS( 100 ), pdMS_TO_TICKS( 20 ), pdMS_TO_TICKS( 100 ) );
    prvCreateAllTasks();
    SchedTCB_t *pxTCB = ( SchedTCB_t * ) pvTaskGetThreadLocalStoragePointer( h, schedTHREAD_LOCAL_STORAGE_POINTER_INDEX );
    pxTCB->xLastWakeTime = 0;

    prvDeadlineMissedHook( pxTCB, 150 );
    TEST_ASSERT_EQUAL_UINT32( 1, pxTCB->xDeadlineMisses );
}

// ---- 3. Execution-budget overrun and recovery ---------------------------

void test_ExecutionBudgetOverrun_AndRecovery( void )
{
    TaskHandle_t h;
    vSchedulerPeriodicTaskCreate( dummyTask, "A", 100, NULL, 0, &h,
                                   0, pdMS_TO_TICKS( 100 ), pdMS_TO_TICKS( 20 ), pdMS_TO_TICKS( 100 ) );
    prvCreateAllTasks();
    SchedTCB_t *pxTCB = ( SchedTCB_t * ) pvTaskGetThreadLocalStoragePointer( h, schedTHREAD_LOCAL_STORAGE_POINTER_INDEX );
    pxTCB->xLastWakeTime = 500;
    pxTCB->xPeriod = pdMS_TO_TICKS( 100 );

    TEST_ASSERT_EQUAL_UINT32( 0, pxTCB->xExecBudgetOverruns );
    prvExecTimeExceedHook( 520, pxTCB );

    TEST_ASSERT_EQUAL_UINT32( 1, pxTCB->xExecBudgetOverruns );
    TEST_ASSERT_TRUE( pxTCB->xSuspended );
    TEST_ASSERT_EQUAL_UINT32( 500 + pdMS_TO_TICKS( 100 ), pxTCB->xAbsoluteUnblockTime );

    // Recovery: prvSchedulerCheckTimingError resumes it once the unblock
    // tick is reached, and clears xSuspended.
    prvSchedulerCheckTimingError( pxTCB->xAbsoluteUnblockTime, pxTCB );
    TEST_ASSERT_FALSE( pxTCB->xSuspended );
}

// ---- 4. Deadline/release times crossing tick wraparound -----------------

// prvCheckDeadline's wraparound-safety comes from `(signed)(a - b) < 0`
// where a, b are TickType_t (uint16_t, configUSE_16_BIT_TICKS==1). This is
// only actually wraparound-*safe* when the subtraction itself happens at
// 16-bit width: on the real target (avr-gcc, confirmed via
// `avr-gcc -dM -E`: __INT_WIDTH__ == 16), `int` IS 16 bits, so a uint16_t
// subtraction stays 16-bit per C's integer promotion rules and wraps
// correctly. On this host (`int` is 32 bits), the SAME uint16_t operands
// promote to 32-bit int *before* subtracting, so the unmodified expression
// in prvCheckDeadline computes a true (non-wrapping) 32-bit difference
// instead -- for genuinely-wrapped inputs that gives the opposite sign from
// what the 16-bit target produces. This is a real, non-obvious platform
// dependency in the existing ICTOH technique, not a bug introduced here;
// asserting prvCheckDeadline's literal output against wrapped inputs on
// this host would silently check 32-bit-int behavior while claiming to
// verify the 16-bit target's, so instead this test isolates the exact
// comparison technique with explicit 16-bit truncation (mirroring what
// avr-gcc does for free) to verify the *algorithm* portably. See
// host/fakes/README.md.
static bool prvIsTickBeforeAtWidth16( TickType_t xDeadline, TickType_t xNow )
{
    return ( int16_t ) ( uint16_t ) ( xDeadline - xNow ) < 0;
}

void test_TickWraparound_ComparisonIsCorrectAcrossRollover( void )
{
    // Deadline computed from a release just before the 16-bit rollover;
    // "now" lands shortly after it -- exactly the case a naive (non-ICTOH)
    // `deadline < now` comparison gets wrong.
    TickType_t xLastWakeTime = ( TickType_t ) 0xFFC0; // 64 ticks before rollover
    TickType_t xRelativeDeadline = 100;
    TickType_t xAbsoluteDeadline = ( TickType_t ) ( xLastWakeTime + xRelativeDeadline ); // wraps to 36

    // 40 ticks after rollover: 24 ticks since release, deadline (100 ticks)
    // not yet reached -- must NOT be "before now" (not missed yet).
    TickType_t xNow = ( TickType_t ) ( 0xFFC0 + 40 );
    TEST_ASSERT_FALSE( prvIsTickBeforeAtWidth16( xAbsoluteDeadline, xNow ) );

    // 150 ticks since release, past the 100-tick deadline -- must now be
    // "before now" (missed).
    xNow = ( TickType_t ) ( 0xFFC0 + 150 );
    TEST_ASSERT_TRUE( prvIsTickBeforeAtWidth16( xAbsoluteDeadline, xNow ) );
}

void test_JobMissesDeadline_NoWraparound_StillWorksOnHost( void )
{
    // Sanity check that prvCheckDeadline itself (not the isolated helper
    // above) still behaves correctly on this host for the ordinary,
    // non-wrapped case -- i.e. this host genuinely does exercise
    // prvCheckDeadline correctly whenever the 32-bit-vs-16-bit promotion
    // difference doesn't come into play.
    TaskHandle_t h;
    vSchedulerPeriodicTaskCreate( dummyTask, "A", 100, NULL, 0, &h,
                                   0, pdMS_TO_TICKS( 100 ), pdMS_TO_TICKS( 20 ), pdMS_TO_TICKS( 100 ) );
    prvCreateAllTasks();
    SchedTCB_t *pxTCB = ( SchedTCB_t * ) pvTaskGetThreadLocalStoragePointer( h, schedTHREAD_LOCAL_STORAGE_POINTER_INDEX );
    pxTCB->xExecutedOnce = pdTRUE;
    pxTCB->xWorkIsDone = pdFALSE;
    pxTCB->xRelativeDeadline = 100;
    pxTCB->xLastWakeTime = 1000;

    prvCheckDeadline( pxTCB, 1050 ); // before the deadline (1100)
    TEST_ASSERT_EQUAL_UINT32( 0, pxTCB->xDeadlineMisses );

    prvCheckDeadline( pxTCB, 1150 ); // past the deadline
    TEST_ASSERT_EQUAL_UINT32( 1, pxTCB->xDeadlineMisses );
}

// ---- 5. Task deletion/recreation and maximum task capacity -------------

void test_TaskDeletion_RecycledSlot_GetsFreshPriorityIsSet( void )
{
    // Regression for the xPriorityIsSet staleness bug: a slot recycled from
    // a deleted task must not be skipped by a later prvSetFixedPriorities()
    // just because the *previous* occupant already had a priority assigned.
    TaskHandle_t hA, hB;
    vSchedulerPeriodicTaskCreate( dummyTask, "A", 100, NULL, 0, &hA,
                                   0, pdMS_TO_TICKS( 500 ), pdMS_TO_TICKS( 50 ), pdMS_TO_TICKS( 500 ) );
    prvSetFixedPriorities(); // marks A's slot xPriorityIsSet = pdTRUE
    prvCreateAllTasks();

    vSchedulerPeriodicTaskDelete( hA );
    TEST_ASSERT_EQUAL_INT( 0, xTaskCounter );

    vSchedulerPeriodicTaskCreate( dummyTask, "B", 100, NULL, 0, &hB,
                                   0, pdMS_TO_TICKS( 300 ), pdMS_TO_TICKS( 50 ), pdMS_TO_TICKS( 300 ) );
    prvSetFixedPriorities();
    prvCreateAllTasks();

    TEST_ASSERT_EQUAL_UINT32( configMAX_PRIORITIES - 2, schedtest_GetPriority( hB ) );
}

void test_DeleteMiddleTask_DoesNotCorruptOtherSlots( void )
{
    // Regression for xTCBArray[pdTRUE == xIndex] (always indexed 0 or 1,
    // never the real index): deleting slot 0 then slot 2 used to check
    // slot 0's (now-stale) xInUse for slot 2's deletion, silently no-op'ing
    // it whenever slot 0 happened to already be free.
    TaskHandle_t h0, h1, h2;
    vSchedulerPeriodicTaskCreate( dummyTask, "0", 100, NULL, 0, &h0, 0, pdMS_TO_TICKS( 500 ), pdMS_TO_TICKS( 50 ), pdMS_TO_TICKS( 500 ) );
    vSchedulerPeriodicTaskCreate( dummyTask, "1", 100, NULL, 0, &h1, 0, pdMS_TO_TICKS( 500 ), pdMS_TO_TICKS( 50 ), pdMS_TO_TICKS( 500 ) );
    vSchedulerPeriodicTaskCreate( dummyTask, "2", 100, NULL, 0, &h2, 0, pdMS_TO_TICKS( 500 ), pdMS_TO_TICKS( 50 ), pdMS_TO_TICKS( 500 ) );
    prvCreateAllTasks();
    TEST_ASSERT_EQUAL_INT( 3, xTaskCounter );

    vSchedulerPeriodicTaskDelete( h0 ); // frees slot 0 first
    vSchedulerPeriodicTaskDelete( h2 ); // then slot 2 -- must still actually free it

    TEST_ASSERT_EQUAL_INT_MESSAGE( 1, xTaskCounter, "deleting slot 2 must decrement the live task count" );
    TEST_ASSERT_TRUE_MESSAGE( schedtest_IsDeleted( h2 ), "deleting slot 2 must delete the real task handle" );
}

void test_MaximumTaskCapacity( void )
{
    TaskHandle_t handles[ schedMAX_NUMBER_OF_PERIODIC_TASKS ];
    for( int i = 0; i < schedMAX_NUMBER_OF_PERIODIC_TASKS; i++ )
    {
        vSchedulerPeriodicTaskCreate( dummyTask, "x", 100, NULL, 0, &handles[ i ],
                                       0, pdMS_TO_TICKS( 500 ), pdMS_TO_TICKS( 50 ), pdMS_TO_TICKS( 500 ) );
    }
    TEST_ASSERT_EQUAL_INT( schedMAX_NUMBER_OF_PERIODIC_TASKS, xTaskCounter );
    TEST_ASSERT_EQUAL_INT( -1, prvFindEmptyElementIndexTCB() );
}

int main( int argc, char **argv )
{
    (void) argc;
    (void) argv;
    UNITY_BEGIN();

    RUN_TEST( test_SingleTask_GetsHighestAvailablePriority );
    RUN_TEST( test_MultipleTasks_KnownDeadlineOrder );
    RUN_TEST( test_EqualDeadlines_DifferentPhases_GetSamePriority );
    RUN_TEST( test_JobMissesDeadlineBeforeCpuTime );
    RUN_TEST( test_DeadlineMiss_CountedExactlyOnce );
    RUN_TEST( test_ExecutionBudgetOverrun_AndRecovery );
    RUN_TEST( test_TickWraparound_ComparisonIsCorrectAcrossRollover );
    RUN_TEST( test_JobMissesDeadline_NoWraparound_StillWorksOnHost );
    RUN_TEST( test_TaskDeletion_RecycledSlot_GetsFreshPriorityIsSet );
    RUN_TEST( test_DeleteMiddleTask_DoesNotCorruptOtherSlots );
    RUN_TEST( test_MaximumTaskCapacity );

    return UNITY_END();
}
