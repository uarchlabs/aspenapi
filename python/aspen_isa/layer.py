# ===================================================================
# SPDX-License-Identifier: Apache-2.0
# Copyright (c) 2026 Jeff Nye, uarchlabs.com
# SPDX-FileCopyrightText: 2026 Jeff Nye <jeff@uarchlabs.com>
# ===================================================================
"""Validate an AspenADL layer file against the schema and riscv-opcodes."""

from __future__ import annotations

import json
import re
from pathlib import Path

import yaml
from jsonschema import Draft202012Validator

from .opcodes import REGISTER_FIELD, InstrSet

SCHEMA_PATH = Path(__file__).resolve().parents[2] / "isa" / "schema" / "layer.schema.json"
REGISTER = re.compile(r"^([xfv])([0-9]|[12][0-9]|3[01])$")


class _UniqueKeyLoader(yaml.SafeLoader):
    """SafeLoader that rejects duplicate mapping keys instead of keeping the last."""


def _construct_mapping(loader, node, deep=False):
    seen = set()
    for key_node, _ in node.value:
        key = loader.construct_object(key_node, deep=deep)
        if key in seen:
            raise yaml.constructor.ConstructorError(
                None, None, f"duplicate key '{key}'", key_node.start_mark)
        seen.add(key)
    return yaml.SafeLoader.construct_mapping(loader, node, deep)


_UniqueKeyLoader.add_constructor(yaml.resolver.BaseResolver.DEFAULT_MAPPING_TAG, _construct_mapping)


def load_yaml(path: Path):
    with open(path, encoding="utf-8") as f:
        return yaml.load(f, Loader=_UniqueKeyLoader)


def load_schema(path: Path = SCHEMA_PATH) -> dict:
    with open(path, encoding="utf-8") as f:
        schema = json.load(f)
    Draft202012Validator.check_schema(schema)
    return schema


def schema_errors(doc, schema: dict) -> list[str]:
    v = Draft202012Validator(schema)
    out = []
    for e in sorted(v.iter_errors(doc), key=lambda e: list(map(str, e.absolute_path))):
        where = "/".join(str(p) for p in e.absolute_path) or "<root>"
        out.append(f"schema: {where}: {e.message}")
    return out


