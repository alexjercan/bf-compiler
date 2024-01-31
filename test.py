#!/usr/bin/python

import os
import glob
import subprocess
from pathlib import Path


os.system("gcc main.c -o main")
os.makedirs(os.path.join("examples", "asm"), exist_ok=True)

examples = sorted(glob.glob(os.path.join("examples", "src", "*.bf")))

passed = 0
for example in examples:
    name = Path(example).stem
    example_asm = os.path.join("examples", "asm", name + ".s")
    result = subprocess.run(["./main", "-f", example, "-o", example_asm], stderr=subprocess.PIPE)

    example_err = os.path.join("examples", "err", name + ".txt")
    with open(example_err, "r") as f:
        gt_err = f.read()

    result_err = result.stderr.decode("utf-8")
    if gt_err != result_err:
        print(f"Failed in compiler stage for {example}")
        print(f"Expected:\n{gt_err}")
        print(f"Got:\n{result_err}")
        continue

    if gt_err != "" and gt_err == result_err:
        passed += 1
        continue

    example_o = os.path.join("examples", "asm", name + ".o")
    result = subprocess.run(["nasm", "-g", "-felf64", example_asm, "-o", example_o], stderr=subprocess.PIPE)
    if result.returncode != 0:
        print(f"Failed during assembler stage for {example}")
        continue

    result = subprocess.run(["ld", example_o, "-o", os.path.join("examples", "asm", name)], stderr=subprocess.PIPE)
    if result.returncode != 0:
        print(f"Failed during linker stage for {example}")
        continue

    result = subprocess.run([os.path.join("examples", "asm", name)], stdout=subprocess.PIPE)
    if result.returncode != 0:
        print(f"Failed during execution stage for {example}")
        continue

    gt = os.path.join("examples", "out", name + ".txt")
    with open(gt, "r") as f:
        gt_out = f.read()

    result_out = result.stdout.decode("utf-8")
    if gt_out != result_out:
        print(f"Failed during exectuing stage for {example}")
        print(f"Expected:\n{gt_out}")
        print(f"Got:\n{result_out}")
        continue

    passed += 1


print(f"Passed {passed}/{len(examples)} tests")
if passed == len(examples):
    exit(0)
else:
    exit(1)
