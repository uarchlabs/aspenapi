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
    # For riscv-opcodes pseudo-ops: the base instruction they are defined on.
    pseudo_of: str | None = None
    # Spike's DECLARE_INSN name for this encoding. Differs from name only for
    # pseudo-ops that riscv-opcodes promotes because their base is not in the
    # profile (zext_h is packw with rs2 = 0; Spike executes it as packw).
    spike_name: str | None = None

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
    field_ranges: dict[str, tuple[int, int]] = field(default_factory=dict)
    csrs: dict[str, int] = field(default_factory=dict)


def load_profile(path: Path) -> dict:
    with open(path, encoding="utf-8") as f:
        prof = yaml.safe_load(f)
    for key in ("format", "name", "riscv_opcodes_commit", "extensions"):
        if key not in prof:
            raise OpcodesError(f"{path}: missing '{key}'")
    return prof


def profile_files(prof: dict, group: str | None = None) -> list[str]:
    """The profile's riscv-opcodes files, in order, optionally for one group."""
    files: list[str] = []
    for ext in prof["extensions"]:
        if group is None or ext.get("group") == group:
            files.extend(f for f in ext.get("files", []) if f not in files)
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


_PSEUDO_LINE = re.compile(r"^\$pseudo_op\s+\S+::(\S+)\s+(\S+)")


def _pseudo_bases(repo: Path, files: list[str]) -> dict[str, str]:
    """Map each pseudo-op name in the profile's files to the base instruction it is defined on.

    A name defined as a real instruction in one of the files (c.ld in rv64_c,
    a pseudo-op of c.flw in RV32 files) is not a pseudo-op for this profile.
    """
    pseudo: dict[str, str] = {}
    real: set[str] = set()
    for name in files:
        for line in (repo / "extensions" / name).read_text(encoding="utf-8").splitlines():
            line = line.strip()
            m = _PSEUDO_LINE.match(line)
            if m:
                pseudo.setdefault(m.group(2).replace(".", "_"), m.group(1).replace(".", "_"))
            elif line and not line.startswith(("#", "$")):
                real.add(line.split()[0].replace(".", "_"))
    return {k: v for k, v in pseudo.items() if k not in real}


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

    pseudo_of = _pseudo_bases(repo, files)
    shared, consts = _import_riscv_opcodes(repo)
    base = shared.create_inst_dict(files, include_pseudo=False)
    with_pseudo = shared.create_inst_dict(files, include_pseudo=True)
    emitted = set(consts.emitted_pseudo_ops)

    def mk(name: str, d: dict, pseudo: bool) -> Instr:
        base_of = pseudo_of.get(name)
        promoted = base_of is not None and name not in emitted and not pseudo
        return Instr(name, list(d["variable_fields"]), list(d["extension"]),
                     int(d["match"], 16), int(d["mask"], 16),
                     pseudo or promoted, base_of,
                     base_of if promoted else name)

    instrs = {n: mk(n, d, False) for n, d in base.items()}
    patterns: dict[str, Instr] = {}
    for n, d in with_pseudo.items():
        if n in base:
            continue
        if n in emitted:
            instrs[n] = mk(n, d, True)
        else:
            patterns[n] = mk(n, d, True)

    # riscv-opcodes' own field table: arg_lut.csv plus the fields it adds in code.
    ranges = {k: (int(v[0]), int(v[1])) for k, v in consts.arg_lut.items()}
    widths = {k: msb - lsb + 1 for k, (msb, lsb) in ranges.items()}

    csrs: dict[str, int] = {}
    with open(repo / "csrs.csv", encoding="utf-8") as f:
        for row in csv.reader(f, skipinitialspace=True):
            if len(row) == 2:
                csrs[row[1]] = int(row[0], 16)

    missing = sorted({f for i in instrs.values() for f in i.fields if f not in ranges})
    if missing:
        raise OpcodesError(f"fields without a bit range in riscv-opcodes: {', '.join(missing)}")
    return InstrSet(prof["name"], commit, dict(sorted(instrs.items())),
                    dict(sorted(patterns.items())), widths, ranges, csrs)
