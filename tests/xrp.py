from contextlib import contextmanager
from typing import Optional, Tuple
from enum import IntEnum
from ragger.backend.interface import BackendInterface, RAPDU
from ragger.firmware import Firmware
from ragger.navigator import Navigator
from ragger.utils.misc import split_message

from .utils import DEFAULT_BIP32_PATH, unpack_get_public_key_response, unpack_configuration_response


MAX_APDU_LEN: int = 255

class Ins(IntEnum):
    GET_PUBLIC_KEY = 0x02
    SIGN = 0x04
    GET_CONFIGURATION = 0x06


class P1(IntEnum):
    NON_CONFIRM = 0x00
    CONFIRM = 0x01
    ONLY = 0x00
    LAST = 0x01
    FIRST = 0x80
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


class Errors(IntEnum):
    """ Application Errors definitions """
    SW_WRONG_LENGTH             = 0x6700
    SW_MISSING_PARAMETER        = 0x6800
    SW_INTERNAL_1               = 0x6803
    SW_INTERNAL_2               = 0x6807
    SW_INTERNAL_3               = 0x6813
    SW_SECURITY_STATUS          = 0x6982
    SW_WRONG_ADDRESS            = 0x6985
    SW_INVALID_PATH             = 0x6A80
    SW_INVALID_DATA             = 0x6A81
    SW_INVALIDP1P2              = 0x6B00
    SW_UNKNOWN                  = 0x6F00
    SW_SIGN_VERIFY_ERROR        = 0x6F01
    SW_SUCCESS                  = 0x9000


class XRPClient:
    CLA = 0xE0

    def __init__(self, client: BackendInterface, firmware: Firmware, navigator: Navigator) -> None:
        if not isinstance(client, BackendInterface):
            raise TypeError("client must be an instance of BackendInterface")
        self._client = client
        self._firmware = firmware
        self._navigator = navigator

    def _exchange(self,
                  ins: int,
                  p1: int = P1.NON_CONFIRM,
                  p2: int = P2.NO_CHAIN_CODE,
                  data: bytes = b"") -> RAPDU:
        return self._client.exchange(self.CLA, ins, p1=p1, p2=p2, data=data)

    def _exchange_async(self,
                  ins: int,
                  p1: int = P1.NON_CONFIRM,
                  p2: int = P2.NO_CHAIN_CODE,
                  data: bytes = b""):
        return self._client.exchange_async(self.CLA, ins, p1=p1, p2=p2, data=data)

    def get_configuration(self) -> str:
        reply = self._exchange(Ins.GET_CONFIGURATION)
        assert reply.status == Errors.SW_SUCCESS

        return unpack_configuration_response(reply.data)

    def get_pubkey_no_confirm(self, path: bytes = DEFAULT_BIP32_PATH,
                              chain_code: bool = False) -> Tuple[int, str, int, str]:
        p2 = P2.CURVE_SECP256K1
        if chain_code:
            p2 |= P2.CHAIN_CODE  # type: ignore[assignment]
        reply = self._exchange(Ins.GET_PUBLIC_KEY, p2=p2, data=path)
        assert reply.status == Errors.SW_SUCCESS

        return unpack_get_public_key_response(reply.data)

    @contextmanager
    def get_pubkey_confirm(self):
        with self._exchange_async(Ins.GET_PUBLIC_KEY,
                                  p1=P1.CONFIRM,
                                  p2=P2.CURVE_SECP256K1,
                                  data=DEFAULT_BIP32_PATH) as reply:
            yield reply

    @contextmanager
    def sign(self, payload):
        messages = split_message(payload, MAX_APDU_LEN)
        if len(messages) == 1:
            # A single message to send
            p1 = P1.ONLY
        else:
            # Send the 1st message
            p1 = P1.FIRST
            for msg in messages[:-1]:
                self._exchange(Ins.SIGN, p1, P2.CURVE_SECP256K1, msg)
                # Send the intermediate messages
                p1 = P1.INTER
            # Send the last message
            p1 = P1.LAST
        with self._exchange_async(Ins.SIGN, p1, P2.CURVE_SECP256K1, messages[-1]) as reply:
            yield reply

    def get_async_response(self) -> Optional[RAPDU]:
        """ Asynchronous APDU reply """
        return self._client.last_async_response
