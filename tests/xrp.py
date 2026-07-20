from contextlib import contextmanager
from typing import Optional, Tuple
from enum import IntEnum
from ragger.backend.interface import BackendInterface, RAPDU
from ragger.navigator import Navigator
from ragger.utils.misc import split_message
from ragger.bip import pack_derivation_path

from .utils import DEFAULT_BIP32_PATH, unpack_get_public_key_response, unpack_configuration_response
from xrpl.core import addresscodec

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

class TRANSACTION_TYPE(IntEnum):
    TRANSACTION_PAYMENT = 0
    TRANSACTION_ESCROW_CREATE = 1
    TRANSACTION_ESCROW_FINISH = 2
    TRANSACTION_ACCOUNT_SET = 3
    TRANSACTION_ESCROW_CANCEL = 4
    TRANSACTION_SET_REGULAR_KEY = 5
    TRANSACTION_OFFER_CREATE = 7
    TRANSACTION_OFFER_CANCEL = 8
    TRANSACTION_SIGNER_LIST_SET = 12
    TRANSACTION_PAYMENT_CHANNEL_CREATE = 13
    TRANSACTION_PAYMENT_CHANNEL_FUND = 14
    TRANSACTION_PAYMENT_CHANNEL_CLAIM = 15
    TRANSACTION_CHECK_CREATE = 16
    TRANSACTION_CHECK_CASH = 17
    TRANSACTION_CHECK_CANCEL = 18
    TRANSACTION_DEPOSIT_PREAUTH = 19
    TRANSACTION_TRUST_SET = 20
    TRANSACTION_ACCOUNT_DELETE = 21

class STI_FIELDS(IntEnum):
    # Normal field types
    STI_UINT16 = 0x01,
    STI_UINT32 = 0x02,
    STI_HASH128 = 0x04,
    STI_HASH256 = 0x05,
    STI_AMOUNT = 0x06,
    STI_VL = 0x07,
    STI_ACCOUNT = 0x08,
    STI_OBJECT = 0x0E,
    STI_ARRAY = 0x0F,
    STI_UINT8 = 0x10,
    STI_PATHSET = 0x12,

    # Custom field types
    STI_CURRENCY = 0xF0,

# Small collection of used field IDs
class FIELDS_IDS(IntEnum):
    XRP_UINT16_TRANSACTION_TYPE = 0x02
    XRP_UINT32_FLAGS = 0x02
    XRP_UINT32_SEQUENCE = 0x04
    XRP_UINT32_EXPIRATION = 0x0A
    XRP_UINT32_TRANSFER_RATE = 0x0B
    XRP_UINT32_QUALITY_IN = 0x14
    XRP_UINT32_QUALITY_OUT = 0x15
    XRP_UINT32_LAST_LEDGER_SEQUENCE = 0x1B
    XRP_UINT32_SET_FLAG = 0x21
    XRP_UINT32_CLEAR_FLAG = 0x22
    XRP_UINT32_CANCEL_AFTER = 0x24
    XRP_UINT32_FINISH_AFTER = 0x25
    XRP_UINT32_SETTLE_DELAY = 0x27
    XRP_VL_SIGNING_PUB_KEY = 0x03
    XRP_VL_DOMAIN = 0x07
    XRP_VL_MEMO_TYPE = 0x0C
    XRP_VL_MEMO_DATA = 0x0D
    XRP_VL_MEMO_FORMAT = 0x0E
    XRP_ACCOUNT_ACCOUNT = 0x01
    XRP_ACCOUNT_DESTINATION = 0x03
    XRP_ACCOUNT_ISSUER = 0x04
    XRP_ACCOUNT_REGULAR_KEY = 0x08
    XRP_CURRENCY_CURRENCY = 0x01
    XRP_UINT64_AMOUNT = 0x01
    XRP_UINT64_FEE = 0x08

TF_FULLY_CANONICAL_SIG = 0x80000000

