/*******************************************************************************
*   XRP Wallet
*   (c) 2017 Ledger
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
********************************************************************************/

#include "xrpParse.h"

#define STI_UINT16 0x01
#define STI_UINT32 0x02
#define STI_AMOUNT 0x06
#define STI_VL 0x07
#define STI_ACCOUNT 0x08

#define XRP_UINT16_TRANSACTION_TYPE 0x02
#define XRP_UINT32_FLAGS 0x02
#define XRP_UINT32_SOURCE_TAG 0x03
#define XRP_UINT32_SEQUENCE 0x04
#define XRP_UINT32_LAST_LEDGER_SEQUENCE 0x1B
#define XRP_UINT32_DESTINATION_TAG 0x0E
#define XRP_AMOUNT_AMOUNT 0x01
#define XRP_AMOUNT_FEES 0x08
#define XRP_VL_SIGNING_PUB_KEY 0x03
#define XRP_ACCOUNT_ACCOUNT 0x01
#define XRP_ACCOUNT_DESTINATION 0x03

void parse_xrp_amount(uint64_t *value, uint8_t *data) {
    *value = ((uint64_t)data[7]) | ((uint64_t)data[6] << 8) | ((uint64_t)data[5] << 16) |
             ((uint64_t)data[4] << 24) | ((uint64_t)data[3] << 32) | ((uint64_t)data[2] << 40) |
             ((uint64_t)data[1] << 48) | ((uint64_t)data[0] << 56);
    *value -= (uint64_t)0x4000000000000000;
}

void parse_uint32(uint32_t *value, uint8_t *data) {
    *value = ((uint32_t)data[3]) | ((uint32_t)data[2] << 8) | ((uint32_t)data[1] << 16) |
             ((uint32_t)data[0] << 24);
}

parserStatus_e processUint16(uint8_t *data, uint32_t length, txContent_t *context, uint32_t *offsetParam) {
    parserStatus_e result = USTREAM_FAULT;
    uint32_t offset = *offsetParam;
    uint8_t fieldId = data[offset] & 0x0f;
    if ((offset + 1 + 2) > length) {
        result = USTREAM_PROCESSING;
        goto error;
    }
    switch(fieldId) {
        case XRP_UINT16_TRANSACTION_TYPE:
            if ((data[offset + 1] != 0x00) || (data[offset + 2] != 0x00)) {
                goto error;
            }
            break;
        default:
            goto error;
    }
    *offsetParam = offset + 1 + 2;
    result = USTREAM_FINISHED;
error:    
    return result;    
}

parserStatus_e processUint32(uint8_t *data, uint32_t length, txContent_t *context, uint32_t *offsetParam) {
    parserStatus_e result = USTREAM_FAULT;
    uint32_t offset = *offsetParam;
    uint8_t fieldId = data[offset] & 0x0f;
    if ((offset + 1 + 4) > length) {
        result = USTREAM_PROCESSING;
        goto error;
    }
    switch(fieldId) {
        case 0: {
            uint8_t fieldId2 = data[offset + 1];
            if ((offset + 4) > length) {
                result = USTREAM_PROCESSING;
                goto error;
            }
            offset++;
            switch(fieldId2) {
                case XRP_UINT32_LAST_LEDGER_SEQUENCE:
                    break;
                default:
                    goto error;
            }

        }

        case XRP_UINT32_FLAGS:
            break;
        case XRP_UINT32_SEQUENCE:
            break;
        case XRP_UINT32_SOURCE_TAG:
            parse_uint32(&context->sourceTag, data + offset + 1);
            context->sourceTagPresent = 1;
            break;
        case XRP_UINT32_DESTINATION_TAG:
            parse_uint32(&context->destinationTag, data + offset + 1);
            context->destinationTagPresent = 1;        
            break;
        default:
            goto error;
    }
    *offsetParam = offset + 1 + 4;
    result = USTREAM_FINISHED;
error:    
    return result;    
}

