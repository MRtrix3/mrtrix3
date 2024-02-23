#!/usr/bin/env python3
import os
import sys
import attrs
from pathlib import Path
import subprocess as sp
import typing as ty
from importlib import import_module
import logging
from traceback import format_exc
import re
from tqdm import tqdm
import click
import black.report
import black.parsing
from fileformats.core import FileSet
from fileformats.medimage_mrtrix3 import ImageFormat, ImageIn, ImageOut, Tracks
from pydra.engine.helpers import make_klass
from pydra.engine import specs


logger = logging.getLogger("pydra-auto-gen")

# Ignore non-standard tools that will need to be added manually
IGNORE = [
    "blend",
    "convert_bruker",
    "gen_scheme",
    "notfound",
    "for_each",
    "mrview",
    "shview",
]


CMD_PREFIXES = [
    "fivett",
    "afd",
    "amp",
    "connectome",
    "dcm",
    "dir",
    "dwi",
    "fixel",
    "fod",
    "label",
    "mask",
    "mesh",
    "mr",
    "mt",
    "peaks",
    "sh",
    "tck",
    "tensor",
    "transform",
    "tsf",
    "voxel",
    "vector",
    "warp",
    "response",
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
@click.option(
    "--log-level",
    type=click.Choice(["debug", "info", "warning", "error", "critical"]),
    default="warning",
    help="logging level",
)
def auto_gen_mrtrix3_pydra(
    cmd_dir: Path,
    mrtrix_version: str,
    output_dir: Path,
    log_errors: bool,
    latest: bool,
    log_level: str,
):

    logging.basicConfig(level=getattr(logging, log_level.upper()))

    pkg_version = "v" + "_".join(mrtrix_version.split(".")[:2])

    cmd_dir = cmd_dir.absolute()

    # Insert output dir to path so we can load the generated tasks in order to
    # generate the tests
    sys.path.insert(0, str(output_dir))

    manual_cmds = []
    manual_path = output_dir / "pydra" / "tasks" / "mrtrix3" / "manual"
    if manual_path.exists():
        for manual_file in manual_path.iterdir():
            manual_cmd = manual_file.stem[:-1]
            if not manual_cmd.startswith(".") and not manual_cmd.startswith("__"):
                manual_cmds.append(manual_cmd)

    cmds = []
    for cmd_name in tqdm(
        sorted(os.listdir(cmd_dir)),
        "generating Pydra interfaces for all MRtrix3 commands",
    ):
        if (
            cmd_name.startswith("_")
            or "." in cmd_name
            or cmd_name in IGNORE
            or cmd_name in manual_cmds
        ):
            continue
        cmd = [str(cmd_dir / cmd_name)]
        cmds.extend(
            auto_gen_cmd(cmd, cmd_name, output_dir, cmd_dir, log_errors, pkg_version)
        )

    # Write init
    init_path = output_dir / "pydra" / "tasks" / "mrtrix3" / pkg_version / "__init__.py"
    imports = "\n".join(f"from .{c}_ import {pascal_case_task_name(c)}" for c in cmds)
    imports += "\n" + "\n".join(
        f"from ..manual.{c}_ import {pascal_case_task_name(c)}" for c in manual_cmds
    )
    init_path.write_text(f"# Auto-generated, do not edit\n\n{imports}\n")

    if latest:
        latest_path = output_dir / "pydra" / "tasks" / "mrtrix3" / "latest.py"
        latest_path.write_text(
            f"# Auto-generated, do not edit\n\nfrom .{pkg_version} import *\n"
        )
        print(f"Generated pydra.tasks.mrtrix3.{pkg_version} package")

    # Test out import
    import_module(f"pydra.tasks.mrtrix3.{pkg_version}")


def auto_gen_cmd(
    cmd: ty.List[str],
    cmd_name: str,
    output_dir: Path,
    cmd_dir: Path,
    log_errors: bool,
    pkg_version: str,
) -> ty.List[str]:
    base_cmd = str(cmd_dir / cmd[0])
    cmd = [base_cmd] + cmd[1:]
    try:
        code_str = sp.check_output(cmd + ["__print_usage_pydra__"]).decode("utf-8")
    except sp.CalledProcessError:
        if log_errors:
            logger.error("Could not generate interface for '%s'", cmd_name)
            logger.error(format_exc())
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
                    cmd_dir,
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
            logger.error(format_exc())
            return []
        else:
            raise
    output_path = (
        output_dir / "pydra" / "tasks" / "mrtrix3" / pkg_version / (cmd_name + "_.py")
    )
    output_path.parent.mkdir(exist_ok=True, parents=True)
    with open(output_path, "w") as f:
        f.write(code_str)
    logger.info("%s", cmd_name)
    try:
        auto_gen_test(cmd_name, output_dir, log_errors, pkg_version)
    except Exception:
        if log_errors:
            logger.error("Test generation failed for '%s'", cmd_name)
            logger.error(format_exc())
            return []
        else:
            raise
    return [cmd_name]


def auto_gen_test(cmd_name: str, output_dir: Path, log_errors: bool, pkg_version: str):
    tests_dir = output_dir / "pydra" / "tasks" / "mrtrix3" / pkg_version / "tests"
    tests_dir.mkdir(exist_ok=True)
    module = import_module(f"pydra.tasks.mrtrix3.{pkg_version}.{cmd_name}_")
    interface = getattr(module, pascal_case_task_name(cmd_name))
    task = interface()

    code_str = f"""# Auto-generated test for {cmd_name}

from fileformats.generic import File, Directory, FsObject  # noqa
from fileformats.medimage import Nifti1  # noqa
from fileformats.medimage_mrtrix3 import ImageFormat, ImageIn, Tracks  # noqa
from pydra.tasks.mrtrix3.{pkg_version} import {cmd_name}


def test_{cmd_name.lower()}(tmp_path, cli_parse_only):

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
                    if ty.get_origin(output_type) in (list, tuple):
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
            value = repr(field.metadata["allowed_values"][0])
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

    with open(tests_dir / f"test_{cmd_name}.py", "w") as f:
        f.write(code_str)


def escape_cmd_name(cmd_name: str) -> str:
    return cmd_name.replace("5tt", "fivett")


def pascal_case_task_name(cmd_name: str) -> str:
    # convert to PascalCase
    if cmd_name == "population_template":
        return "PopulationTemplate"
    try:
        return "".join(
            g.capitalize()
            for g in re.match(
                rf"({'|'.join(CMD_PREFIXES)}?)(2?)([^_]+)(_?)(.*)", cmd_name
            ).groups()
        )
    except AttributeError as e:
        raise ValueError(
            f"Could not convert {cmd_name} to PascalCase, please add its prefix to CMD_PREFIXES"
        ) from e


if __name__ == "__main__":
    from pathlib import Path

    script_dir = Path(__file__).parent

    mrtrix_version = sp.check_output(
        "git describe --tags --abbrev=0", cwd=script_dir, shell=True
    ).decode("utf-8")

    auto_gen_mrtrix3_pydra(sys.argv[1:])
