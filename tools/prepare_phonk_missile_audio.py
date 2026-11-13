#!/usr/bin/env python3
"""Synthesize the source Phonk missile effects; no external recordings."""
import math
import random
import struct
import wave
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1] / "src/assets/audio"
RATE = 22050


def render(name, duration, seed, low, high, decay, pitch):
    rng = random.Random(seed)
    lowpass = highpass = 0.0
    with wave.open(str(ROOT / name), "wb") as out:
        out.setparams((1, 2, RATE, 0, "NONE", "not compressed"))
        frames = bytearray()
        for i in range(int(duration * RATE)):
            t = i / RATE
            noise = rng.uniform(-1.0, 1.0)
            lowpass += low * (noise - lowpass)
            highpass += high * (noise - highpass)
            attack = min(1.0, t / .035)
            envelope = attack * math.exp(-decay * t)
            rumble = math.sin(2 * math.pi * (pitch * t - pitch * .12 * t * t))
            sample = math.tanh((lowpass * 1.9 + (noise-highpass) * .23 + rumble * .16) * envelope * 2.3)
            frames += struct.pack('<h', round(max(-1.0, min(1.0, sample)) * 27000))
        out.writeframes(frames)


if __name__ == '__main__':
    ROOT.mkdir(parents=True, exist_ok=True)
    render('missile-heavy.wav', 1.8, 6101, .015, .12, 1.4, 67)
    render('missile-cruise.wav', 1.5, 6102, .055, .2, 1.7, 110)
    render('missile-interceptor.wav', .8, 6103, .095, .3, 3.1, 180)
    render('missile-flyby.wav', .6, 6104, .09, .22, 3.4, 150)
    render('missile-impact.wav', 1.7, 6105, .012, .12, 2.1, 53)
