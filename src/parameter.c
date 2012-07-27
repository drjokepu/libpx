//
//  parameter.c
//  libpx
//
//  Created by Tamas Czinege on 17/05/2012.
//  Copyright (c) 2012 Tamas Czinege. All rights reserved.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "parameter.h"
#include "utility.h"

static void px_parameter_free_value(px_parameter *parameter);

const int px_parameter_null_value_length = -1;

px_parameter *px_parameter_new(void)
{
    return calloc(1, sizeof(px_parameter));
}

px_parameter *px_parameter_copy(const px_parameter *restrict old)
{
    px_parameter *new = px_parameter_new();
    px_parameter_copy_to(new, old);
    return new;
}

void px_parameter_copy_to(px_parameter *restrict new, const px_parameter *restrict old)
{
    new->type = old->type;
    new->length = old->length;
    new->value = px_copy_string(old->value);
}

void px_parameter_clear(px_parameter *restrict parameter)
{
    memset(parameter, 0, sizeof(px_parameter));
}

void px_parameter_delete(px_parameter *parameter)
{
    if (parameter == NULL) return;
    if (parameter->value != NULL) free(parameter->value);
    free(parameter);
}

void px_parameter_delete_members(px_parameter *parameter)
{
    if (parameter->value != NULL)
    {
        free(parameter->value);
        parameter->value = NULL;
    }
}

void px_parameter_bind_null(px_parameter *restrict parameter)
{
    px_parameter_free_value(parameter);
    parameter->length = px_parameter_null_value_length;
}

void px_parameter_bind(px_parameter *restrict parameter, const char *restrict text_value,
                       const px_datatype datatype)
{
    px_parameter_free_value(parameter);
    if (text_value == NULL)
    {
        px_parameter_bind_null(parameter);
    }
    else
    {
        parameter->value = px_copy_string(text_value);
        parameter->length = strlen(parameter->value);
    }
    parameter->type = datatype;
}

void px_parameter_bind_string(px_parameter *restrict parameter, const char *restrict str)
{
    px_parameter_bind(parameter, str, px_data_type_varcharu);
}

px_parameter *px_parameter_new_string(const char *restrict str)
{
    px_parameter *parameter = px_parameter_new();
    px_parameter_bind_string(parameter, str);
    return parameter;
}

void px_parameter_bind_oid(px_parameter *restrict parameter, const unsigned int oid)
{
    char text_value[64];
    sprintf(text_value, "%u", oid);
    px_parameter_bind(parameter, text_value, px_data_type_oid);
}

px_parameter *px_parameter_new_oid(const unsigned int oid)
{
    px_parameter *parameter = px_parameter_new();
    px_parameter_bind_oid(parameter, oid);
    return parameter;
}

static void px_parameter_free_value(px_parameter *parameter)
{
    if (parameter == NULL) return;
    if (parameter->value != NULL)
    {
        free(parameter->value);
        parameter->value = NULL;
    }
}