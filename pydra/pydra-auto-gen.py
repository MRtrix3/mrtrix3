import os
from pathlib import Path
import subprocess as sp
import logging
import click
import black.report
import black.parsing

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
    "--log-errors/--raise-errors", type=bool,
    help="whether to log errors (and skip to the next tool) instead of raising them"
)
def auto_gen_mrtrix3_pydra(cmd_dir: Path, output_dir: Path, log_errors: bool):

    env = os.environ.copy()
    env["PATH"] = str(cmd_dir) + ":" + env["PATH"]

    for cmd_name in sorted(os.listdir(cmd_dir)):
        if cmd_name.startswith("_") or "." in cmd_name or cmd_name in IGNORE:
            continue
        try:
            code_str = sp.check_output(
                [cmd_name, "__print_usage_pydra__"], env=env
            ).decode("utf-8")
        except sp.CalledProcessError:
            if log_errors:
                logger.error("Could not generate interface for '%s'", cmd_name)
                continue
            else:
                raise

        if cmd_name.startswith("5tt"):
            code_str == code_str.replace(
                "5tt_input_spec", "five_tissue_type_input_spec"
            )
            code_str == code_str.replace("5tt_put_spec", "five_tissue_type_input_spec")
            cmd_name = cmd_name.replace("5tt", "five_tissue_type")
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


if __name__ == "__main__":
    from pathlib import Path

    script_dir = Path(__file__).parent

    auto_gen_mrtrix3_pydra(
        [
            str(script_dir.parent / "bin"),
            str(script_dir / "src" / "pydra" / "tasks" / "mrtrix3" / "latest"),
            "--log-errors",
        ]
    )
