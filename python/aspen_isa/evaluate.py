# ===================================================================
# SPDX-License-Identifier: Apache-2.0
# Copyright (c) 2026 Jeff Nye, uarchlabs.com
# SPDX-FileCopyrightText: 2026 Jeff Nye <jeff@uarchlabs.com>
# ===================================================================
"""Reference evaluator for the layer's semantics.

Given an instruction and its raw field values, returns the tags, attributes
and implicit operands of that instance. Generated decoders (A5, A6) must
agree with this on every case the tests cover.

Order: the entry's tags and attributes (after attribute defaults), then
the global rules, then the entry's variants in file order. Within one
variant, tags are added, then removed; a later `set` wins.

A compressed entry (expands_to) is evaluated as its base instruction with
the mapped field values, including the base's rules and variants; then its
own tags, `set` and variants apply on top. Immediates are mapped as raw
values, so across a mapping only zero and non-zero comparisons carry over.
"""

from __future__ import annotations

import re
from dataclasses import dataclass, field

_REG = re.compile(r"^[xfv]([0-9]|[12][0-9]|3[01])$")


@dataclass
class Result:
    name: str
    tags: set[str] = field(default_factory=set)
    attributes: dict = field(default_factory=dict)
    implicit: list[dict] = field(default_factory=list)
    applied: list[str] = field(default_factory=list)  # rules and variants that matched

    def as_dict(self) -> dict:
        return {"name": self.name, "tags": sorted(self.tags), "attributes": self.attributes,
                "implicit": self.implicit, "applied": self.applied}


class Evaluator:
    def __init__(self, db: dict):
        self.db = db
        self.by_name = {r["name"]: r for r in db["instructions"]}
        self.rules = {r["name"]: r for r in db.get("rules", [])}
        self.csr_classes = {k: set(v) for k, v in db.get("csr_classes", {}).items()}
        self.csrs = db.get("csrs", {})

    # -- values ---------------------------------------------------------
    @staticmethod
    def arch_values(rec: dict, raw: dict[str, int]) -> dict[str, int]:
        """Raw field values to comparable values: x8-x15 for prime register fields."""
        out = {}
        for f in rec["fields"]:
            v = raw.get(f["name"], 0)
            out[f["name"]] = v + 8 if f.get("prime") else v
        return out

    def _scalar(self, value: int, want) -> bool:
        if isinstance(want, str):
            m = _REG.match(want)
            if m:
                return value == int(m.group(1))
            return value == self.csrs.get(want, -1)
        return value == want

    def _match(self, values: dict[str, int], fld: str, m) -> bool:
        v = values[fld]
        if isinstance(m, list):
            return any(self._scalar(v, x) for x in m)
        if isinstance(m, dict):
            if "eq" in m:
                return v == values[m["eq"]]
            if "ne" in m:
                return v != values[m["ne"]]
            if "not" in m:
                return not self._match(values, fld, m["not"])
            if "all" in m:
                return all(self._match(values, fld, x) for x in m["all"])
            if "csr_class" in m:
                return v in self.csr_classes.get(m["csr_class"], set())
            if "mask" in m:
                return (v & m["mask"]) == m["value"]
            raise ValueError(f"unknown matcher {m}")
        return self._scalar(v, m)

    def _when(self, values: dict[str, int], cond: dict) -> bool:
        return all(self._match(values, f, m) for f, m in cond.items())

    @staticmethod
    def _apply(res: Result, eff: dict, label: str) -> None:
        res.tags |= set(eff.get("add_tags", []))
        res.tags -= set(eff.get("remove_tags", []))
        res.attributes.update(eff.get("set", {}))
        res.implicit += [dict(x) for x in eff.get("add_implicit", [])]
        res.applied.append(label)

    # -- evaluation -------------------------------------------------------
    def evaluate(self, name: str, raw: dict[str, int]) -> Result:
        rec = self.by_name[name]
        if rec["layer"] == "missing":
            raise KeyError(f"{name} has no layer entry yet")
        exp = rec.get("expands_to")
        if exp:
            values = self.arch_values(rec, raw)
            base_raw = {}
            for bf, src in exp["operands"].items():
                if isinstance(src, int):
                    base_raw[bf] = src
                elif _REG.match(src):
                    base_raw[bf] = int(src[1:])
                else:
                    base_raw[bf] = values[src]
            res = self._evaluate_entry(self.by_name[exp["instruction"]], base_raw)
            res.name = name
            own = rec.get("own", {})
            res.tags |= set(own.get("tags", []))
            res.attributes.update(own.get("set", {}))
            res.implicit += [dict(x) for x in own.get("implicit", [])]
            for i, var in enumerate(rec.get("variants", [])):
                if self._when(values, var["when"]):
                    self._apply(res, var, f"{name}:{var.get('name', i)}")
            return res
        return self._evaluate_entry(rec, raw)

    def _evaluate_entry(self, rec: dict, raw: dict[str, int]) -> Result:
        values = self.arch_values(rec, raw)
        res = Result(rec["name"], set(rec["tags"]), dict(rec["attributes"]),
                     [dict(x) for x in rec.get("implicit", [])])
        for rname in rec.get("rules", []):
            rule = self.rules[rname]
            if self._when(values, rule["when"]):
                self._apply(res, rule, f"rule:{rname}")
        for i, var in enumerate(rec.get("variants", [])):
            if self._when(values, var["when"]):
                self._apply(res, var, f"{rec['name']}:{var.get('name', i)}")
        return res
