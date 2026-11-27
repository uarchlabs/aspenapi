# ===================================================================
# SPDX-License-Identifier: Apache-2.0
# Copyright (c) 2026 Jeff Nye, uarchlabs.com
# SPDX-FileCopyrightText: 2026 Jeff Nye <jeff@uarchlabs.com>
# ===================================================================
"""Operand-pattern catalogue (task A9).

Builds, for every instruction of the profile, the list of operand patterns
that might matter to timing, from three sources:

- a sweep: every integer register field equal to x0, every immediate equal
  to zero, every pair of source registers of the same file equal, and the
  special fields (rounding mode, vector mask, aq/rl, segment count, fence
  sets);
- riscv-opcodes pseudo-ops defined on the instruction (mv, ret, fmv.d...);
- patterns written from the ISA manual's HINT tables and the assembly
  manual's pseudoinstruction tables (isa/catalogue/patterns.yaml).

Each entry is resolved by the first matching pattern in patterns.yaml, or
automatically when its witness word decodes as a different instruction (or
as nothing). An entry no pattern resolves is 'unresolved', which the check
treats as an error, so a new riscv-opcodes commit cannot add unreviewed
patterns silently.

Conditions use the layer's `when` language, so a `variant` entry can be
copied into the layer as it stands. The layer check builds a witness word
for each such entry and evaluates it with the reference evaluator.
"""

from __future__ import annotations

import fnmatch
import json
import re
from dataclasses import dataclass, field
from pathlib import Path

import yaml

from . import decode as dec
from .evaluate import Evaluator
from .opcodes import Instr, InstrSet

FORMAT = 1
KINDS = ("variant", "rule", "hint", "reserved", "encoding", "none", "review")

_REG = re.compile(r"^([xfv])([0-9]|[12][0-9]|3[01])$")
_SPECIAL = {"rm", "vm", "aq", "rl", "aqrl", "fm", "pred", "succ", "nf", "csr", "zimm11", "zimm10", "wd"}
# Field values that select the plain form of an instruction in a witness:
# unmasked, not a segment access, plain fence, static rounding, sscratch.
_NEUTRAL = {"vm": 1, "nf": 0, "fm": 0, "aq": 0, "rl": 0, "rm": 0, "csr": 0x140, "pred": 3, "succ": 3}
_FP_FILES = {"rv_f", "rv64_f", "rv_d", "rv64_d", "rv_zfhmin", "rv_d_zfhmin", "rv_f_zfa", "rv_d_zfa"}
_FP_INT_DEST = re.compile(r"^(fmv_x_[wdh]|fclass_[sdh]|feq_[sdh]|flt_[sdh]|fle_[sdh]|fleq_[sdh]|fltq_[sdh]"
                          r"|fcvt_(w|wu|l|lu)_[sdh]|fcvtmod_w_d|fmvh_x_d)$")
_FP_INT_SRC = re.compile(r"^(fmv_[wdh]_x|fcvt_[sdh]_(w|wu|l|lu))$")


# ---------------------------------------------------------------- operand files
def register_file(ins: Instr, fld: str) -> str | None:
    """The register file of a register field, or None for a non-register field.

    A heuristic from the instruction's extension and name; layer `operands`
    declarations, where present, take precedence.
    """
    if not dec.is_register_field(fld):
        return None
    ext = set(ins.extension)
    n = ins.name
    if ext & {"rv_v", "rv_zvbb"}:
        if fld.startswith(("vd", "vs")):
            return "v"
        if fld == "rd":
            return "f" if n == "vfmv_f_s" else "x"
        if fld == "rs1":
            return "f" if n.endswith(("_vf", "_wf", "_vfm")) or n in ("vfmv_v_f", "vfmv_s_f") else "x"
        return "x"
    if ext & _FP_FILES:
        if n.startswith("fli_") and fld == "rs1":
            return None                     # the constant's index, not a register
        if n in ("flw", "fld", "flh", "fsw", "fsd", "fsh") and fld == "rs1":
            return "x"
        if fld == "rd" and _FP_INT_DEST.match(n):
            return "x"
        if fld == "rs1" and _FP_INT_SRC.match(n):
            return "x"
        return "f"
    if "rv_c_d" in ext:
        if n in ("c_fld", "c_fsd") and fld == "rs1_p":
            return "x"
        return "f"
    return "x"


def _is_dest(fld: str) -> bool:
    return fld.startswith(("rd", "vd")) and not fld.startswith("rd_rs1")


def _imm_groups(ins: Instr) -> dict[str, list[str]]:
    groups: dict[str, list[str]] = {}
    for f in ins.fields:
        if dec.is_register_field(f) or f in _SPECIAL or "=" in f:
            continue
        key = re.sub(r"(hi|lo)$", "", f)
        groups.setdefault(key, []).append(f)
    return groups


