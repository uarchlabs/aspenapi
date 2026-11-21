# ===================================================================
# SPDX-License-Identifier: Apache-2.0
# Copyright (c) 2026 Jeff Nye, uarchlabs.com
# SPDX-FileCopyrightText: 2026 Jeff Nye <jeff@uarchlabs.com>
# ===================================================================
from conftest import IDS

from aspen_isa import ids as idreg


def test_shipped_registry_is_complete(iset):
    errors, _ = idreg.check(idreg.load(IDS), iset)
    assert errors == []


def test_id_set_matches_spike_decode(iset):
    # Base entries plus the pseudo-ops riscv-opcodes emits into encoding.h.
    for name in ("addi", "jalr", "c_addi4spn", "vle32_v", "pause", "prefetch_r", "mop_r_0", "c_mop_1"):
        assert name in iset.instrs
    # Other pseudo-ops are operand patterns, not IDs.
    for name in ("mv", "ret", "fmv_d", "nop", "ntl_all"):
        assert name not in iset.instrs
        assert name in iset.pseudo_patterns


def test_missing_id(iset):
    reg = idreg.load(IDS)
    del reg["addi"]
    errors, _ = idreg.check(reg, iset)
    assert any("'addi' has no ID" in e for e in errors)


def test_duplicate_number(iset):
    reg = idreg.load(IDS)
    reg["xor"] = reg["addi"]
    errors, _ = idreg.check(reg, iset)
    assert any("already belongs" in e for e in errors)


def test_append_only_against_base(iset):
    base = idreg.load(IDS)
    reg = dict(base)
    del reg["addi"]
    reg["xor"] = 10_000
    errors, _ = idreg.check(reg, iset, base)
    assert any("ids/addi: removed" in e for e in errors)
    assert any("ids/xor: changed" in e for e in errors)


def test_retired_name_is_a_warning(iset):
    reg = idreg.load(IDS)
    reg["old_custom_op"] = max(reg.values()) + 1
    errors, warnings = idreg.check(reg, iset)
    assert errors == []
    assert any("old_custom_op" in w for w in warnings)


def test_assign_appends_and_keeps_numbers(iset, tmp_path):
    base = idreg.load(IDS)
    reg = dict(base)
    gone = ["vadd_vv", "add"]
    for n in gone:
        del reg[n]
    added = idreg.assign(reg, iset)
    assert sorted(added) == sorted(gone)
    top = max(base.values())
    assert all(reg[n] > top for n in gone)
    assert all(reg[n] == base[n] for n in base if n not in gone)
    p = tmp_path / "ids.yaml"
    idreg.dump(reg, p)
    assert idreg.load(p) == reg
