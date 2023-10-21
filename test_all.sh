#!/bin/bash

(\
  bazel test //... && \
  bazel test //... -c dbg && \
  bazel test //... -c opt && \
  bazel test //... --config=asan && \
  bazel test //... --config=asan -c dbg && \
  bazel test //... --config=asan -c opt \
) && echo "PASS" || echo "FAIL"