# ---------------------------------------------------------------- candidates
@dataclass
class Entry:
    when: dict
    origins: list[str] = field(default_factory=list)
    labels: list[str] = field(default_factory=list)
    kind: str = "unresolved"
    pattern: str | None = None
    tags: list[str] = field(default_factory=list)
    remove_tags: list[str] = field(default_factory=list)
    set: dict = field(default_factory=dict)
    add_implicit: list = field(default_factory=list)
    rule: str | None = None
    decodes_as: str | None = None
    note: str | None = None

    def as_dict(self) -> dict:
        d = {"when": self.when, "from": self.origins, "kind": self.kind}
        for k in ("pattern", "rule", "decodes_as"):
            if getattr(self, k):
                d[k] = getattr(self, k)
        if self.note and not self.pattern:     # pattern notes live in patterns.yaml
            d["note"] = self.note
        for k in ("tags", "remove_tags", "add_implicit"):
            if getattr(self, k):
                d[k] = getattr(self, k)
        if self.set:
            d["set"] = self.set
        return d


def sweep(ins: Instr) -> list[tuple[str, dict]]:
    """(label, when) for every swept candidate of one instruction."""
    out: list[tuple[str, dict]] = []
    regs = [f for f in ins.fields if dec.is_register_field(f)]
    for f in regs:
        # Compressed 3-bit register fields cannot name x0.
        if register_file(ins, f) == "x" and dec.register_value(f, 0) == 0:
            out.append((f"x0:{f}", {f: "x0"}))
    for key, flds in _imm_groups(ins).items():
        out.append((f"zero:{key}", {f: 0 for f in flds}))
    srcs = [f for f in regs if not _is_dest(f) and register_file(ins, f) is not None]
    for i, a in enumerate(srcs):
        for b in srcs[i + 1:]:
            if register_file(ins, a) == register_file(ins, b):
                lo, hi = sorted((a, b))
                out.append((f"eq:{lo}={hi}", {b: {"eq": a}}))
    fs = set(ins.fields)
    if "rm" in fs:
        out.append(("special:rm=dyn", {"rm": 7}))
        out.append(("special:rm=reserved", {"rm": [5, 6]}))
    if "vm" in fs:
        out.append(("special:vm=masked", {"vm": 0}))
        if "vd" in fs:
            out.append(("special:vm=masked_vd_v0", {"vm": 0, "vd": "v0"}))
    for f in ("aq", "rl"):
        if f in fs:
            out.append((f"special:{f}=1", {f: 1}))
    if "nf" in fs:
        out.append(("special:nf=segment", {"nf": {"not": 0}}))
    if {"pred", "succ"} <= fs:
        out.append(("special:pred=0", {"pred": 0}))
        out.append(("special:succ=0", {"succ": 0}))
    if "fm" in fs:
        out.append(("special:fm=tso", {"fm": 8}))
    return out


def pseudo_condition(base: Instr, pseudo: Instr, ranges: dict[str, tuple[int, int]]) -> dict | None:
    """The `when` that selects a pseudo-op inside its base instruction."""
    extra = pseudo.mask & ~base.mask
    when: dict = {}
    for f in base.fields:
        msb, lsb = ranges[f]
        fmask = ((1 << (msb - lsb + 1)) - 1) << lsb
        fixed = extra & fmask
        if not fixed:
            continue
        value = (pseudo.match & fixed) >> lsb
        if fixed == fmask:
            if dec.is_register_field(f):
                when[f] = f"x{dec.register_value(f, value)}" if not f.startswith(("vd", "vs")) else f"v{value}"
            else:
                when[f] = value
        else:
            when[f] = {"mask": fixed >> lsb, "value": value}
    for f in pseudo.fields:
        if "=" in f:
            a, b = f.split("=")
            when[a] = {"eq": b}
    return when or None


# ---------------------------------------------------------------- patterns
def load_patterns(path: Path) -> dict:
    with open(path, encoding="utf-8") as f:
        doc = yaml.safe_load(f)
    for i, p in enumerate(doc.get("patterns", [])):
        if "id" not in p or "kind" not in p:
            raise ValueError(f"{path}: pattern {i} needs id and kind")
        if p["kind"] not in KINDS:
            raise ValueError(f"{path}: pattern {p['id']}: unknown kind {p['kind']}")
        if "when" not in p and "covers" not in p:
            raise ValueError(f"{path}: pattern {p['id']}: needs when or covers")
    return doc


def _applies(p: dict, name: str) -> bool:
    if any(fnmatch.fnmatchcase(name, g) for g in p.get("except", [])):
        return False
    return any(fnmatch.fnmatchcase(name, g) for g in p.get("instructions", ["*"]))


