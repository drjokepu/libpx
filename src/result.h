//
//  result.h
//  libpx
//
//  Created by Tamas Czinege on 08/05/2012.
//  Copyright (c) 2012 Tamas Czinege. All rights reserved.
//

#ifndef libpx_result_h
#define libpx_result_h

#include <stdbool.h>
#include <stdio.h>
#include "typedef.h"
#include "message_type.h"

struct px_data_row
{
    size_t cellCount;
    void *cellData;
    px_data_cell *cells;
};

struct px_result
{
    struct
    {
        size_t count;
        px_row_description_column *values;
    } headers;
    struct
    {
        size_t count;
        size_t capacity;
        px_data_row *values;
    } rows;
    char *command_tag;
    px_command_type command_type;
    unsigned int affected_rows;
    unsigned int row_oid;
};

struct px_result_list
{
    unsigned int capacity;
    unsigned int count;
    px_result **results;
};

px_result *px_result_new(void);
void px_result_delete(px_result *result);

void px_result_list_delete(px_result_list *result_list, bool keepElements);

void px_result_add_headers(px_result *restrict result, const size_t count, const px_row_description_column *restrict headers);

void px_result_add_data_row(px_result *result, const size_t cell_count, const px_data_cell *restrict cells);
void px_result_parse_command_tag(px_result *restrict result, const char *restrict command_tag);

// headers
const unsigned int px_result_get_column_count(const px_result *restrict result) __attribute__((pure));
const unsigned int px_result_get_row_count(const px_result *restrict result) __attribute__((pure));
const char *px_result_get_column_name(const px_result *restrict result, const unsigned int index) __attribute__((pure));
const unsigned int px_result_get_column_datatype(const px_result *restrict result, const unsigned int index) __attribute__((pure));
char *px_result_copy_column_datatype_as_string(const px_result *restrict result, const unsigned int index);

// values
const char* px_result_get_command_tag(const px_result *restrict result) __attribute__((pure));
const px_command_type px_result_get_command_type(const px_result *restrict result) __attribute__((pure));
const unsigned int px_result_get_affected_rows(const px_result *restrict result) __attribute__((pure));
const unsigned int px_result_get_row_oid(const px_result *restrict result) __attribute__((pure));

bool px_result_is_db_null(const px_result *restrict result, const unsigned int column, const unsigned int row) __attribute__((pure));
char *px_result_copy_cell_value_as_string(const px_result *restrict result, const unsigned int column, const unsigned int row);

#endif
