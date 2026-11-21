# ===================================================================
# SPDX-License-Identifier: Apache-2.0
# Copyright (c) 2026 Jeff Nye, uarchlabs.com
# SPDX-FileCopyrightText: 2026 Jeff Nye <jeff@uarchlabs.com>
# ===================================================================
"""The append-only instruction ID registry (isa/ids.yaml).

One ID per riscv-opcodes entry of the profile. IDs never change and are
never reused; an instruction that leaves the profile keeps its ID. ID 0 is
reserved for "unknown".
"""

from __future__ import annotations

from pathlib import Path

import yaml

from .opcodes import InstrSet

HEADER = """\
# Instruction ID registry. Append-only: never edit or remove an entry.
# New entries are added by: python3 -m aspen_isa ids --assign
# ID 0 means unknown. Names are riscv-opcodes instr_dict keys.
"""


def load(path: Path) -> dict[str, int]:
    with open(path, encoding="utf-8") as f:
        doc = yaml.safe_load(f) or {}
    if doc.get("format") != 1:
        raise ValueError(f"{path}: expected format: 1")
    return dict(doc.get("ids") or {})


def dump(ids: dict[str, int], path: Path) -> None:
    lines = [HEADER, "format: 1", "ids:"]
    for name, num in sorted(ids.items(), key=lambda kv: kv[1]):
        lines.append(f"  {name}: {num}")
    Path(path).write_text("\n".join(lines) + "\n", encoding="utf-8")


def check(ids: dict[str, int], iset: InstrSet, base: dict[str, int] | None = None) -> tuple[list[str], list[str]]:
    errors: list[str] = []
    warnings: list[str] = []
    seen: dict[int, str] = {}
    for name, num in ids.items():
        if not isinstance(num, int) or isinstance(num, bool) or num < 1:
            errors.append(f"ids/{name}: ID must be a positive integer, got {num!r}")
            continue
        if num in seen:
            errors.append(f"ids/{name}: ID {num} already belongs to '{seen[num]}'")
        seen[num] = name
    for name in iset.instrs:
        if name not in ids:
            errors.append(f"ids: '{name}' has no ID (run: python3 -m aspen_isa ids --assign)")
    for name in ids:
        if name not in iset.instrs:
            warnings.append(f"ids/{name}: not in profile {iset.profile} (kept; IDs are never reused)")
    if base is not None:
        for name, num in base.items():
            if name not in ids:
                errors.append(f"ids/{name}: removed; the registry is append-only (was {num})")
            elif ids[name] != num:
                errors.append(f"ids/{name}: changed from {num} to {ids[name]}; IDs never change")
    return errors, warnings


def assign(ids: dict[str, int], iset: InstrSet, order: list[str] | None = None) -> list[str]:
    """Give every profile instruction without an ID the next free number.

    New names are numbered in profile file order, then by name, so a fresh
    registry groups extensions together. Returns the names added.
    """
    order = order or []
    rank = {f: i for i, f in enumerate(order)}

    def key(name: str):
        ext = iset.instrs[name].extension
        return (min((rank.get(e, len(rank)) for e in ext), default=len(rank)), name)

    new = sorted((n for n in iset.instrs if n not in ids), key=key)
    nxt = max(ids.values(), default=0) + 1
    for n in new:
        ids[n] = nxt
        nxt += 1
    return new
