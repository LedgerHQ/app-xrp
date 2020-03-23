/*******************************************************************************
*   XRP Wallet
*   (c) 2020 Towo Labs
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

#include "numberHelpers.h"
#include "os.h"
#include "string.h"

typedef enum {
    NUM_INTEGER,        // 1000
    NUM_SEMI_DECIMAL,   // 1000.5
    NUM_DECIMAL,        // 0.123
    NUM_EXTREME         // 1.34e40
} presentationType_e;

void formatNumberData(char* dst, uint16_t len, int16_t exponent, uint64_t mantissa);
void removeTrailingZeros(char* dst, int16_t *exponentParam);
presentationType_e determineFormat(char *dst, int16_t exponent);

void parseInteger(char *dst, uint16_t len, int16_t *exponentParam);
void parseSemiDecimal(char *dst, uint16_t len, int16_t *exponentParam);
void parseDecimal(char *dst, uint16_t len, int16_t *exponentParam);
void parseExtreme(char *dst, uint16_t len, int16_t *exponentParam);


void parseDecimalNumber(char* dst, uint16_t len, uint8_t sign, int16_t exponent, uint64_t mantissa) {
    uint16_t textPos = 0;

    if (sign == 0 && exponent == 0 && mantissa == 0) {
        // Special case: Value is zero
        dst[textPos] = '0';
    } else {
        if (sign == 0) {
            dst[textPos++] = '-';
        }

        formatNumberData(dst + textPos, len - textPos, exponent, mantissa);
    }
}

void formatNumberData(char* dst, uint16_t len, int16_t exponent, uint64_t mantissa) {
    print_uint64_t(dst, len, mantissa);
    removeTrailingZeros(dst, &exponent);

    presentationType_e format = determineFormat(dst, exponent);
    switch (format) {
        case NUM_INTEGER:
            parseInteger(dst, len, &exponent);
            break;
        case NUM_SEMI_DECIMAL:
            parseSemiDecimal(dst, len, &exponent);
            break;
        case NUM_DECIMAL:
            parseDecimal(dst, len, &exponent);
            break;
        case NUM_EXTREME:
            parseExtreme(dst, len, &exponent);
            break;
        default:
            THROW(INVALID_PARAMETER);
    }

    if (exponent != 0) {
        size_t mantissaLength = strlen(dst);

        int numWritten = snprintf(dst + mantissaLength, len - mantissaLength, "e%d", exponent);
        if (numWritten < 0 || (size_t)numWritten >= len - mantissaLength) {
            snprintf(dst, len, "Formatting failed");
        }
    }
}

void removeTrailingZeros(char* dst, int16_t *exponentParam) {
    size_t strEnd = strlen(dst);
    for (size_t i = strEnd - 1; i > 0; --i) {
        if (dst[i] == '0') {
            (*exponentParam)++;
            dst[i] = '\0';
        } else {
            break;
        }
    }
}

presentationType_e determineFormat(char *dst, int16_t exponent) {
    size_t mantissaLength = strlen(dst);
    int32_t baseLength = mantissaLength + exponent;

    if (baseLength > 0 && baseLength < 10) {
        if (exponent > 0) {
            return NUM_INTEGER;
        } else {
            return NUM_SEMI_DECIMAL;
        }
    } else if (exponent < 0 && exponent > -10) {
        return NUM_DECIMAL;
    } else {
        return NUM_EXTREME;
    }
}

void parseInteger(char *dst, uint16_t len, int16_t *exponentParam) {
    size_t mantissaLength = strlen(dst);
    if (mantissaLength + *exponentParam >= len - 1) {
        THROW(NOT_ENOUGH_SPACE);
    }

    while (*exponentParam > 0) {
        dst[mantissaLength] = '0';

        mantissaLength++;
        (*exponentParam)--;
    }
}

void parseSemiDecimal(char *dst, uint16_t len, int16_t *exponentParam) {
    size_t mantissaLength = strlen(dst);
    int16_t numDecimals = -*exponentParam;

    if (numDecimals < 0) {
        THROW(INVALID_PARAMETER);
    }

    uint16_t uNumDecimals = (uint16_t) numDecimals;
    char *decimalBreakpoint = dst + mantissaLength - uNumDecimals;

    if (uNumDecimals > 0) {
        if (mantissaLength + 1 >= len - 1) {
            THROW(NOT_ENOUGH_SPACE);
        }

        os_memmove(decimalBreakpoint + 1, decimalBreakpoint, uNumDecimals);
        *decimalBreakpoint = '.';
    }

    *exponentParam = 0;
}

void parseDecimal(char *dst, uint16_t len, int16_t *exponentParam) {
    size_t mantissaLength = strlen(dst);
    int32_t numZeros = -*exponentParam - mantissaLength + 1;
    int32_t posShift = numZeros + 1;

    if (posShift < 0) {
        THROW(INVALID_PARAMETER);
    }

    if (mantissaLength + posShift >= len - 1) {
        THROW(NOT_ENOUGH_SPACE);
    }

    uint32_t uPosShift = (uint32_t) posShift;
    os_memmove(dst + posShift, dst, mantissaLength);
    os_memset(dst, '0', uPosShift);

    dst[1] = '.';

    *exponentParam = 0;
}

void parseExtreme(char *dst, uint16_t len, int16_t *exponentParam) {
    size_t mantissaLength = strlen(dst);
    if (mantissaLength == 0) {
        THROW(INVALID_STATE);
    }

    if (*exponentParam == 0) {
        // Don't add a decimal if it does not provide any additional value
        return;
    }

    if (mantissaLength + 1 >= len - 1) {
        THROW(NOT_ENOUGH_SPACE);
    }

    os_memmove(dst + 1, dst, mantissaLength);
    dst[1] = '.';

    *exponentParam += mantissaLength - 1;
}

char intToNumberChar(uint64_t value) {
    if (value > 9) {
        return '?';
    }

    return (char) ('0' + value);
}

void print_uint64_t(char *dst, uint16_t len, uint64_t value) {
    uint16_t numDigits = 0, i;
    uint64_t base = 1;
    while (base <= value) {
        base *= 10;
        numDigits++;
    }
    if (numDigits > len - 1) {
        THROW(NOT_ENOUGH_SPACE);
    }
    base /= 10;
    for (i=0; i<numDigits; i++) {
        dst[i] = intToNumberChar((value / base) % 10);
        base /= 10;
    }
    dst[i] = '\0';
}
