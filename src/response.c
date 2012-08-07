//
//  response.c
//  libpx
//
//  Created by Tamas Czinege on 04/05/2012.
//  Copyright (c) 2012 Tamas Czinege. All rights reserved.
//

#include "response.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <arpa/inet.h>
#include "connection.h"
#include "error.h"

// #define DEBUG_RESPONSE 1

static void px_response_delete_contents(px_response *response);
static void px_response_delete_without_contents(px_response *response);

static bool px_response_read_bytes_from_file(const int file_descriptor, px_message_class *out_message_class, void *restrict* out_message_bytes, size_t *restrict out_message_length);

#ifdef DEBUG_RESPONSE
static char* px_message_type_string(px_message_type type);
#endif /* DEBUG_RESPONSE */

static bool px_response_parse(px_response *restrict response);
static bool px_response_parse_authentication_request(px_response *restrict response);
static bool px_response_parse_bind_complete(px_response *restrict response);
static bool px_response_parse_cancellation_key_data(px_response *restrict response);
static bool px_response_parse_close_complete(px_response *restrict response);
static bool px_response_parse_command_complete(px_response *restrict response);
static bool px_response_parse_data_row(px_response *restrict response);
static bool px_response_parse_error(px_response *restrict response);
static bool px_response_parse_parse_complete(px_response *restrict response);
static bool px_response_parse_ready_for_query(px_response *restrict response);
static bool px_response_parse_row_description(px_response *restrict response);
static bool px_response_parse_runtime_parameter_status_report(px_response *restrict response);

px_response *px_response_new(void)
{
    px_response *response = calloc(1, sizeof(px_response));
    return response;
}

void px_response_delete(px_response *response)
{
    if (response == NULL) return;
    px_response_delete_contents(response);
    px_response_delete_without_contents(response);
}

void px_response_list_delete(px_response_list *response_list)
{
    if (response_list == NULL) return;
    for (unsigned int i = 0 ; i < response_list->count; i++)
    {
        px_response_delete_contents(response_list->responses + i);
    }
    free(response_list);
}

static void px_response_delete_contents(px_response *response)
{
    if (response->message_bytes != NULL)
    {
        free(response->message_bytes);
    }
    
    if (response->message_type == px_message_type_row_description &&
        response->response_data.row_description.columns != NULL)
    {
        free(response->response_data.row_description.columns);
    }
    else if (response->message_type == px_message_class_data_row &&
             response->response_data.data_row.cells != NULL)
    {
        free(response->response_data.data_row.cells);
    }
}

static void px_response_delete_without_contents(px_response *response)
{
    free(response);
}

px_response *px_response_read_with_timeout(px_connection *restrict connection, const int timeout)
{   
    if (px_connection_poll(connection, timeout))
    {
        return px_response_read(connection);
    }
    else
    {
        px_connection_set_last_error(connection, px_error_new_io_error());
        return NULL;
    }
}

px_response *px_response_read(px_connection *restrict connection)
{
    void *message_bytes = NULL;
    px_message_class message_class = px_message_class_undefined;
    size_t message_length = 0;
    
    if (!px_response_read_bytes_from_file(connection->socket_number,
                                          &message_class,
                                          &message_bytes,
                                          &message_length))
    {
        if (message_bytes != NULL)
            free(message_bytes);
        return NULL;
    }
    
    px_response *response = px_response_new();
    response->message_length = message_length;
    response->message_bytes = message_bytes;
    response->message_class = message_class;
    
    if (!px_response_parse(response))
    {
        px_response_delete(response);
        return NULL;
    }
#ifdef DEBUG_RESPONSE
    else
    {
        printf("Parsed: %c %s (%i)\n", (char)response->message_class, px_message_type_string(response->message_type), response->message_type);
    }
#endif /* DEBUG_RESPONSE */
    
    if (response->message_type == px_message_type_error)
        px_connection_set_last_error(connection, px_error_new(response));
    
    return response;
}

static bool px_response_read_bytes_from_file(const int fileDescriptor, px_message_class *restrict outMessageClass, void *restrict* outMessageBytes, size_t *restrict outMessageLength)
{
    static const size_t header_length = 5;
    char response_header[header_length];

    if (read(fileDescriptor, response_header, header_length) != header_length)
    {
        return false;
    }
    
    const size_t message_length = ntohl(*((unsigned int*)(response_header + 1)));
    
    void *message_bytes = calloc(message_length, sizeof(char));
    memcpy(message_bytes, response_header + 1, header_length - 1);
    
    read(fileDescriptor, message_bytes + header_length - 1, message_length - header_length + 1);
    
    if (outMessageClass != NULL)
        *outMessageClass = (px_message_class)response_header[0];
    
    if (outMessageBytes != NULL)
        *outMessageBytes = message_bytes;
    
    if (outMessageLength != NULL)
        *outMessageLength = message_length;
    
    return true;
}

