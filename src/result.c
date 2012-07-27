//
//  result.c
//  libpx
//
//  Created by Tamas Czinege on 08/05/2012.
//  Copyright (c) 2012 Tamas Czinege. All rights reserved.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "result.h"
#include "response.h"
#include "utility.h"

static const char *px_result_get_fixed_datatype_as_string(const px_datatype type) __attribute__((const));

px_result *px_result_new(void)
{
    return calloc(1, sizeof(px_result));
}

void px_result_delete(px_result *result)
{
    if (result == NULL) return;
    
    if (result->headers.values != NULL)
    {
        for (unsigned int i = 0; i < result->headers.count; i++)
        {
            free(result->headers.values[i].field_name);
        }
        free(result->headers.values);
    }
       
    if (result->rows.capacity > 0)
    {       
        for (unsigned int i = 0; i < result->rows.count; i++)
        {
            if (result->rows.values[i].cellCount > 0 && result->rows.values[i].cells != NULL)
            {
                free(result->rows.values[i].cellData);
                free(result->rows.values[i].cells);
            }
        }
        
        free(result->rows.values);
    }
    
    if (result->command_tag != NULL)
    {
        free(result->command_tag);
    }
    
    free(result);
}

void px_result_list_delete(px_result_list *result_list, bool keepElements)
{
    if (result_list == NULL) return;
    if (!keepElements)
    {
        for (unsigned int i = 0; i < result_list->count; i++)
        {
            px_result_delete(result_list->results[i]);
        }
    }
    free(result_list->results);
    free(result_list);
}

void px_result_add_headers(px_result *restrict result, const size_t count, const px_row_description_column *restrict headers)
{
    result->headers.count = count;
    
    if (count > 0)
    {
        result->headers.values = malloc(count * sizeof(px_row_description_column));
        memcpy(result->headers.values, headers, count * sizeof(px_row_description_column));
        
        for (unsigned int i = 0; i < count; i++)
        {
            result->headers.values[i].field_name = px_copy_string(result->headers.values[i].field_name);
        }
    }
}

void px_result_add_data_row(px_result *result, const size_t cell_count, const px_data_cell *restrict cells)
{
    //expand the row list if needed
    if (result->rows.capacity == 0)
    {
        result->rows.capacity = 32;
        result->rows.values = malloc(result->rows.capacity * sizeof(px_data_row));
    }
    else if (result->rows.count + 1 >= result->rows.capacity)
    {
        result->rows.capacity *= 2;
        result->rows.values = realloc(result->rows.values, result->rows.capacity * sizeof(px_data_row));
    }
    
    px_data_row *newRow = result->rows.values + ((result->rows.count)++);
    newRow->cellCount = 0;
    newRow->cellData = NULL;
    newRow->cells = NULL;
    
    newRow->cellCount = cell_count;
    
    if (cell_count == 0)
    {
        newRow->cells = NULL;
    }
    else // the new row has cells
    {
        newRow->cells = malloc(cell_count * sizeof(px_data_cell));
        
        unsigned int rowLength = 0;
        for (unsigned int i = 0; i < cell_count; i++)
        {
            if (cells[i].length > 0)
                rowLength += (unsigned int)cells[i].length;
        }
        
        // allocate a single block for data, or nothing if there's no data
        void *rowData = rowLength > 0 ? malloc(rowLength) : NULL;
        unsigned int rowDataOffset = 0;
        
        for (unsigned int i = 0; i < cell_count; i++)
        {
            newRow->cells[i].length = cells[i].length;
            
            if (newRow->cells[i].length <= 0)
            {
                newRow->cells[i].data = NULL;
            }
            else
            {
                if (rowData != NULL)
                {
                    // copy the data to the data block of the row
                    memcpy(rowData + rowDataOffset, cells[i].data, cells[i].length);
                    newRow->cells[i].data = rowData + rowDataOffset;
                    rowDataOffset += (unsigned int)cells[i].length;
                }
            }
        }
        
        newRow->cellData = rowData;
    }
}

const unsigned int px_result_get_column_count(const px_result *restrict result)
{
    return result->headers.count;
}

const unsigned int px_result_get_row_count(const px_result *restrict result)
{
    return result->rows.count;
}

const char *px_result_get_column_name(const px_result *restrict result, const unsigned int index)
{
    return result->headers.values[index].field_name;
}

bool px_result_is_db_null(const px_result *restrict result, const unsigned int column, const unsigned int row)
{
    return result->rows.values[row].cells[column].length < 0;
}

const unsigned int px_result_get_column_datatype(const px_result *restrict result, const unsigned int index)
{
    return result->headers.values[index].datatype_oid;
}

char *px_result_copy_column_datatype_as_string(const px_result *restrict result, const unsigned int index)
{
    const unsigned int data_type = (px_datatype)result->headers.values[index].datatype_oid;
    
    switch (data_type)
    {
        case px_data_type_varcharn:
        {
            char *str = malloc(32);
            sprintf(str, "varchar(%u)", result->headers.values[index].datatype_size);
            return str;
        }
        default:
        {
            const char *fixed_type = px_result_get_fixed_datatype_as_string(data_type);
            if (fixed_type != NULL)
            {
                return px_copy_string(fixed_type);
            }
            else
            {
                char *str = malloc(32);
                sprintf(str, "#%u", data_type);
                return str;
            }
        }
    }
}

