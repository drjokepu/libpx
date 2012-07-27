//
//  security.c
//  libpx
//
//  Created by Tamas Czinege on 09/05/2012.
//  Copyright (c) 2012 Tamas Czinege. All rights reserved.
//

#include <stdio.h>
#include <stdlib.h>
#include "security.h"

unsigned char *px_security_md5(const void *data, size_t length)
{
    unsigned char *buffer = malloc(64);
    px_security_md5_to_buffer(data, length, buffer);
    return buffer;
}

void px_security_print_md5(const unsigned char *restrict md5, char *restrict str)
{
    sprintf(str,
            "%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x",
            (unsigned int)md5[0],
            (unsigned int)md5[1],
            (unsigned int)md5[2],
            (unsigned int)md5[3],
            (unsigned int)md5[4],
            (unsigned int)md5[5],
            (unsigned int)md5[6],
            (unsigned int)md5[7],
            (unsigned int)md5[8],
            (unsigned int)md5[9],
            (unsigned int)md5[10],
            (unsigned int)md5[11],
            (unsigned int)md5[12],
            (unsigned int)md5[13],
            (unsigned int)md5[14],
            (unsigned int)md5[15]);
}
