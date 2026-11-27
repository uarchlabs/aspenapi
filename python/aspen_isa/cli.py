# ===================================================================
# SPDX-License-Identifier: Apache-2.0
# Copyright (c) 2026 Jeff Nye, uarchlabs.com
# SPDX-FileCopyrightText: 2026 Jeff Nye <jeff@uarchlabs.com>
# ===================================================================
"""Command line: python3 -m aspen_isa {validate,ids,list,merge,decode,profile,spike,catalogue} ..."""

from __future__ import annotations

import argparse
import json
import sys
from pathlib import Path

import yaml

from . import catalogue as cat
from . import decode as dec
from . import evaluate as ev
from . import ids as idreg
from . import layer as lay
from . import merge as mrg
from . import opcodes
from . import profile as prf
from . import spike as spk

ROOT = Path(__file__).resolve().parents[2]
DEFAULTS = {
    "profile": ROOT / "isa" / "profiles" / "rva23s64.yaml",
    "layer": ROOT / "isa" / "layer" / "rva23s64.yaml",
    "ids": ROOT / "isa" / "ids.yaml",
    "opcodes": ROOT / "ext" / "riscv-opcodes",
    "patterns": ROOT / "isa" / "catalogue" / "patterns.yaml",
    "catalogue": ROOT / "isa" / "catalogue" / "rva23s64.yaml",
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
    prof, iset = _iset(args)
    errors: list[str] = list(prf.check(prof, args.opcodes))
    warnings: list[str] = []
    ov = dec.analyse(iset)
    errors += [f"encodings {a} and {b}: {why}" for a, b, why in ov.conflicts]
    for path in args.layer or [DEFAULTS["layer"]]:
        doc = lay.load_yaml(path)
        e, w = lay.validate(doc, iset)
        w += [f"instructions/{n}: no instruction word decodes to it (fully covered by more specific entries)"
              for n in ov.shadowed if n in (doc.get("instructions") or {})]
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


def _db(args) -> dict:
    prof, iset = _iset(args)
    layer = lay.load_yaml(args.layer_file)
    errors, _ = lay.validate(layer, iset)
    if errors:
        raise ValueError(f"{args.layer_file.name} does not validate; run: python3 -m aspen_isa validate")
    return mrg.build(layer, iset, idreg.load(args.ids), prof, args.opcodes)


def cmd_merge(args) -> int:
    db = _db(args)
    text = json.dumps(db, indent=1, sort_keys=False) + "\n"
    if args.output:
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_text(text, encoding="utf-8")
    else:
        sys.stdout.write(text)
    cov = ", ".join(f"{g} {c['entry']}+{c['inherited']} of {sum(c.values())}" for g, c in db["coverage"].items())
    print(f"merged {len(db['instructions'])} instructions; layer coverage: {cov}", file=sys.stderr)
    return 0


def cmd_decode(args) -> int:
    prof, iset = _iset(args)
    db = _db(args)
    d = dec.Decoder(iset, db["decode_order"])
    e = ev.Evaluator(db)
    rc = 0
    for w in args.words:
        word = int(w, 16)
        hit = d.decode(word)
        if hit is None:
            print(f"{word:#010x}: no instruction")
            rc = 1
            continue
        name, raw = hit
        fields = " ".join(f"{k}={v}" for k, v in raw.items())
        line = f"{word:#010x}: {name} {fields}".rstrip()
        if e.by_name[name]["layer"] != "missing":
            r = e.evaluate(name, raw)
            line += f"  tags=[{', '.join(sorted(r.tags))}]"
            changed = {k: v for k, v in r.attributes.items()
                       if db["attributes"].get(k, {}).get("default") != v}
            if changed:
                line += f" attributes={changed}"
            if r.implicit:
                line += f" implicit={r.implicit}"
        else:
            line += "  (no layer entry yet)"
        print(line)
    return rc


def cmd_profile(args) -> int:
    prof = opcodes.load_profile(args.profile)
    errors = prf.check(prof, args.opcodes)
    if args.spec:
        missing, extra = prf.compare_with_spec(prof, args.spec)
        errors += [f"profile: {n} is mandatory in {prof['name']} but not listed" for n in missing]
        errors += [f"profile: {n} is listed but not mandatory in {prof['name']}" for n in extra]
    rc = _report(errors, [])
    if rc == 0:
        print(f"ok: {len(prof['extensions'])} extensions, {len(opcodes.profile_files(prof))} riscv-opcodes files"
              + (", matches the profile text" if args.spec else ""))
    return rc


def cmd_spike(args) -> int:
    _, iset = _iset(args)
    errors, notes = spk.compare(iset, args.encoding_h)
    for n in notes:
        print(f"note: {n}")
    rc = _report(errors, [])
    if rc == 0:
        print(f"ok: all {len(iset.instrs)} encodings match {args.encoding_h}")
    return rc


def cmd_catalogue(args) -> int:
    prof, iset = _iset(args)
    layer = lay.load_yaml(args.layer_file)
    built = cat.build(iset, cat.load_patterns(args.patterns), layer)
    if args.show:
        for name in args.show:
            for e in built["instructions"].get(name, []):
                print(f"{name}: {json.dumps(e)}")
        return 0
    if args.write:
        cat.dump(built, args.catalogue)
        print(f"wrote {args.catalogue}")
    errors: list[str] = [f"unresolved: {u}" for u in cat.unresolved(built)]
    warnings: list[str] = []
    if args.check and not args.write:
        if not args.catalogue.exists():
            errors.append(f"{args.catalogue} is missing; run: python3 -m aspen_isa catalogue --write")
        elif yaml.safe_load(args.catalogue.read_text(encoding="utf-8")) != json.loads(json.dumps(built)):
            errors.append(f"{args.catalogue.name} is out of date; run: python3 -m aspen_isa catalogue --write")
    db = mrg.build(layer, iset, idreg.load(args.ids), prof, args.opcodes)
    problems, checked = cat.check_layer(built, iset, db)
    (errors if args.strict_layer else warnings).extend(f"layer: {p}" for p in problems)
    new_tags = cat.undeclared_tags(built, layer)
    rc = _report(errors, warnings, args.quiet)
    summary = ", ".join(f"{k} {v}" for k, v in built["summary"].items() if v)
    print(f"catalogue: {summary}")
    print(f"layer check: {checked} entries checked against layer data, {len(problems)} mismatches")
    if new_tags:
        print(f"tags proposed but not yet in the layer: {', '.join(new_tags)}")
    return rc


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

    m = sub.add_parser("merge", help="write the merged instruction database (JSON)")
    _common(m)
    m.add_argument("--layer-file", type=Path, default=DEFAULTS["layer"])
    m.add_argument("-o", "--output", type=Path, help="output file (default: stdout)")
    m.set_defaults(fn=cmd_merge)

    d = sub.add_parser("decode", help="decode instruction words (hex) and evaluate their variants")
    _common(d)
    d.add_argument("--layer-file", type=Path, default=DEFAULTS["layer"])
    d.add_argument("words", nargs="+")
    d.set_defaults(fn=cmd_decode)

    pr = sub.add_parser("profile", help="check the profile file, optionally against the profile text")
    _common(pr)
    pr.add_argument("--spec", type=Path, help="riscv-profiles src/rva23-profile.adoc")
    pr.set_defaults(fn=cmd_profile)

    sp = sub.add_parser("spike", help="compare encodings with Spike's riscv/encoding.h")
    _common(sp)
    sp.add_argument("encoding_h", type=Path)
    sp.set_defaults(fn=cmd_spike)

    c = sub.add_parser("catalogue", help="build or check the operand-pattern catalogue (A9)")
    _common(c)
    c.add_argument("--layer-file", type=Path, default=DEFAULTS["layer"])
    c.add_argument("--patterns", type=Path, default=DEFAULTS["patterns"])
    c.add_argument("--catalogue", type=Path, default=DEFAULTS["catalogue"])
    c.add_argument("--write", action="store_true", help="regenerate the catalogue file")
    c.add_argument("--check", action="store_true", help="fail if the catalogue file is out of date")
    c.add_argument("--strict-layer", action="store_true", help="treat layer mismatches as errors")
    c.add_argument("--show", nargs="+", metavar="NAME", help="print the entries for these instructions")
    c.add_argument("-q", "--quiet", action="store_true", help="hide warnings")
    c.set_defaults(fn=cmd_catalogue)

    args = ap.parse_args(argv)
    try:
        return args.fn(args)
    except (opcodes.OpcodesError, OSError, ValueError, yaml.YAMLError) as e:
        print(f"error: {e}", file=sys.stderr)
        return 2
