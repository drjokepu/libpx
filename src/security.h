//
//  security.h
//  libpx
//
//  Created by Tamas Czinege on 09/05/2012.
//  Copyright (c) 2012 Tamas Czinege. All rights reserved.
//

#ifndef libpx_security_h
#define libpx_security_h

unsigned char *px_security_md5(const void *data, size_t length);
void px_security_print_md5(const unsigned char *restrict md5, char *restrict str);

// implementation-specific
void px_security_md5_to_buffer(const void *data, size_t length, unsigned char *buffer);

#endif
