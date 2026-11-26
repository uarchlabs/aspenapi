# ===================================================================
# SPDX-License-Identifier: Apache-2.0
# Copyright (c) 2026 Jeff Nye, uarchlabs.com
# SPDX-FileCopyrightText: 2026 Jeff Nye <jeff@uarchlabs.com>
# ===================================================================
"""Compare the instruction set with Spike's riscv/encoding.h."""

from __future__ import annotations

import re
from pathlib import Path

from .opcodes import InstrSet

_COMMIT = re.compile(r"riscv-opcodes \(([0-9a-f]{7,40})\)")
_DEFINE = re.compile(r"^#define (MATCH|MASK)_(\w+) (0x[0-9a-fA-F]+)", re.M)
_DECLARE = re.compile(r"^DECLARE_INSN\((\w+), MATCH_(\w+), MASK_\w+\)", re.M)


def read(path: Path) -> tuple[str | None, dict[str, tuple[int, int]]]:
    """(riscv-opcodes commit named in the header, name -> (match, mask))."""
    text = Path(path).read_text(encoding="utf-8")
    m = _COMMIT.search(text)
    vals: dict[tuple[str, str], int] = {}
    for kind, sym, v in _DEFINE.findall(text):
        vals[(kind, sym)] = int(v, 16)
    insns = {}
    for name, sym in _DECLARE.findall(text):
        insns[name] = (vals[("MATCH", sym)], vals[("MASK", sym)])
    return (m.group(1) if m else None), insns


def compare(iset: InstrSet, path: Path) -> tuple[list[str], list[str]]:
    """(errors, notes) comparing our encodings with Spike's."""
    commit, spike = read(path)
    errors: list[str] = []
    notes: list[str] = []
    if commit is None:
        errors.append(f"{path}: no riscv-opcodes commit in the header")
    elif not iset.commit.startswith(commit):
        errors.append(f"Spike's encoding.h is from riscv-opcodes {commit}, the pin is {iset.commit[:7]}")
    for name, ins in iset.instrs.items():
        sp = spike.get(ins.spike_name)
        if sp is None:
            errors.append(f"{name}: Spike declares no '{ins.spike_name}'")
            continue
        match, mask = sp
        if ins.spike_name == name:
            if (match, mask) != (ins.match, ins.mask):
                errors.append(f"{name}: encoding differs from Spike's "
                              f"(ours {ins.match:#x}/{ins.mask:#x}, Spike {match:#x}/{mask:#x})")
        elif (ins.mask & mask) != mask or (ins.match & mask) != match:
            errors.append(f"{name}: not inside Spike's {ins.spike_name}")
        else:
            notes.append(f"{name}: executed by Spike as {ins.spike_name}")
    return errors, notes
