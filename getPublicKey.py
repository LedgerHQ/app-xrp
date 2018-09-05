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
parser.add_argument('--confirm', help="Confirm on screen", action='store_true')
parser.add_argument('--ed25519', help="Derive on ed25519 curve", action='store_true')
parser.add_argument("--apdu", help="Display APDU log", action='store_true')
args = parser.parse_args()

if args.path == None:
	if args.ed25519:
		args.path = "44'/144'/0'/0'/0'"
	else:	
		args.path = "44'/144'/0'/0'/0/0"

donglePath = parse_bip32_path(args.path)
if args.confirm:
	p1 = "01"
else:
	p1 = "00"
if args.ed25519:
	p2 = "81"
else:
	p2 = "41" 

apdu = "e002" + p1 + p2	
apdu = apdu.decode('hex') + chr(len(donglePath) + 1) + chr(len(donglePath) / 4) + donglePath

dongle = getDongle(args.apdu)
result = dongle.exchange(bytes(apdu))
offset = 0 
publicKeyLength = result[offset]
publicKey = result[offset + 1 : offset + 1 + publicKeyLength]
offset = offset + 1 + publicKeyLength
addressLength = result[offset] 
print addressLength
address = result[offset + 1 : offset + 1 + addressLength]
offset = offset + 1 + addressLength
chainCode = result[offset: offset + 32]

print "Public key " + str(publicKey).encode('hex')
print "Chaincode " + str(chainCode).encode('hex')
print "Address " + str(address)