static bool px_response_parse(px_response *restrict response)
{
    switch (response->message_class)
    {
        case px_message_class_authentication_request:
            return px_response_parse_authentication_request(response);
        
        case px_message_class_bind_complete:
            return px_response_parse_bind_complete(response);
            
        case px_message_class_cancellation_key_data:
            return px_response_parse_cancellation_key_data(response);
        
        case px_message_class_close_complete:
            return px_response_parse_close_complete(response);
        
        case px_message_class_command_complete:
            return px_response_parse_command_complete(response);
            
        case px_message_class_error:
            return px_response_parse_error(response);
            
        case px_message_class_parse_complete:
            return px_response_parse_parse_complete(response);
            
        case px_message_class_runtime_parameter_status_report:
            return px_response_parse_runtime_parameter_status_report(response);
            
        case px_message_class_ready_for_query:
            return px_response_parse_ready_for_query(response);
        
        case px_message_class_row_description:
            return px_response_parse_row_description(response);
            
        case px_message_class_data_row:
            return px_response_parse_data_row(response);
            
        default:
            fprintf(stderr, "Unknown PostgreSQL message class: %c\n", (char)response->message_class);
            return false;
    }
}

static bool px_response_parse_authentication_request(px_response *restrict response)
{
    const int code = htonl(*((int*)(response->message_bytes + 4)));
    
    switch (code)
    {
        case 0:
            response->message_type = px_message_type_authentication_ok;
            return true;
        case 5:
            response->message_type = px_message_type_authentication_md5_password;
            memcpy(response->response_data.authentication_md5_password.salt, response->message_bytes + 8, 4);
            return true;
        default:
            fprintf(stderr, "Unknown PostgreSQL authentication response: %i\n", code);
            return false;
    }
}

static bool px_response_parse_cancellation_key_data(px_response *restrict response)
{
    response->message_type = px_message_type_backend_key_data;
    response->response_data.backend_key_data.process_id = ntohl(*((int*)(response->message_bytes + 4)));
    response->response_data.backend_key_data.secret_key = ntohl(*((int*)(response->message_bytes + 8)));
    return true;
}

static bool px_response_parse_runtime_parameter_status_report(px_response *restrict response)
{
    char *param_name = (char*)(response->message_bytes + 4);
    const size_t param_name_length = strlen(param_name);
    char *param_value = param_name + param_name_length + 1;
    
    response->message_type = px_message_type_parameter_status;
    response->response_data.runtime_parameter_status.param_name = param_name;
    response->response_data.runtime_parameter_status.param_value = param_value;
    
    return true;
}

static bool px_response_parse_bind_complete(px_response *restrict response)
{
    response->message_type = px_message_type_bind_complete;
    return true;
}

static bool px_response_parse_close_complete(px_response *restrict response)
{
    response->message_type = px_message_type_close_complete;
    return true;
}

static bool px_response_parse_command_complete(px_response *restrict response)
{
    response->message_type = px_message_type_command_complete;
    response->response_data.command_complete.command_tag = ((char*)response->message_bytes + 4);
    
    return true;
}

static bool px_response_parse_parse_complete(px_response *restrict response)
{
    response->message_type = px_message_type_parse_complete;
    return true;
}

static bool px_response_parse_ready_for_query(px_response *restrict response)
{
    response->message_type = px_message_type_ready_for_query;
    response->response_data.ready_for_query.transaction_status = (enum px_transaction_status)(*(char*)(response->message_bytes + 4));
    
    return true;
}

static bool px_response_parse_error(px_response *restrict response)
{
    response->message_type = px_message_type_error;
    
    char *cursor = (char*)response->message_bytes + 4;
    while (*cursor != 0)
    {
        char field_type = *cursor;
        char *field_value = cursor + 1;
        const size_t field_length = strlen(field_value);
        cursor = field_value + field_length + 1;
        
        switch (field_type)
        {
            case 'S':
                response->response_data.error.severity = field_value;
                break;
            case 'C':
                response->response_data.error.sqlstate = field_value;
                break;
            case 'M':
                response->response_data.error.message = field_value;
                break;
            case 'D':
                response->response_data.error.detail = field_value;
                break;
            case 'H':
                response->response_data.error.hint = field_value;
                break;
            case 'P':
                response->response_data.error.position = field_value;
                break;
            case 'p':
                response->response_data.error.internal_position = field_value;
                break;
            case 'q':
                response->response_data.error.internal_query = field_value;
                break;
            case 'W':
                response->response_data.error.where = field_value;
                break;
            case 'F':
                response->response_data.error.file = field_value;
                break;
            case 'L':
                response->response_data.error.line = field_value;
                break;
            case 'R':
                response->response_data.error.routine = field_value;
                break;
            default:
                break;
        }
    }
    
    return true;
}

