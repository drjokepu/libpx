//
//  error.c
//  libpx
//
//  Created by Tamas Czinege on 08/05/2012.
//  Copyright (c) 2012 Tamas Czinege. All rights reserved.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "error.h"
#include "response.h"
#include "utility.h"

static void px_error_delete_string(char *str);

px_error *px_error_new(const px_response *restrict response)
{
    px_error *error = calloc(1, sizeof(px_error));
    
    error->severity = px_copy_string(response->response_data.error.severity);
    error->sqlState = px_copy_string(response->response_data.error.sqlstate);
    error->message = px_copy_string(response->response_data.error.message);
    error->detail = px_copy_string(response->response_data.error.detail);
    error->hint = px_copy_string(response->response_data.error.hint);
    error->position = px_copy_string(response->response_data.error.position);
    error->internal_position = px_copy_string(response->response_data.error.internal_position);
    error->internal_query = px_copy_string(response->response_data.error.internal_query);
    error->where = px_copy_string(response->response_data.error.where);
    error->file = px_copy_string(response->response_data.error.file);
    error->line = px_copy_string(response->response_data.error.line);
    error->routine = px_copy_string(response->response_data.error.routine);
    
    return error;
}

px_error *px_error_new_custom(const char *sqlState, const char *message)
{
    px_error *error = calloc(1, sizeof(px_error));
    error->sqlState = px_copy_string(sqlState);
    error->message = px_copy_string(message);
    
    return error;
}

void px_error_delete(px_error *error)
{
    if (error == NULL) return;
    
    px_error_delete_string(error->severity);
    px_error_delete_string(error->sqlState);
    px_error_delete_string(error->message);
    px_error_delete_string(error->detail);
    px_error_delete_string(error->hint);
    px_error_delete_string(error->position);
    px_error_delete_string(error->internal_position);
    px_error_delete_string(error->internal_query);
    px_error_delete_string(error->where);
    px_error_delete_string(error->file);
    px_error_delete_string(error->line);
    px_error_delete_string(error->routine);
    
    free(error);
}

static void px_error_delete_string(char *str)
{
    if (str != NULL)
        free(str);
}

const char *px_error_get_severity(const px_error *restrict error)
{
    return error == NULL ? "" : px_null_coalesce(error->severity);
}

const char *px_error_get_sqlstate(const px_error *restrict error)
{
    return error == NULL ? "" : px_null_coalesce(error->sqlState);
}

const char *px_error_get_message(const px_error *restrict error)
{
    return error == NULL ? "" : px_null_coalesce(error->message);
}

const char *px_error_get_detail(const px_error *restrict error)
{
    return error == NULL ? "" : px_null_coalesce(error->detail);
}

const char *px_error_get_hint(const px_error *restrict error)
{
    return error == NULL ? "" : px_null_coalesce(error->hint);
}

const char *px_error_get_position(const px_error *restrict error)
{
    return error == NULL ? "" : px_null_coalesce(error->position);
}

const char *px_error_get_internal_position(const px_error *restrict error)
{
    return error == NULL ? "" : px_null_coalesce(error->internal_position);
}

const char *px_error_get_internal_query(const px_error *restrict error)
{
    return error == NULL ? "" : px_null_coalesce(error->internal_query);
}

const char *px_error_get_where(const px_error *restrict error)
{
    return error == NULL ? "" : px_null_coalesce(error->where);
}

const char *px_error_get_file(const px_error *restrict error)
{
    return error == NULL ? "" : px_null_coalesce(error->file);
}

const char *px_error_get_line(const px_error *restrict error)
{
    return error == NULL ? "" : px_null_coalesce(error->line);
}

const char *px_error_get_routine(const px_error *restrict error)
{
    return error == NULL ? "" : px_null_coalesce(error->routine);
}

px_error *px_error_new_authentication_failure()
{
    return px_error_new_custom("28P01", "authentication failure");
}

px_error *px_error_new_io_error()
{
    return px_error_new_custom("58030", "io error");
}