parserStatus_e processAmount(uint8_t *data, uint32_t length, txContent_t *context, uint32_t *offsetParam) {
    parserStatus_e result = USTREAM_FAULT;
    uint32_t offset = *offsetParam;
    uint8_t fieldId = data[offset] & 0x0f;
    if ((offset + 1 + 8) > length) {
        result = USTREAM_PROCESSING;
        goto error;
    }
    switch(fieldId) {
        case XRP_AMOUNT_AMOUNT:
            parse_xrp_amount(&context->amount, data + offset + 1);
            break;
        case XRP_AMOUNT_FEES:
            parse_xrp_amount(&context->fees, data + offset + 1);
            break;
        default:
            goto error;
    }
    *offsetParam = offset + 1 + 8;
    result = USTREAM_FINISHED;
error:    
    return result;    
}


parserStatus_e processVl(uint8_t *data, uint32_t length, txContent_t *context, uint32_t *offsetParam) {
    parserStatus_e result = USTREAM_FAULT;
    uint32_t offset = *offsetParam;
    uint8_t fieldId = data[offset] & 0x0f;
    uint8_t dataLength = data[offset + 1];
    if ((offset + 1) > length) {
        result = USTREAM_PROCESSING;
        goto error;
    }
    if ((offset + 1 + dataLength) > length) {
        result = USTREAM_PROCESSING;
        goto error;        
    }
    offset += 1 + 1;
    switch(fieldId) {
        case XRP_VL_SIGNING_PUB_KEY:
            if (dataLength != 33) {
                goto error;
            }
            // TODO : check key
            break;

        default:
            goto error;
    }
    *offsetParam = offset + dataLength;
    result = USTREAM_FINISHED;
error:    
    return result;    
}

parserStatus_e processAccount(uint8_t *data, uint32_t length, txContent_t *context, uint32_t *offsetParam) {
    parserStatus_e result = USTREAM_FAULT;
    uint32_t offset = *offsetParam;
    uint8_t fieldId = data[offset] & 0x0f;
    uint8_t dataLength = data[offset + 1];
    if ((offset + 1) > length) {
        result = USTREAM_PROCESSING;
        goto error;
    }
    if ((offset + 1 + dataLength) > length) {
        result = USTREAM_PROCESSING;
        goto error;        
    }
    offset += 1 + 1;
    switch(fieldId) {
        case XRP_ACCOUNT_ACCOUNT:
            if (dataLength != 20) {
                goto error;
            }
            os_memmove(context->account, data + offset, 20);
            break;

        case XRP_ACCOUNT_DESTINATION:
            if (dataLength != 20) {
                goto error;
            }
            os_memmove(context->destination, data + offset, 20);
            break;

        default:
            goto error;
    }
    *offsetParam = offset + dataLength;
    result = USTREAM_FINISHED;
error:    
    return result;    
}


parserStatus_e parseTxInternal(uint8_t *data, uint32_t length, txContent_t *context) {
    uint32_t offset = 0;
    parserStatus_e result = USTREAM_FAULT;
    while (offset != length) {
        if (offset > length) {
            goto error;
        }
        uint8_t dataType = data[offset] >> 4;
        switch(dataType) {
            case STI_UINT16:
                result = processUint16(data, length, context, &offset);
                break;
            case STI_UINT32:
                result = processUint32(data, length, context, &offset);
                break;
            case STI_AMOUNT:
                result = processAmount(data, length, context, &offset);
                break;
            case STI_VL:
                result = processVl(data, length, context, &offset);
                break;
            case STI_ACCOUNT:
                result = processAccount(data, length, context, &offset);
                break;
            default:
                goto error;
        }
        if (result != USTREAM_FINISHED) {
            goto error;
        }
        result = USTREAM_FAULT;
    }
    result = USTREAM_FINISHED;
error:    
    return result;
}

parserStatus_e parseTx(uint8_t *data, uint32_t length, txContent_t *context) {
    parserStatus_e result;
    BEGIN_TRY {
        TRY {
            os_memset(context, 0, sizeof(txContent_t));
            result = parseTxInternal(data, length, context);
        }
        CATCH_OTHER(e) {
            result = USTREAM_FAULT;
        }
        FINALLY {
        }
    }
    END_TRY;
    return result;
}
