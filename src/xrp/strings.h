#pragma once

#include "fields.h"

bool is_purely_ascii(const uint8_t *data, uint16_t length, bool allow_suffix);
