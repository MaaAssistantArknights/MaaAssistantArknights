"""Resource regression against recorded BlackFlow matcher scores.

Run directly with Python. Optional first argument selects a release directory.
This checks score separation, not controller input or game transitions.
"""

import json
import sys
import unittest
from pathlib import Path

REPO = Path(__file__).resolve().parents[2]
RESOURCE_ROOT = REPO / "resource"
FIXTURE = Path(__file__).with_name("blackflow_encounter_scores.json")
MATCH_TASK = "BlackFlow@RoguelikeEncounterOptionAnalyzer-Option"


class BlackFlowEncounterMatchingTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.tasks = json.loads(
            (RESOURCE_ROOT / "tasks/Roguelike/BlackFlow.json").read_text(
                encoding="utf-8"
            )
        )
        cls.threshold = cls.tasks[MATCH_TASK].get("templThreshold", 0.8)
        cls.fixture = json.loads(FIXTURE.read_text(encoding="utf-8"))

    def test_hidden_first_option_is_rejected(self):
        # The first two templates incorrectly resolved to the same disabled row.
        hidden = self.fixture["hidden_target"]
        self.assertLess(hidden[0]["score"], self.threshold)
        for item in hidden[1:]:
            self.assertGreaterEqual(item["score"], self.threshold)
        self.assertEqual(hidden[0]["y"], hidden[1]["y"])

    def test_visible_first_option_is_accepted(self):
        # Recorded matching scores with the actual first row visible.
        for item in self.fixture["visible_target"]:
            self.assertGreaterEqual(item["score"], self.threshold)

    def test_other_events_keep_recorded_best_position_matches(self):
        # These are observational positives from the existing log, not hand-labelled images.
        for event, sample in self.fixture["best_positions"].items():
            with self.subTest(event=event):
                self.assertGreaterEqual(sample["min_score"], self.threshold)

    def test_scroll_overlap_matches_are_accepted(self):
        for event, sample in self.fixture["scroll_overlaps"].items():
            with self.subTest(event=event):
                self.assertGreaterEqual(sample["min_score"], self.threshold)

    def test_all_recorded_duplicate_candidates_are_rejected(self):
        self.assertLess(
            self.fixture["duplicate_candidates"]["max_score"], self.threshold
        )

    def test_three_and_four_option_strategy_is_unchanged(self):
        config = json.loads(
            (RESOURCE_ROOT / "roguelike/BlackFlow/encounter/default.json").read_text(
                encoding="utf-8"
            )
        )
        event = next(event for event in config["stage"] if event["name"] == "险路尽头")
        self.assertEqual(event["option_num"], 3)
        self.assertEqual(event["choose"], 1)
        self.assertIn([4, 1], event["fallback_choices"])


if __name__ == "__main__":
    if len(sys.argv) > 1 and not sys.argv[1].startswith("-"):
        RESOURCE_ROOT = Path(sys.argv.pop(1)) / "resource"
    unittest.main(verbosity=2)
