# ===================================================================
# SPDX-License-Identifier: Apache-2.0
# Copyright (c) 2026 Jeff Nye, uarchlabs.com
# SPDX-FileCopyrightText: 2026 Jeff Nye <jeff@uarchlabs.com>
# ===================================================================
"""Merge riscv-opcodes, the ID registry and the layer into one database.

The database is the single input for the generators (A5, A6) and for the
reference evaluator. It is plain JSON so other tools can read it too.
"""

from __future__ import annotations

import copy
from pathlib import Path

from . import decode
from .opcodes import InstrSet, profile_files

FORMAT = 1


def _group_of(prof: dict) -> dict[str, str]:
    out = {}
    for ext in prof["extensions"]:
        for f in ext.get("files", []):
            out[f] = ext.get("group", "")
    return out


def _defaults(layer: dict) -> dict:
    return {k: d["default"] for k, d in layer.get("attributes", {}).items() if "default" in d}


def build(layer: dict, iset: InstrSet, ids: dict[str, int], prof: dict, repo: Path) -> dict:
    ranges = iset.field_ranges
    ov = decode.analyse(iset)
    groups = _group_of(prof)
    entries = layer.get("instructions", {})
    defaults = _defaults(layer)
    inside: dict[str, list[str]] = {}
    for general, specific in ov.nested:
        inside.setdefault(specific, []).append(general)

    instrs = []
    for name in sorted(iset.instrs, key=lambda n: ids.get(n, 1 << 30)):
        ins = iset.instrs[name]
        entry = entries.get(name)
        base_name = entry.get("expands_to", {}).get("instruction") if entry else None
        base = entries.get(base_name) if base_name else None
        src = base if base is not None else entry
        ops = (entry or {}).get("operands", {})
        fields = []
        for f in ins.fields:
            msb, lsb = ranges[f]
            rec = {"name": f, "msb": msb, "lsb": lsb, "register": decode.is_register_field(f)}
            if rec["register"]:
                rec["prime"] = decode.register_value(f, 0) == 8
            if f in ops:
                rec.update(ops[f])
            fields.append(rec)
        group = next((groups[e] for e in ins.extension if e in groups), "")
        rec = {
            "name": name,
            "id": ids.get(name),
            "spike_name": ins.spike_name,
            "pseudo_of": ins.pseudo_of,
            "match": ins.match,
            "mask": ins.mask,
            "length": ins.length,
            "extensions": ins.extension,
            "group": group,
            "fields": fields,
            "inside": sorted(inside.get(name, [])),
            "shadowed": name in ov.shadowed,
            "layer": "missing" if entry is None else ("inherited" if base is not None else "entry"),
        }
        if src is not None:
            attrs = dict(defaults)
            attrs.update(src.get("set", {}))
            if base is not None:
                attrs.update(entry.get("set", {}))
            tags = list(src.get("tags", []))
            if base is not None:
                tags += [t for t in entry.get("tags", []) if t not in tags]
            rec.update({
                "fu_class": src.get("fu_class"),
                "tags": tags,
                "attributes": attrs,
                "implicit": copy.deepcopy(src.get("implicit", [])) + (
                    copy.deepcopy(entry.get("implicit", [])) if base is not None else []),
                "uops": copy.deepcopy(src.get("uops")),
                "vector": copy.deepcopy(src.get("vector")),
                "expands_to": copy.deepcopy(entry.get("expands_to")),
                "variants": copy.deepcopy(entry.get("variants", [])),
                "rules": [r["name"] for r in _rules_for(layer, iset, base_name or name, src)],
            })
            if base is not None:
                # What the compressed entry adds on top of its base, for the evaluator.
                rec["own"] = {"tags": list(entry.get("tags", [])), "set": dict(entry.get("set", {})),
                              "implicit": copy.deepcopy(entry.get("implicit", []))}
        instrs.append(rec)

    csr_classes = {
        c: sorted(iset.csrs[n] for n in d["csrs"]) for c, d in layer.get("csr_classes", {}).items()
    }
    counts: dict[str, dict[str, int]] = {}
    for r in instrs:
        g = counts.setdefault(r["group"] or "other", {"entry": 0, "inherited": 0, "missing": 0})
        g[r["layer"]] += 1
    return {
        "format": FORMAT,
        "profile": prof["name"],
        "riscv_opcodes_commit": iset.commit,
        "files": profile_files(prof),
        "fu_classes": layer.get("fu_classes", {}),
        "tags": layer.get("tags", {}),
        "attributes": layer.get("attributes", {}),
        "uop_patterns": layer.get("uop_patterns", {}),
        "csr_classes": csr_classes,
        "csrs": dict(sorted(iset.csrs.items(), key=lambda kv: kv[1])),
        "rules": copy.deepcopy(layer.get("rules", [])),
        "fusion": copy.deepcopy(layer.get("fusion", [])),
        "decode_order": ov.order,
        "overlaps": [list(p) for p in ov.nested],
        "coverage": counts,
        "instructions": instrs,
    }


def _rules_for(layer: dict, iset: InstrSet, name: str, entry: dict) -> list[dict]:
    fields = set(iset.instrs[name].fields)
    out = []
    for rule in layer.get("rules", []):
        if not set(rule["when"]) <= fields:
            continue
        if name in rule.get("except", []):
            continue
        only = rule.get("only_fu_classes")
        if only and entry.get("fu_class") not in only:
            continue
        out.append(rule)
    return out
