import pytest
import os
from pathlib import Path
from ragger.conftest import configuration


###########################
### CONFIGURATION START ###
###########################
SCRIPT_DIR = os.path.dirname(os.path.realpath(__file__))
configuration.OPTIONAL.BACKEND_SCOPE = "session"

# Retrieve all test cases
def pytest_generate_tests(metafunc):
    # retrieve the list of .raw files in the testcases directory
    testcases_dir = os.path.join(SCRIPT_DIR, "testcases")
    paths = Path(testcases_dir).rglob("*.raw")
    paths = [str(path) for path in paths]
    # if a test function has a raw_tx_path parameter, give the list of raw tx
    # paths
    if "raw_tx_path" in metafunc.fixturenames:
        metafunc.parametrize("raw_tx_path", paths, scope="function")


#########################
### CONFIGURATION END ###
#########################

# Pull all features from the base ragger conftest using the overridden configuration
pytest_plugins = ("ragger.conftest.base_conftest",)
