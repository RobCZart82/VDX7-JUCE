"""Validate CTest's JSON inventory without executing tests or reading a ROM."""
import argparse
import copy
import json
import math

ROM_FREE = set("""user_bank deferred_midi latest_display status_priority bounded_file
voice_data algorithms resampling rom_diagnostics gui_header mono_correction version_identity""".split())
ROM_FREE.add("pre_rom_state")
ROM_FREE.add("factory_banks")
ROM_FREE.add("cc120_timeline_unit")
LOCAL_ROM = set("""stability midi_range timing processor stress v18_profile host_reset
reactivation reset_history_pair firmware_ownership history_retirement overlap_retirement
reset_gate_overflow expanded_lifecycle mono_boundary_characterization
mono_trace_characterization deferred_partition portamento wheel_delivery
mono_candidate_experiment supported_note_range_acceptance direct_rom_reload_boundary
state_rom_identity pending_rom_content_identity mono_corrected_processor mono_soak""".split())
LOCAL_ROM.update({"pre_rom_state_integration", "midi_reset"})
LOCAL_ROM.update({"state_transitions", "cc120_timeline", "controller_reset"})
LOCAL_ROM.add("export_acknowledgement")
LOCAL_ROM.update({"init_preset", "init_preset_gui", "init_preset_audio_matrix"})


def validate(document, rom_enabled=True, factory_bank_enabled=False):
    if factory_bank_enabled and not rom_enabled:
        raise ValueError("Factory bank integration requires local ROM tests")
    tests = document.get("tests", [])
    expected = {"vdx7_" + n for n in ROM_FREE | (LOCAL_ROM if rom_enabled else set())}
    if factory_bank_enabled:
        expected.add("vdx7_factory_bank_library")
    actual = [test["name"] for test in tests]
    if len(actual) != len(set(actual)) or set(actual) != expected:
        raise ValueError(f"Test inventory mismatch: missing={expected-set(actual)}, extra={set(actual)-expected}")
    for test in tests:
        name = test["name"]
        properties = {p["name"]: p["value"] for p in test["properties"]}
        timeout = properties.get("TIMEOUT", 0)
        if not isinstance(timeout, (int, float)) or not math.isfinite(timeout) or timeout <= 0:
            raise ValueError(f"{name}: missing/invalid timeout")
        if "SKIP_RETURN_CODE" in properties or "SKIP_REGULAR_EXPRESSION" in properties:
            raise ValueError(f"{name}: skip policy would hide fixture/test failures")
        if properties.get("DISABLED", False):
            raise ValueError(f"{name}: disabled test")
        labels = set(properties.get("LABELS", []))
        local = name.removeprefix("vdx7_") in LOCAL_ROM or name == "vdx7_factory_bank_library"
        if ("local-rom" in labels) != local or ("rom-free" in labels) == local:
            raise ValueError(f"{name}: incorrect ROM classification")
        required = properties.get("FIXTURES_REQUIRED", [])
        setup = properties.get("FIXTURES_SETUP", [])
        if name == "vdx7_v18_profile":
            if setup != ["vdx7_v18_rom"] or required:
                raise ValueError("Profile must set up, not depend on, the ROM fixture")
        elif setup or (local and required != ["vdx7_v18_rom"]) or (not local and required):
            raise ValueError(f"{name}: incorrect fixture relationship")


def self_test():
    tests = []
    for name in sorted(ROM_FREE | LOCAL_ROM):
        local = name in LOCAL_ROM
        props = {"TIMEOUT": 60, "LABELS": ["local-rom" if local else "rom-free"]}
        if local:
            props["FIXTURES_SETUP" if name == "v18_profile" else "FIXTURES_REQUIRED"] = ["vdx7_v18_rom"]
        tests.append({"name": "vdx7_" + name, "properties": [{"name": k, "value": v} for k,v in props.items()]})
    good = {"tests": tests}
    validate(good)
    factory = copy.deepcopy(good)
    bank_test = copy.deepcopy(next(t for t in factory["tests"] if t["name"] == "vdx7_export_acknowledgement"))
    bank_test["name"] = "vdx7_factory_bank_library"
    factory["tests"].append(bank_test)
    validate(factory, factory_bank_enabled=True)
    validate({"tests": [t for t in tests if t["name"].removeprefix("vdx7_") in ROM_FREE]}, False)
    mutations = []
    try:
        validate(factory)
    except ValueError:
        pass
    else:
        raise RuntimeError("Undeclared private bank test incorrectly passed")
    try:
        validate(good, factory_bank_enabled=True)
    except ValueError:
        pass
    else:
        raise RuntimeError("Missing requested private bank test incorrectly passed")
    missing = copy.deepcopy(good); missing["tests"].pop(); mutations.append(missing)
    duplicate = copy.deepcopy(good); duplicate["tests"].append(duplicate["tests"][0]); mutations.append(duplicate)
    for key, value in [("TIMEOUT", 0), ("LABELS", []), ("DISABLED", True),
                       ("SKIP_RETURN_CODE", 77), ("FIXTURES_REQUIRED", ["wrong"])]:
        bad = copy.deepcopy(good)
        target = next(t for t in bad["tests"] if t["name"] == "vdx7_pending_rom_content_identity")
        target["properties"] = [p for p in target["properties"] if p["name"] != key]
        target["properties"].append({"name": key, "value": value})
        mutations.append(bad)
    for bad in mutations:
        try:
            validate(bad)
        except ValueError:
            continue
        raise RuntimeError("Negative control incorrectly passed")
    print("PASS: registration checker default/private-bank positive and nine negative controls")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("listing", nargs="?")
    parser.add_argument("--rom-free-only", action="store_true")
    parser.add_argument("--factory-bank-library", action="store_true",
                        help="Require the opt-in private eight-bank integration test")
    parser.add_argument("--self-test", action="store_true")
    args = parser.parse_args()
    if args.self_test:
        self_test()
    if args.listing:
        with open(args.listing, encoding="utf-8-sig") as stream:
            validate(json.load(stream), not args.rom_free_only, args.factory_bank_library)
        print("PASS: complete CTest registration, labels, fixtures, timeouts and failure policy")
    elif not args.self_test:
        parser.error("provide a CTest JSON listing or --self-test")
