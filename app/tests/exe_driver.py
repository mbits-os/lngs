import json
import os
import re
import shlex
import subprocess
import sys
from difflib import unified_diff
from stat import S_IREAD, S_IRGRP, S_IROTH

env = {name: os.environ[name] for name in os.environ}

driven = sys.argv[1]
if os.name == "nt":
    driven = driven.replace("/", "\\")
datadir = os.path.abspath(sys.argv[2])
sharedir = sys.argv[3]

stat = os.stat(os.path.join(datadir, "readonly.file"))
os.chmod(os.path.join(datadir, "readonly.file"), S_IREAD | S_IRGRP | S_IROTH)

testsuite = []
for root, dirs, files in os.walk(os.path.join(datadir, "lngs_tests")):
    dirs[:] = []
    for filename in files:
        testsuite.append(os.path.join(root, filename))

l = len(testsuite)
digits = 1
while l > 9:
    digits += 1
    l = l // 10

had_errors = False
counter = 0

flds = ["Return code", "Standard out", "Standard err"]

MSVC_LN_COL = re.compile(r"^\s*([^(]+)\(([0-9]+),([0-9]+)\):(.*)$")


def fix(input, patches, has_sources):
    if os.name == "nt":
        input = input.replace(b"\r\n", b"\n")
    input = input.decode("UTF-8")
    input = input.replace(datadir, "$DATA")
    if os.name == "nt":
        input = input.replace("$DATA\\", "$DATA/")
    input = input.replace(sharedir, "$SHARE")
    if not len(patches) and not has_sources:
        return input

    input = input.split("\n")
    if has_sources and os.name == "nt":
        for lineno in range(len(input)):
            match = MSVC_LN_COL.match(input[lineno])
            if match:
                input[lineno] = "{}:{}:{}:{}".format(
                    match.group(1), match.group(2), match.group(3), match.group(4)
                )
    for patch in patches:
        patched = patches[patch]
        ptrn = re.compile(patch)
        for lineno in range(len(input)):
            if ptrn.match(input[lineno]):
                input[lineno] = patched
    return "\n".join(input)


def last_enter(s):
    if len(s) and s[-1] == "\n":
        s = s[:-1] + "\\n"
    return s + "\n"


def diff(expected, actual):
    expected = last_enter(expected).splitlines(keepends=True)
    actual = last_enter(actual).splitlines(keepends=True)
    return "".join(list(unified_diff(expected, actual))[2:])


for filename in sorted(testsuite):
    counter += 1
    with open(filename) as f:
        data = json.load(f)
    try:
        args, name, expected = data["args"], data["name"], data["expected"]
    except KeyError:
        continue

    patches = data.get("patches", {})
    lang = data.get("lang", "en")
    has_sources = data.get("has-sources", False)
    skippable = data.get("skippable", False)

    env["LANGUAGE"] = lang

    expanded = [arg.replace("$DATA", datadir) for arg in args]

    print(f"[{counter:>{digits}}/{len(testsuite)}] {repr(name)}")
    proc = subprocess.run([driven, *expanded], capture_output=True, env=env)
    actual = [
        proc.returncode,
        fix(proc.stdout, patches, has_sources),
        fix(proc.stderr, patches, False),
    ]
    if expected is None:
        data["expected"] = actual
        with open(filename, "w") as f:
            json.dump(data, f, separators=(", ", ": "))
        print(f"[{counter:>{digits}}/{len(testsuite)}] {repr(name)} saved")
    elif actual == expected:
        print(f"[{counter:>{digits}}/{len(testsuite)}] {repr(name)} PASSED")
    elif skippable:
        print(f"[{counter:>{digits}}/{len(testsuite)}] {repr(name)} SKIPPED")
    else:
        for ndx in range(len(actual)):
            if actual[ndx] != expected[ndx]:
                if ndx:
                    print(
                        f"{flds[ndx]}\n  Expected:\n    {repr(expected[ndx])}\n  Actual:\n    {repr(actual[ndx])}\nDiff:\n{diff(expected[ndx], actual[ndx])}"
                    )
                else:
                    print(
                        f"{flds[ndx]}\n  Expected:\n    {repr(expected[ndx])}\n  Actual:\n    {repr(actual[ndx])}"
                    )
        print(" ".join([shlex.quote(arg) for arg in [driven, *expanded]]))
        print(f"[{counter:>{digits}}/{len(testsuite)}] {repr(name)} **FAILED**")
        had_errors = True


os.chmod(os.path.join(datadir, "readonly.file"), stat.st_mode)

if had_errors:
    sys.exit(1)
