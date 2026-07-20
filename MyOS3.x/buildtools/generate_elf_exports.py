#!/usr/bin/env python3
"""Generate the kernel export table consumed by IDynamicLoadingImpl."""

import argparse
import pathlib
import subprocess


def symbols(path: pathlib.Path, undefined: bool) -> set[str]:
    args = ["nm", "--format=posix"]
    if undefined:
        args.append("--undefined-only")
    else:
        args.extend(["--defined-only", "--extern-only"])
    args.append(str(path))
    result = subprocess.run(args, check=True, text=True, capture_output=True)
    found = set()
    for line in result.stdout.splitlines():
        fields = line.split()
        if not fields:
            continue
        name = fields[0]
        if name.endswith(":") or name.startswith("."):
            continue
        found.add(name)
    return found


def quote(value: str) -> str:
    return value.replace("\\", "\\\\").replace('"', '\\"')


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--core", required=True, type=pathlib.Path)
    parser.add_argument("--module", required=True, type=pathlib.Path)
    parser.add_argument("--output", required=True, type=pathlib.Path)
    args = parser.parse_args()

    core = symbols(args.core, undefined=False)
    module = symbols(args.module, undefined=True)

    # Module objects use GCC's legacy -fleading-underscore ABI. The exported
    # name retains that prefix while its table address names the core symbol.
    exports = sorted((name, name[1:]) for name in module if name.startswith("_") and name[1:] in core)

    lines = [
        '.section .myos_exports,"aw",@progbits',
        ".balign 4",
        ".globl _EXPORT_TABLE_",
        "_EXPORT_TABLE_:",
        "  .long 0, 0, 0, .Ldll_name, 1",
        f"  .long {len(exports)}, {len(exports)}, .Lfunctions, .Lnames, .Lordinals",
        ".Lfunctions:",
    ]
    lines.extend(f"  .long {address}" for _, address in exports)
    lines.append(".Lnames:")
    lines.extend(f"  .long .Lname_{index}" for index, _ in enumerate(exports))
    lines.append(".Lordinals:")
    lines.extend(f"  .short {index}" for index, _ in enumerate(exports))
    lines.extend([".balign 4", '.Ldll_name: .asciz "MyOS3.x"'])
    for index, (name, _) in enumerate(exports):
        lines.append(f'.Lname_{index}: .asciz "{quote(name)}"')
    lines.append('.section .note.GNU-stack,"",@progbits')

    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text("\n".join(lines) + "\n", encoding="utf-8")
    print(f"Generated {len(exports)} kernel exports in {args.output}")


if __name__ == "__main__":
    main()
