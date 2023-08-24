# Created from https://github.com/nipype/pydra-fsl/blob/master/pydra/tasks/fsl/conftest.py

import os
import pytest
from pathlib import Path
from fileformats.generic import File


@File.generate_sample_data.register
def file_generate_sample_data(file: File, dest_dir: Path):
    a_file = dest_dir / "a_file.x"
    a_file.write_text("a sample file")
    return [a_file]


@pytest.fixture(autouse=True)
def set_cli_parse_only():
    os.environ['MRTRIX_CLI_PARSE_ONLY'] = '1'
    # You can set more environment variables here if needed
    yield
    # Clean up or reset environment variables if necessary
    del os.environ['MRTRIX_CLI_PARSE_ONLY']


# For debugging in IDE's don't catch raised exceptions and let the IDE
# break at it
if os.getenv("_PYTEST_RAISE", "0") != "0":

    @pytest.hookimpl(tryfirst=True)
    def pytest_exception_interact(call):
        raise call.excinfo.value

    @pytest.hookimpl(tryfirst=True)
    def pytest_internalerror(excinfo):
        raise excinfo.value
