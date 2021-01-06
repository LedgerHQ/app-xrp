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

#include <string.h>

#include "field_sort.h"
#include "transaction_types.h"

uint8_t get_priority_score(field_t *field) {
    if (is_transaction_type_field(field)) {
        return 0;
    }

    if (is_normal_account_field(field)) {
        return 1;
    }

    return 2;
}

bool is_preceding(field_t *field, field_t *other_field) {
    return get_priority_score(field) < get_priority_score(other_field);
}

void swap_fields(parseResult_t *result, uint8_t idx1, uint8_t idx2) {
    field_t buffer_field;
    memcpy(&buffer_field, &result->fields[idx1], sizeof(field_t));
    memcpy(&result->fields[idx1], &result->fields[idx2], sizeof(field_t));
    memcpy(&result->fields[idx2], &buffer_field, sizeof(field_t));
}

void sort_fields(parseResult_t *result) {
    for (uint8_t i = 0; i < result->num_fields - 1; ++i) {
        field_t *cur_field = &result->fields[i];
        field_t *next_field = &result->fields[i + 1];

        if (is_preceding(next_field, cur_field) && !is_preceding(cur_field, next_field)) {
            swap_fields(result, i, i + 1);
            i = MAX(0, i - 1) - 1;  // Start next iteration at i - 1 or 0, whatever is largest
        }
    }
}
