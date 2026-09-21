#!/usr/bin/env python3
"""
Regenerates the portfolio's RM-vs-DM comparison plot from trace CSVs (the
{tick,task,event,value} schema both host/sim/simulate.py and
Code/scheduler-final.cpp's dumpTraceCSV() emit).

Usage:
    python3 host/sim/simulate.py tasksets/constrained_deadline.json rms > /tmp/rms.csv
    python3 host/sim/simulate.py tasksets/constrained_deadline.json dms > /tmp/dms.csv
    python3 host/sim/plot_results.py /tmp/rms.csv rms /tmp/dms.csv dms \
        --deadlines tasksets/constrained_deadline.json \
        --out results/rm_vs_dm_constrained_deadline.png
"""
import argparse
import csv
import json
from collections import defaultdict

import matplotlib

matplotlib.use("Agg")
import matplotlib.pyplot as plt

# Palette: colorblind-safe, consistent across light/dark (see repo's chart
# conventions -- kept deliberately small and high-contrast for a 2-series,
# grouped-bar comparison).
COLOR_MISS = "#D64550"
COLOR_OK = "#2E86AB"


def read_trace(path):
    misses = defaultdict(int)
    response_times = defaultdict(list)
    with open(path) as f:
        reader = csv.DictReader(f)
        for row in reader:
            if row["event"] == "DEADLINE_MISS":
                misses[row["task"]] += 1
            elif row["event"] == "COMPLETE":
                response_times[row["task"]].append(int(row["value"]))
    return misses, response_times


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("trace_a")
    ap.add_argument("label_a")
    ap.add_argument("trace_b")
    ap.add_argument("label_b")
    ap.add_argument("--deadlines", required=True, help="task-set JSON, for task order and deadline_ms")
    ap.add_argument("--out", required=True)
    args = ap.parse_args()

    with open(args.deadlines) as f:
        tasks = json.load(f)["tasks"]
    names = [t["name"] for t in tasks]
    deadlines = {t["name"]: t["deadline_ms"] for t in tasks}

    misses_a, rt_a = read_trace(args.trace_a)
    misses_b, rt_b = read_trace(args.trace_b)

    fig, (ax_rt, ax_miss) = plt.subplots(1, 2, figsize=(10, 4.2))
    fig.suptitle("RM vs. DM on a constrained-deadline task set (simulated)")

    x = range(len(names))
    width = 0.35

    worst_a = [max(rt_a[n], default=0) for n in names]
    worst_b = [max(rt_b[n], default=0) for n in names]

    ax_rt.bar([i - width / 2 for i in x], worst_a, width, label=args.label_a.upper(), color=COLOR_OK)
    ax_rt.bar([i + width / 2 for i in x], worst_b, width, label=args.label_b.upper(), color="#6C757D")
    for i, n in enumerate(names):
        ax_rt.plot([i - 0.5, i + 0.5], [deadlines[n]] * 2, "k--", linewidth=1)
    ax_rt.set_xticks(list(x))
    ax_rt.set_xticklabels(names)
    ax_rt.set_ylabel("Worst-case response time (ms)")
    ax_rt.set_title("Response time vs. deadline (dashed)")
    ax_rt.legend()
    # A missed job is deleted and recreated (prvDeadlineMissedHook), not
    # completed late -- it contributes no response-time sample. Without this
    # note, a task with misses (see the right panel) can misleadingly look
    # like it's comfortably under its deadline here.
    ax_rt.text(0.5, -0.22, "bars include only jobs that completed -- see deadline misses, right",
               transform=ax_rt.transAxes, ha="center", fontsize=8, color="#555555")

    miss_a = [misses_a.get(n, 0) for n in names]
    miss_b = [misses_b.get(n, 0) for n in names]
    ax_miss.bar([i - width / 2 for i in x], miss_a, width, label=args.label_a.upper(),
                color=[COLOR_MISS if m > 0 else COLOR_OK for m in miss_a])
    ax_miss.bar([i + width / 2 for i in x], miss_b, width, label=args.label_b.upper(),
                color=[COLOR_MISS if m > 0 else "#6C757D" for m in miss_b])
    ax_miss.set_xticks(list(x))
    ax_miss.set_xticklabels(names)
    ax_miss.set_ylabel("Deadline misses")
    ax_miss.set_title("Deadline misses per task")
    ax_miss.legend()

    fig.tight_layout()
    fig.savefig(args.out, dpi=150)
    print(f"wrote {args.out}")


if __name__ == "__main__":
    main()
