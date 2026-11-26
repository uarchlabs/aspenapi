# ===================================================================
# SPDX-License-Identifier: Apache-2.0
# Copyright (c) 2026 Jeff Nye, uarchlabs.com
# SPDX-FileCopyrightText: 2026 Jeff Nye <jeff@uarchlabs.com>
# ===================================================================
import json

from aspen_isa.cli import main


def test_merge_writes_database(tmp_path, capsys):
    out = tmp_path / "db.json"
    assert main(["merge", "-o", str(out)]) == 0
    db = json.loads(out.read_text())
    assert db["profile"] == "rva23s64"
    assert len(db["instructions"]) == 746
    assert db["instructions"][0]["id"] == 1
    assert "layer coverage" in capsys.readouterr().err


def test_decode_prints_tags(capsys):
    assert main(["decode", "00008067"]) == 0
    assert "jalr" in capsys.readouterr().out


def test_validate_and_profile():
    assert main(["validate", "-q"]) == 0
    assert main(["profile"]) == 0
