"""
./speculos.py --log-level automation:DEBUG ~/app-xrp/bin/app.elf &

export LEDGER_PROXY_ADDRESS=127.0.0.1 LEDGER_PROXY_PORT=9999
pytest-3 -v -s
"""
from pathlib import Path
import pytest
from ledgerwallet.params import Bip32Path  # type: ignore [import]
from ledgered.devices import Device, DeviceType  # type: ignore [import]
from ragger.backend import BackendInterface, RaisePolicy
from ragger.navigator import Navigator, NavInsID, NavIns
from ragger.navigator.navigation_scenario import NavigateWithScenario
from ragger.bip import calculate_public_key_and_chaincode, CurveChoice
from ragger.error import ExceptionRAPDU
from .xrp import XRPClient, Errors
from .utils import DEFAULT_PATH, DEFAULT_BIP32_PATH
from .utils import verify_ecdsa_secp256k1, verify_version


def test_app_configuration(backend: BackendInterface,
                           navigator: Navigator,
                           default_screenshot_path: Path):
    xrp = XRPClient(backend, navigator)
    version = xrp.get_configuration()
    verify_version(default_screenshot_path, version)


def test_sign_too_large(backend: BackendInterface, navigator: Navigator):
    xrp = XRPClient(backend, navigator)
    max_size = 10001
    payload = DEFAULT_BIP32_PATH + b"a" * (max_size - 4)
    try:
        backend.raise_policy = RaisePolicy.RAISE_ALL_BUT_0x9000
        with xrp.sign(payload):
            pass
        pytest.fail("Expected SW_WRONG_LENGTH or SW_INTERNAL_3 but exchange succeeded")
    except ExceptionRAPDU as rapdu:
        assert rapdu.status in [Errors.SW_WRONG_LENGTH, Errors.SW_INTERNAL_3]


def test_sign_invalid_tx(backend: BackendInterface, navigator: Navigator):
    xrp = XRPClient(backend, navigator)
    payload = DEFAULT_BIP32_PATH + b"a" * (40)
    try:
        backend.raise_policy = RaisePolicy.RAISE_ALL_BUT_0x9000
        with xrp.sign(payload):
            pass
        pytest.fail("Expected SW_INTERNAL_1 or SW_INTERNAL_2 but exchange succeeded")
    except ExceptionRAPDU as rapdu:
        assert rapdu.status in [Errors.SW_INTERNAL_1, Errors.SW_INTERNAL_2]


def test_path_too_long(backend: BackendInterface, navigator: Navigator):
    xrp = XRPClient(backend, navigator)
    path = Bip32Path.build(DEFAULT_PATH + "/0/0/0/0/0/0")
    try:
        xrp.get_pubkey_no_confirm(path)
        pytest.fail("Expected SW_INVALID_PATH but exchange succeeded")
    except ExceptionRAPDU as rapdu:
        assert rapdu.status == Errors.SW_INVALID_PATH

def test_path_buffer_bytes_and_length_consistency(backend: BackendInterface, navigator: Navigator):
    xrp = XRPClient(backend, navigator)
    path = Bip32Path.build(DEFAULT_PATH)
    # Manually construct an APDU with inconsistent path length and byte buffer size
    # The path length indicates 5 elements, but we only provide bytes for 4 elements
    payload = path[:-4]  # Remove last 4 bytes to create inconsistency
    try:
        backend.raise_policy = RaisePolicy.RAISE_ALL_BUT_0x9000
        xrp.get_pubkey_no_confirm(path=payload)
        pytest.fail("Expected SW_INVALID_PATH but exchange succeeded")
    except ExceptionRAPDU as rapdu:
        assert rapdu.status == Errors.SW_INVALID_PATH


def test_get_public_key_no_confirm(backend: BackendInterface,
                                   navigator: Navigator):
    xrp = XRPClient(backend, navigator)
    key_len, key_data, chain_len, chain_data = xrp.get_pubkey_no_confirm(chain_code=True)
    ref_public_key, ref_chain_code = calculate_public_key_and_chaincode(
        CurveChoice.Secp256k1, DEFAULT_PATH, compress_public_key=True)
    assert key_data == ref_public_key
    assert chain_data == ref_chain_code
    print(f"   Pub Key[{key_len}]: {key_data}")
    print(f"Chain code[{chain_len}]: {ref_chain_code}")


def test_get_public_key_confirm(backend: BackendInterface,
                                navigator: Navigator,
                                scenario_navigator: NavigateWithScenario):
    xrp = XRPClient(backend, navigator)
    with xrp.get_pubkey_confirm():
        scenario_navigator.address_review_approve()

    # Check the status (Asynchronous)
    reply = xrp.get_async_response()
    assert reply and reply.status == Errors.SW_SUCCESS


def test_get_public_key_reject(backend: BackendInterface,
                               navigator: Navigator,
                               scenario_navigator: NavigateWithScenario):
    xrp = XRPClient(backend, navigator)

    with pytest.raises(ExceptionRAPDU) as err:
        with xrp.get_pubkey_confirm():
            scenario_navigator.address_review_reject()

    # Assert we have received a refusal
    assert err.value.status == Errors.SW_WRONG_ADDRESS
    assert len(err.value.data) == 0


