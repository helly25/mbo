#!/bin/bash

set -e

bazel run @hedron_compile_commands//:refresh_all && echo "OK" || echo "FAIL"
