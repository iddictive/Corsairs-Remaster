"""Arithmetic counterexample for the current producer, not a game replay."""
import json

rows = []
for fps in (60, 100, 120, 240):
    clock = 0.0
    previous = None
    windows = []
    speed = 20.0
    for frame in range(1, fps + 1):
        clock += 1 / fps
        sample_time = clock if clock >= .035 else 0.0
        hit = speed * frame / fps
        old = previous
        previous = hit  # rays is refreshed even when sample_time is zero
        if sample_time:
            clock -= sample_time
            if old is not None:
                windows.append((old, hit))
    rows.append(dict(fps=fps, travel_m=speed,
        sampled_centerline_m=sum(b - a for a, b in windows),
        windows=len(windows), beam_diameter_m=.32,
        largest_unsampled_centerline_gap_m=max(
            windows[n + 1][0] - windows[n][1] for n in range(len(windows) - 1))))
print(json.dumps(rows, indent=2))
