import os
import sys
import subprocess
from pathlib import Path

def find_tests(directory: str, extension: str = None) -> list[str]:
    paths = []

    for entry in Path(directory).rglob("*"):
        if not entry.is_file():
            continue

        if extension and entry.suffix != extension:
            continue

        if entry.stem.startswith("LD") and entry.stem.endswith("Test"):
            paths.append(str(entry))

    return paths

def run_tests(tests: list[str]):
    fail_list = []

    for test in tests:
        result = subprocess.run([test])
        print(f"process [{test}] returns {result.returncode}")
        if result.returncode != 0:
            fail_list.append(test)

    return fail_list

if __name__ == '__main__':
    if len(sys.argv) < 2:
        print("usage: python RunTests.py <BuildDirectory>")
        sys.exit(1)

    extension = ".exe" if os.name == "nt" else None
    build_dir = sys.argv[1]
    tests = find_tests(build_dir, extension)
    test_count = len(tests)
    fail_list = run_tests(tests)
    fail_count = len(fail_list)
    pass_count = test_count - fail_count
    print(f"{pass_count}/{test_count} passed")
    if fail_count > 0:
        print("the following tests failed:")
        for test in fail_list:
            print(f"- [{test}]")