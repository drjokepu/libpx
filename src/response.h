//
//  response.h
//  libpx
//
//  Created by Tamas Czinege on 04/05/2012.
//  Copyright (c) 2012 Tamas Czinege. All rights reserved.
//

#ifndef libpx_response_h
#define libpx_response_h

#include <stdio.h>
#include "typedef.h"
#include "data_type.h"
#include "message_type.h"

typedef enum px_transaction_status
{
    px_transaction_status_idle = 'I',
    px_transaction_status_in_transaction = 'T',
    px_transaction_status_in_failed_transaction = 'E'
} px_transaction_status;

struct px_row_description_column
{
    char *field_name;
    unsigned int table_oid;
    unsigned int column_id;
    unsigned int datatype_oid;
    unsigned int datatype_size;
    unsigned int type_modifier;
    unsigned int format_code;
};

struct px_data_cell
{
    int length;
    void *data;
};

typedef union px_response_data
{
    struct
    {
        char *param_name;
        char *param_value;
    } runtime_parameter_status;
    struct
    {
        int process_id;
        int secret_key;
    } backend_key_data;
    struct
    {
        char *severity;
        char *sqlstate;
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
    } error;
    struct
    {
        enum px_transaction_status transaction_status;
    } ready_for_query;
    struct
    {
        unsigned int column_count;
        px_row_description_column* columns;
    } row_description;
    struct
    {
        unsigned int cell_count;
        px_data_cell *cells;
    } data_row;
    struct
    {
        char *command_tag;
    } command_complete;
    struct
    {
        unsigned char salt[4];
    } authentication_md5_password;
} px_response_data;

struct px_response
{
    size_t message_length;
    void *message_bytes;
    px_message_type message_type;
    px_message_class message_class;
    px_response_data response_data;
};

struct px_response_list
{
    size_t count;
    px_response *responses;
};

px_response *px_response_new(void);
void px_response_delete(px_response *response);
void px_response_list_delete(px_response_list *response_list);

px_response *px_response_read(px_connection *restrict connection);
px_response *px_response_read_with_timeout(px_connection *restrict connection, const int timeout);

px_response_list *px_response_read_all(px_connection *restrict connection);

#endif
