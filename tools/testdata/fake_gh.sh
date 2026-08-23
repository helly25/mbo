#!/usr/bin/env bash

# SPDX-FileCopyrightText: Copyright (c) The helly25 authors (helly25.com)
# SPDX-License-Identifier: Apache-2.0

set -euo pipefail

printf '%s\n' "${*}" >>"${GH_LOG}"
