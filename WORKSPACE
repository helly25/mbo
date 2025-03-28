# SPDX-FileCopyrightText: Copyright (c) The helly25 authors (helly25.com)
# SPDX-License-Identifier: Apache-2.0
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

workspace(name = "com_helly25_mbo")

load("//bzl/workspace:load_modules.bzl", "helly25_mbo_load_modules")

helly25_mbo_load_modules()

load("//bzl/workspace:init_modules.bzl", "helly25_mbo_init_modules")

helly25_mbo_init_modules()

################################################################################

load("//bzl/workspace:load_extras.bzl", "helly25_mbo_load_extras")

helly25_mbo_load_extras()  # Adds Hedron + LLVM

load("//bzl/workspace:init_extras.bzl", "helly25_mbo_init_extras")

helly25_mbo_init_extras()  # Init Hedron + LLVM

load("//bzl/workspace:init_extras_llvm.bzl", "helly25_mbo_init_extras_llvm")

helly25_mbo_init_extras_llvm()  # Init LLVM/Part 2
