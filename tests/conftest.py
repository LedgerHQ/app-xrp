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
    testcases_dir = os.path.join(SCRIPT_DIR, "testcases")

    if "raw_tx_path" in metafunc.fixturenames:
        paths = [
            str(p)
            for p in sorted(Path(testcases_dir).rglob("*.raw"))
            if "/blind-sign/" not in str(p)
        ]
        metafunc.parametrize("raw_tx_path", paths, scope="function")

    if "blind_sign_raw_tx_path" in metafunc.fixturenames:
        paths = [
            str(p) for p in sorted((Path(testcases_dir) / "blind-sign").glob("*.raw"))
        ]
        metafunc.parametrize("blind_sign_raw_tx_path", paths, scope="function")


#########################
### CONFIGURATION END ###
#########################

# Pull all features from the base ragger conftest using the overridden configuration
pytest_plugins = ("ragger.conftest.base_conftest",)
