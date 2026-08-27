from ledger_app_clients.exchange.cal_helper import CurrencyConfiguration
from ragger.bip import pack_derivation_path
from ragger.utils import create_currency_config

from ..standalone.utils import DEFAULT_PATH

# Define a configuration for each currency used in our tests: native coins and tokens

# XRP token currency definition
XRP_CONF = create_currency_config("XRP", "XRP")
# Serialized derivation path for the XRP app
XRP_PACKED_DERIVATION_PATH = pack_derivation_path("m/" + DEFAULT_PATH)
# Coin configuration mock as stored in CAL for the SWAP feature
XRP_CURRENCY_CONFIGURATION = CurrencyConfiguration(ticker="XRP", conf=XRP_CONF, packed_derivation_path=XRP_PACKED_DERIVATION_PATH)