def _norm(when: dict) -> str:
    return json.dumps(when, sort_keys=True)


def _resolve(e: Entry, p: dict) -> None:
    e.kind = p["kind"]
    e.pattern = p["id"]
    e.tags = list(p.get("tags", []))
    e.remove_tags = list(p.get("remove_tags", []))
    e.set = dict(p.get("set", {}))
    e.add_implicit = list(p.get("add_implicit", []))
    e.rule = p.get("rule")
    e.note = p.get("note")


# ---------------------------------------------------------------- witnesses
class Witness:
    """Builds an instruction word whose fields satisfy a `when`."""

    def __init__(self, iset: InstrSet, csr_classes: dict[str, list[int]]):
        self.iset = iset
        self.ev = Evaluator({"instructions": [], "csr_classes": csr_classes, "csrs": iset.csrs})

    def _pool(self, ins: Instr, f: str, cond) -> list[int]:
        width = self.iset.field_widths[f]
        pool: list[int] = []

        def add(v):
            if 0 <= v < (1 << width) and v not in pool:
                pool.append(v)

        def consts(m):
            if isinstance(m, list):
                for x in m:
                    consts(x)
            elif isinstance(m, dict):
                for k in ("not", "all"):
                    if k in m:
                        consts(m[k])
                if "mask" in m:
                    add(m["value"])
                if "csr_class" in m:
                    for a in self.ev.csr_classes.get(m["csr_class"], []):
                        add(a)
            elif isinstance(m, str):
                r = _REG.match(m)
                if r:
                    add(int(r.group(2)) - (8 if dec.register_value(f, 0) == 8 else 0))
                elif m in self.iset.csrs:
                    add(self.iset.csrs[m])
            elif isinstance(m, int):
                add(m)

        consts(cond)
        for v in (10, 11, 12, 13, 1, 2, 5, 7, 3, 4, 6, 8, 9, 0, 15, 31, 255, 4095):
            add(v)
        return pool

    def build(self, ins: Instr, when: dict, seed: int = 0) -> dict[str, int] | None:
        """A witness; seeds 0-2 vary the fields the condition leaves free."""
        defaults = {}
        n = 10 + 3 * seed
        imm = (1, 5, 16)[seed % 3]
        for f in ins.fields:
            if "=" in f:
                continue
            width = self.iset.field_widths[f]
            if dec.is_register_field(f):
                defaults[f] = n % (1 << width) if dec.register_value(f, 0) != 8 else (n - 8) % 8
                n += 1
            else:
                defaults[f] = _NEUTRAL.get(f, imm % (1 << width) if width > 1 else 0)
        values = dict(defaults)
        for _ in range(3):
            ok = True
            for f, cond in when.items():
                if f not in values:
                    return None
                if self._ok(ins, values, {f: cond}):
                    continue
                ok = False
                for v in self._pool(ins, f, cond) + [values[c["eq"]] for c in [cond] if isinstance(cond, dict) and "eq" in cond and c["eq"] in values]:
                    values[f] = v
                    if self._ok(ins, values, {f: cond}):
                        break
            if ok:
                return values
        return values if self._ok(ins, values, when) else None

    def _ok(self, ins: Instr, values: dict[str, int], when: dict) -> bool:
        arch = {f: dec.register_value(f, v) if dec.is_register_field(f) else v for f, v in values.items()}
        try:
            return self.ev._when(arch, when)
        except KeyError:
            return False


# ---------------------------------------------------------------- build
def build(iset: InstrSet, patterns: dict, layer: dict) -> dict:
    csr_classes = {c: sorted(iset.csrs[n] for n in d["csrs"])
                   for c, d in layer.get("csr_classes", {}).items()}
    ov = dec.analyse(iset)
    decoder = dec.Decoder(iset, ov.order)
    wit = Witness(iset, csr_classes)
    pats = patterns.get("patterns", [])
    out: dict[str, list[dict]] = {}
    counts = {k: 0 for k in KINDS + ("unresolved",)}

    for name, ins in iset.instrs.items():
        if name in ov.shadowed:
            continue
        entries: dict[str, Entry] = {}

        def add(when: dict, origin: str, label: str | None = None):
            key = _norm(when)
            e = entries.setdefault(key, Entry(when))
            if origin not in e.origins:
                e.origins.append(origin)
            if label and label not in e.labels:
                e.labels.append(label)

        for label, when in sweep(ins):
            add(when, "sweep", label)
        for pname, p in iset.pseudo_patterns.items():
            if p.pseudo_of == name:
                cond = pseudo_condition(ins, p, iset.field_ranges)
                if cond:
                    add(cond, f"riscv-opcodes:{pname}")
        for p in pats:
            if "when" in p and _applies(p, name) and set(p["when"]) <= set(ins.fields):
                for src in p.get("source") or [f"pattern:{p['id']}"]:
                    add(p["when"], src)

        for e in entries.values():
            if e.when:
                # Decided by decode only if no witness decodes as this instruction.
                hits = []
                for seed in range(3):
                    raw = wit.build(ins, e.when, seed)
                    if raw is not None:
                        h = decoder.decode(dec.encode(ins, raw, iset.field_ranges))
                        hits.append(h[0] if h else None)
                if hits and name not in hits:
                    if all(h is None for h in hits):
                        e.kind, e.note = "reserved", "no instruction decodes this encoding"
                    else:
                        e.kind, e.decodes_as = "encoding", next(h for h in hits if h)
            if e.kind == "unresolved":
                for p in pats:
                    if not _applies(p, name):
                        continue
                    if "when" in p and _norm(p["when"]) == _norm(e.when):
                        _resolve(e, p)
                        break
                    tags = e.labels + e.origins
                    if any(fnmatch.fnmatchcase(t, c) for t in tags for c in p.get("covers", [])):
                        _resolve(e, p)
                        break
            counts[e.kind] += 1
        out[name] = [e.as_dict() for e in sorted(entries.values(), key=lambda x: _norm(x.when))]
    return {"format": FORMAT, "profile": iset.profile, "riscv_opcodes_commit": iset.commit,
            "sources": patterns.get("sources", {}), "summary": counts, "instructions": out}


