# ===================================================================
# SPDX-License-Identifier: Apache-2.0
# Copyright (c) 2026 Jeff Nye, uarchlabs.com
# SPDX-FileCopyrightText: 2026 Jeff Nye <jeff@uarchlabs.com>
# ===================================================================
"""The layer's semantics, case by case, through decode and evaluation.

These cases are the contract the generated decoders (A5, A6) must meet.
"""
import pytest


def run(decoder, evaluator, word):
    name, raw = decoder.decode(word)
    return name, evaluator.evaluate(name, raw)


@pytest.mark.parametrize("word, name, tags, attrs", [
    # jalr and the return-address stack (unprivileged spec, RAS hints table)
    (0x00008067, "jalr", {"jump", "indirect", "return", "no_dest"}, {}),           # ret
    (0x000080E7, "jalr", {"jump", "indirect", "call"}, {}),                        # jalr x1, 0(x1): same link, push only
    (0x000280E7, "jalr", {"jump", "indirect", "call", "return"}, {}),              # jalr x1, 0(x5): pop then push
    (0x00030367, "jalr", {"jump", "indirect"}, {}),                                # jalr x6, 0(x6)
    (0x00008367, "jalr", {"jump", "indirect", "return"}, {}),                      # jalr x6, 0(x1)
    (0x000300E7, "jalr", {"jump", "indirect", "call"}, {}),                        # jalr x1, 0(x6)
    (0x008000EF, "jal", {"jump", "direct", "call"}, {}),                           # jal x1, 8
    (0x0080006F, "jal", {"jump", "direct", "no_dest"}, {}),                        # j 8
    # rename idioms
    (0x00000013, "addi", {"nop", "no_dest", "constant", "zero_idiom"}, {}),         # nop
    (0x00058513, "addi", {"move"}, {}),                                            # mv a0, a1
    (0x00500513, "addi", {"constant"}, {}),                                        # li a0, 5
    (0x00A54533, "xor", {"zero_idiom", "constant"}, {}),                           # xor a0, a0, a0
    (0x00B54533, "xor", set(), {}),                                                # xor a0, a0, a1
    (0x0000852E, "c_mv", {"move"}, {}),                                            # c.mv a0, a1, via add
    (0x22B58553, "fsgnj_d", {"move"}, {}),                                         # fmv.d fa0, fa1
    # memory and ordering
    (0x00813503, "ld", {"stack_access"}, {}),                                      # ld a0, 8(sp)
    (0x0330000F, "fence", {"memory_only", "store_load", "no_dest"}, {"serialize": "drain_before"}),  # fence rw,rw
    (0x0220000F, "fence", {"memory_only", "no_dest"}, {"serialize": "drain_before"}),  # fence r,r
    (0x06B5252F, "amoadd_w", {"acquire", "release"}, {}),                          # amoadd.w.aqrl a0, a1, (a0)
    # CSRs
    (0xC00022F3, "csrrs", {"csr_read", "counter_read"}, {}),                       # csrr t0, cycle
    (0x1802A073, "csrrs", {"csr_read", "csr_write", "no_dest"}, {"serialize": "flush"}),  # csrs satp, t0
    (0x0022A073, "csrrs", {"csr_read", "csr_write", "no_dest"}, {"serialize": "rename"}),  # csrs frm, t0
    # system and FP
    (0x12000073, "sfence_vma", {"tlb_flush", "tlb_all_addresses", "tlb_all_asids"}, {"serialize": "flush"}),
    (0x02B57553, "fadd_d", {"dyn_rm"}, {}),                                        # fadd.d fa0, fa0, fa1 (rm=dyn)
    (0x02B50553, "fadd_d", set(), {}),                                             # rm=rne
    # compressed reserved encoding
    (0x00000000, "c_addi4spn", {"move"}, {"reserved": True}),
])
def test_cases(decoder, evaluator, word, name, tags, attrs):
    got_name, res = run(decoder, evaluator, word)
    assert got_name == name
    assert res.tags == tags
    for k, v in attrs.items():
        assert res.attributes[k] == v


def test_masked_vector_reads_v0(decoder, evaluator):
    _, res = run(decoder, evaluator, 0x002081D7)       # vadd.vv v3, v2, v1, v0.t
    assert res.tags == {"masked"}
    assert res.implicit == [{"reg": "v0", "role": "src"}]
    _, res = run(decoder, evaluator, 0x022081D7)       # unmasked
    assert res.tags == set() and res.implicit == []


def test_vsetvli_forms(decoder, evaluator):
    assert run(decoder, evaluator, 0x0D057557)[1].tags == {"vtype_static", "vl_from_rs1"}  # vsetvli a0, a0, e64
    assert run(decoder, evaluator, 0x0D007557)[1].tags == {"vtype_static", "vl_max"}       # vsetvli a0, x0, e64
    assert run(decoder, evaluator, 0x0D007057)[1].tags == {"vtype_static", "vl_keep", "no_dest"}


def test_compressed_inherits_base(db):
    rec = {r["name"]: r for r in db["instructions"]}
    assert rec["c_mv"]["layer"] == "inherited"
    assert rec["c_mv"]["fu_class"] == "alu"
    assert rec["c_addi4spn"]["fu_class"] == "alu"
    assert rec["sw"]["layer"] == "missing"


def test_missing_entry_is_an_error(evaluator):
    with pytest.raises(KeyError):
        evaluator.evaluate("sw", {})