XRP_ACCOUNT_SIZE = 20
XRP_CURRENCY_SIZE = 20

XRP_PUBKEY_SIZE = 33

class RippleErrors(IntEnum):
    SW_SWAP_CHECKING_FAIL = 0x6985

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

    def __init__(self, client: BackendInterface, navigator: Optional[Navigator] = None) -> None:
        if not isinstance(client, BackendInterface):
            raise TypeError("client must be an instance of BackendInterface")
        self._client = client
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

    def _craft_simple_tx(self, fees: int, memo: str, destination: str, send_amount: int) -> bytes:
        tx: bytes = b""

        tx += int.to_bytes(STI_FIELDS.STI_UINT16 << 4 | FIELDS_IDS.XRP_UINT16_TRANSACTION_TYPE, length=1, byteorder='big')
        tx += int.to_bytes(TRANSACTION_TYPE.TRANSACTION_PAYMENT, length=2, byteorder='big')

        tx += int.to_bytes(STI_FIELDS.STI_UINT32 << 4 | FIELDS_IDS.XRP_UINT32_FLAGS, length=1, byteorder='big')
        tx += int.to_bytes(TF_FULLY_CANONICAL_SIG, length=4, byteorder='big') # sequence number

        tx += int.to_bytes(STI_FIELDS.STI_UINT32 << 4 | FIELDS_IDS.XRP_UINT32_SEQUENCE, length=1, byteorder='big')
        tx += int.to_bytes(1234, length=4, byteorder='big') # sequence number

        tx += int.to_bytes(STI_FIELDS.STI_UINT32 << 4 | FIELDS_IDS.XRP_VL_MEMO_FORMAT, length=1, byteorder='big')
        tx += int.to_bytes(int(memo), length=4, byteorder='big')

        tx += int.to_bytes(STI_FIELDS.STI_AMOUNT << 4 | FIELDS_IDS.XRP_UINT64_AMOUNT, length=1, byteorder='big')
        tx += int.to_bytes(0x4000000000000000 | send_amount, length=8, byteorder='big')

        tx += int.to_bytes(STI_FIELDS.STI_AMOUNT << 4 | FIELDS_IDS.XRP_UINT64_FEE, length=1, byteorder='big')
        tx += int.to_bytes(0x4000000000000000 | fees, length=8, byteorder='big')

        tx += int.to_bytes(STI_FIELDS.STI_VL << 4 | FIELDS_IDS.XRP_VL_SIGNING_PUB_KEY, length=1, byteorder='big')
        tx += int.to_bytes(XRP_PUBKEY_SIZE, length=1, byteorder='big')
        tx += addresscodec.decode_account_public_key("aBPM4Dk4bxMFEEBx93yU8DF2FSoUt19SDNPcGRsdzr6h9vhhPAGe")

        tx += int.to_bytes(STI_FIELDS.STI_ACCOUNT << 4 | FIELDS_IDS.XRP_ACCOUNT_ACCOUNT, length=1, byteorder='big')
        tx += int.to_bytes(XRP_ACCOUNT_SIZE, length=1, byteorder='big')
        tx += addresscodec.decode_classic_address("rTooLkitCksh5mQa67eaa2JaY7gzNePtD")

        tx += int.to_bytes(STI_FIELDS.STI_ACCOUNT << 4 | FIELDS_IDS.XRP_ACCOUNT_DESTINATION, length=1, byteorder='big')
        tx += int.to_bytes(XRP_ACCOUNT_SIZE, length=1, byteorder='big')
        tx += addresscodec.decode_classic_address(destination)

        return tx

    def send_simple_sign_tx(self, path: str, fees: int, memo: str, destination: str, send_amount: int) -> RAPDU:
        packed_path = pack_derivation_path(path)
        tx = self._craft_simple_tx(fees=fees, memo=memo, destination=destination, send_amount=send_amount)
        return self._client.exchange(self.CLA, Ins.SIGN, P1.ONLY, P2.CURVE_SECP256K1, packed_path + tx)
