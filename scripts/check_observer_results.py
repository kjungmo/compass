#!/usr/bin/env python3
"""Check all published ablation cells against their labelled raw CSV groups.

Kept as a focused entry point for existing users/CI. The shared implementation
also checks non-observer metrics, preventing a correct value in another cell or
variant from masking a mismatch. Failures remain enabled with python -O.
"""
import sys

from check_repo_consistency import ROOT, check_result_tables, raw_groups


def main():
    try:
        check_result_tables(ROOT, raw_groups(ROOT))
    except (OSError, ValueError, KeyError, IndexError, TypeError) as exc:
        print(f"RED: {exc}", file=sys.stderr)
        return 1
    print("GREEN: labelled raw CSV -> every R1, Korean draft and English LaTeX aggregate/scenario cell")
    return 0


if __name__ == "__main__":
    sys.exit(main())
