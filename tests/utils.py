from pathlib import Path
import json
import re
from typing import Tuple
from struct import unpack

from hashlib import sha256, sha512
from Crypto.Hash import RIPEMD160

from ledgerwallet.params import Bip32Path  # type: ignore [import]

from ecdsa.util import sigdecode_der  # type: ignore [import]
from ecdsa import VerifyingKey, SECP256k1  # type: ignore [import]

from ragger.bip import calculate_public_key_and_chaincode, CurveChoice


ROOT_SCREENSHOT_PATH = Path(__file__).parent.resolve()
DEFAULT_PATH = "44'/144'/0'/0'/0"
DEFAULT_BIP32_PATH = Bip32Path.build(DEFAULT_PATH)
TX_PREFIX_SINGLE = [0x53, 0x54, 0x58, 0x00]
TX_PREFIX_MULTI = [0x53, 0x4D, 0x54, 0x00]


def pop_size_prefixed_buf_from_buf(buffer: bytes) -> Tuple[bytes, int, bytes]:
    """Returns remainder, data_len, data"""

    data_len = buffer[0]
    return buffer[1 + data_len :], data_len, buffer[1 : data_len + 1]


def unpack_configuration_response(reply: bytes) -> str:
    """Unpack reply for 'get_configuration' APDU:
    TEST (1)
    MAJOR (1)
    MINOR (1)
    PATCH (1)
    """

    assert len(reply) == 4
    test, major, minor, patch = unpack("BBBB", reply)
    assert test == 0x00
    version = f"{major}.{minor}.{patch}"
    return version


def unpack_get_public_key_response(reply: bytes) -> Tuple[int, str, int, str]:
    """Unpack reply for 'get_public_key' APDU:
    pub_key (65)
    pub_key_str (65 * 2)
    """

    reply, key_len, key_data = pop_size_prefixed_buf_from_buf(reply)
    chain_data, _, _ = pop_size_prefixed_buf_from_buf(reply)
    return key_len, key_data.hex(), len(chain_data), chain_data.hex()


def verify_version(version: str) -> None:
    """Verify the app version, based on defines in Makefile"""

    print(f"version: {version}")
    parent = Path(ROOT_SCREENSHOT_PATH).parent.resolve()
    makefile = f"{parent}/Makefile"
    print(f"{makefile}")
    with open(makefile, "r", encoding="utf-8") as f_p:
        lines = f_p.readlines()

    version_re = re.compile(r"^APPVERSION_(?P<part>\w)=(?P<val>\d)", re.I)
    vers_dict = {}
    vers_str = ""
    for line in lines:
        info = version_re.match(line)
        if info:
            dinfo = info.groupdict()
            vers_dict[dinfo["part"]] = dinfo["val"]
    try:
        vers_str = f"{vers_dict['M']}.{vers_dict['N']}.{vers_dict['P']}"
    except KeyError:
        pass
    assert version == vers_str


def verify_ecdsa_secp256k1(tx: bytes, sig: bytes, raw_tx_path: str) -> bool:
    """Verify the transaction signature"""

    # Get Public key
    key_data, _ = calculate_public_key_and_chaincode(
        CurveChoice.Secp256k1, DEFAULT_PATH, compress_public_key=True
    )
    pub_key = bytearray.fromhex(key_data)

    # Check single/multi signature
    test_config = Path(raw_tx_path.replace(".raw", ".json"))
    with open(test_config, "r", encoding="utf-8") as fp:
        cfg_data = json.load(fp)
    if "SigningPubKey" in cfg_data and cfg_data["SigningPubKey"] == "":
        hdr = bytes(TX_PREFIX_MULTI)
        # Computes sha256 + ripemd160 hash of pubkey
        hash1 = sha256()
        hash1.update(pub_key)
        key_hash = hash1.digest()

        hash2 = RIPEMD160.new()
        hash2.update(key_hash)
        key_hash = hash2.digest()
        key_hash = key_hash[:20]
    else:
        hdr = bytes(TX_PREFIX_SINGLE)
        key_hash = bytes()

    # Computes the payload with fixed prefix and suffix
    _hash = sha512()
    _hash.update(hdr + tx + key_hash)
    data = _hash.digest()
    # Keep only the 1st 32 bytes
    data = data[:32]

    vk: VerifyingKey = VerifyingKey.from_string(pub_key, SECP256k1, sha256)
    return vk.verify_digest(sig, data, sigdecode_der)
