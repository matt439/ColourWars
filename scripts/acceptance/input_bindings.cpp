#include "game/pch.h"
#include "scripts/acceptance/input_bindings/fixtures.h"
#include "scripts/acceptance/support/probe.h"
#include <cstdlib>
#include <iostream>

using namespace input_bindings_acceptance;

int main(int argc, char** argv)
{
    return acceptance::run_probe("input bindings", [&]
    {
        if (argc == 4) { check_relaunch(argv[1], argv[2], std::filesystem::path(argv[3])); return EXIT_SUCCESS; }
        require(argc == 2, "Usage: input_bindings.exe <content> [relaunch-write|relaunch-read <private profile root>]");
        const GameTuning tuning((std::filesystem::path(argv[1]) / "tuning.json").string().c_str());
        check_domain_and_mapping(tuning.input()); check_pad_domain_and_mapping(tuning.input());
        check_actual_reader(argv[1]); check_actual_pad_reader(argv[1]); check_actual_resume(argv[1]);
        const IsolatedSaveDirectory directory; check_save_schema_and_recovery(directory);
        check_save_validation_failures_and_independence(directory, argv[1]);
        check_pad_save(directory, argv[1]);
        check_keyboard_domain_and_mapping(tuning.input());
        check_keyboard_reader(argv[1]);
        check_keyboard_save(directory);
        std::cout << "PASS input bindings keyboard actions: explicit seven-action domains (A-Z/Space, Shoot's and Sub's five mouse "
            "buttons, optional extra Pause), reserved keys, defaults equal to the old literals, per-profile conflicts, "
            "effective mapping with fixed Escape/Down/intro/menu aliases (F1 in Debug only), actual reader remap/held adoption/focus, "
            "save schema/round trip, missing/damaged/conflicting keyboard-object recovery and pad independence\n";
        std::cout << "PASS input bindings: independent 27-key names/legends/domain, A/D/S conflicts and W/R aliases, "
            "isolated effective Jump with fixed menu/intro/pad controls; real readers, repeated snapshots/focus/remapping, "
            "held-J ResumeCountdown release gating, version-0/1/2 migration/version-3/future known sections, malformed-section isolation, "
            "validation before mutation, real staged-write/replacement failures with active-before-write preference and retry, "
            "independent Settings/Controls/display/records/remembered saves; per-slot Jump/Shoot/Pause over eleven controls with "
            "alternates, conflicts and trigger thresholds, legacy face migration, local recovery, "
            "sparse-slot/remap/reconnect/resume and failed-pad-save checks; GUID-isolated profile, no window/device\n";
        return EXIT_SUCCESS;
    });
}
