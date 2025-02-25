# SPDX-FileCopyrightText: Copyright (c) The helly25/mbo authors (helly25.com)
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

"""Generator for //mbo/container:internal/limited_ordered_config.bzl"."""

load("@bazel_skylib//rules:common_settings.bzl", "BuildSettingInfo")

def _config_gen_impl(ctx):
    limited_ordered_max_unroll_capacity = ctx.attr._limited_ordered_max_unroll_capacity[BuildSettingInfo].value
    require_throws = ctx.attr._require_throws[BuildSettingInfo].value
    ctx.actions.expand_template(
        template = ctx.file.template,
        output = ctx.outputs.output,
        substitutions = {
            "kRequireThrows = false;": "kRequireThrows = " + repr(require_throws).lower() + ";",
            "kUnrollMaxCapacityDefault = 16;": "kUnrollMaxCapacityDefault = " + repr(limited_ordered_max_unroll_capacity) + ";",
        },
    )

config_gen = rule(
    attrs = {
        "output": attr.output(
            mandatory = True,
        ),
        "template": attr.label(
            allow_single_file = True,
            mandatory = True,
        ),
        "_limited_ordered_max_unroll_capacity": attr.label(default = Label("//mbo/config:limited_ordered_max_unroll_capacity")),
        "_require_throws": attr.label(default = Label("//mbo/config:require_throws")),
    },
    implementation = _config_gen_impl,
)
