# ===================================================================
# SPDX-License-Identifier: Apache-2.0
# Copyright (c) 2026 Jeff Nye, uarchlabs.com
# SPDX-FileCopyrightText: 2026 Jeff Nye <jeff@uarchlabs.com>
# ===================================================================
import random

import pytest

from aspen_isa import decode


def test_no_unresolved_overlaps(iset):
    assert decode.analyse(iset).conflicts == []


def test_known_nested_overlaps(iset):
    nested = set(decode.analyse(iset).nested)
    for general, specific in [("c_addi", "c_nop"), ("fence", "pause"), ("ori", "prefetch_r"),
                              ("c_lui", "c_addi16sp"), ("c_lui", "c_mop_1"), ("c_mv", "c_jr"),
                              ("c_add", "c_ebreak"), ("mop_r_N", "mop_r_0")]:
        assert (general, specific) in nested


def test_generic_mops_are_shadowed(iset):
    assert decode.analyse(iset).shadowed == ["c_mop_N", "mop_r_N", "mop_rr_N"]


def test_order_puts_specific_first(iset):
    ov = decode.analyse(iset)
    pos = {n: i for i, n in enumerate(ov.order)}
    assert all(pos[s] < pos[g] for g, s in ov.nested)


def test_every_reachable_entry_decodes_to_itself(iset, decoder):
    """Sample words from each entry's own space (outside its more specific entries)."""
    ov = decode.analyse(iset)
    kids = {}
    for g, s in ov.nested:
        kids.setdefault(g, []).append(iset.instrs[s])
    rng = random.Random(23)
    for name, ins in iset.instrs.items():
        if name in ov.shadowed:
            continue
        width = 16 if ins.length == 16 else 32
        free = ((1 << width) - 1) & ~ins.mask
        for _ in range(2000):
            word = ins.match | (rng.getrandbits(width) & free)
            if not any((word & k.mask) == k.match for k in kids.get(name, [])):
                break
        else:
            pytest.fail(f"no word found in {name}'s own space")
        assert decoder.decode(word)[0] == name, f"{word:#x}"


def test_encode_extract_round_trip(iset):
    ins = iset.instrs["sd"]
    word = decode.encode(ins, {"rs1": 2, "rs2": 8, "imm12hi": 1, "imm12lo": 8}, iset.field_ranges)
    assert decode.extract(word, ins, iset.field_ranges) == {"imm12hi": 1, "rs1": 2, "rs2": 8, "imm12lo": 8}
    with pytest.raises(ValueError):
        decode.encode(ins, {"rs1": 32}, iset.field_ranges)


def test_prime_registers():
    assert decode.register_value("rd_p", 2) == 10
    assert decode.register_value("rd_rs1_p", 0) == 8
    assert decode.register_value("rd", 2) == 2
    assert decode.register_value("rd_n0", 2) == 2


def test_spike_names(iset):
    assert iset.instrs["zext_h"].spike_name == "packw"
    assert iset.instrs["zext_h"].pseudo_of == "packw"
    assert iset.instrs["pause"].spike_name == "pause"
    assert iset.instrs["c_ld"].pseudo_of is None
    assert [n for n, i in iset.instrs.items() if i.spike_name != n] == ["zext_h"]
