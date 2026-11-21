<!-- SPDX-License-Identifier: CC-BY-4.0                        -->
<!-- Copyright (c) 2026 Jeff Nye, uarchlabs.com                -->
<!-- SPDX-FileCopyrightText: 2026 Jeff Nye <jeff@uarchlabs.com -->
# ISA layer

Encodings come from [riscv-opcodes](https://github.com/riscv/riscv-opcodes),
pinned as the submodule `ext/riscv-opcodes`. This directory adds the
microarchitecture data riscv-opcodes does not hold. Generators (task A5,
A6) turn the two into the timing model's decoder and pacino's decode
tables.

| Path | What it is |
| --- | --- |
| `profiles/rva23s64.yaml` | The riscv-opcodes extension files that make up RVA23S64, and the pinned commit |
| `schema/layer.schema.json` | JSON Schema for layer files |
| `layer/rva23s64.yaml` | The layer: vocabularies, rules, per-instruction entries, fusion pairs |
| `ids.yaml` | Append-only instruction ID registry |
| `../python/aspen_isa` | Loader, validator and ID tool |

## Names and IDs

Instruction names are riscv-opcodes `instr_dict` keys, the same names as
Spike's `DECLARE_INSN`: dots become underscores (`c_addi4spn`, `vle32_v`).

The instruction set is every base entry in the profile's files, plus the
pseudo-ops riscv-opcodes emits into `encoding.h` (`prefetch_*`, `pause`,
`mop_*`, `c_mop_*`), so it matches what Spike decodes. Each gets one ID.
Other pseudo-ops (`mv`, `ret`, `fmv_d`, `ntl_*`, ...) are operand patterns
of a base instruction: they become variants, not IDs.
`python3 -m aspen_isa list --patterns` lists them.

`ids.yaml` is append-only: an ID never changes and is never reused, and a
name that leaves the profile keeps its ID. ID 0 means unknown. CI compares
the registry against the previous commit.

## Layer file

No latencies: execution units specify them in Sparta parameters.

| Section | Contents |
| --- | --- |
| `fu_classes`, `tags`, `uop_patterns` | Vocabularies, name to description. Every name used must be declared |
| `attributes` | Typed values (`boolean`, `integer`, `enum`) with defaults, e.g. `serialize`, `reserved` |
| `csr_classes` | Groups of CSRs by `csrs.csv` name, for `csr_class` matchers |
| `rules` | Variants that apply to every instruction having the fields they name, e.g. `rd: x0` gives `no_dest` |
| `instructions` | Per-instruction entries, keyed by name |
| `fusion` | Pairs of adjacent instructions that decode as one |

An instruction entry has either `fu_class` or `expands_to`:

- `fu_class`, `tags`, `set` (attributes), `operands` (role of every
  riscv-opcodes field: `src`, `dest`, `src_dest` with a register `file`, or
  `imm`, `csr`, `rm`, `mask`, `other`), `implicit` operands, `uops`, and for
  vector instructions `vector` (`kind`, `eew`, `mem_mode`, `width`,
  `uop_pattern`).
- `expands_to` maps a compressed instruction onto its base instruction:
  each base field gets a compressed field, a fixed register or a fixed
  value. The entry inherits the base entry, including its variants, which
  are evaluated on the mapped operands (so `c_mv` is a `move` through
  `add`). Its own variants apply on top.

### Variants

A variant is `when` (conditions) plus effects: `add_tags`, `remove_tags`,
`set`, `add_implicit`. All conditions must hold. Field values are raw, as
encoded; compressed register fields are compared after decoding to
architectural registers.

| Matcher | Example | Meaning |
| --- | --- | --- |
| register | `rs1: x0` | Field is that register |
| number | `imm12: 0` | Raw field value |
| CSR name | `csr: satp` | `csr` field is that CSR |
| list | `rd: [x1, x5]` | Any of these |
| `eq`, `ne` | `rs2: {eq: rs1}` | Equal or not to another field |
| `not` | `rs1: {not: x0}` | Negation |
| `all` | `rs1: {all: [[x1, x5], {ne: rd}]}` | Every matcher holds |
| `csr_class` | `csr: {csr_class: counters}` | CSR is in the class |
| `mask`, `value` | `pred: {mask: 12, value: 0}` | `field & mask == value` |

Order: the entry's own `tags` and `set`, then rules, then variants in file
order. Within a variant tags are added, then removed; a later `set` wins.

Fusion conditions qualify fields by side: `second.rs1: {eq: first.rd}`.

## Commands

```sh
git submodule update --init
pip install -r python/requirements.txt

PYTHONPATH=python python3 -m aspen_isa validate      # layer + ID registry
PYTHONPATH=python python3 -m aspen_isa ids --assign  # IDs for new instructions
PYTHONPATH=python python3 -m aspen_isa list          # instructions and their fields
python3 -m pytest -q python/tests
```

When the pinned riscv-opcodes commit moves, update the submodule and
`riscv_opcodes_commit` in the profile together, then run `ids --assign`.
