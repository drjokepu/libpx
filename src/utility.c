//
//  utility.c
//  libpx
//
//  Created by Tamas Czinege on 08/05/2012.
//  Copyright (c) 2012 Tamas Czinege. All rights reserved.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "utility.h"

char *px_copy_string(const char *restrict str)
{
    if (str == NULL) return NULL;
    
    const size_t length = strlen(str) + 1;
    char *copy = malloc(length);
    memcpy(copy, str, length);
    
    return copy;
}

const char *px_null_coalesce(const char *restrict str)
{
    if (str == NULL)
        return "";
    else
        return str;
}

#define ONEMASK ((size_t)(-1) / 0xFF)

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wcast-align"

size_t px_utf8_strlen(const char *str)
{
	const char *s;
	size_t count = 0;
	size_t u;
	unsigned char b;
    
	/* Handle any initial misaligned bytes. */
	for (s = str; (uintptr_t)(s) & (sizeof(size_t) - 1); s++) {
		b = (unsigned char)*s;
        
		/* Exit if we hit a zero byte. */
		if (b == '\0')
			goto done;
        
		/* Is this byte NOT the first byte of a character? */
		count += (b >> 7) & ((~b) >> 6);
	}
    
	/* Handle complete blocks. */
	for (; ; s += sizeof(size_t)) {
		/* Prefetch 256 bytes ahead. */
		__builtin_prefetch(&s[256], 0, 0);
        
		/* Grab 4 or 8 bytes of UTF-8 data. */
		u = *(size_t *)(s);
        
		/* Exit the loop if there are any zero bytes. */
		if ((u - ONEMASK) & (~u) & (ONEMASK * 0x80))
			break;
        
		/* Count bytes which are NOT the first byte of a character. */
		u = ((u & (ONEMASK * 0x80)) >> 7) & ((~u) >> 6);
		count += (u * ONEMASK) >> ((sizeof(size_t) - 1) * 8);
	}
    
	/* Take care of any left-over bytes. */
	for (; ; s++) {
		b = (unsigned char)*s;
        
		/* Exit if we hit a zero byte. */
		if (b == '\0')
			break;
        
		/* Is this byte NOT the first byte of a character? */
		count += (b >> 7) & ((~b) >> 6);
	}
    
done:
	return ((size_t)(s - str) - count);
}

#pragma clang diagnostic pop
