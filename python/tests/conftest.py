# ===================================================================
# SPDX-License-Identifier: Apache-2.0
# Copyright (c) 2026 Jeff Nye, uarchlabs.com
# SPDX-FileCopyrightText: 2026 Jeff Nye <jeff@uarchlabs.com>
# ===================================================================
import copy
import sys
from pathlib import Path

import pytest

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "python"))

from aspen_isa import layer as lay  # noqa: E402
from aspen_isa import opcodes  # noqa: E402

PROFILE = ROOT / "isa" / "profiles" / "rva23s64.yaml"
LAYER = ROOT / "isa" / "layer" / "rva23s64.yaml"
IDS = ROOT / "isa" / "ids.yaml"
OPCODES = ROOT / "ext" / "riscv-opcodes"


@pytest.fixture(scope="session")
def iset():
    return opcodes.load(OPCODES, opcodes.load_profile(PROFILE))


@pytest.fixture(scope="session")
def schema():
    return lay.load_schema()


@pytest.fixture(scope="session")
def _layer_doc():
    return lay.load_yaml(LAYER)


@pytest.fixture
def layer(_layer_doc):
    """A fresh copy of the shipped layer that a test may modify."""
    return copy.deepcopy(_layer_doc)


@pytest.fixture
def check(iset, schema):
    def run(doc):
        errors, _ = lay.validate(doc, iset, schema)
        return errors
    return run


@pytest.fixture(scope="session")
def profile_doc():
    return opcodes.load_profile(PROFILE)


@pytest.fixture(scope="session")
def db(iset, _layer_doc, profile_doc):
    from aspen_isa import ids as idreg
    from aspen_isa import merge
    return merge.build(_layer_doc, iset, idreg.load(IDS), profile_doc, OPCODES)


@pytest.fixture(scope="session")
def decoder(iset, db):
    from aspen_isa import decode
    return decode.Decoder(iset, db["decode_order"])


@pytest.fixture(scope="session")
def evaluator(db):
    from aspen_isa import evaluate
    return evaluate.Evaluator(db)
