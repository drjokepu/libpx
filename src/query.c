//
//  query.c
//  libpx
//
//  Created by Tamas Czinege on 08/05/2012.
//  Copyright (c) 2012 Tamas Czinege. All rights reserved.
//

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include "query.h"
#include "connection.h"
#include "error.h"
#include "message.h"
#include "parameter.h"
#include "response.h"
#include "result.h"
#include "utility.h"

static bool px_query_can_use_simple_query(const px_query *restrict query) __attribute__((pure));
static px_result_list *px_query_execute_sync_simple(const px_query *restrict query);
static px_result_list *px_query_execute_sync_extended(const px_query *restrict query);

static bool px_query_parse(const px_query *restrict query);
static bool px_query_bind(const px_query *restrict query);
static bool px_query_describe_portal(const px_query *restrict query);
static bool px_query_execute_portal(const px_query *restrict query);
static bool px_query_close_portal(const px_query *restrict query);
static bool px_query_close_statement(const px_query *restrict query);

px_query *px_query_new(const char *restrict command_text, px_connection *restrict connection)
{
    px_query *query = calloc(1, sizeof(px_query));
    query->command_text = px_copy_string(command_text);
    query->connection = connection;
    return query;
}

void px_query_delete(px_query *query)
{
    if (query == NULL) return;
    if (query->command_text != NULL) free(query->command_text);
    if (query->parameters.values != NULL)
    {
        for (unsigned int i = 0; i < query->parameters.count; i++)
        {
            px_parameter_delete_members(query->parameters.values + i);
        }
        free(query->parameters.values);
    }
    
    free(query);
}

void px_query_add_parameter(px_query *restrict query, const px_parameter *restrict parameter)
{
    if (query->parameters.values == NULL || query->parameters.capacity == 0)
    {
        query->parameters.capacity = 4;
        query->parameters.values = calloc(query->parameters.capacity, sizeof(px_parameter));
    }
    else if (query->parameters.capacity >= query->parameters.count + 1)
    {
        query->parameters.capacity *= 2;
        query->parameters.values = realloc(query->parameters.values, query->parameters.capacity * sizeof(px_parameter));
    }
    
    px_parameter *new_parameter = query->parameters.values + (query->parameters.count++);
    px_parameter_clear(new_parameter);
    px_parameter_copy_to(new_parameter, parameter);
}

static bool px_query_can_use_simple_query(const px_query *restrict query)
{
    return query->parameters.count == 0;
}

px_result_list *px_query_execute(const px_query *restrict query)
{
#ifdef DEBUG_QUERY
    printf("query: %s\n", query->command_text);
#endif
    if (px_query_can_use_simple_query(query))
    {
        return px_query_execute_sync_simple(query);
    }
    else
    {
        return px_query_execute_sync_extended(query);
    }
}

static px_result_list *px_query_execute_sync_simple(const px_query *restrict query)
{
    px_message *query_message = px_message_new("QTs", query->command_text);
    px_message_send(query_message, query->connection->socket_number);
    px_message_delete(query_message);
    
    px_result_list *result_list = calloc(1, sizeof(px_result_list));
    result_list->capacity = 1;
    result_list->results = calloc(result_list->capacity, sizeof(px_result*));
    
    px_result *result = px_result_new();
    
    bool ready_for_query = false;
    while (!ready_for_query)
    {
        px_response *response = px_response_read(query->connection);
        if (response == NULL) return NULL;
        
        switch (response->message_type)
        {
            case px_message_type_ready_for_query:
                ready_for_query = true;
                break;
            case px_message_type_row_description:
                if (result == NULL) break;
                px_result_add_headers(result,
                                      response->response_data.row_description.column_count,
                                      response->response_data.row_description.columns);
                break;
            case px_message_type_data_row:
                if (result == NULL) break;
                px_result_add_data_row(result,
                                       response->response_data.data_row.cell_count,
                                       response->response_data.data_row.cells);
                break;
            case px_message_type_error:
                if (result == NULL) break;
                px_result_delete(result);
                result = NULL;
                break;
            case px_message_type_command_complete:
                if (result == NULL) break;
                px_result_parse_command_tag(result, response->response_data.command_complete.command_tag);
                
                if (result_list->capacity >= result_list->count + 1)
                {
                    result_list->capacity *= 2;
                    result_list->results = realloc(result_list->results, result_list->capacity + sizeof(px_result*));
                }
                result_list->results[result_list->count++] = result;
                result = px_result_new();
                
                break;
            default:
                fprintf(stderr, "unhandled message type: %c %i\n", (char)response->message_class, response->message_type);
                break;
        }
        
        px_response_delete(response);
    }
    
    if (result != NULL) px_result_delete(result);
    
    return result_list;
}

