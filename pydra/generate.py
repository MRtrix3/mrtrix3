import os
import sys
import attrs
from pathlib import Path
import subprocess as sp
import typing as ty
from importlib import import_module
import logging
import re
import click
import black.report
import black.parsing
from fileformats.core import FileSet
from fileformats.mrtrix3 import ImageFormat, ImageIn, ImageOut, Tracks
from pydra.engine.helpers import make_klass
from pydra.engine import specs


logging.basicConfig(level=logging.WARNING)

logger = logging.getLogger("pydra-auto-gen")

# Ignore non-standard tools that will need to be added manually
IGNORE = [
    "blend",
    "convert_bruker",
    "gen_scheme",
    "notfound",
]


@click.command(
    help="""Loops through all MRtrix commands to generate Pydra
(https://pydra.readthedocs.io) task interfaces for them

CMD_DIR the command directory to list the commands from

OUTPUT_DIR the output directory to write the generated files to

MRTRIX_VERSION the version of MRTrix the commands are generated for
"""
)
@click.argument(
    "cmd_dir",
    type=click.Path(exists=True, file_okay=False, dir_okay=True, path_type=Path),
)
@click.argument("output_dir", type=click.Path(exists=False, path_type=Path))
@click.argument("mrtrix_version", type=str)
@click.option(
    "--log-errors/--raise-errors",
    type=bool,
    help="whether to log errors (and skip to the next tool) instead of raising them",
)
@click.option(
    "--latest/--not-latest",
    type=bool,
    default=False,
    help="whether to write 'latest' module",
)
def auto_gen_mrtrix3_pydra(
    cmd_dir: Path, mrtrix_version: str, output_dir: Path, log_errors: bool, latest: bool
):
    pkg_version = "v" + "_".join(mrtrix_version.split(".")[:2])

    cmd_dir = cmd_dir.absolute()

    # Insert output dir to path so we can load the generated tasks in order to
    # generate the tests
    sys.path.insert(0, str(output_dir))

    cmds = []

    for cmd_name in sorted(os.listdir(cmd_dir)):
        if cmd_name.startswith("_") or "." in cmd_name or cmd_name in IGNORE:
            continue
        cmd = [str(cmd_dir / cmd_name)]
        cmds.extend(auto_gen_cmd(cmd, cmd_name, output_dir, log_errors, pkg_version))

    # Write init
    init_path = output_dir / "pydra" / "tasks" / "mrtrix3" / pkg_version / "__init__.py"
    imports = "\n".join(f"from .{c}_ import {c}" for c in cmds)
    init_path.write_text(f"# Auto-generated, do not edit\n\n{imports}\n")

    if latest:
        latest_path = output_dir / "pydra" / "tasks" / "mrtrix3" / "latest.py"
        latest_path.write_text(
            f"# Auto-generated, do not edit\n\nfrom .{pkg_version} import *\n"
        )
        print(f"Generated pydra.tasks.mrtrix3.{pkg_version} package")


def auto_gen_cmd(
    cmd: ty.List[str],
    cmd_name: str,
    output_dir: Path,
    log_errors: bool,
    pkg_version: str,
) -> ty.List[str]:
    try:
        code_str = sp.check_output(cmd + ["__print_usage_pydra__"]).decode("utf-8")
    except sp.CalledProcessError:
        if log_errors:
            logger.error("Could not generate interface for '%s'", cmd_name)
            return []
        else:
            raise

    if re.match(r"(\w+,)+\w+", code_str):
        sub_cmds = []
        for algorithm in code_str.split(","):
            sub_cmds.extend(
                auto_gen_cmd(
                    cmd + [algorithm],
                    f"{cmd_name}_{algorithm}",
                    output_dir,
                    log_errors,
                    pkg_version,
                )
            )
        return sub_cmds

    # Since Python identifiers can't start with numbers we need to rename 5tt*
    # with fivett*
    if cmd_name.startswith("5tt"):
        old_name = cmd_name
        cmd_name = escape_cmd_name(cmd_name)
        code_str = code_str.replace(f"class {old_name}", f"class {cmd_name}")
        code_str = code_str.replace(f"{old_name}_input", f"{cmd_name}_input")
        code_str = code_str.replace(f"{old_name}_output", f"{cmd_name}_output")
        code_str = re.sub(r"(?<!\w)5tt_in(?!\w)", "in_5tt", code_str)
    try:
        code_str = black.format_file_contents(
            code_str, fast=False, mode=black.FileMode()
        )
    except black.report.NothingChanged:
        pass
    except black.parsing.InvalidInput:
        if log_errors:
            logger.error("Could not parse generated interface for '%s'", cmd_name)
        else:
            raise
    output_path = (
        output_dir / "pydra" / "tasks" / "mrtrix3" / pkg_version / (cmd_name + "_.py")
    )
    output_path.parent.mkdir(exist_ok=True, parents=True)
    with open(output_path, "w") as f:
        f.write(code_str)
    logger.info("%s", cmd_name)
    auto_gen_test(cmd_name, output_dir, log_errors, pkg_version)
    return [cmd_name]


