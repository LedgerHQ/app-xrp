import os
import pathlib

SCRIPT_DIR = os.path.dirname(os.path.realpath(__file__))

def pytest_generate_tests(metafunc):
    # retrieve the list of .raw files in the testcases directory
    testcases_dir = os.path.join(SCRIPT_DIR, "testcases")
    paths = pathlib.Path(testcases_dir).rglob("*.raw")
    paths = [ str(path) for path in paths ]

    # if a test function has a raw_tx_path parameter, give the list of raw tx
    # paths
    if "raw_tx_path" in metafunc.fixturenames:
        metafunc.parametrize("raw_tx_path", paths, scope="function")
