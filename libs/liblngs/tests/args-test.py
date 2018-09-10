#!/usr/bin/env python

from __future__ import print_function
import sys, subprocess

exe = sys.argv[1]

print(exe)

def mktest(result, title):
  return (int(result), title)

def tests():
  p = subprocess.Popen([exe], stdout=subprocess.PIPE)
  out, err = p.communicate()
  if p.returncode:
    sys.exit(p.returncode)

  return [mktest(*line.split(':', 1)) for line in out.split('\n') if line]

tests_failed = 0
test_id = 0
tests = tests()
for result, title in tests:
  print("[ RUN      ]", title)
  sys.stdout.flush()
  p = subprocess.Popen([exe, "%s" % test_id])
  test_id += 1
  p.communicate()
  failed = p.returncode != 0
  should_fail = result != 0
  if should_fail != failed:
    tests_failed += 1
    print('''Expected equality of these values:
  expected
    Which is: {result}
  actual
    Which is: {returncode}
[  FAILED  ]'''.format(result = result, returncode=p.returncode), title)
  else:
    print("[       OK ]", title)

print("Result: {failed}/{total} failed".format(failed=tests_failed, total=len(tests)))
sys.exit(tests_failed)