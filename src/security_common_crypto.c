//
//  security_common_crypto.c
//  libpx
//
//  Created by Tamas Czinege on 09/05/2012.
//  Copyright (c) 2012 Tamas Czinege. All rights reserved.
//

#include <stdio.h>
#include <stdlib.h>
#include <CommonCrypto/CommonDigest.h>

void px_security_md5_to_buffer(const void *data, size_t length, unsigned char *buffer)
{
    CC_MD5(data, length, buffer);
}
