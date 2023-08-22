# Created from https://github.com/nipype/pydra-fsl/blob/master/pydra/tasks/fsl/conftest.py

import os
import shutil
import pytest
from tempfile import mkdtemp
from pathlib import Path
from fileformats.generic import File


@File.generate_sample_data.register
def file_generate_sample_data(file: File, dest_dir: Path):
    a_file = dest_dir / "a_file.x"
    a_file.write_text("a sample file")
    return [a_file]

# For debugging in IDE's don't catch raised exceptions and let the IDE
# break at it
if os.getenv("_PYTEST_RAISE", "0") != "0":

    @pytest.hookimpl(tryfirst=True)
    def pytest_exception_interact(call):
        raise call.excinfo.value

    @pytest.hookimpl(tryfirst=True)
    def pytest_internalerror(excinfo):
        raise excinfo.value