static const char *px_result_get_fixed_datatype_as_string(const px_datatype type)
{
    switch (type)
    {
        case px_data_type_char:                    return "char";
        case px_data_type_bool:                    return "boolean";
        case px_data_type_int16:                   return "smallint";
        case px_data_type_int32:                   return "integer";
        case px_data_type_int64:                   return "bigint";
        case px_data_type_single:                  return "real";
        case px_data_type_double:                  return "double precision";
        case px_data_type_oid:                     return "oid";
        case px_data_type_cid:                     return "cid";
        case px_data_type_xid:                     return "xid";
        case px_data_type_tid:                     return "tid";
        case px_data_type_name:                    return "name";
        case px_data_type_inet:                    return "inet";
        case px_data_type_varcharu:                return "varchar";
        case px_data_type_timestamp:               return "timestamp";
        case px_data_type_timestampz:              return "timestamp with time zone";
        case px_data_type_uuid:                    return "uuid";
        case px_data_type_acl:                     return "acl";
            
        case px_data_type_texta:                   return "text[]";
        case px_data_type_acla:                    return "acl[]";
        case px_data_type_oidau:                   return "oid[]";
        case px_data_type_int16au:                 return "smallint[]";
            
        default:
            return NULL;
    }
}

char *px_result_copy_cell_value_as_string(const px_result *restrict result, const unsigned int column, const unsigned int row)
{
    if (px_result_is_db_null(result, column, row))
    {
        return px_copy_string("NULL");
    }
    
    switch ((px_datatype)result->headers.values[column].datatype_oid)
    {
        case px_data_type_bool:
        {
            return px_copy_string(*((char*)(result->rows.values[row].cells[column].data)) == 't' ? "true" : "false");
        }
        case px_data_type_int16:
        case px_data_type_int32:
        case px_data_type_int64:
        case px_data_type_single:
        case px_data_type_double:
        case px_data_type_char:
        case px_data_type_varcharu:
        case px_data_type_varcharn:
        case px_data_type_uuid:
        case px_data_type_oid:
        case px_data_type_tid:
        case px_data_type_xid:
        case px_data_type_cid:
        case px_data_type_name:
        case px_data_type_inet:
        case px_data_type_timestamp:
        case px_data_type_timestampz:
        case px_data_type_int16a:
        case px_data_type_int16au:
        case px_data_type_int32a:
        case px_data_type_oida:
        case px_data_type_oidau:
        {
            const unsigned int length = (unsigned int)result->rows.values[row].cells[column].length;
            char *str = malloc(length + 1);
            memcpy(str, result->rows.values[row].cells[column].data, length);
            str[length] = 0;
            return str;
        }
        default:
        {
            const unsigned int length = (unsigned int)result->rows.values[row].cells[column].length;
            char *valueStr = calloc(length + 1, sizeof(char));
            memcpy(valueStr, result->rows.values[row].cells[column].data, length);
            
            char *str = malloc(128 + length + 1);
            sprintf(str, "#%u (%i) \"%s\"", result->headers.values[column].datatype_oid, result->rows.values[row].cells[column].length, valueStr);
            free(valueStr);
            
            return str;
        }
    }
}

void px_result_parse_command_tag(px_result *restrict result, const char *restrict command_tag)
{
    if (strncmp(command_tag, "SELECT ", 7) == 0)
    {
        result->command_tag = px_copy_string(command_tag);
        result->command_type = px_command_type_select;      
        sscanf(command_tag, "SELECT %u", &result->affected_rows);
    }
    else if (strncmp(command_tag, "INSERT ", 7) == 0)
    {
        result->command_tag = px_copy_string(command_tag);
        result->command_type = px_command_type_insert;
        sscanf(command_tag, "INSERT %u %u", &result->row_oid, &result->affected_rows);
    }
    else if (strncmp(command_tag, "DELETE ", 7) == 0)
    {
        result->command_tag = px_copy_string(command_tag);
        result->command_type = px_command_type_delete;      
        sscanf(command_tag, "DELETE %u", &result->affected_rows);
    }
    else if (strncmp(command_tag, "UPDATE ", 7) == 0)
    {
        result->command_tag = px_copy_string(command_tag);
        result->command_type = px_command_type_update;      
        sscanf(command_tag, "UPDATE %u", &result->affected_rows);
    }
}

const char* px_result_get_command_tag(const px_result *restrict result)
{
    return result->command_tag;
}

const px_command_type px_result_get_command_type(const px_result *restrict result)
{
    return result->command_type;
}

const unsigned int px_result_get_affected_rows(const px_result *restrict result)
{
    return result->affected_rows;
}

const unsigned int px_result_get_row_oid(const px_result *restrict result)
{
    return result->row_oid;
}