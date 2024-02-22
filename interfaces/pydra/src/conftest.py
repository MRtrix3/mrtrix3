import os
import pytest
from pathlib import Path

os.environ["PATH"] = str(Path(__file__).parent.parent.parent / "bin") + ":" + os.environ["PATH"]
os.environ["MRTRIX_CLI_PARSE_ONLY"] = "0"


@pytest.fixture
def cli_parse_only():
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
