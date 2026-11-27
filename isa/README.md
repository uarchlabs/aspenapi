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
| `profiles/rva23s64.yaml` | Every mandatory RVA23S64 extension with its riscv-opcodes files, the pinned commit, and the profile text it was checked against |
| `schema/layer.schema.json` | JSON Schema for layer files |
| `layer/rva23s64.yaml` | The layer: vocabularies, rules, per-instruction entries, fusion pairs |
| `ids.yaml` | Append-only instruction ID registry |
| `catalogue/patterns.yaml` | Decisions for the operand-pattern catalogue, written from the ISA and assembly manuals |
| `catalogue/rva23s64.yaml` | The catalogue itself, generated: every operand pattern of every instruction and its resolution |
| `../python/aspen_isa` | Loader, validator, ID tool, overlap analysis, merged database, reference evaluator |

## Names and IDs

Instruction names are riscv-opcodes `instr_dict` keys, the same names as
Spike's `DECLARE_INSN`: dots become underscores (`c_addi4spn`, `vle32_v`).

The instruction set is every base entry in the profile's files, plus the
pseudo-ops riscv-opcodes emits into `encoding.h` (`prefetch_*`, `pause`,
`mop_*`, `c_mop_*`), so it matches what Spike decodes. Each gets one ID.
Other pseudo-ops (`mv`, `ret`, `fmv_d`, `ntl_*`, ...) are operand patterns
of a base instruction: they become variants, not IDs.
`python3 -m aspen_isa list --patterns` lists them.

One promoted pseudo-op has an ID: RV64 `zext_h` is defined in riscv-opcodes
as `packw` with `rs2 = 0`, and `packw` belongs to Zbkb, which RVA23 does not
include. Spike executes it as `packw`; the merged database records that as
`spike_name`.

`ids.yaml` is append-only: an ID never changes and is never reused, and a
name that leaves the profile keeps its ID. ID 0 means unknown. CI compares
the registry against the previous commit.

## Decode

Two entries overlap when some word matches both. Every overlap must be
nested: one entry's fixed bits strictly include the other's, and the more
specific entry wins inside the other's space (`c_nop` inside `c_addi`,
`pause` inside `fence`, `prefetch_r` inside `ori`). The validator rejects
any other overlap. Decode order is by number of fixed bits, most first.

An entry whose whole space is covered by more specific entries is
*shadowed*: no word decodes to it. Today that is `mop_r_N`, `mop_rr_N` and
`c_mop_N`, the generic forms of the numbered `mop_r_0`... entries.

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

Order: attribute defaults, the entry's own `tags` and `set`, then rules,
then variants in file order. Within a variant tags are added, then removed;
a later `set` wins.

A compressed entry is evaluated as its base instruction on the mapped
operands, including the base's rules and variants, then its own tags, `set`
and variants apply. Immediates map as raw values, so across a mapping only
zero and non-zero comparisons carry over.

`python/aspen_isa/evaluate.py` implements these rules and is the reference:
the generated decoders must agree with it, and `python/tests/test_evaluate.py`
holds the cases.

Fusion conditions qualify fields by side: `second.rs1: {eq: first.rd}`.

## Operand-pattern catalogue

`catalogue/rva23s64.yaml` lists, for every instruction, the operand
patterns that might matter to timing, and how each is resolved. Patterns
come from three places:

- **A sweep:** every integer register field equal to x0, every immediate
  equal to zero, every pair of same-file source registers equal, and the
  special fields (rounding mode, vector mask, aq/rl, segment count, fence
  sets).
- **riscv-opcodes pseudo-ops** defined on the instruction (`mv`, `ret`,
  `fmv_d`, `ntl_*`...).
- **`catalogue/patterns.yaml`**, written from the ISA manual's HINT tables
  and the assembly manual's pseudoinstruction tables, with commits pinned.

`patterns.yaml` resolves each entry with its first matching pattern:
`variant` (a proposed layer variant), `rule` (a global layer rule),
`hint`, `reserved`, `none` or `review`. An entry whose witness words all
decode as another instruction is resolved automatically (`encoding`). CI
fails if any entry is unresolved, if the committed catalogue is out of
date, or if a layer entry disagrees with a `variant` or `rule` entry: the
check builds a witness word for each one and evaluates it.

`reserved` means the encoding decodes as illegal. An encoding that decodes
but traps later, such as a write to a read-only CSR (0xC00-0xFFF), is a
`variant` with the `csr_write_trap` tag instead.

Reviewed 2026-11-27 against pacino's decode RTL (uarchlabs/pacino
`0913cd0`): every witness word was run through pacino's RVC expander and
`instr_decoder`, and decode legality agrees with this catalogue except for
masked vector instructions with vd = v0 (patterns `v_masked_vd_v0` and
`v_carry_merge_vd_v0`, 271 instructions). The catalogue and Spike reserve
those; pacino still decodes them, which is a pacino RTL change.

To change a decision, edit `patterns.yaml` and run `catalogue --write`.
When A4 and A7 add layer entries, the catalogue's `variant` entries are
the variants to write, and the layer check holds them to it.

Known limit: riscv-opcodes field names like `rd_n0` mean "not zero", but
the reference decoder does not reject those values yet; the catalogue
lists the reserved ones (`c.jr x0`, `c.lwsp x0`, ...).

## Commands

```sh
git submodule update --init
pip install -r python/requirements.txt

PYTHONPATH=python python3 -m aspen_isa validate      # layer + ID registry
PYTHONPATH=python python3 -m aspen_isa ids --assign  # IDs for new instructions
PYTHONPATH=python python3 -m aspen_isa list          # instructions and their fields
PYTHONPATH=python python3 -m aspen_isa merge -o build/rva23s64.db.json   # merged database
PYTHONPATH=python python3 -m aspen_isa decode 00008067 852e              # decode and evaluate words
PYTHONPATH=python python3 -m aspen_isa profile --spec <riscv-profiles>/src/rva23-profile.adoc
PYTHONPATH=python python3 -m aspen_isa spike <ua-spike>/riscv/encoding.h
PYTHONPATH=python python3 -m aspen_isa catalogue --check --strict-layer       # A9 catalogue
PYTHONPATH=python python3 -m aspen_isa catalogue --write                      # after editing patterns.yaml
PYTHONPATH=python python3 -m aspen_isa catalogue --show addi jalr             # one instruction's entries
python3 -m pytest -q python/tests
```

The merged database (`merge`) is the input for the generators: every
instruction with its ID, encoding, fields and bit ranges, decode order,
overlaps, and its layer data with inherited data filled in. Its `coverage`
counts how many instructions per group have layer entries yet.

CI also runs `profile --spec` against the ratified profile text and `spike`
against ua-spike's current `encoding.h`. The second check fails when
ua-spike has been rebased onto a newer riscv-opcodes and the pin here has
not moved yet; moving it is step 4 of ua-spike's monthly rebase.

When the pinned riscv-opcodes commit moves, update the submodule and
`riscv_opcodes_commit` in the profile together, then run `ids --assign`.
