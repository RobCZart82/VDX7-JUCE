"""Guard the ROM-free processor-state sanitizer admission, not runtime success."""
from pathlib import Path
import re
import unittest


class SanitizerWorkflowTests(unittest.TestCase):
    def test_processor_is_built_and_selected(self):
        workflow = (Path(__file__).resolve().parents[1] / ".github/workflows/sanitizers.yml").read_text()
        build = workflow.split("cmake --build", 1)[1].split("- name:", 1)[0]
        self.assertIn("vdx7_processor_tests", build.split())
        selection = re.search(r"-R '([^']+)'", workflow).group(1)
        self.assertIsNotNone(re.fullmatch(selection, "vdx7_pre_rom_state"))
        self.assertIsNotNone(re.fullmatch(selection, "vdx7_rom_diagnostics"))
        self.assertIn("vdx7_imported_banks_tests", build.split())
        self.assertIsNotNone(re.fullmatch(selection, "vdx7_imported_banks"))
        self.assertIn("vdx7_imported_processor_tests", build.split())
        self.assertIsNotNone(re.fullmatch(selection, "vdx7_imported_bank_state"))
        self.assertIn("vdx7_sound_mode_prototype_tests", build.split())
        self.assertIsNotNone(re.fullmatch(selection, "vdx7_sound_mode_prototype"))
        self.assertIn("vdx7_sound_mode_state_tests", build.split())
        self.assertIsNotNone(re.fullmatch(selection, "vdx7_sound_mode_state"))
        self.assertIn("vdx7_sound_mode_ownership_tests", build.split())
        self.assertIsNotNone(re.fullmatch(selection, "vdx7_sound_mode_ownership"))
        self.assertIn("vdx7_processor_tests", build.split())
        self.assertIsNotNone(re.fullmatch(selection, "vdx7_sound_mode_admission"))
        for private_test in ("vdx7_processor", "vdx7_pre_rom_state_integration", "vdx7_host_reset", "vdx7_imported_bank_recall"):
            self.assertIsNone(re.fullmatch(selection, private_test))
        self.assertIn("-DVDX7_ENABLE_ROM_TESTS=OFF", workflow)


if __name__ == "__main__":
    unittest.main()
