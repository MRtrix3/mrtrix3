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
from fileformats.mrtrix3 import ImageIn, Tracks
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
"""
)
@click.argument(
    "cmd_dir",
    type=click.Path(exists=True, file_okay=False, dir_okay=True, path_type=Path),
)
@click.argument("output_dir", type=click.Path(exists=False, path_type=Path))
@click.option(
    "--log-errors/--raise-errors",
    type=bool,
    help="whether to log errors (and skip to the next tool) instead of raising them",
)
def auto_gen_mrtrix3_pydra(cmd_dir: Path, output_dir: Path, log_errors: bool):
    cmd_dir = cmd_dir.absolute()

    # Insert output dir to path so we can load the generated tasks in order to
    # generate the tests
    sys.path.insert(0, str(output_dir))

    for cmd_name in sorted(os.listdir(cmd_dir)):
        if cmd_name.startswith("_") or "." in cmd_name or cmd_name in IGNORE:
            continue
        cmd = [str(cmd_dir / cmd_name)]
        auto_gen_cmd(cmd, cmd_name, output_dir, log_errors)

def auto_gen_cmd(cmd: ty.List[str], cmd_name: str, output_dir: Path, log_errors: bool):
    try:
        code_str = sp.check_output(cmd + ["__print_usage_pydra__"]).decode("utf-8")
    except sp.CalledProcessError:
        if log_errors:
            logger.error("Could not generate interface for '%s'", cmd_name)
            return
        else:
            raise

    if re.match(r"(\w+,)+\w+", code_str):
        for algorithm in code_str.split(","):
            auto_gen_cmd(
                cmd + [algorithm], f"{cmd_name}_{algorithm}", output_dir, log_errors
            )
        return

    # Since Python identifiers can't start with numbers we need to rename 5tt*
    # with fivett*
    if cmd_name.startswith("5tt"):
        old_name = cmd_name
        cmd_name = escape_cmd_name(cmd_name)
        code_str = code_str.replace(f"class {old_name}", f"class {cmd_name}")
        code_str = code_str.replace(f"{old_name}_input_spec", f"{cmd_name}_input_spec")
        code_str = code_str.replace(f"{old_name}_output_spec", f"{cmd_name}_output_spec")
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
    output_dir.mkdir(exist_ok=True)
    with open(output_dir / (cmd_name + ".py"), "w") as f:
        f.write(code_str)
    logger.info("%s", cmd_name)
    auto_gen_test(cmd_name, output_dir, log_errors)


def auto_gen_test(cmd_name: str, output_dir: Path, log_errors: bool):
    tests_dir = output_dir / "tests"
    tests_dir.mkdir(exist_ok=True)
    module = import_module(cmd_name)
    interface = getattr(module, cmd_name)
    task = interface()

    code_str = f"""# Auto-generated tests for {cmd_name}
from fileformats.generic import File  # noqa
from fileformats.mrtrix3 import ImageIn, Tracks  # noqa
from .{cmd_name} import {cmd_name}

def test_{cmd_name}(tmp_path):

    task = {cmd_name}(
"""
    input_fields = attrs.fields(type(task.inputs))
    output_fields = attrs.fields(make_klass(task.output_spec))

    for field in input_fields:
        if field.name == "executable":
            continue

        def get_value(type_):
            if type_ is ImageIn:
                value = "NiftiGz.sample()"
            elif type_ is Tracks:
                value = "Tracks.sample()"
            elif type_ is int:
                value = "99999"
            elif type_ is float:
                value = "0.99999"
            elif type_ is str:
                value = '"sample-string"'
            elif type_ is bool:
                value = "True"
            elif type_ is Path:
                ext = ""
                try:
                    output_field = getattr(output_fields, field.name)
                except AttributeError:
                    pass
                else:
                    try:
                        ext = output_field.type.strext
                    except AttributeError:
                        pass
                value = f"tmp_path / {field.name}{ext}"
            elif ty.get_origin(type_) is specs.MultiInputObj:
                value = get_value(ty.get_args(type_)[0])
            elif ty.get_origin(type_) and issubclass(ty.get_origin(type_), ty.Sequence):
                value = (
                    ty.get_origin(type_).__name__
                    + "("
                    + ", ".join(get_value(a) for a in ty.get_args(type_))
                    + ")"
                )
            elif type_ is ty.Any or issubclass(type_, FileSet):
                value = "File.sample()"
            else:
                raise NotImplementedError
            return value

        if field.default is not attrs.NOTHING:
            value = field.default
        else:
            value = get_value(field.type)

        code_str += f"        {field.name}={value},\n"

    code_str += """
    )
    result = task(plugin="serial")
    assert result.exit_code == 0
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
    pkg_version = "v" + "_".join(mrtrix_version.split(".")[:2])

    auto_gen_mrtrix3_pydra(
        [
            str(script_dir.parent / "bin"),
            str(script_dir / "src" / "pydra" / "tasks" / "mrtrix3" / pkg_version),
            "--raise-errors",
        ]
    )

    latest = script_dir / "src" / "pydra" / "tasks" / "mrtrix3" / "latest.py"
    latest.write_text(
        f"# Auto-generated, do not edit\n\nfrom .{pkg_version} import *\n"
    )
    print(f"Generated pydra.tasks.mrtrix3.{pkg_version} package")
