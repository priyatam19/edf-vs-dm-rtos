#!/usr/bin/env python3
"""
Host-side fixed-priority scheduling simulator for edf-vs-dm-rtos.

This is the honest source of the portfolio's "one reproducible result": it
does not measure real hardware timing (no board is available -- see the
root README's verification-status section), but it reproduces RM's and DM's
priority *assignment* exactly the way Code/scheduler-final.cpp's
prvSetFixedPriorities() does (shortest period/deadline wins, ties share a
priority level), then runs a millisecond-resolution preemptive simulation
using each task's declared budget as its execution time, to produce response
times and deadline misses under each policy.

Usage:
    python3 host/sim/simulate.py tasksets/constrained_deadline.json rms > out_rms.csv
    python3 host/sim/simulate.py tasksets/constrained_deadline.json dms > out_dms.csv

Output is the same {tick,task,event,value} CSV schema
Code/scheduler-final.cpp's dumpTraceCSV() emits, so host/sim/plot_results.py
can plot either a simulated trace or one collected from real hardware.
"""
import json
import sys
from math import gcd


def lcm(a, b):
    return a * b // gcd(a, b)


def load_taskset(path):
    with open(path) as f:
        data = json.load(f)
    return data["tasks"]


def assign_priorities(tasks, policy):
    """Mirrors prvSetFixedPriorities(): sort by the policy's key ascending,
    highest priority (largest number) to the shortest; equal keys share a
    priority level (no gap left behind, matching the real algorithm's
    xPreviousShortest tie-break)."""
    key = (lambda t: t["period_ms"]) if policy == "rms" else (lambda t: t["deadline_ms"])
    ordered = sorted(tasks, key=key)

    priorities = {}
    level = len(tasks) - 1  # highest task priority is (task count - 1); scheduler task would sit at task count
    prev_key = None
    for t in ordered:
        k = key(t)
        if prev_key is not None and k != prev_key:
            level -= 1
        priorities[t["name"]] = level
        prev_key = k
    return priorities


def simulate(tasks, priorities, horizon_ms):
    """Millisecond-resolution preemptive fixed-priority simulation. Each
    task's budget_ms is treated as its actual execution time per job (a
    simplifying, documented assumption -- this simulates the worst case,
    not a sampled/randomized one)."""
    events = []  # (tick, task, event, value)
    jobs = []  # active jobs: dict(name, release, remaining, deadline)
    next_release = {t["name"]: t["phase_ms"] for t in tasks}
    deadline_misses = {t["name"]: 0 for t in tasks}
    response_times = {t["name"]: [] for t in tasks}

    for now in range(horizon_ms):
        for t in tasks:
            if next_release[t["name"]] == now:
                jobs.append({
                    "name": t["name"],
                    "release": now,
                    "remaining": t["budget_ms"],
                    "deadline": now + t["deadline_ms"],
                })
                events.append((now, t["name"], "RELEASE", 0))
                next_release[t["name"]] += t["period_ms"]

        # deadline misses: any job whose deadline has passed and isn't done
        for job in jobs:
            if job["deadline"] == now and job["remaining"] > 0:
                deadline_misses[job["name"]] += 1
                events.append((now, job["name"], "DEADLINE_MISS", 0))

        jobs = [j for j in jobs if not (j["deadline"] <= now and j["remaining"] > 0)]

        if jobs:
            running = min(jobs, key=lambda j: -priorities[j["name"]])
            running["remaining"] -= 1
            if running["remaining"] == 0:
                response_time = now + 1 - running["release"]
                response_times[running["name"]].append(response_time)
                events.append((now + 1, running["name"], "COMPLETE", response_time))
                jobs.remove(running)

    return events, deadline_misses, response_times


def main():
    if len(sys.argv) != 3 or sys.argv[2] not in ("rms", "dms"):
        print("usage: simulate.py <taskset.json> <rms|dms>", file=sys.stderr)
        sys.exit(1)

    tasks = load_taskset(sys.argv[1])
    policy = sys.argv[2]
    priorities = assign_priorities(tasks, policy)

    horizon = 1
    for t in tasks:
        horizon = lcm(horizon, t["period_ms"])
    horizon = min(horizon * 2, 60000)  # cap runaway horizons; two hyperperiods is enough to see steady state

    events, misses, response_times = simulate(tasks, priorities, horizon)

    print("tick,task,event,value")
    for tick, name, event, value in events:
        print(f"{tick},{name},{event},{value}")

    print(f"# policy,{policy}", file=sys.stderr)
    print(f"# priorities,{priorities}", file=sys.stderr)
    for t in tasks:
        rts = response_times[t["name"]]
        worst = max(rts) if rts else 0
        print(f"# {t['name']},deadline_misses={misses[t['name']]},worst_response_ms={worst},jobs={len(rts)}", file=sys.stderr)


if __name__ == "__main__":
    main()