def auto_gen_test(cmd_name: str, output_dir: Path, log_errors: bool, pkg_version: str):
    tests_dir = output_dir / "pydra" / "tasks" / "mrtrix3" / pkg_version / "tests"
    tests_dir.mkdir(exist_ok=True)
    module = import_module(f"pydra.tasks.mrtrix3.{pkg_version}.{cmd_name}_")
    interface = getattr(module, cmd_name)
    task = interface()

    code_str = f"""# Auto-generated test for {cmd_name}

from fileformats.generic import File  # noqa
from fileformats.medimage import Nifti1  # noqa
from fileformats.mrtrix3 import ImageFormat, ImageIn, Tracks  # noqa
from pydra.tasks.mrtrix3.{pkg_version} import {cmd_name}


def test_{cmd_name}(tmp_path):

    task = {cmd_name}(
"""
    input_fields = attrs.fields(type(task.inputs))
    output_fields = attrs.fields(make_klass(task.output_spec))

    for field in input_fields:
        if field.name in (
            "executable",
            "help",
            "version",
            "quiet",
            "info",
            "nthreads",
            "config",
            "args",
        ):
            continue

        def get_value(type_):
            if type_ is ImageIn:
                value = "Nifti1.sample()"
            elif type_ is Tracks:
                value = "Tracks.sample()"
            elif type_ is int:
                value = "1"
            elif type_ is float:
                value = "1.0"
            elif type_ is str:
                value = '"a-string"'
            elif type_ is bool:
                value = "True"
            elif type_ is Path:
                try:
                    output_field = getattr(output_fields, field.name)
                except AttributeError:
                    pass
                else:
                    output_type = output_field.type
                    if ty.get_origin(output_type) is specs.MultiInputObj:
                        output_type = ty.get_args(output_type)[0]
                    if ty.get_origin(output_type) is tuple:
                        output_type = ty.get_args(output_type)[0]
                    if output_type is ImageOut:
                        output_type = ImageFormat
                value = f"{output_type.__name__}.sample()"
            elif ty.get_origin(type_) is ty.Union:
                value = get_value(ty.get_args(type_)[0])
            elif ty.get_origin(type_) is specs.MultiInputObj:
                value = "[" + get_value(ty.get_args(type_)[0]) + "]"
            elif ty.get_origin(type_) and issubclass(ty.get_origin(type_), ty.Sequence):
                value = (
                    ty.get_origin(type_).__name__
                    + "(["
                    + ", ".join(get_value(a) for a in ty.get_args(type_))
                    + "])"
                )
            elif type_ is ty.Any or issubclass(type_, FileSet):
                value = "File.sample()"
            else:
                raise NotImplementedError
            return value

        if field.default is not attrs.NOTHING:
            value = field.default
        elif "allowed_values" in field.metadata:
            value = field.metadata["allowed_values"][0]
        else:
            value = get_value(field.type)

        code_str += f"        {field.name}={value},\n"

    code_str += """
    )
    result = task(plugin="serial")
    assert not result.errored
"""

    try:
        code_str = black.format_file_contents(
            code_str, fast=False, mode=black.FileMode()
        )
    except black.report.NothingChanged:
        pass
    except black.parsing.InvalidInput:
        if log_errors:
            logger.error("Could not parse generated interface for '%s'", cmd_name)
        else:
            raise

    with open(tests_dir / f"test_{cmd_name}.py", "w") as f:
        f.write(code_str)


def escape_cmd_name(cmd_name: str) -> str:
    return cmd_name.replace("5tt", "fivett")


if __name__ == "__main__":
    from pathlib import Path

    script_dir = Path(__file__).parent

    mrtrix_version = sp.check_output(
        "git describe --tags --abbrev=0", cwd=script_dir, shell=True
    ).decode("utf-8")

    auto_gen_mrtrix3_pydra(
        [
            str(script_dir.parent / "bin"),
            str(script_dir / "src"),
            mrtrix_version,
            "--raise-errors",
            "--latest",
        ]
    )
