"""
./speculos.py --log-level automation:DEBUG ~/app-xrp/bin/app.elf &

export LEDGER_PROXY_ADDRESS=127.0.0.1 LEDGER_PROXY_PORT=9999
pytest-3 -v -s
"""
import os
import pathlib
import pytest
from ledgerwallet.params import Bip32Path  # type: ignore [import]
from ragger.backend import RaisePolicy
from ragger.error import ExceptionRAPDU
from .xrp import XRPClient, DEFAULT_PATH


def test_sign_too_large(backend, firmware, navigator):
    xrp = XRPClient(backend, firmware, navigator)
    max_size = 10001
    path = Bip32Path.build(DEFAULT_PATH)
    payload = path + b"a" * (max_size - 4)
    try:
        backend.raise_policy = RaisePolicy.RAISE_ALL_BUT_0x9000
        xrp.sign(payload, False)
    except ExceptionRAPDU as rapdu:
        assert rapdu.status in [0x6700, 0x6813]


def test_sign_invalid_tx(backend, firmware, navigator):
    xrp = XRPClient(backend, firmware, navigator)
    path = Bip32Path.build(DEFAULT_PATH)
    payload = path + b"a" * (40)
    try:
        backend.raise_policy = RaisePolicy.RAISE_ALL_BUT_0x9000
        xrp.sign(payload, False)
    except ExceptionRAPDU as rapdu:
        assert rapdu.status in [0x6803, 0x6807]


def test_path_too_long(backend, firmware, navigator):
    xrp = XRPClient(backend, firmware, navigator)
    path = Bip32Path.build(DEFAULT_PATH + "/0/0/0/0/0/0")
    try:
        xrp.get_pubkey(default_path=False, path=path)
    except ExceptionRAPDU as rapdu:
        assert rapdu.status == 0x6A80


def test_get_public_key(backend, firmware, navigator):
    xrp = XRPClient(backend, firmware, navigator)
    xrp.get_pubkey()


def test_sign_valid_tx_and_compare_screens(backend, raw_tx_path, firmware, navigator):
    xrp = XRPClient(backend, firmware, navigator)

    prefix = (
        os.path.dirname(os.path.realpath(__file__)) + f"/snapshots/{firmware.device}/"
    )
    full_snappath = pathlib.Path(
        raw_tx_path.replace("/testcases/", f"/snapshots/{firmware.device}/")
    ).with_suffix("")
    no_prefix_snappath = str(full_snappath)[len(prefix) :]

    if raw_tx_path.endswith("19-really-stupid-tx.raw"):
        pytest.skip(f"skip invalid tx {raw_tx_path}")

    with open(raw_tx_path, "rb") as fp:
        tx = fp.read()

    path = Bip32Path.build(DEFAULT_PATH)
    payload = path + tx

    backend.wait_for_home_screen()
    xrp.sign(payload, True, no_prefix_snappath)
    assert backend.last_async_response.status == 0x9000

    # Verify tx signature (Does not work...)
    # key = xrp.get_pubkey()
    # print(f"RECEIVED PUBKEY : {key}")
    # print(f"RECEIVED PUBKEY binascii: {binascii.hexlify(key)}")
    # print(f"RECEIVED PUBKEY binascii str:")
    # print(str(binascii.hexlify(key), encoding="utf-8"))
    # xrp.verify_ecdsa_secp256k1(tx, signature, str(binascii.hexlify(b'22'), encoding="utf-8"))
