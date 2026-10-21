#!/usr/bin/env python3
"""Focused regression tests for the project hygiene guard."""

from __future__ import annotations

import unittest

from tools import agent_context


class EnvironmentHygieneTests(unittest.TestCase):
    def test_rejects_deprecated_runtime_action(self) -> None:
        text = """
version = 1
[[actions]]
name = "Project state"
icon = "test"
command = "python3 tools/agent_context.py --check"
[[actions]]
name = "Launch old runtime"
icon = "play"
command = "./experiments/native-storm/run.sh"
"""
        self.assertTrue(
            any("deprecated runtime" in item for item in agent_context.environment_hygiene_errors(text))
        )

    def test_allows_historical_probe(self) -> None:
        text = """
version = 1
[[actions]]
name = "Project state"
icon = "test"
command = "python3 tools/agent_context.py --check"
[[actions]]
name = "Patch probes"
icon = "test"
command = "python3 experiments/native-storm/deck_walk_probe.py"
"""
        self.assertEqual([], agent_context.environment_hygiene_errors(text))


if __name__ == "__main__":
    unittest.main()
