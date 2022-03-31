#!/usr/bin/env python

# see app/src/cmds/cmd_write_res.cpp

import argparse
import os

parser = argparse.ArgumentParser(description="Embed a list of files into a resource file")
parser.add_argument("-O", "--output", metavar="<file>", required=True, help="set the output code file")
parser.add_argument("-I", "--inc-dir", metavar="<dir>", required=True, help="set the prefix for #include")
parser.add_argument("-H", "--header", metavar="<file>", required=False, help="set the output header file")
parser.add_argument("--ns", metavar="<namespace>", required=False, help="set the namespace for the resources")
parser.add_argument('input', nargs='+', metavar="<input>", help="set the file to be embedded")
args = parser.parse_args()

if args.header is None:
    base, ext = os.path.splitext(args.output)
    ext = {
        '.cc': '.hh',
        '.cpp': '.hpp',
        '.cxx': '.hxx'
    }[ext]
    args.header = base + ext

idents = {}
inputs = [os.path.abspath(input) for input in args.input]
prefix = os.path.commonpath(inputs)
for input in inputs:
    ident = os.path.splitext(input[len(prefix):])[0].replace('\\', '/')
    if not len(ident): continue
    if ident[0] == '/': ident = ident[1:]
    # TODO? safe idents out of sub-dir names...
    if '/' in ident: continue
    idents[input] = ident

with open(args.header, "w", encoding="UTF-8") as header:
    header.write("""// THIS FILE IS AUTOGENERATED
#pragma once

#include <cstddef>
#include <string_view>

// clang-format off
""")

    prefix = ""
    if args.ns is not None:
        header.write(f"""namespace {args.ns} {{
""")
        prefix = "    "

    for input in inputs:
        ident = idents[input]
        header.write(f"""{prefix}struct {ident} {{
{prefix}    static const char* data() noexcept;
{prefix}    static std::size_t size() noexcept;
{prefix}    static std::string_view view() noexcept {{ return {{data(), size()}}; }}
{prefix}}};
""")

    if args.ns is not None:
        header.write(f"""}} // namespace {args.ns}
""")
    header.write(f"""// clang-format on
""")

ROW_WIDTH = 16
with open(args.output, "w", encoding="UTF-8") as source:
    include = os.path.relpath(args.header, args.inc_dir)

    source.write(f"""// THIS FILE IS AUTOGENERATED
#include "{include}"

// clang-format off
""")

    prefix = ""
    if args.ns is not None:
        source.write(f"""namespace {args.ns} {{
""")
        prefix = "    "

    source.write(f"""{prefix}namespace {{
""")

    first = True
    for input in inputs:
        if first: first = False
        else: source.write("\n");
        ident = idents[input]
        source.write(f"""{prefix}    const char __{ident}[] = {{
""")
        with open(input, "rb") as file:
            content = file.read()
            offset = 0
            for b in content:
                if (offset % ROW_WIDTH) == 0:
                    source.write(f"""{prefix}        \"""");
                source.write(f"\\x{b:02x}")
                if ((offset + 1) % ROW_WIDTH) == 0:
                    source.write("\"\n")
                offset += 1
            if (offset % ROW_WIDTH) != 0:
                source.write("\"\n")

        source.write(f"""{prefix}    }}; // __{ident}
""")

    source.write(f"""{prefix}}} // namespace
""")
    for input in inputs:
        ident = idents[input]
        source.write(f"""
{prefix}/*static*/ const char* {ident}::data() noexcept {{ return __{ident}; }}
{prefix}/*static*/ std::size_t {ident}::size() noexcept {{ return sizeof(__{ident}) - 1; }}
""")

    if args.ns is not None:
        source.write(f"""}} // namespace {args.ns}
""")
    source.write(f"""// clang-format on
""")