def dump(cat: dict, path: Path) -> None:
    """Write the catalogue: a short header, then one line per entry."""
    def flow(x) -> str:
        text = yaml.safe_dump(x, default_flow_style=True, sort_keys=False, width=10_000).strip()
        return text[:-3].rstrip() if text.endswith("\n...") else text

    lines = [
        "# Operand-pattern catalogue, generated by: python3 -m aspen_isa catalogue --write",
        "# Do not edit by hand: change isa/catalogue/patterns.yaml and regenerate.",
        "# Each entry: the condition, where it came from, and how patterns.yaml resolves it.",
        f"format: {cat['format']}",
        f"profile: {cat['profile']}",
        f"riscv_opcodes_commit: {cat['riscv_opcodes_commit']}",
        "sources:",
    ]
    lines += [f"  {k}: {flow(v)}" for k, v in cat["sources"].items()]
    lines.append(f"summary: {flow(cat['summary'])}")
    lines.append("instructions:")
    for name, entries in cat["instructions"].items():
        lines.append(f"  {name}:" + ("" if entries else " []"))
        lines += [f"  - {flow(e)}" for e in entries]
    Path(path).write_text("\n".join(lines) + "\n", encoding="utf-8")


def unresolved(cat: dict) -> list[str]:
    return [f"{n}: {json.dumps(e['when'])} from {', '.join(e['from'])}"
            for n, es in cat["instructions"].items() for e in es if e["kind"] == "unresolved"]


# ---------------------------------------------------------------- layer check
def check_layer(cat: dict, iset: InstrSet, db: dict) -> tuple[list[str], int]:
    """Mismatches between `variant`/`rule` entries and what the layer evaluates to.

    Returns (problems, entries checked). Only instructions with layer data
    are checked; the rest wait for A4 and A7.
    """
    ev = Evaluator(db)
    wit = Witness(iset, db.get("csr_classes", {}))
    problems: list[str] = []
    checked = 0
    for name, entries in cat["instructions"].items():
        rec = ev.by_name.get(name)
        if rec is None or rec["layer"] == "missing":
            continue
        ins = iset.instrs[name]
        for e in entries:
            if e["kind"] not in ("variant", "rule") or not e["when"]:
                continue
            raw = wit.build(ins, e["when"])
            if raw is None:
                problems.append(f"{name}: no witness for {json.dumps(e['when'])}")
                continue
            res = ev.evaluate(name, raw)
            checked += 1
            missing = set(e.get("tags", [])) - res.tags
            kept = set(e.get("remove_tags", [])) & res.tags
            wrong = {k: v for k, v in e.get("set", {}).items() if res.attributes.get(k) != v}
            if missing or kept or wrong:
                what = []
                if missing:
                    what.append(f"missing tags {sorted(missing)}")
                if kept:
                    what.append(f"tags not removed {sorted(kept)}")
                if wrong:
                    what.append(f"attributes {wrong}")
                problems.append(f"{name} {json.dumps(e['when'])} ({e.get('pattern')}): {'; '.join(what)}")
    return problems, checked


def undeclared_tags(cat: dict, layer: dict) -> list[str]:
    """Tags proposed by the catalogue that the layer does not declare yet."""
    declared = set(layer.get("tags", {}))
    used = {t for es in cat["instructions"].values() for e in es for t in e.get("tags", []) + e.get("remove_tags", [])}
    return sorted(used - declared)
