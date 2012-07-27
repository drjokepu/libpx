//
//  query.h
//  libpx
//
//  Created by Tamas Czinege on 08/05/2012.
//  Copyright (c) 2012 Tamas Czinege. All rights reserved.
//

#ifndef libpx_query_h
#define libpx_query_h

#include "typedef.h"

struct px_query
{
    char *command_text;
    px_connection *connection;
    struct
    {
        unsigned int count;
        unsigned int capacity;
        px_parameter *values;
    } parameters;
};

px_query *px_query_new(const char *restrict command_text, px_connection *restrict connection);
void px_query_delete(px_query *query);

void px_query_add_parameter(px_query *restrict query, const px_parameter *restrict parameter);
px_result_list *px_query_execute(const px_query *restrict query);

#endif
