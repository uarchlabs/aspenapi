# ===================================================================
# SPDX-License-Identifier: Apache-2.0
# Copyright (c) 2026 Jeff Nye, uarchlabs.com
# SPDX-FileCopyrightText: 2026 Jeff Nye <jeff@uarchlabs.com>
# ===================================================================
import copy
import os
from pathlib import Path

import pytest
from conftest import OPCODES

from aspen_isa import opcodes, profile

SPEC = os.environ.get("RVA23_PROFILE_ADOC")


def test_profile_is_consistent(profile_doc):
    assert profile.check(profile_doc, OPCODES) == []


def test_every_file_once(profile_doc):
    bad = copy.deepcopy(profile_doc)
    bad["extensions"][1]["files"].append("rv_i")
    assert any("already listed" in e for e in profile.check(bad, OPCODES))


def test_unknown_file(profile_doc):
    bad = copy.deepcopy(profile_doc)
    bad["extensions"][0]["files"].append("rv_nosuch")
    assert any("rv_nosuch" in e for e in profile.check(bad, OPCODES))


def test_groups(profile_doc):
    assert "rv_v" in opcodes.profile_files(profile_doc, "vector")
    assert "rv_v" not in opcodes.profile_files(profile_doc, "scalar")
    assert "rv_h" in opcodes.profile_files(profile_doc, "privileged")


@pytest.mark.skipif(not SPEC, reason="set RVA23_PROFILE_ADOC to riscv-profiles src/rva23-profile.adoc")
def test_matches_ratified_profile(profile_doc):
    missing, extra = profile.compare_with_spec(profile_doc, Path(SPEC))
    assert (missing, extra) == ([], [])
