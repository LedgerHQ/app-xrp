#pragma once

#include "fields.h"
#include "xrp_parse.h"

void format_field(field_t *field, field_value_t *dst);

// A Number shown in the base units of an asset the transaction does not
// name, which the device therefore cannot show (see find_number_currency)
bool is_number_without_asset(field_t *field);

// Whether any field of the transaction is one; the review warns first
bool has_number_without_asset(parseResult_t *transaction);
