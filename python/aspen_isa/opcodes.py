# ===================================================================
# SPDX-License-Identifier: Apache-2.0
# Copyright (c) 2026 Jeff Nye, uarchlabs.com
# SPDX-FileCopyrightText: 2026 Jeff Nye <jeff@uarchlabs.com>
# ===================================================================
"""Load the pinned riscv-opcodes checkout and a profile's instruction set."""

from __future__ import annotations

import csv
import importlib
import logging
import re
import subprocess
import sys
from dataclasses import dataclass, field
from pathlib import Path

import yaml

# Register-valued riscv-opcodes fields (rd, rs1_p, rd_n0, c_rs2_n0, vd, vs3, ...).
REGISTER_FIELD = re.compile(r"^(rd|rs[123]|rt|vd|vs[123]|c_rs[12]|c_sreg[12])(_[a-z0-9]+)*$")


class OpcodesError(Exception):
    pass


@dataclass
class Instr:
    name: str
    fields: list[str]
    extension: list[str]
    match: int
    mask: int
    pseudo: bool = False

    @property
    def length(self) -> int:
        return 16 if (self.match & 3) != 3 else 32


@dataclass
class InstrSet:
    profile: str
    commit: str
    instrs: dict[str, Instr]
    pseudo_patterns: dict[str, Instr] = field(default_factory=dict)
    field_widths: dict[str, int] = field(default_factory=dict)
    csrs: dict[str, int] = field(default_factory=dict)


def load_profile(path: Path) -> dict:
    with open(path, encoding="utf-8") as f:
        prof = yaml.safe_load(f)
    for key in ("format", "name", "riscv_opcodes_commit", "extension_files"):
        if key not in prof:
            raise OpcodesError(f"{path}: missing '{key}'")
    return prof


def profile_files(prof: dict) -> list[str]:
    groups = prof["extension_files"]
    files: list[str] = []
    for group in groups.values():
        files.extend(group)
    return files


def checkout_commit(repo: Path) -> str:
    try:
        out = subprocess.run(
            ["git", "-C", str(repo), "rev-parse", "HEAD"],
            check=True, capture_output=True, text=True,
        )
    except (OSError, subprocess.CalledProcessError) as e:
        raise OpcodesError(f"cannot read the riscv-opcodes commit in {repo}: {e}") from e
    return out.stdout.strip()


def _import_riscv_opcodes(repo: Path):
    src = str((repo / "src").resolve())
    if src not in sys.path:
        sys.path.insert(0, src)
    logging.getLogger().setLevel(logging.WARNING)
    try:
        shared = importlib.import_module("riscv_opcodes.shared_utils")
        consts = importlib.import_module("riscv_opcodes.constants")
    except ImportError as e:
        raise OpcodesError(f"cannot import riscv_opcodes from {src}: {e}") from e
    return shared, consts


def load(repo: Path, prof: dict, check_commit: bool = True) -> InstrSet:
    """Build the profile's instruction set from riscv-opcodes."""
    repo = Path(repo)
    if not (repo / "extensions").is_dir():
        raise OpcodesError(f"{repo} is not a riscv-opcodes checkout (run git submodule update --init)")
    commit = checkout_commit(repo)
    pin = prof["riscv_opcodes_commit"]
    if check_commit and commit != pin:
        raise OpcodesError(f"riscv-opcodes is at {commit}, profile pins {pin}")

    files = profile_files(prof)
    missing = [f for f in files if not (repo / "extensions" / f).is_file()]
    if missing:
        raise OpcodesError(f"profile names extension files not in riscv-opcodes: {', '.join(missing)}")

    shared, consts = _import_riscv_opcodes(repo)
    base = shared.create_inst_dict(files, include_pseudo=False)
    with_pseudo = shared.create_inst_dict(files, include_pseudo=True)
    emitted = set(consts.emitted_pseudo_ops)

    def mk(name: str, d: dict, pseudo: bool) -> Instr:
        return Instr(name, list(d["variable_fields"]), list(d["extension"]),
                     int(d["match"], 16), int(d["mask"], 16), pseudo)

    instrs = {n: mk(n, d, False) for n, d in base.items()}
    patterns: dict[str, Instr] = {}
    for n, d in with_pseudo.items():
        if n in base:
            continue
        if n in emitted:
            instrs[n] = mk(n, d, True)
        else:
            patterns[n] = mk(n, d, True)

    widths: dict[str, int] = {}
    with open(repo / "arg_lut.csv", encoding="utf-8") as f:
        for row in csv.reader(f, skipinitialspace=True):
            if len(row) == 3:
                widths[row[0]] = int(row[1]) - int(row[2]) + 1

    csrs: dict[str, int] = {}
    with open(repo / "csrs.csv", encoding="utf-8") as f:
        for row in csv.reader(f, skipinitialspace=True):
            if len(row) == 2:
                csrs[row[1]] = int(row[0], 16)

    return InstrSet(prof["name"], commit, dict(sorted(instrs.items())),
                    dict(sorted(patterns.items())), widths, csrs)
