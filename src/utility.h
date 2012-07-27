//
//  utility.h
//  libpx
//
//  Created by Tamas Czinege on 08/05/2012.
//  Copyright (c) 2012 Tamas Czinege. All rights reserved.
//

#ifndef libpx_utility_h
#define libpx_utility_h

#include <stdio.h>

char *px_copy_string(const char *restrict str);
const char *px_null_coalesce(const char *restrict str) __attribute__((const));
const size_t px_utf8_strlen(const char *str) __attribute__((const));

#endif
