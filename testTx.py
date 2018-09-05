#!/usr/bin/env python
"""
*******************************************************************************
*   Ledger Blue
*   (c) 2016 Ledger
*
*  Licensed under the Apache License, Version 2.0 (the "License");
*  you may not use this file except in compliance with the License.
*  You may obtain a copy of the License at
*
*      http://www.apache.org/licenses/LICENSE-2.0
*
*  Unless required by applicable law or agreed to in writing, software
*  distributed under the License is distributed on an "AS IS" BASIS,
*  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*  See the License for the specific language governing permissions and
*  limitations under the License.
********************************************************************************
"""
from ledgerblue.comm import getDongle
from ledgerblue.commException import CommException
import argparse
import struct

#TEST_TX = "12000022800000002400000001201B000F4D98614000000000A7D8C068400000000000000C7321ED7810474C1DBEFAB9B271346E95ECA5CF931B5B8A5CDDEDAA2E73EB97BB72D796811444CC4E9B00F8668E5F642043E2206DB9BEFC65E5831439E57D524E0AF4110DBEDB32B0A858C8896DD6D4".decode('hex')
#TEST_TX = "12000022800000002400000025201B01BF7A086140000000000186A0684000000000000C717321039E4E8FE03FFB2B732A06DA881A4705ADBA7AF723545A89DA437427D1B0C3E58F81146E0394EBAA83660BC0C0CABCBE07FA5D61345FB2831423ED3371E6B8058FD14799CFF14D155366098E54".decode('hex')

# soure tag
TEST_TX =  "120000228000000023ABCD12342400000001201B0012316D61400000000BEBC20068400000000000000C732102C2FB8D65461479B7F69A8945DDA0FFFB77354FF48528AEC5C558B88140FA25578114026D56EE9F14933E58DCF0DCC127E70963064F618314B420EAADA2AA969F2138268DAB6F0B858CB635C8".decode('hex')
# dest tag
#TEST_TX =  "120000228000000024000000012E12345678201B0012316D61400000000BEBC20068400000000000000C732102C2FB8D65461479B7F69A8945DDA0FFFB77354FF48528AEC5C558B88140FA25578114026D56EE9F14933E58DCF0DCC127E70963064F618314B420EAADA2AA969F2138268DAB6F0B858CB635C8".decode('hex')
#src and dest tags
#TEST_TX =  "120000228000000023ABCD123424000000012E12345678201B0012316D61400000000BEBC20068400000000000000C732102C2FB8D65461479B7F69A8945DDA0FFFB77354FF48528AEC5C558B88140FA25578114026D56EE9F14933E58DCF0DCC127E70963064F618314B420EAADA2AA969F2138268DAB6F0B858CB635C8".decode('hex')

def parse_bip32_path(path):
	if len(path) == 0:
		return ""
	result = ""
	elements = path.split('/')
	for pathElement in elements:
		element = pathElement.split('\'')
		if len(element) == 1:
			result = result + struct.pack(">I", int(element[0]))			
		else:
			result = result + struct.pack(">I", 0x80000000 | int(element[0]))
	return result

parser = argparse.ArgumentParser()
parser.add_argument('--path', help="BIP 32 path to retrieve")
parser.add_argument('--ed25519', help="Derive on ed25519 curve", action='store_true')
parser.add_argument("--apdu", help="Display APDU log", action='store_true')
args = parser.parse_args()

if args.path == None:
        if args.ed25519:
                args.path = "44'/144'/0'/0'/0'"
        else:
                args.path = "44'/144'/0'/0'/0/0"

donglePath = parse_bip32_path(args.path)
if args.ed25519:
	p2 = "81"
else:
	p2 = "41" 

apdu = "e00400" + p2	
apdu = apdu.decode('hex') + chr(len(donglePath) + 1 + len(TEST_TX)) + chr(len(donglePath) / 4) + donglePath + TEST_TX

dongle = getDongle(args.apdu)
result = dongle.exchange(bytes(apdu))
print str(result).encode('hex')

