# ===================================================================
# SPDX-License-Identifier: Apache-2.0
# Copyright (c) 2026 Jeff Nye, uarchlabs.com
# SPDX-FileCopyrightText: 2026 Jeff Nye <jeff@uarchlabs.com>
# ===================================================================
import json

import pytest
import yaml
from conftest import ROOT

from aspen_isa import catalogue as cat

PATTERNS = ROOT / "isa" / "catalogue" / "patterns.yaml"
CATALOGUE = ROOT / "isa" / "catalogue" / "rva23s64.yaml"


@pytest.fixture(scope="session")
def built(iset, _layer_doc):
    return cat.build(iset, cat.load_patterns(PATTERNS), _layer_doc)


def entry(built, name, when):
    hits = [e for e in built["instructions"][name] if e["when"] == when]
    assert hits, f"{name}: no entry for {when}"
    return hits[0]


def test_committed_catalogue_is_current(built):
    assert yaml.safe_load(CATALOGUE.read_text()) == json.loads(json.dumps(built))


def test_every_entry_is_resolved(built):
    assert cat.unresolved(built) == []
    assert built["summary"]["unresolved"] == 0


def test_layer_agrees_with_catalogue(built, iset, db):
    problems, checked = cat.check_layer(built, iset, db)
    assert problems == []
    assert checked > 50


def test_layer_declares_every_proposed_tag(built, _layer_doc):
    assert cat.undeclared_tags(built, _layer_doc) == []


@pytest.mark.parametrize("name, when, kind, extra", [
    ("addi", {"imm12": 0}, "variant", {"tags": ["move"]}),
    ("addi", {"rd": "x0", "rs1": "x0", "imm12": 0}, "variant", {"tags": ["nop"]}),
    ("xor", {"rs2": {"eq": "rs1"}}, "variant", {"tags": ["zero_idiom", "constant"]}),
    ("sltu", {"rs2": "x0"}, "variant", {"tags": ["zero_idiom", "constant"]}),
    ("div", {"rs2": "x0"}, "variant", {"tags": ["constant"]}),
    ("csrrw", {"rd": "x0"}, "variant", {"remove_tags": ["csr_read"]}),
    ("csrrw", {"csr": {"mask": 3072, "value": 3072}}, "reserved", {}),
    ("add", {"rd": "x0", "rs1": "x0", "rs2": "x3"}, "hint", {"tags": ["ntl"]}),
    ("fence", {"succ": 0}, "hint", {}),
    ("beq", {"rs2": {"eq": "rs1"}}, "variant", {"tags": ["always_taken"]}),
    ("bne", {"rs2": {"eq": "rs1"}}, "variant", {"tags": ["never_taken"]}),
    ("fadd_d", {"rm": [5, 6]}, "reserved", {}),
    ("c_addi", {"rd_rs1_n0": "x0"}, "encoding", {"decodes_as": "c_nop"}),
    ("c_jr", {"rs1_n0": "x0"}, "reserved", {}),
    ("c_addi4spn", {"c_nzuimm10": 0}, "reserved", {}),
    ("ori", {"rd": "x0"}, "rule", {}),            # some words are prefetch.*, not all
    ("vadd_vv", {"vm": 0, "vd": "v0"}, "reserved", {}),
    ("vmseq_vv", {"vm": 0, "vd": "v0"}, "none", {}),
    ("vmseq_vv", {"vs1": {"eq": "vs2"}}, "variant", {"tags": ["ones_idiom"]}),
    ("vxor_vv", {"vs1": {"eq": "vs2"}}, "variant", {"tags": ["zero_idiom"]}),
    ("vadd_vx", {"rs1": "x0"}, "variant", {"tags": ["move"]}),
    ("fmv_d_x", {"rs1": "x0"}, "variant", {"tags": ["zero_idiom", "constant"]}),
])
def test_resolutions(built, name, when, kind, extra):
    e = entry(built, name, when)
    assert e["kind"] == kind
    for k, v in extra.items():
        assert e[k] == v


def test_pseudo_op_conditions(iset):
    r = iset.field_ranges
    base, p = iset.instrs, iset.pseudo_patterns
    assert cat.pseudo_condition(base["addi"], p["mv"], r) == {"imm12": 0}
    assert cat.pseudo_condition(base["jalr"], p["ret"], r) == {"rd": "x0", "rs1": "x1", "imm12": 0}
    assert cat.pseudo_condition(base["fsgnj_d"], p["fmv_d"], r) == {"rs2": {"eq": "rs1"}}


def test_register_files(iset):
    i = iset.instrs
    assert cat.register_file(i["fadd_d"], "rs1") == "f"
    assert cat.register_file(i["fmv_x_d"], "rd") == "x"
    assert cat.register_file(i["fcvt_d_w"], "rs1") == "x"
    assert cat.register_file(i["flw"], "rs1") == "x"
    assert cat.register_file(i["fli_d"], "rs1") is None
    assert cat.register_file(i["vadd_vx"], "rs1") == "x"
    assert cat.register_file(i["vfadd_vf"], "rs1") == "f"
    assert cat.register_file(i["vfmv_f_s"], "rd") == "f"
    assert cat.register_file(i["c_fld"], "rd_p") == "f"
    assert cat.register_file(i["c_fld"], "rs1_p") == "x"


def test_unknown_kind_is_rejected(tmp_path):
    p = tmp_path / "p.yaml"
    p.write_text("patterns:\n  - {id: x, kind: maybe, covers: ['x0:*']}\n")
    with pytest.raises(ValueError, match="unknown kind"):
        cat.load_patterns(p)


def test_stale_catalogue_is_reported(tmp_path, capsys):
    from aspen_isa.cli import main
    stale = tmp_path / "cat.yaml"
    stale.write_text(CATALOGUE.read_text().replace("kind: hint", "kind: none", 1))
    assert main(["catalogue", "--check", "-q", "--catalogue", str(stale)]) == 1
    assert "out of date" in capsys.readouterr().out
