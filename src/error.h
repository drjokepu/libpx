//
//  error.h
//  libpx
//
//  Created by Tamas Czinege on 08/05/2012.
//  Copyright (c) 2012 Tamas Czinege. All rights reserved.
//

#ifndef libpx_error_h
#define libpx_error_h

#include "typedef.h"

struct px_error
{
    char *severity;
    char *sqlState;
    char *message;
    char *detail;
    char *hint;
    char *position;
    char *internal_position;
    char *internal_query;
    char *where;
    char *file;
    char *line;
    char *routine;
};

// create & delete
px_error *px_error_new(const px_response *restrict response);
px_error *px_error_new_custom(const char *sqlState, const char *message);

px_error *px_error_new_authentication_failure();
px_error *px_error_new_io_error();

void px_error_delete(px_error *error);

// getters
const char *px_error_get_severity(const px_error *restrict error) __attribute__((pure));
const char *px_error_get_sqlstate(const px_error *restrict error) __attribute__((pure));
const char *px_error_get_message(const px_error *restrict error) __attribute__((pure));
const char *px_error_get_detail(const px_error *restrict error) __attribute__((pure));
const char *px_error_get_hint(const px_error *restrict error) __attribute__((pure));
const char *px_error_get_position(const px_error *restrict error) __attribute__((pure));
const char *px_error_get_internal_position(const px_error *restrict error) __attribute__((pure));
const char *px_error_get_internal_query(const px_error *restrict error) __attribute__((pure));
const char *px_error_get_where(const px_error *restrict error) __attribute__((pure));
const char *px_error_get_file(const px_error *restrict error) __attribute__((pure));
const char *px_error_get_line(const px_error *restrict error) __attribute__((pure));
const char *px_error_get_routine(const px_error *restrict error) __attribute__((pure));

#endif
