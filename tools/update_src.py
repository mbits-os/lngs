#!/usr/bin/env python

# see app/src/cmds/cmd_write_res.cpp

import argparse
import os
import shutil
from typing import Tuple, Union

parser = argparse.ArgumentParser(
    description="Update source files based on files generated in binary dir"
)
parser.add_argument(
    "-S", metavar="<dir>", required=True, help="set the source directory"
)
parser.add_argument(
    "-B", metavar="<dir>", required=True, help="set the binary directory"
)
parser.add_argument(
    "source", nargs="+", metavar="<source>", help="set the file to be updated"
)
args = parser.parse_args()


def exists(dir_name: str, file_name: str) -> Union[float, None]:
    try:
        return os.path.isfile(os.path.join(dir_name, file_name))
    except FileNotFoundError:
        return None


def split_or_dup(input: str) -> Tuple[str, str]:
    if ":" in input:
        b, s = input.split(":", 1)
        return (b, s)
    return (input, input)


sources = [
    split_or_dup(source)
    for source in args.source
    if exists(args.B, split_or_dup(source)[0])
]
for bin_filename, src_filename in sources:
    copy = not os.path.exists(os.path.join(args.S, src_filename))
    if not copy:
        with open(os.path.join(args.B, bin_filename), "r") as left_file:
            left = left_file.read()
        with open(os.path.join(args.S, src_filename), "r") as right_file:
            right = right_file.read()
        copy = left != right

    if copy:
        print("Updating {}".format(src_filename))
        shutil.copy(
            os.path.join(args.B, bin_filename), os.path.join(args.S, src_filename)
        )
