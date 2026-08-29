#!/usr/bin/env python3
# SPDX-FileCopyrightText: Copyright (c) M. Boerger and the MBO Works authors
# SPDX-License-Identifier: Apache-2.0
"""Select all but the newest GitHub Actions cache matching a key prefix."""

from __future__ import annotations

import json
import sys
from collections.abc import Iterable
from typing import Any


def expired_cache_ids(caches: Iterable[dict[str, Any]], prefix: str) -> list[int]:
    """Return matching cache IDs except the newest one."""
    matching = [cache for cache in caches if cache["key"].startswith(prefix)]
    matching.sort(key=lambda cache: cache["createdAt"])
    return [cache["id"] for cache in matching[:-1]]


def main(argv: list[str]) -> int:
    if len(argv) != 2:
        print(f"Usage: {argv[0]} CACHE_KEY_PREFIX", file=sys.stderr)
        return 2
    caches = json.load(sys.stdin)
    if not isinstance(caches, list):
        raise ValueError("GitHub cache JSON must be an array")
    for cache_id in expired_cache_ids(caches, argv[1]):
        print(cache_id)
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
