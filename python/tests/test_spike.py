# ===================================================================
# SPDX-License-Identifier: Apache-2.0
# Copyright (c) 2026 Jeff Nye, uarchlabs.com
# SPDX-FileCopyrightText: 2026 Jeff Nye <jeff@uarchlabs.com>
# ===================================================================
import os
from pathlib import Path

import pytest

from aspen_isa import spike

ENCODING_H = os.environ.get("SPIKE_ENCODING_H")


@pytest.mark.skipif(not ENCODING_H, reason="set SPIKE_ENCODING_H to ua-spike's riscv/encoding.h")
def test_encodings_match_spike(iset):
    errors, notes = spike.compare(iset, Path(ENCODING_H))
    assert errors == []
    assert notes == ["zext_h: executed by Spike as packw"]


def test_reads_encoding_h(tmp_path, iset):
    ins = iset.instrs["addi"]
    p = tmp_path / "encoding.h"
    p.write_text(f"""/*
 * https://github.com/riscv/riscv-opcodes ({iset.commit[:7]})
 */
#define MATCH_ADDI {ins.match:#x}
#define MASK_ADDI {ins.mask:#x}
DECLARE_INSN(addi, MATCH_ADDI, MASK_ADDI)
""")
    commit, insns = spike.read(p)
    assert commit == iset.commit[:7]
    assert insns == {"addi": (ins.match, ins.mask)}
