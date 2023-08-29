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

"""Rules mope() and mope(test).

* Rule `mope` takes one or more `srcs` and produces matching `outs`.

* Rule `mope_test` uses `mope` to generate outputs and compares them to existing golden files.
"""

load("//mbo/diff:diff.bzl", "diff_test")

def _rule_name(str):
    res = ""
    for pos in range(len(str)):
        if str[pos].isalnum():
            res += str[pos]
        else:
            res += "_"
    return res

def _template_base_no_ext(file):
    # Templates are defined to only accept '.tpl' or '.mope'.
    if type(file) == "string":
        name = file.split(":")[-1]
    else:
        name = file.basename
    return ".".join(name.split(".")[:-1])

def _template_path_no_ext(file):
    # Templates are defined to only accept '.tpl' or '.mope'.
    if type(file) == "string":
        name = file
    else:
        name = file.short_path
    return ".".join(name.split(".")[:-1])

def _gen_name_base_no_ext(file):
    # Remove the optional ".golden" and ".gen" extension and the remaining extension.
    if type(file) == "string":
        name = file.split(":")[-1]
    else:
        name = file.basename
    return name.removesuffix(".golden").removesuffix(".gen")

def _build_relative(ctx, file):
    path = "/".join(ctx.build_file_path.split("/")[:-1])
    return file.short_path.removeprefix(path).removeprefix("/")

def _mope_rule_impl(ctx):
    osrcs = [src.files.to_list()[0] for src in ctx.attr.srcs]
    srcs = {_template_base_no_ext(src): src for src in osrcs}
    outs = {_gen_name_base_no_ext(out): ctx.actions.declare_file(_build_relative(ctx, out)) for out in ctx.outputs.outs}
    if srcs.keys() != outs.keys():
        fail("Files in `srcs` and `outs` do not match on basenames without extensions.")
    for key, src in srcs.items():
        out = outs[key]
        ctx.actions.run(
            arguments = ctx.attr.args + [
                "--template=" + src.path,
                "--generate=" + out.path,
            ],
            executable = ctx.executable._mope_tool,
            inputs = [src],
            mnemonic = "Mope",
            outputs = [out],
            progress_message = "Mope over " + key,
            tools = ctx.attr._mope_tool.files,
            use_default_shell_env = True,
        )
    return [
        DefaultInfo(
            files = depset(ctx.outputs.outs),
            runfiles = ctx.runfiles(files = ctx.outputs.outs),
        ),
    ]

mope = rule(
    attrs = {
        "args": attr.string_list(
            doc = "Args to be passed to the mope tool.",
        ),
        "outs": attr.output_list(
            doc = "List of generated file(s). This must match srcs by matching the basenames without extensions (" +
                  "e.g. a template `foo.cc.tpl` must have an output `foo.cc`).",
            mandatory = True,
        ),
        "srcs": attr.label_list(
            doc = "List of templates.",
            allow_files = [".tpl", ".mope"],
            mandatory = True,
        ),
        "_mope_tool": attr.label(
            doc = "The `mope` tool.",
            default = Label("//mbo/mope"),
            executable = True,
            cfg = "exec",
        ),
    },
    doc = "Expand the given `srcs` template(s) to generate `outs`. For each file in `srcs` the required '.tpl' or " +
          "'.mope' extension will be stripped of. The resulting filename must have a corresponding metch in outs " +
          "when a possible '.gen' or '.golden' extension is stripped of.",
    implementation = _mope_rule_impl,
    output_to_genfiles = True,
    provides = [DefaultInfo],
)

def mope_test(
        name,
        srcs,
        outs,
        args = [],
        **kwargs
    ):
    """Run mope over all `srcs` and compare the results with `outs`.

    Args:
        name: Name of the resulting test_suite
        srcs: The mope template(s).
        outs: The corresponding golden generated files.
        args: Arguments to pass to the mope tool.
        **kwargs: Other args (e.g. tags, visibility).
    """
    gens = {_template_base_no_ext(src): _template_path_no_ext(src) + ".gen" for src in srcs}
    outs = {_gen_name_base_no_ext(out): out for out in outs}
    if gens.keys() != outs.keys():
        fail("Files in `srcs` and `outs` do not match on basenames without extensions.")
    mope(
        name = name + "_mope",
        srcs = srcs,
        outs = gens.values(),
        args = args,
        testonly = 1,
        **kwargs
    )
    tests = []
    for key, gen in gens.items():
        test_name = name + "_" + _rule_name(key) + "_diff_test"
        tests.append(test_name)
        diff_test(
            name = test_name,
            file_old = outs[key],
            file_new = gen,
            failure_message = "The existing old file and the newly generated file differ.",
            **kwargs
        )
    native.test_suite(
        name = name,
        tests = tests,
        **kwargs
    )
