# ===================================================================
# SPDX-License-Identifier: Apache-2.0
# Copyright (c) 2026 Jeff Nye, uarchlabs.com
# SPDX-FileCopyrightText: 2026 Jeff Nye <jeff@uarchlabs.com>
# ===================================================================
"""Command line: python3 -m aspen_isa {validate,ids,list} ..."""

from __future__ import annotations

import argparse
import sys
from pathlib import Path

import yaml

from . import ids as idreg
from . import layer as lay
from . import opcodes

ROOT = Path(__file__).resolve().parents[2]
DEFAULTS = {
    "profile": ROOT / "isa" / "profiles" / "rva23s64.yaml",
    "layer": ROOT / "isa" / "layer" / "rva23s64.yaml",
    "ids": ROOT / "isa" / "ids.yaml",
    "opcodes": ROOT / "ext" / "riscv-opcodes",
}


def _common(p: argparse.ArgumentParser) -> None:
    p.add_argument("--profile", type=Path, default=DEFAULTS["profile"])
    p.add_argument("--opcodes", type=Path, default=DEFAULTS["opcodes"], help="riscv-opcodes checkout")
    p.add_argument("--ids", type=Path, default=DEFAULTS["ids"])
    p.add_argument("--no-pin-check", action="store_true", help="accept a riscv-opcodes commit other than the pin")


def _iset(args):
    prof = opcodes.load_profile(args.profile)
    return prof, opcodes.load(args.opcodes, prof, check_commit=not args.no_pin_check)


def _report(errors: list[str], warnings: list[str], quiet_warnings: bool = False) -> int:
    for w in [] if quiet_warnings else warnings:
        print(f"warning: {w}")
    for e in errors:
        print(f"error: {e}")
    return 1 if errors else 0


def cmd_validate(args) -> int:
    _, iset = _iset(args)
    errors: list[str] = []
    warnings: list[str] = []
    for path in args.layer or [DEFAULTS["layer"]]:
        e, w = lay.validate(lay.load_yaml(path), iset)
        errors += [f"{path.name}: {x}" for x in e]
        warnings += [f"{path.name}: {x}" for x in w]
    base = idreg.load(args.ids_base) if args.ids_base and args.ids_base.exists() else None
    e, w = idreg.check(idreg.load(args.ids), iset, base)
    errors += e
    warnings += w
    rc = _report(errors, warnings, args.quiet)
    if rc == 0:
        print(f"ok: {iset.profile}, {len(iset.instrs)} instructions, riscv-opcodes {iset.commit[:7]}")
    return rc


def cmd_ids(args) -> int:
    prof, iset = _iset(args)
    reg = idreg.load(args.ids) if args.ids.exists() else {}
    if args.assign:
        added = idreg.assign(reg, iset, opcodes.profile_files(prof))
        idreg.dump(reg, args.ids)
        print(f"{len(added)} IDs added, {len(reg)} in registry")
        return 0
    base = idreg.load(args.ids_base) if args.ids_base and args.ids_base.exists() else None
    return _report(*idreg.check(reg, iset, base))


def cmd_list(args) -> int:
    _, iset = _iset(args)
    table = iset.pseudo_patterns if args.patterns else iset.instrs
    for name, ins in table.items():
        print(f"{name:24} {' '.join(ins.fields):40} {','.join(ins.extension)}")
    return 0


def main(argv=None) -> int:
    ap = argparse.ArgumentParser(prog="aspen_isa", description="AspenADL ISA layer tools")
    sub = ap.add_subparsers(dest="cmd", required=True)

    v = sub.add_parser("validate", help="validate layer files and the ID registry")
    _common(v)
    v.add_argument("layer", nargs="*", type=Path)
    v.add_argument("--ids-base", type=Path, help="earlier ids.yaml to check append-only against")
    v.add_argument("-q", "--quiet", action="store_true", help="hide warnings")
    v.set_defaults(fn=cmd_validate)

    i = sub.add_parser("ids", help="check or extend the ID registry")
    _common(i)
    i.add_argument("--assign", action="store_true", help="add IDs for profile instructions that lack one")
    i.add_argument("--ids-base", type=Path)
    i.set_defaults(fn=cmd_ids)

    ls = sub.add_parser("list", help="list the profile's instructions and fields")
    _common(ls)
    ls.add_argument("--patterns", action="store_true", help="list pseudo-op operand patterns instead")
    ls.set_defaults(fn=cmd_list)

    args = ap.parse_args(argv)
    try:
        return args.fn(args)
    except (opcodes.OpcodesError, OSError, ValueError, yaml.YAMLError) as e:
        print(f"error: {e}", file=sys.stderr)
        return 2
