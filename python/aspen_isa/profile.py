# ===================================================================
# SPDX-License-Identifier: Apache-2.0
# Copyright (c) 2026 Jeff Nye, uarchlabs.com
# SPDX-FileCopyrightText: 2026 Jeff Nye <jeff@uarchlabs.com>
# ===================================================================
"""Check a profile file: its extension list against the ratified profile
text, and its riscv-opcodes files against the checkout."""

from __future__ import annotations

import re
from pathlib import Path

_HEADING = re.compile(r"^(=+)\s+(.*\S)\s*$")
_ITEM = re.compile(r"^(-|\*\*)\s+\*([A-Za-z0-9]+)\*")


def check(prof: dict, repo: Path) -> list[str]:
    errors: list[str] = []
    seen_names: set[str] = set()
    seen_files: dict[str, str] = {}
    for ext in prof["extensions"]:
        name = ext.get("name")
        if not name:
            errors.append(f"profile: extension without a name: {ext}")
            continue
        if name in seen_names:
            errors.append(f"profile/{name}: listed twice")
        seen_names.add(name)
        if "files" not in ext:
            errors.append(f"profile/{name}: needs 'files' (use [] for an extension with no instructions)")
        files = ext.get("files", [])
        if files and ext.get("group") not in ("scalar", "privileged", "vector"):
            errors.append(f"profile/{name}: an extension with files needs group scalar, privileged or vector")
        for f in files:
            if not (Path(repo) / "extensions" / f).is_file():
                errors.append(f"profile/{name}: riscv-opcodes has no extension file '{f}'")
            if f in seen_files:
                errors.append(f"profile/{name}: file '{f}' is already listed under {seen_files[f]}")
            seen_files[f] = name
    return errors


def mandatory_from_spec(adoc: Path, profile: str = "RVA23S64") -> list[str]:
    """Mandatory base and extensions of a profile, read from the profile's AsciiDoc source.

    An S-mode profile includes every mandatory extension of the matching
    U-mode profile, as the profile text states. Sub-items (the parts of Sha)
    are included.
    """
    text = Path(adoc).read_text(encoding="utf-8").splitlines()

    def section(title: str) -> list[str]:
        out: list[str] = []
        inside = False
        level = 0
        for line in text:
            h = _HEADING.match(line)
            if h:
                if inside and len(h.group(1)) <= level:
                    break
                if h.group(2) == title:
                    inside, level = True, len(h.group(1))
                continue
            if inside:
                m = _ITEM.match(line.strip())
                if m:
                    out.append(m.group(2))
                b = re.search(r"\b(RV64I|RV32I)\b is the mandatory base", line)
                if b:
                    out.append(b.group(1))
        return out

    names: list[str] = []
    profiles = [profile]
    if profile.endswith(("S64", "S32")):
        profiles.insert(0, profile.replace("S", "U", 1) if profile.startswith("RVA") else profile)
    for p in profiles:
        for part in ("Mandatory Base", "Mandatory Extensions"):
            for n in section(f"{p} {part}"):
                if n not in names:
                    names.append(n)
    return names


def compare_with_spec(prof: dict, adoc: Path) -> tuple[list[str], list[str]]:
    """(missing from the profile file, in the profile file but not mandatory)."""
    spec = mandatory_from_spec(adoc, prof["name"].upper())
    ours = [e["name"] for e in prof["extensions"]]
    return [n for n in spec if n not in ours], [n for n in ours if n not in spec]
