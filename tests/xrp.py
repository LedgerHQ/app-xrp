#!/usr/bin/env python3

from typing import Generator
from contextlib import contextmanager
from enum import IntEnum
from time import sleep
from ragger.backend.interface import BackendInterface, RAPDU
from ragger.navigator import NavInsID, NavIns
from ledgerwallet.params import Bip32Path
import pathlib

from ecdsa.util import sigdecode_der
from ecdsa import VerifyingKey, SECP256k1
from hashlib import sha256

DEFAULT_PATH = "44'/144'/0'/0'/0"


class Ins(IntEnum):
    GET_PUBLIC_KEY = 0x02
    SIGN = 0x04


class P1(IntEnum):
    NON_CONFIRM = 0x00
    CONFIRM = 0x01
    FIRST = 0x00
    NEXT = 0x01
    ONLY = 0x00
    MORE = 0x80
    INTER = 0x81


class P2(IntEnum):
    NO_CHAIN_CODE = 0x00
    CHAIN_CODE = 0x01
    CURVE_SECP256K1 = 0x40
    CURVE_ED25519 = 0x80


class Action(IntEnum):
    NAVIGATE = 0
    COMPARE = 1
    NONE = 2


DEFAULT_PATH = "44'/144'/0'/0'/0"


class XRPClient:
    CLA = 0xE0

    def __init__(self, client: BackendInterface, firmware, navigator):
        if not isinstance(client, BackendInterface):
            raise TypeError("client must be an instance of BackendInterface")
        self._client = client
        self._firmware = firmware
        self._navigator = navigator

    def _exchange(self, ins: int, p1: int, p2: int, payload: bytes = b"") -> RAPDU:
        return self._client.exchange(self.CLA, ins, p1=p1, p2=p2, data=payload)

    # Does not work :/
    def verify_ecdsa_secp256k1(self, msg, sig, pub_key):
        vk = VerifyingKey.from_string(
            pub_key, curve=SECP256k1, hashfunc=sha256, validate_point=False
        )
        return vk.verify(sig, msg, sha256, sigdecode=sigdecode_der)

    def get_pubkey(
        self, default_path: bool = True, confirm: bool = False, path: str = ""
    ):
        if confirm:
            p1 = P1.CONFIRM
        else:
            p1 = P1.NON_CONFIRM
        if default_path:
            my_path = Bip32Path.build(DEFAULT_PATH)
        else:
            my_path = path
        reply = self._exchange(
            Ins.GET_PUBLIC_KEY, p1=p1, p2=P2.CURVE_SECP256K1, payload=my_path
        )
        return reply.data[1 : reply.data[0]]

    @contextmanager
    def sign(self, payload, navigate: bool = False, snappath: str = ""):
        chunk_size = 255
        first = True
        while payload:
            if first:
                p1 = P1.FIRST
                first = False
            else:
                p1 = P1.NEXT

            size = min(len(payload), chunk_size)
            if size != len(payload):
                p1 |= P1.MORE

            p2 = P2.CURVE_SECP256K1

            if p1 in [P1.MORE, P1.INTER]:
                self._exchange(ins=Ins.SIGN, p1=p1, p2=p2, payload=payload[:size])
            elif p1 in [P1.FIRST, P1.NEXT]:
                with self._client.exchange_async(
                    self.CLA, ins=Ins.SIGN, p1=p1, p2=p2, data=payload[:size]
                ) as r:
                    if navigate:
                        if self._firmware.device == "stax":
                            self._navigator.navigate_until_text_and_compare(
                                NavInsID.USE_CASE_REVIEW_TAP,
                                [NavInsID.USE_CASE_REVIEW_CONFIRM],
                                "Hold to confirm",
                                pathlib.Path(__file__).parent.resolve(),
                                snappath,
                                screen_change_after_last_instruction=False,
                            )
                        else:
                            text_to_find = "Sign transaction"
                            if self._firmware.device == "nanox":
                                text_to_find = text_to_find[1:]
                            self._navigator.navigate_until_text_and_compare(
                                NavInsID.RIGHT_CLICK,
                                [NavInsID.BOTH_CLICK],
                                text_to_find,
                                pathlib.Path(__file__).parent.resolve(),
                                snappath,
                                screen_change_after_last_instruction=False,
                            )
                    else:
                        pass
            payload = payload[size:]
