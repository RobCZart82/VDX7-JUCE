"""Validate the 14.912 s REAPER MONO fixture; not a pitch/dropout oracle.

Usage: python3 scripts/check_reaper_mono_matrix.py LOCAL_RENDER_DIRECTORY
Proprietary project/voice/ROM data must remain outside the repository.
"""
import argparse
import json
from pathlib import Path
import wave


def inspect(path, expected_rate):
    with wave.open(str(path), 'rb') as stream:
        if stream.getnchannels() != 2 or stream.getsampwidth() != 3:
            raise ValueError('Expected stereo 24-bit PCM')
        rate, frames = stream.getframerate(), stream.getnframes()
        data = stream.readframes(frames)
    if rate != expected_rate:
        raise ValueError(f'Expected {expected_rate} Hz, got {rate}')
    if len(data) != frames * 6 or not 14.90 <= frames / rate <= 14.93:
        raise ValueError('Truncated or wrong-duration fixture')
    samples = [int.from_bytes(data[i:i + 3], 'little', signed=True)
               for i in range(0, len(data), 3)]

    def peak(start, end):
        return max(map(abs, samples[int(start * rate) * 2:int(end * rate) * 2]))

    first, last, tail = peak(4.45, 5.2), peak(11.05, 11.85), peak(13, 14.9)
    clipped = sum(v >= 8388607 or v <= -8388608 for v in samples)
    errors = []
    if min(first, last) < 8389:  # -60 dBFS; reject silence/numerical residue
        errors.append('Missing subsequent-note audio')
    if tail != 0:
        errors.append('Nonzero final tail')
    if clipped:
        errors.append('Full-scale clipping')
    return dict(file=path.name, first_peak=first / 8388608,
                last_peak=last / 8388608, tail_peak=tail / 8388608,
                clipped_samples=clipped, errors=errors)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('directory', type=Path)
    args = parser.parse_args()
    cases = [(f'mono-8instances-{rate}-{block}-online.wav', rate)
             for rate in (44100, 48000, 96000) for block in (64, 128, 256, 512)]
    cases.append(('mono-8instances-reopened-online.wav', 44100))
    results = []
    for name, rate in cases:
        try:
            result = inspect(args.directory / name, rate)
        except (OSError, ValueError, EOFError, wave.Error) as error:
            result = dict(file=name, errors=[str(error)])
        results.append(result)
    print(json.dumps(results, indent=2))
    return int(any(result['errors'] for result in results))


if __name__ == '__main__':
    raise SystemExit(main())
