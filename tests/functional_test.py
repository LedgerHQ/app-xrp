"""
./speculos.py --log-level automation:DEBUG ~/app-xrp/bin/app.elf &

export LEDGER_PROXY_ADDRESS=127.0.0.1 LEDGER_PROXY_PORT=9999
pytest-3 -v -s
"""
from pathlib import Path
import pytest
from ledgerwallet.params import Bip32Path  # type: ignore [import]
from ragger.backend import BackendInterface, RaisePolicy
from ragger.firmware import Firmware
from ragger.navigator import Navigator
from ragger.navigator.navigation_scenario import NavigateWithScenario
from ragger.bip import calculate_public_key_and_chaincode, CurveChoice
from ragger.error import ExceptionRAPDU
from .xrp import XRPClient, Errors
from .utils import DEFAULT_PATH, DEFAULT_BIP32_PATH
from .utils import ROOT_SCREENSHOT_PATH, verify_ecdsa_secp256k1, verify_version


def test_app_configuration(backend: BackendInterface, firmware: Firmware, navigator: Navigator):
    xrp = XRPClient(backend, firmware, navigator)
    version = xrp.get_configuration()
    verify_version(version)


def test_sign_too_large(backend: BackendInterface, firmware: Firmware, navigator: Navigator):
    xrp = XRPClient(backend, firmware, navigator)
    max_size = 10001
    payload = DEFAULT_BIP32_PATH + b"a" * (max_size - 4)
    try:
        backend.raise_policy = RaisePolicy.RAISE_ALL_BUT_0x9000
        xrp.sign(payload)
    except ExceptionRAPDU as rapdu:
        assert rapdu.status in [Errors.SW_WRONG_LENGTH, Errors.SW_INTERNAL_3]


def test_sign_invalid_tx(backend: BackendInterface, firmware: Firmware, navigator: Navigator):
    xrp = XRPClient(backend, firmware, navigator)
    payload = DEFAULT_BIP32_PATH + b"a" * (40)
    try:
        backend.raise_policy = RaisePolicy.RAISE_ALL_BUT_0x9000
        xrp.sign(payload)
    except ExceptionRAPDU as rapdu:
        assert rapdu.status in [Errors.SW_INTERNAL_1, Errors.SW_INTERNAL_2]


def test_path_too_long(backend: BackendInterface, firmware: Firmware, navigator: Navigator):
    xrp = XRPClient(backend, firmware, navigator)
    path = Bip32Path.build(DEFAULT_PATH + "/0/0/0/0/0/0")
    try:
        xrp.get_pubkey_no_confirm(path)
    except ExceptionRAPDU as rapdu:
        assert rapdu.status == Errors.SW_INVALID_PATH


def test_get_public_key_no_confirm(backend: BackendInterface,
                                   firmware: Firmware,
                                   navigator: Navigator):
    xrp = XRPClient(backend, firmware, navigator)
    key_len, key_data, chain_len, chain_data = xrp.get_pubkey_no_confirm(chain_code=True)
    ref_public_key, ref_chain_code = calculate_public_key_and_chaincode(
        CurveChoice.Secp256k1, DEFAULT_PATH, compress_public_key=True)
    assert key_data == ref_public_key
    assert chain_data == ref_chain_code
    print(f"   Pub Key[{key_len}]: {key_data}")
    print(f"Chain code[{chain_len}]: {ref_chain_code}")


def test_get_public_key_confirm(backend: BackendInterface,
                                firmware: Firmware,
                                navigator: Navigator,
                                scenario_navigator: NavigateWithScenario,
                                test_name: str):
    xrp = XRPClient(backend, firmware, navigator)
    with xrp.get_pubkey_confirm():
        scenario_navigator.address_review_approve(ROOT_SCREENSHOT_PATH, test_name)

    # Check the status (Asynchronous)
    reply = xrp.get_async_response()
    assert reply and reply.status == Errors.SW_SUCCESS


def test_get_public_key_reject(backend: BackendInterface,
                               firmware: Firmware,
                               navigator: Navigator,
                               scenario_navigator: NavigateWithScenario,
                               test_name: str):
    xrp = XRPClient(backend, firmware, navigator)

    with pytest.raises(ExceptionRAPDU) as err:
        with xrp.get_pubkey_confirm():
            scenario_navigator.address_review_reject(ROOT_SCREENSHOT_PATH, test_name)

    # Assert we have received a refusal
    assert err.value.status == Errors.SW_WRONG_ADDRESS
    assert len(err.value.data) == 0


def test_sign_reject(backend: BackendInterface,
                     firmware: Firmware,
                     navigator: Navigator,
                     scenario_navigator: NavigateWithScenario,
                     test_name: str):
    xrp = XRPClient(backend, firmware, navigator)

    # pragma pylint: disable=line-too-long
    # Transaction extracted from testcases/01-payment/01-basic.raw
    transaction = "120000228000000024000000036140000000000F424068400000000000000F732102B79DA34F4551CA976B66AA78A55C43707EC2BB2BEC39F95BD53F24E2E45A9E6781140511E17DB83BB6F113939D67BC8EA539EDC926FC83140511E17DB83BB6F113939D67BC8EA539EDC926FC"
    # pragma pylint: enable=line-too-long

    # Convert message to bytes
    message = bytes.fromhex(transaction)

    # Send the APDU (Asynchronous)
    with pytest.raises(ExceptionRAPDU) as err:
        with xrp.sign(DEFAULT_BIP32_PATH + message):
            scenario_navigator.review_reject(ROOT_SCREENSHOT_PATH, test_name)

    # Assert we have received a refusal
    assert err.value.status == Errors.SW_WRONG_ADDRESS
    assert len(err.value.data) == 0


def test_sign_valid_tx(backend: BackendInterface,
                       firmware: Firmware,
                       navigator: Navigator,
                       scenario_navigator: NavigateWithScenario,
                       raw_tx_path: str):
    if raw_tx_path.endswith("19-really-stupid-tx.raw"):
        pytest.skip(f"skip invalid tx {raw_tx_path}")

    xrp = XRPClient(backend, firmware, navigator)

    with open(raw_tx_path, "rb") as fp:
        tx = fp.read()

    index = raw_tx_path.index("/testcases/") + len("/testcases/")
    snapdir = str(Path(raw_tx_path[index :]).with_suffix(""))

    backend.wait_for_home_screen()
    if firmware.device.startswith("nano"):
        text = "^Sign transaction$"
    else:
        text = "^Hold to sign$"
    with xrp.sign(DEFAULT_BIP32_PATH + tx):
        scenario_navigator.review_approve(ROOT_SCREENSHOT_PATH, snapdir, text)

    reply = xrp.get_async_response()
    assert reply and reply.status == Errors.SW_SUCCESS

    # Verify signature
    verify_ecdsa_secp256k1(tx, reply.data, raw_tx_path)
