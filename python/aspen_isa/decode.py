# ===================================================================
# SPDX-License-Identifier: Apache-2.0
# Copyright (c) 2026 Jeff Nye, uarchlabs.com
# SPDX-FileCopyrightText: 2026 Jeff Nye <jeff@uarchlabs.com>
# ===================================================================
"""Overlapping encodings, decode order, and field extraction.

Two entries overlap when some instruction word matches both. Overlaps are
resolved by specificity: if one entry's fixed bits are a strict superset of
the other's, the more specific entry wins inside the other's space
(c_nop inside c_addi, pause inside fence). Any other overlap is an error.

Field values here are raw, as encoded. Compressed register fields (rd_p and
friends) are 3-bit and map to x8-x15; register_value() does that mapping.
"""

from __future__ import annotations

import re
from dataclasses import dataclass
from itertools import combinations

from .opcodes import REGISTER_FIELD, Instr, InstrSet

# Compressed 3-bit register fields name x8-x15.
_PRIME = re.compile(r"(^|_)(rd|rs1|rs2|rd_rs1)_p$")
# Enumerate an entry's space only up to this many free bits when checking shadowing.
_MAX_ENUM_BITS = 20


def popcount(x: int) -> int:
    return bin(x).count("1")


def overlaps(a: Instr, b: Instr) -> bool:
    return ((a.match ^ b.match) & a.mask & b.mask) == 0


def more_specific(a: Instr, b: Instr) -> bool:
    """True if a's fixed bits strictly include b's (a wins where they overlap)."""
    return a.mask != b.mask and (a.mask & b.mask) == b.mask


@dataclass
class Overlaps:
    nested: list[tuple[str, str]]      # (general, specific)
    conflicts: list[tuple[str, str, str]]  # (a, b, why)
    shadowed: list[str]                # entries no word decodes to
    order: list[str]                   # decode order: most specific first


def analyse(iset: InstrSet) -> Overlaps:
    ins = list(iset.instrs.values())
    nested: list[tuple[str, str]] = []
    conflicts: list[tuple[str, str, str]] = []
    children: dict[str, list[Instr]] = {}
    for a, b in combinations(ins, 2):
        if not overlaps(a, b):
            continue
        if more_specific(a, b):
            nested.append((b.name, a.name))
            children.setdefault(b.name, []).append(a)
        elif more_specific(b, a):
            nested.append((a.name, b.name))
            children.setdefault(a.name, []).append(b)
        elif a.mask == b.mask:
            conflicts.append((a.name, b.name, "identical encodings"))
        else:
            conflicts.append((a.name, b.name, "partial overlap: neither is more specific"))
    shadowed = sorted(n for n, kids in children.items() if _covered(iset.instrs[n], kids))
    # A strict superset of fixed bits means strictly more of them, so sorting
    # by fixed-bit count puts every specific entry before its general one.
    order = sorted(iset.instrs, key=lambda n: (-popcount(iset.instrs[n].mask), n))
    return Overlaps(sorted(nested), conflicts, shadowed, order)


def _covered(general: Instr, kids: list[Instr]) -> bool:
    """True if every word in general's space also matches one of kids."""
    width = 16 if general.length == 16 else 32
    free = [b for b in range(width) if not (general.mask >> b) & 1]
    if len(free) > _MAX_ENUM_BITS:
        # Too large to enumerate; count instead, which is exact when the
        # most general children are pairwise disjoint.
        top = [k for k in kids if not any(more_specific(k, o) for o in kids if o is not k)]
        if any(overlaps(x, y) for x, y in combinations(top, 2)):
            return False
        return sum(1 << (width - popcount(k.mask)) for k in top) == 1 << len(free)
    for i in range(1 << len(free)):
        word = general.match
        for j, bit in enumerate(free):
            if (i >> j) & 1:
                word |= 1 << bit
        if not any((word & k.mask) == k.match for k in kids):
            return False
    return True


# ---------------------------------------------------------------- fields
def extract(word: int, instr: Instr, ranges: dict[str, tuple[int, int]]) -> dict[str, int]:
    out = {}
    for f in instr.fields:
        msb, lsb = ranges[f]
        out[f] = (word >> lsb) & ((1 << (msb - lsb + 1)) - 1)
    return out


def encode(instr: Instr, values: dict[str, int], ranges: dict[str, tuple[int, int]]) -> int:
    """Build a word from raw field values; fields not given are 0."""
    word = instr.match
    for f in instr.fields:
        msb, lsb = ranges[f]
        width = msb - lsb + 1
        v = values.get(f, 0)
        if v >> width:
            raise ValueError(f"{instr.name}: {v} does not fit the {width}-bit field {f}")
        word |= v << lsb
    if word & instr.mask != instr.match:
        raise ValueError(f"{instr.name}: field values overlap fixed bits")
    return word


def register_value(field: str, raw: int) -> int:
    """Architectural register number for a raw register-field value."""
    return raw + 8 if _PRIME.search(field) else raw


def is_register_field(field: str) -> bool:
    return bool(REGISTER_FIELD.match(field))


class Decoder:
    """Reference decoder: the most specific matching entry wins."""

    def __init__(self, iset: InstrSet, order: list[str] | None = None):
        self.iset = iset
        self.ranges = iset.field_ranges
        order = order or analyse(iset).order
        self.order = [iset.instrs[n] for n in order]

    def decode(self, word: int) -> tuple[str, dict[str, int]] | None:
        if (word & 3) != 3:
            word &= 0xFFFF
        for ins in self.order:
            if (word & ins.mask) == ins.match:
                return ins.name, extract(word, ins, self.ranges)
        return None
