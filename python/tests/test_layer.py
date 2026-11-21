# ===================================================================
# SPDX-License-Identifier: Apache-2.0
# Copyright (c) 2026 Jeff Nye, uarchlabs.com
# SPDX-FileCopyrightText: 2026 Jeff Nye <jeff@uarchlabs.com>
# ===================================================================
import pytest
import yaml

from aspen_isa import layer as lay


def has(errors, *words):
    return any(all(w in e for w in words) for e in errors)


def test_shipped_layer_is_valid(layer, check):
    assert check(layer) == []


# ---------------------------------------------------------------- schema
def test_latency_is_rejected(layer, check):
    layer["instructions"]["addi"]["latency"] = 1
    assert has(check(layer), "schema", "latency")


def test_entry_needs_fu_class_or_expansion(layer, check):
    del layer["instructions"]["xor"]["fu_class"]
    assert has(check(layer), "schema", "instructions/xor")


def test_entry_cannot_have_both(layer, check):
    layer["instructions"]["c_mv"]["fu_class"] = "alu"
    assert has(check(layer), "schema", "instructions/c_mv")


def test_register_role_needs_file(layer, check):
    layer["instructions"]["add"]["operands"]["rd"] = {"role": "dest"}
    assert has(check(layer), "schema", "operands/rd")


def test_duplicate_yaml_key(tmp_path):
    p = tmp_path / "dup.yaml"
    p.write_text("a: {rd: x1, rd: x5}\n")
    with pytest.raises(yaml.YAMLError, match="duplicate key 'rd'"):
        lay.load_yaml(p)


# ---------------------------------------------------------------- names and vocabularies
def test_unknown_instruction(layer, check):
    layer["instructions"]["frobnicate"] = {"fu_class": "alu"}
    assert has(check(layer), "frobnicate", "not an instruction")


def test_pseudo_op_points_to_base(layer, check):
    layer["instructions"]["mv"] = {"fu_class": "alu"}
    assert has(check(layer), "mv", "pseudo-op")


def test_undeclared_tag(layer, check):
    layer["instructions"]["add"]["variants"][0]["add_tags"] = ["teleport"]
    assert has(check(layer), "teleport", "not declared")


def test_undeclared_fu_class(layer, check):
    layer["instructions"]["add"]["fu_class"] = "warp"
    assert has(check(layer), "warp", "not declared")


def test_attribute_type(layer, check):
    layer["instructions"]["fence"]["set"] = {"serialize": "sometimes"}
    assert has(check(layer), "serialize", "does not fit")


def test_undeclared_csr_in_class(layer, check):
    layer["csr_classes"]["counters"]["csrs"].append("nosuchcsr")
    assert has(check(layer), "nosuchcsr", "csrs.csv")


# ---------------------------------------------------------------- operands
def test_operands_must_cover_fields(layer, check):
    del layer["instructions"]["sd"]["operands"]["imm12lo"]
    assert has(check(layer), "sd", "missing fields", "imm12lo")


def test_operand_role_must_fit_field(layer, check):
    layer["instructions"]["addi"]["operands"]["imm12"] = {"role": "src", "file": "x"}
    assert has(check(layer), "imm12", "does not fit")


# ---------------------------------------------------------------- variants
def test_unknown_field_in_when(layer, check):
    layer["instructions"]["addi"]["variants"][0]["when"] = {"rs2": "x0"}
    assert has(check(layer), "addi", "no field 'rs2'")


def test_number_on_register_field(layer, check):
    layer["instructions"]["add"]["variants"][0]["when"] = {"rs1": 0}
    assert has(check(layer), "register field")


def test_register_on_immediate_field(layer, check):
    layer["instructions"]["addi"]["variants"][0]["when"] = {"imm12": "x0"}
    assert has(check(layer), "not a register field")


def test_register_file_mismatch(layer, check):
    layer["instructions"]["fsgnj_d"]["variants"][0]["when"] = {"rs1": "x3"}
    assert has(check(layer), "x3", "'f' file")


def test_value_too_wide(layer, check):
    layer["instructions"]["fadd_d"]["variants"][0]["when"] = {"rm": 8}
    assert has(check(layer), "3-bit field 'rm'")


def test_eq_must_name_a_field(layer, check):
    layer["instructions"]["xor"]["variants"][0]["when"] = {"rs2": {"eq": "rs9"}}
    assert has(check(layer), "rs9", "not a field")


def test_mask_value_outside_mask(layer, check):
    layer["instructions"]["fence"]["variants"][0]["when"] = {"pred": {"mask": 1, "value": 2}}
    assert has(check(layer), "outside the mask")


def test_csr_name_and_class(layer, check):
    v = layer["instructions"]["csrrs"]["variants"]
    v[1]["when"] = {"csr": "nosuchcsr"}
    v[2]["when"] = {"csr": {"csr_class": "nosuchclass"}}
    errors = check(layer)
    assert has(errors, "nosuchcsr", "csrs.csv")
    assert has(errors, "nosuchclass", "not declared")


def test_csr_class_only_on_csr_field(layer, check):
    layer["instructions"]["add"]["variants"][0]["when"] = {"rs1": {"csr_class": "counters"}}
    assert has(check(layer), "only to the csr field")


def test_add_and_remove_same_tag(layer, check):
    v = layer["instructions"]["add"]["variants"][0]
    v["remove_tags"] = list(v["add_tags"])
    assert has(check(layer), "both added and removed")


def test_nested_matchers_are_checked(layer, check):
    w = layer["instructions"]["jalr"]["variants"][2]["when"]
    w["rs1"] = {"all": [["x1", "x5"], {"ne": "rs9"}]}
    assert has(check(layer), "rs9", "not a field")


# ---------------------------------------------------------------- expansions, rules, fusion, vector
def test_expansion_must_map_every_base_field(layer, check):
    del layer["instructions"]["c_mv"]["expands_to"]["operands"]["rs1"]
    assert has(check(layer), "c_mv", "not mapped", "rs1")


def test_expansion_source_field_must_exist(layer, check):
    layer["instructions"]["c_addi4spn"]["expands_to"]["operands"]["rd"] = "rd_q"
    assert has(check(layer), "c_addi4spn", "no field 'rd_q'")


def test_expansion_base_needs_entry(layer, check):
    del layer["instructions"]["add"]
    assert has(check(layer), "c_mv", "no layer entry")


def test_rule_must_match_something(layer, check):
    layer["rules"].append({"name": "nothing", "when": {"nofield": 0}, "add_tags": ["nop"]})
    assert has(check(layer), "rules/1", "no instruction has")


def test_fusion_field_must_exist(layer, check):
    layer["fusion"][0]["when"]["second.rs2"] = {"eq": "first.rd"}
    assert has(check(layer), "fusion/0", "second.rs2")


def test_fusion_eq_reference(layer, check):
    layer["fusion"][0]["when"]["second.rs1"] = {"eq": "first.rs1"}
    assert has(check(layer), "first.rs1", "not a field")


def test_vector_data_only_on_vector_instructions(layer, check):
    layer["instructions"]["add"]["vector"] = {"kind": "arith"}
    assert has(check(layer), "add", "not a vector instruction")


def test_undeclared_uop_pattern(layer, check):
    layer["instructions"]["vadd_vv"]["vector"]["uop_pattern"] = "per_moon"
    assert has(check(layer), "per_moon", "not declared")


def test_wrong_profile(layer, check):
    layer["profile"] = "rv64gc"
    assert has(check(layer), "rv64gc")
