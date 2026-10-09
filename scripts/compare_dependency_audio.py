"""Compare explicitly supplied PRIVATE processor fingerprints; never upload PCM."""
import argparse
import hashlib
import json
import math
from pathlib import Path
import re
import struct

CASES = {f"{rate}-{block}-{mode}.f32" for rate in (44100, 48000, 96000)
         for block in (64, 128, 256, 512, 1024) for mode in ("native", "corrected")}


def load(folder):
    rows = json.loads((folder / "fingerprint.json").read_text(encoding="utf-8"))
    if not isinstance(rows, list) or len(rows) != len(CASES):
        raise ValueError("Expected complete 30-case matrix")
    result = {}
    for row in rows:
        name = row["file"]
        if name not in CASES or name in result:
            raise ValueError("Unexpected or duplicate case")
        expected = f'{row["rate"]}-{row["block"]}-{"corrected" if row["corrected"] else "native"}.f32'
        if type(row["corrected"]) is not bool or name != expected:
            raise ValueError("Case metadata mismatch")
        data = (folder / name).read_bytes()
        if type(row["samples"]) is not int or row["samples"] <= 0 or row["samples"] % 2:
            raise ValueError("Expected positive stereo sample-value count")
        if len(data) != row["samples"] * 4 or hashlib.sha256(data).hexdigest() != row["sha256"]:
            raise ValueError("PCM size/hash mismatch")
        if any(not math.isfinite(v[0]) for v in struct.iter_unpack("<f", data)):
            raise ValueError("Nonfinite PCM")
        for key in ("stateSha256", "recallSha256"):
            if not isinstance(row[key], str) or not re.fullmatch(r"[0-9a-f]{64}", row[key]):
                raise ValueError("Invalid project fingerprint")
        if any(not math.isfinite(row[key]) or row[key] < 0 for key in ("peak", "rms")):
            raise ValueError("Invalid audio metric")
        if type(row["silentBlocks"]) is not int or row["silentBlocks"] < 0:
            raise ValueError("Invalid silent block count")
        result[name] = (row, data)
    return result


def compare(old, new):
    old, new = load(old), load(new)
    summary = {"cases": len(CASES), "sampleValues": 0, "differentSampleValues": 0,
               "maxAbsDifference": 0.0, "firstDifference": None, "stateDifferences": [],
               "metricDifferences": []}
    for name in sorted(CASES):
        a, x = old[name]
        b, y = new[name]
        if len(x) != len(y):
            raise ValueError("Sample count differs: " + name)
        summary["sampleValues"] += a["samples"]
        for offset in range(0, len(x), 4):
            if x[offset:offset + 4] != y[offset:offset + 4]:
                summary["differentSampleValues"] += 1
                if summary["firstDifference"] is None:
                    summary["firstDifference"] = {"case": name, "frame": offset // 8, "channel": (offset // 4) % 2}
                difference = abs(struct.unpack_from("<f", x, offset)[0] - struct.unpack_from("<f", y, offset)[0])
                summary["maxAbsDifference"] = max(summary["maxAbsDifference"], difference)
        for key in ("stateSha256", "recallSha256"):
            if a[key] != b[key]:
                summary["stateDifferences"].append({"case": name, "field": key})
        if any(a[key] != b[key] for key in ("peak", "rms", "silentBlocks")):
            summary["metricDifferences"].append(name)
    summary["pass"] = (summary["differentSampleValues"] == 0
                       and not summary["stateDifferences"] and not summary["metricDifferences"])
    return summary


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("old", type=Path)
    parser.add_argument("new", type=Path)
    args = parser.parse_args()
    result = compare(args.old, args.new)
    print(json.dumps(result, sort_keys=True))
    raise SystemExit(0 if result["pass"] else 1)
