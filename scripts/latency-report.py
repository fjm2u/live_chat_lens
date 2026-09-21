#!/usr/bin/env python3
"""Summarize receipt-to-recommendation latency; no third-party packages."""
import collections
import json
import math
import sys
values = []
counts = collections.Counter()
for path in sys.argv[1:]:
    with open(path, encoding="utf-8") as handle:
        for line in handle:
            try:
                row = json.loads(line)
            except json.JSONDecodeError:
                continue  # A crashed process may leave a truncated last line.
            counts[row.get("event", "unknown")] += 1
            value = row.get("latency_ms")
            if row.get("event") == "comment_shown" and isinstance(value, (int, float)) and value >= 0:
                values.append(value)
values.sort()
def percentile(p):
    return values[max(0, math.ceil(len(values) * p) - 1)] if values else None
print(json.dumps({"shown_samples": len(values), "p50_ms": percentile(.5),
                  "p95_ms": percentile(.95), "events": counts,
                  "p95_target_under_1000ms": percentile(.95) < 1000 if values else None}, ensure_ascii=False, indent=2))