class _Checker:
    def __init__(self, layer: dict, iset: InstrSet):
        self.layer = layer
        self.iset = iset
        self.errors: list[str] = []
        self.warnings: list[str] = []
        self.fu = set(layer.get("fu_classes", {}))
        self.tags = set(layer.get("tags", {}))
        self.attrs = layer.get("attributes", {})
        self.patterns = set(layer.get("uop_patterns", {}))
        self.csr_classes = layer.get("csr_classes", {})
        self.instrs = layer.get("instructions", {})

    def err(self, where: str, msg: str) -> None:
        self.errors.append(f"{where}: {msg}")

    def warn(self, where: str, msg: str) -> None:
        self.warnings.append(f"{where}: {msg}")

    # -- vocabularies -------------------------------------------------
    def fu_class(self, where: str, name: str) -> None:
        if name not in self.fu:
            self.err(where, f"functional-unit class '{name}' is not declared in fu_classes")

    def tag_list(self, where: str, names) -> None:
        for t in names or []:
            if t not in self.tags:
                self.err(where, f"tag '{t}' is not declared in tags")

    def attr_value(self, where: str, name: str, value) -> None:
        decl = self.attrs.get(name)
        if decl is None:
            self.err(where, f"attribute '{name}' is not declared in attributes")
            return
        kind = decl["type"]
        if kind == "boolean":
            ok = isinstance(value, bool)
        elif kind == "integer":
            ok = isinstance(value, int) and not isinstance(value, bool) and value >= decl.get("minimum", value)
        else:
            ok = value in decl.get("values", [])
        if not ok:
            self.err(where, f"value {value!r} does not fit attribute '{name}' ({kind})")

    def attr_set(self, where: str, values) -> None:
        for k, v in (values or {}).items():
            self.attr_value(f"{where}/{k}", k, v)

    def vocab(self) -> None:
        for name, decl in self.attrs.items():
            if "default" in decl:
                self.attr_value(f"attributes/{name}/default", name, decl["default"])
        for cls, decl in self.csr_classes.items():
            for csr in decl["csrs"]:
                if csr not in self.iset.csrs:
                    self.err(f"csr_classes/{cls}", f"CSR '{csr}' is not in riscv-opcodes csrs.csv")

    # -- matchers -----------------------------------------------------
    def field_kind(self, instr: str, fld: str) -> str:
        if fld == "csr":
            return "csr"
        return "reg" if REGISTER_FIELD.match(fld) else "imm"

    def declared_file(self, instr: str, fld: str):
        ops = self.instrs.get(instr, {}).get("operands", {})
        return ops.get(fld, {}).get("file")

    def scalar(self, where: str, instr: str, fld: str, value) -> None:
        kind = self.field_kind(instr, fld)
        if isinstance(value, bool):
            self.err(where, "booleans are not field values")
            return
        if isinstance(value, int):
            if kind == "reg":
                self.err(where, f"'{fld}' is a register field; use a register name such as x0")
                return
            width = self.iset.field_widths.get(fld)
            if width is not None and value >= (1 << width):
                self.err(where, f"value {value} does not fit the {width}-bit field '{fld}'")
            return
        m = REGISTER.match(value)
        if m:
            if kind != "reg":
                self.err(where, f"'{fld}' is not a register field")
                return
            file = self.declared_file(instr, fld)
            if file and file != m.group(1):
                self.err(where, f"register {value} is not in the '{file}' file declared for '{fld}'")
            return
        if kind == "csr":
            if value not in self.iset.csrs:
                self.err(where, f"CSR '{value}' is not in riscv-opcodes csrs.csv")
            return
        self.err(where, f"'{value}' is not a register, a number or (for csr) a CSR name")

    def matcher(self, where: str, instr: str, fld: str, m, ref_ok) -> None:
        if isinstance(m, list):
            for i, v in enumerate(m):
                self.scalar(f"{where}[{i}]", instr, fld, v)
        elif isinstance(m, dict):
            if "eq" in m or "ne" in m:
                ref = m.get("eq", m.get("ne"))
                if not ref_ok(ref):
                    self.err(where, f"'{ref}' is not a field to compare with")
            elif "all" in m:
                for i, sub in enumerate(m["all"]):
                    self.matcher(f"{where}/all[{i}]", instr, fld, sub, ref_ok)
            elif "not" in m:
                self.matcher(f"{where}/not", instr, fld, m["not"], ref_ok)
            elif "csr_class" in m:
                if fld != "csr":
                    self.err(where, "csr_class applies only to the csr field")
                if m["csr_class"] not in self.csr_classes:
                    self.err(where, f"CSR class '{m['csr_class']}' is not declared in csr_classes")
            elif "mask" in m:
                width = self.iset.field_widths.get(fld)
                if self.field_kind(instr, fld) == "reg":
                    self.err(where, "mask matchers apply to non-register fields")
                if width is not None and m["mask"] >= (1 << width):
                    self.err(where, f"mask does not fit the {width}-bit field '{fld}'")
                if m["value"] & ~m["mask"]:
                    self.err(where, "value has bits outside the mask")
        else:
            self.scalar(where, instr, fld, m)

    def when(self, where: str, instr: str, cond: dict) -> None:
        fields = set(self.iset.instrs[instr].fields)
        for fld, m in cond.items():
            if fld not in fields:
                self.err(f"{where}/{fld}", f"'{instr}' has no field '{fld}' (fields: {', '.join(sorted(fields)) or 'none'})")
                continue
            self.matcher(f"{where}/{fld}", instr, fld, m, lambda r, fields=fields: r in fields)

    def effects(self, where: str, eff: dict) -> None:
        self.tag_list(f"{where}/add_tags", eff.get("add_tags"))
        self.tag_list(f"{where}/remove_tags", eff.get("remove_tags"))
        both = set(eff.get("add_tags", [])) & set(eff.get("remove_tags", []))
        if both:
            self.err(where, f"tags both added and removed: {', '.join(sorted(both))}")
        self.attr_set(f"{where}/set", eff.get("set"))

    # -- sections -----------------------------------------------------
    def instruction(self, name: str, entry: dict) -> None:
        where = f"instructions/{name}"
        instr = self.iset.instrs.get(name)
        if instr is None:
            hint = " (a pseudo-op: write it as a variant of its base instruction)" if name in self.iset.pseudo_patterns else ""
            self.err(where, f"'{name}' is not an instruction of profile {self.iset.profile}{hint}")
            return
        fields = set(instr.fields)

        if "fu_class" in entry:
            self.fu_class(f"{where}/fu_class", entry["fu_class"])
        self.tag_list(f"{where}/tags", entry.get("tags"))
        self.attr_set(f"{where}/set", entry.get("set"))

        ops = entry.get("operands")
        if ops is not None:
            for fld, op in ops.items():
                if fld not in fields:
                    self.err(f"{where}/operands/{fld}", f"'{name}' has no field '{fld}'")
                    continue
                is_reg = op["role"] in ("src", "dest", "src_dest")
                if is_reg != bool(REGISTER_FIELD.match(fld)):
                    self.err(f"{where}/operands/{fld}", f"role '{op['role']}' does not fit field '{fld}'")
            missing = fields - set(ops)
            if missing:
                self.err(f"{where}/operands", f"missing fields: {', '.join(sorted(missing))}")

        for i, u in enumerate(entry.get("uops", [])):
            self.fu_class(f"{where}/uops/{i}/fu_class", u["fu_class"])
            self.tag_list(f"{where}/uops/{i}/tags", u.get("tags"))

        vec = entry.get("vector")
        if vec is not None:
            if not any(e.startswith(("rv_v", "rv_zv")) for e in instr.extension):
                self.err(f"{where}/vector", f"'{name}' is not a vector instruction")
            if "uop_pattern" in vec and vec["uop_pattern"] not in self.patterns:
                self.err(f"{where}/vector/uop_pattern", f"'{vec['uop_pattern']}' is not declared in uop_patterns")

        exp = entry.get("expands_to")
        if exp is not None:
            self.expansion(where, name, fields, exp)

        for i, var in enumerate(entry.get("variants", [])):
            vw = f"{where}/variants/{i}"
            self.when(f"{vw}/when", name, var["when"])
            self.effects(vw, var)

    def expansion(self, where: str, name: str, fields: set, exp: dict) -> None:
        base = exp["instruction"]
        b = self.iset.instrs.get(base)
        if b is None:
            self.err(f"{where}/expands_to", f"base '{base}' is not an instruction of the profile")
            return
        if base not in self.instrs:
            self.err(f"{where}/expands_to", f"base '{base}' has no layer entry to inherit from")
        elif "expands_to" in self.instrs[base]:
            self.err(f"{where}/expands_to", f"base '{base}' itself expands; chains are not allowed")
        bfields = set(b.fields)
        for bf, src in exp["operands"].items():
            ow = f"{where}/expands_to/operands/{bf}"
            if bf not in bfields:
                self.err(ow, f"base '{base}' has no field '{bf}'")
            elif isinstance(src, str) and REGISTER.match(src):
                if not REGISTER_FIELD.match(bf):
                    self.err(ow, f"fixed register for non-register field '{bf}'")
            elif isinstance(src, str):
                if src not in fields:
                    self.err(ow, f"'{name}' has no field '{src}'")
        missing = bfields - set(exp["operands"])
        if missing:
            self.err(f"{where}/expands_to/operands", f"base fields not mapped: {', '.join(sorted(missing))}")

    def rules(self) -> None:
        for i, rule in enumerate(self.layer.get("rules", [])):
            where = f"rules/{i}"
            for fc in rule.get("only_fu_classes", []):
                self.fu_class(f"{where}/only_fu_classes", fc)
            for n in rule.get("except", []):
                if n not in self.iset.instrs:
                    self.err(f"{where}/except", f"'{n}' is not an instruction of the profile")
            self.effects(where, rule)
            hits = [n for n, ins in self.iset.instrs.items() if set(rule["when"]) <= set(ins.fields)]
            if not hits:
                self.err(where, f"no instruction has all of: {', '.join(rule['when'])}")
                continue
            for fld, m in rule["when"].items():
                self.matcher(f"{where}/when/{fld}", hits[0], fld, m,
                             lambda r: all(r in self.iset.instrs[h].fields for h in hits))

    def fusion(self) -> None:
        for i, pair in enumerate(self.layer.get("fusion", [])):
            where = f"fusion/{i}"
            self.fu_class(f"{where}/fu_class", pair["fu_class"])
            self.tag_list(f"{where}/tags", pair.get("tags"))
            sides = {"first": pair["first"], "second": pair["second"]}
            for side, names in sides.items():
                for n in names:
                    if n not in self.iset.instrs:
                        self.err(f"{where}/{side}", f"'{n}' is not an instruction of the profile")
            def has(q: str) -> bool:
                side, _, fld = q.partition(".")
                return side in sides and all(
                    n in self.iset.instrs and fld in self.iset.instrs[n].fields for n in sides[side])
            for q, m in pair.get("when", {}).items():
                if not has(q):
                    self.err(f"{where}/when/{q}", f"not every '{q.partition('.')[0]}' instruction has field '{q.partition('.')[2]}'")
                    continue
                side, _, fld = q.partition(".")
                self.matcher(f"{where}/when/{q}", sides[side][0], fld, m, has)

    def run(self) -> None:
        if self.layer.get("profile") != self.iset.profile:
            self.err("profile", f"layer is for '{self.layer.get('profile')}', instruction set is '{self.iset.profile}'")
        self.vocab()
        for name, entry in self.instrs.items():
            self.instruction(name, entry)
        self.rules()
        self.fusion()


def validate(layer: dict, iset: InstrSet, schema: dict | None = None) -> tuple[list[str], list[str]]:
    """Return (errors, warnings). Semantic checks run only if the schema passes."""
    errs = schema_errors(layer, schema or load_schema())
    if errs:
        return errs, []
    c = _Checker(layer, iset)
    c.run()
    return c.errors, c.warnings