static px_result_list *px_query_execute_sync_extended(const px_query *restrict query)
{
    if (!px_query_parse(query)) return NULL;
    if (!px_query_bind(query)) return NULL;
    if (!px_query_describe_portal(query)) return NULL;
    if (!px_query_execute_portal(query)) return NULL;
    if (!px_query_close_portal(query)) return NULL;
    if (!px_query_close_statement(query)) return NULL;
    if (!px_connection_sync(query->connection, false)) return NULL;
    
    px_result *result = px_result_new();
    
    bool ready_for_query = false;
    while (!ready_for_query || px_connection_has_incoming_data(query->connection))
    {
        px_response *response = px_response_read(query->connection);
        if (response == NULL) return NULL;
        
        switch (response->message_type)
        {
            case px_message_type_ready_for_query:
                ready_for_query = true;
                break;
            case px_message_type_row_description:
                if (result == NULL) break;
                px_result_add_headers(result,
                                      response->response_data.row_description.column_count,
                                      response->response_data.row_description.columns);
                break;
            case px_message_type_data_row:
                if (result == NULL) break;
                px_result_add_data_row(result,
                                       response->response_data.data_row.cell_count,
                                       response->response_data.data_row.cells);
                break;
            case px_message_type_error:
                if (result == NULL) break;
                px_result_delete(result);
                result = NULL;
                break;
            case px_message_type_command_complete:
                px_result_parse_command_tag(result, response->response_data.command_complete.command_tag);
                break;
            case px_message_type_parse_complete:
            case px_message_type_bind_complete:
            case px_message_type_close_complete:
                break;
            default:
                fprintf(stderr, "unhandled message type: %c %i\n", (char)response->message_class, response->message_type);
                break;
        }
    }
    
    if (result == NULL)
    {
        return NULL;
    }
    else
    {
        px_result_list *result_list = calloc(1, sizeof(px_result_list));
        result_list->capacity = 1;
        result_list->count = 1;
        result_list->results = calloc(1, sizeof(px_result*));
        result_list->results[0] = result;
        
        return result_list;
    }
}

static bool px_query_parse(const px_query *restrict query)
{
    void **parameters = malloc((query->parameters.count + 4) * sizeof(void*));
    parameters[0] = "";
    parameters[1] = query->command_text;
    parameters[2] = (void*)query->parameters.count;
    parameters[3] = (void*)query->parameters.count;
    
    for (unsigned int i = 0; i < query->parameters.count; i++)
    {
        parameters[i + 4] = (void*)(query->parameters.values[i].type);
    }
    
    px_message *message = px_message_new_with_array("PTssw(i)", parameters);
    free(parameters);
    px_message_send(message, query->connection->socket_number);
    px_message_delete(message);
    
    return true;
}

static bool px_query_bind(const px_query *restrict query)
{
    void **parameters = malloc(((query->parameters.count * 2) + 5 + 1) * sizeof(void*));
    parameters[0] = "";
    parameters[1] = "";
    parameters[2] = 0;
    parameters[3] = (void*)query->parameters.count;
    parameters[4] = (void*)query->parameters.count;
    
    for (unsigned int i = 0; i < query->parameters.count; i++)
    {
        parameters[(i * 2 + 0) + 5] = (void*)(query->parameters.values[i].length);
        parameters[(i * 2 + 1) + 5] = (void*)(query->parameters.values[i].value);
    }
    
    parameters[(query->parameters.count * 2) + 5] = (void*)0;
    
    px_message *message = px_message_new_with_array("BTssww(iS)w", parameters);
    free(parameters);
    px_message_send(message, query->connection->socket_number);
    px_message_delete(message);
    
    return true;
}

static bool px_query_describe_portal(const px_query *restrict query)
{
    px_message *message = px_message_new("DTcs", 'P', "");
    const bool success = px_message_send(message, query->connection->socket_number);
    px_message_delete(message);
    return success;
}

static bool px_query_execute_portal(const px_query *restrict query)
{
    px_message *message = px_message_new("ETsi", "", 0);
    const bool success = px_message_send(message, query->connection->socket_number);
    px_message_delete(message);
    return success;
}

static bool px_query_close_portal(const px_query *restrict query)
{
    px_message *message = px_message_new("CTcs", 'P', "");
    const bool success = px_message_send(message, query->connection->socket_number);
    px_message_delete(message);
    return success;
}

static bool px_query_close_statement(const px_query *restrict query)
{
    px_message *message = px_message_new("CTcs", 'S', "");
    const bool success = px_message_send(message, query->connection->socket_number);
    px_message_delete(message);
    return success;
}
