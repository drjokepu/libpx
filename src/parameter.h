//
//  parameter.h
//  libpx
//
//  Created by Tamas Czinege on 17/05/2012.
//  Copyright (c) 2012 Tamas Czinege. All rights reserved.
//

#ifndef libpx_parameter_h
#define libpx_parameter_h

#include "typedef.h"
#include "data_type.h"

struct px_parameter
{
    px_datatype type;
    int length;
    char *value;
};

extern const int px_parameter_null_value_length; // -1

px_parameter *px_parameter_new(void);
px_parameter *px_parameter_new_string(const char *restrict str);
px_parameter *px_parameter_new_oid(const unsigned int oid);

px_parameter *px_parameter_copy(const px_parameter *restrict old);
void px_parameter_copy_to(px_parameter *restrict new, const px_parameter *restrict old);

void px_parameter_clear(px_parameter *parameter);

void px_parameter_delete(px_parameter *parameter);
void px_parameter_delete_members(px_parameter *parameter);

void px_parameter_bind(px_parameter *restrict parameter, const char *restrict text_value,
                       const px_datatype datatype);

void px_parameter_bind_null(px_parameter *restrict parameter);
void px_parameter_bind_string(px_parameter *restrict parameter, const char *restrict str);
void px_parameter_bind_oid(px_parameter *restrict parameter, const unsigned int oid);

#endif