def test_sign_reject(backend: BackendInterface,
                     navigator: Navigator,
                     scenario_navigator: NavigateWithScenario):
    xrp = XRPClient(backend, navigator)

    # pragma pylint: disable=line-too-long
    # Transaction extracted from testcases/01-payment/01-basic.raw
    transaction = "120000228000000024000000036140000000000F424068400000000000000F732102B79DA34F4551CA976B66AA78A55C43707EC2BB2BEC39F95BD53F24E2E45A9E6781140511E17DB83BB6F113939D67BC8EA539EDC926FC83140511E17DB83BB6F113939D67BC8EA539EDC926FC"
    # pragma pylint: enable=line-too-long

    # Convert message to bytes
    message = bytes.fromhex(transaction)

    # Send the APDU (Asynchronous)
    with pytest.raises(ExceptionRAPDU) as err:
        with xrp.sign(DEFAULT_BIP32_PATH + message):
            scenario_navigator.review_reject()

    # Assert we have received a refusal
    assert err.value.status == Errors.SW_WRONG_ADDRESS
    assert len(err.value.data) == 0


_blind_signing_on = [False]


def _enable_blind_signing(device: Device, navigator: Navigator) -> None:
    if _blind_signing_on[0]:
        return
    if device.type is DeviceType.APEX_P:
        coordinates = (263, 95)
    else:
        coordinates = (348, 132)
    navigator.navigate(
        [
            NavInsID.USE_CASE_HOME_SETTINGS,
            NavIns(NavInsID.TOUCH, coordinates),
            NavInsID.USE_CASE_SETTINGS_MULTI_PAGE_EXIT,
        ],
        screen_change_before_first_instruction=False,
    )
    _blind_signing_on[0] = True


def _check_blind_sign_rejection(backend: BackendInterface, xrp: XRPClient, message: bytes) -> None:
    backend.raise_policy = RaisePolicy.RAISE_ALL_BUT_0x9000
    with pytest.raises(ExceptionRAPDU) as e:
        with xrp.sign(DEFAULT_BIP32_PATH + message):
            pass  # error APDU sent synchronously from ui_error_blind_signing()
    assert e.value.status == Errors.SW_WRONG_ADDRESS


def test_blind_signing_disabled_go_to_settings(
        backend: BackendInterface, navigator: Navigator,
        test_name: str, default_screenshot_path: Path) -> None:
    if backend.device.is_nano:
        pytest.skip("This feature does not exist on Nano devices")

    xrp = XRPClient(backend, navigator)
    with open(Path(__file__).parent / "testcases/blind-sign/02-unknown-tx-type.raw", "rb") as f:
        message = f.read()

    _check_blind_sign_rejection(backend, xrp, message)
    navigator.navigate_until_text_and_compare(
        navigate_instruction=NavInsID.USE_CASE_CHOICE_CONFIRM,
        validation_instructions=[NavInsID.USE_CASE_SETTINGS_MULTI_PAGE_EXIT],
        text="^Blind signing$",
        path=default_screenshot_path,
        test_case_name=test_name,
    )


def test_blind_signing_disabled_go_to_menu(
        backend: BackendInterface, navigator: Navigator,
        test_name: str, default_screenshot_path: Path) -> None:
    xrp = XRPClient(backend, navigator)
    with open(Path(__file__).parent / "testcases/blind-sign/02-unknown-tx-type.raw", "rb") as f:
        message = f.read()

    if backend.device.is_nano:
        validation_instructions = [NavInsID.BOTH_CLICK]
        pattern = "Blind signing"
    else:
        validation_instructions = [NavInsID.USE_CASE_CHOICE_REJECT]
        pattern = "Enable blind signing"

    _check_blind_sign_rejection(backend, xrp, message)
    navigator.navigate_until_text_and_compare(
        navigate_instruction=NavInsID.USE_CASE_CHOICE_CONFIRM,
        validation_instructions=validation_instructions,
        text=pattern,
        path=default_screenshot_path,
        test_case_name=test_name,
    )
    backend.wait_for_home_screen()

def test_blind_sign_enabled(backend: BackendInterface,
                            device: Device,
                            navigator: Navigator,
                            scenario_navigator: NavigateWithScenario,
                            blind_sign_raw_tx_path: str):
    """Blind signing enabled: review and approve any blind-sign tx."""
    if backend.device.is_nano:
        pytest.skip("This feature does not exist on Nano devices")
    xrp = XRPClient(backend, navigator)
    with open(blind_sign_raw_tx_path, "rb") as f:
        message = f.read()

    snapname = Path(blind_sign_raw_tx_path).stem

    _enable_blind_signing(device, navigator)

    backend.wait_for_home_screen()
    with xrp.sign(DEFAULT_BIP32_PATH + message):
        scenario_navigator.review_approve_with_warning(
            test_name=f"blind-sign/{snapname}",
        )

    print("Blind signing enabled, user should be able to review and approve the transaction")
    reply = xrp.get_async_response()
    print(f"Received reply: {reply}")
    assert reply and reply.status == Errors.SW_SUCCESS


def test_sign_valid_tx(backend: BackendInterface,
                       device: Device,
                       navigator: Navigator,
                       scenario_navigator: NavigateWithScenario,
                       raw_tx_path: str):
    if raw_tx_path.endswith("19-really-stupid-tx.raw"):
        pytest.skip(f"skip invalid tx from '{Path(raw_tx_path).stem}'")

    xrp = XRPClient(backend, navigator)

    with open(raw_tx_path, "rb") as fp:
        tx = fp.read()

    index = raw_tx_path.index("/testcases/") + len("/testcases/")
    snapdir = str(Path(raw_tx_path[index :]).with_suffix(""))

    backend.wait_for_home_screen()
    if not device.touchable:
        text = "^Sign transaction?"
    else:
        text = "^Hold to sign$"
    with xrp.sign(DEFAULT_BIP32_PATH + tx):
        scenario_navigator.review_approve(test_name=snapdir, custom_screen_text=text)

    reply = xrp.get_async_response()
    assert reply and reply.status == Errors.SW_SUCCESS

    # Verify signature
    verify_ecdsa_secp256k1(tx, reply.data, raw_tx_path)