static bool px_response_parse_row_description(px_response *restrict response)
{
    response->message_type = px_message_type_row_description;
    response->response_data.row_description.column_count = ntohs(*((short int*)(response->message_bytes + 4)));
    
    if (response->response_data.row_description.column_count > 0)
    {
        response->response_data.row_description.columns =
            calloc(response->response_data.row_description.column_count,
                   sizeof(px_row_description_column));
    }
       
    void *cursor = response->message_bytes + 6;
    for (unsigned int i = 0; i < response->response_data.row_description.column_count; i++)
    {
        response->response_data.row_description.columns[i].field_name = cursor;
        const size_t fieldNameLength = strlen(response->response_data.row_description.columns[i].field_name);
        
        response->response_data.row_description.columns[i].table_oid =
            ntohl(*((unsigned int*)(cursor + fieldNameLength + 1)));
        response->response_data.row_description.columns[i].column_id =
            ntohs(*((unsigned short int*)(cursor + fieldNameLength + 5)));
        response->response_data.row_description.columns[i].datatype_oid =
            ntohl(*((unsigned int*)(cursor + fieldNameLength + 7)));
        response->response_data.row_description.columns[i].datatype_size =
            ntohs(*((unsigned short*)(cursor + fieldNameLength + 11)));
        response->response_data.row_description.columns[i].type_modifier =
            ntohl(*((unsigned int*)(cursor + fieldNameLength + 13)));
        response->response_data.row_description.columns[i].format_code =
            ntohs(*((unsigned short*)(cursor + fieldNameLength + 17)));
        
        cursor += fieldNameLength + 19;
    }
    
    return true;
}

static bool px_response_parse_data_row(px_response *restrict response)
{
    response->message_type = px_message_type_data_row;
    response->response_data.data_row.cell_count = ntohs(*((short int*)(response->message_bytes + 4)));
    if (response->response_data.data_row.cell_count > 0)
    {
        response->response_data.data_row.cells = calloc(response->response_data.data_row.cell_count,
                                                        sizeof(px_data_cell));
    }
    
    void *cursor = response->message_bytes + 6;
    for (unsigned int i = 0; i < response->response_data.data_row.cell_count; i++)
    {
        response->response_data.data_row.cells[i].length =
            ntohl(*((unsigned int*)(cursor)));
        
        if (response->response_data.data_row.cells[i].length > 0)
        {
            response->response_data.data_row.cells[i].data =
                cursor + 4;
        }
        else
        {
            response->response_data.data_row.cells[i].data = NULL;
        }
        
        int dataLength = response->response_data.data_row.cells[i].length;
        if (dataLength < 0) dataLength = 0;
        
        cursor += 4 + dataLength;
    }
    
    return true;
}

//px_response_list *px_response_read_all(px_connection *restrict connection)
//{
//    size_t count = 0;
//    size_t capacity = 0;
//    px_response *responses = NULL;
//    
//    while(px_connection_has_incoming_data(connection))
//    {
//        if (capacity == 0)
//        {
//            capacity = 4;
//            responses = malloc(sizeof(px_response) * 4);
//        }
//        else if (count + 1 >= capacity)
//        {
//            capacity *= 2;
//            responses = realloc(responses, sizeof(px_response) * capacity);
//        }
//        
//        px_response *response = px§
//    }
//}

#ifdef DEBUG_RESPONSE
static char* px_message_type_string(px_message_type type)
{
    switch (type)
    {
        case px_message_type_authentication_ok:
            return "authentication ok";
        case px_message_type_authentication_md5_password:
            return "authentication md5 password";
        case px_message_type_backend_key_data:
            return "backend key data";
        case px_message_type_bind_complete:
            return "bind complete";
        case px_message_type_close_complete:
            return "close complete";
        case px_message_type_command_complete:
            return "command complete";
        case px_message_type_data_row:
            return "data row";
        case px_message_type_error:
            return "error";
        case px_message_type_parameter_status:
            return "parameter status";
        case px_message_type_parse_complete:
            return "parse complete";
        case px_message_type_ready_for_query:
            return "ready for query";
        case px_message_type_row_description:
            return "row description";
        default:
            return "unknown";
    }
}
#endif /* DEBUG_RESPONSE */
