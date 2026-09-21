#ifndef SCHED_POLICY_H_
#define SCHED_POLICY_H_

/*
 * Scheduling policy selection, shared between FreeRTOSConfig.h (which sizes
 * configMAX_PRIORITIES and the efficient-EDF trace macros from it) and
 * scheduler.h / scheduler-final.cpp. Both files include this one first.
 *
 * Every value here can be overridden with a build flag
 * (e.g. -D schedSCHEDULING_POLICY=schedSCHEDULING_POLICY_RMS) so a single
 * source tree can be built as any of the four configurations without hand
 * editing this file -- see platformio.ini, envs rms/dms/edf_naive/edf_efficient.
 *
 * Naming and values match upstream ESFree (Robin Kase, github.com/RobinK2/ESFree)
 * so this stays a faithful port: MANUAL=0, RMS=1, DMS=2, EDF=3.
 */
#define schedSCHEDULING_POLICY_MANUAL 0 /* Priorities are set by the caller. */
#define schedSCHEDULING_POLICY_RMS    1 /* Rate-monotonic scheduling. */
#define schedSCHEDULING_POLICY_DMS    2 /* Deadline-monotonic scheduling. */
#define schedSCHEDULING_POLICY_EDF    3 /* Earliest deadline first. */

#ifndef schedSCHEDULING_POLICY
#define schedSCHEDULING_POLICY schedSCHEDULING_POLICY_EDF
#endif

#if( schedSCHEDULING_POLICY != schedSCHEDULING_POLICY_MANUAL && \
     schedSCHEDULING_POLICY != schedSCHEDULING_POLICY_RMS && \
     schedSCHEDULING_POLICY != schedSCHEDULING_POLICY_DMS && \
     schedSCHEDULING_POLICY != schedSCHEDULING_POLICY_EDF )
    #error "schedSCHEDULING_POLICY must be one of schedSCHEDULING_POLICY_{MANUAL,RMS,DMS,EDF} (see Code/schedPolicy.h)"
#endif

/*
 * If the scheduling policy is EDF, choose exactly one implementation.
 * schedEDF_EFFICIENT/schedEDF_NAIVE are only *defined* when EDF is the
 * active policy -- matching upstream ESFree. Code that tests either flag
 * without first checking schedSCHEDULING_POLICY==schedSCHEDULING_POLICY_EDF
 * will see an undefined macro (== 0 in a preprocessor #if), not silently
 * apply EDF-only behaviour to RMS/DMS builds. This is the fix for the bug
 * where schedEDF_EFFICIENT used to be unconditionally 1.
 */
#if( schedSCHEDULING_POLICY == schedSCHEDULING_POLICY_EDF )
    #ifndef schedEDF_EFFICIENT
    #define schedEDF_EFFICIENT 0 /* Default to naive EDF unless overridden. */
    #endif

    #if( schedEDF_EFFICIENT != 0 && schedEDF_EFFICIENT != 1 )
        #error "schedEDF_EFFICIENT must be 0 or 1 (see Code/schedPolicy.h)"
    #endif

    /* Exactly one of these is 1; derived so the two can never disagree. */
    #define schedEDF_NAIVE ( 1 - schedEDF_EFFICIENT )
#endif /* schedSCHEDULING_POLICY_EDF */

/*
 * Which task set Code/main/main.ino's setup() creates:
 *   0 = the original 4-task set, deadline == period for every task (can't
 *       show RM and DM choosing different priority orders, since fixed-
 *       priority order only depends on whichever of period/deadline you
 *       sort by when the two coincide).
 *   1 = a 3-task constrained-deadline set (deadline < period, and the
 *       deadline ordering deliberately differs from the period ordering),
 *       see tasksets/constrained_deadline.json for the numbers and why.
 * Both FreeRTOSConfig.h (via schedACTIVE_NUMBER_OF_PERIODIC_TASKS below) and
 * main.ino read this, so it must be decided here, before either is
 * processed -- override with e.g. -D SCHED_TASKSET=1.
 */
#ifndef SCHED_TASKSET
#define SCHED_TASKSET 0
#endif

/*
 * Number of periodic tasks actually created by Code/main/main.ino's setup()
 * for the selected SCHED_TASKSET. configMAX_PRIORITIES (FreeRTOSConfig.h) is
 * sized from this number per upstream ESFree's own sizing rule (thesis sec.
 * 4.2.2): periodic tasks + 1 for RMS/DMS/naive EDF, or a fixed 3 for
 * efficient EDF. vSchedulerStart() asserts the task count created actually
 * matches this at runtime, so if you add/remove a task in main.ino without
 * updating this, you get a clear assertion instead of a silent
 * priority-range underflow.
 */
#ifndef schedACTIVE_NUMBER_OF_PERIODIC_TASKS
    #if( SCHED_TASKSET == 1 )
        #define schedACTIVE_NUMBER_OF_PERIODIC_TASKS 3
    #else
        #define schedACTIVE_NUMBER_OF_PERIODIC_TASKS 4
    #endif
#endif

#endif /* SCHED_POLICY_H_ */
