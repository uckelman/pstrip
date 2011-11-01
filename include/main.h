#pragma once

#include <libpff.h>

#include "json_writer.h"

void handle_item(libpff_item_t* item, JSON_writer& json);
void handle_subitems(libpff_item_t* item, JSON_writer& json);

