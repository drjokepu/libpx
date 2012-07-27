//
//  message.h
//  libpx
//
//  Created by Tamas Czinege on 04/05/2012.
//  Copyright (c) 2012 Tamas Czinege. All rights reserved.
//

#ifndef libpx_message_h
#define libpx_message_h

#include <stdbool.h>
#include <stdio.h>
#include "typedef.h"

struct px_message
{
    size_t messageLength;
    void *messageBytes;
};

// build messages
px_message *px_message_new(const char *restrict pattern, ...);
px_message *px_message_new_with_array(const char *restrict pattern, void** parameters);
void px_message_delete(px_message *message);

bool px_message_send(const px_message *restrict message, const int file_descriptor);

bool px_message_send_sync(const int file_descriptor);

#endif
