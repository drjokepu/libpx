//
//  connection.c
//  libpx
//
//  Created by Tamas Czinege on 03/05/2012.
//  Copyright (c) 2012 Tamas Czinege. All rights reserved.
//

#include "connection.h"
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "connection_params.h"
#include "error.h"
#include "message.h"
#include "response.h"
#include "security.h"

typedef struct px_sockaddr_with_length
{
    socklen_t length;
    struct sockaddr *sockaddr;
} px_sockaddr_with_length;

static const unsigned int px_connection_protocol_version = 196608;

static px_sockaddr_with_length px_socket_address_new(const px_connection_params *restrict connection_params);
static void px_socket_address_delete(px_sockaddr_with_length socket_address);

static bool px_connection_open_socket(px_connection *restrict connection);
static px_connection_attempt_result px_connection_startup(px_connection *restrict connection);
static bool px_connection_read_authentication_response(px_connection *restrict connection);

static bool px_authentication_method_needs_password(px_authentication_method method) __attribute__((const));
static void px_connection_send_authentication(px_connection *restrict connection);

static px_connection_attempt_result px_connection_wait_for_server_startup(px_connection *restrict connection);

static bool px_connection_send_startup_message(const px_connection *restrict connection);
static bool px_connection_send_terminate_message(const px_connection *restrict connection);
static bool px_connection_send_sync_message(const px_connection *restrict connection);
static bool px_connection_send_password_message(const px_connection *restrict connection, const char *restrict password);

static bool px_connection_send_password_message_md5(const px_connection *restrict connection);

static void px_connection_add_runtime_parameter(px_connection *restrict connection, const char *restrict param_name, const char *restrict param_value);

px_connection *px_connection_new(const px_connection_params *restrict connection_params)
{
    px_connection *connection = calloc(1, sizeof(px_connection));
    connection->connection_params = px_connection_params_copy(connection_params);
    
    return connection;
}

void px_connection_delete(px_connection *connection)
{
    if (connection->connection_status != px_connection_status_closed)
    {
        px_connection_close(connection);
    }
    
    if (connection->connection_params != NULL)
    {
        px_connection_params_delete(connection->connection_params);
    }
    
    if (connection->runtime_params.params != NULL)
    {
        for (unsigned int i = 0; i < connection->runtime_params.count; i++)
        {
            free(connection->runtime_params.params[i].name);
        }
        free(connection->runtime_params.params);
    }
    
    if (connection->last_error != NULL)
    {
        px_error_delete(connection->last_error);
    }
    
    free(connection);
}

px_connection_params *px_connection_get_connection_params(const px_connection *restrict connection)
{
    return connection->connection_params;
}

const px_error *px_connection_get_last_error(const px_connection *restrict connection)
{
    return connection->last_error;
}

void px_connection_set_last_error(px_connection *restrict connection, px_error *error)
{
    if (connection->last_error != NULL)
        px_error_delete(connection->last_error);
    connection->last_error = error;
}

px_connection_status px_connection_get_status(const px_connection *restrict connection)
{
    return connection->connection_status;
}

void px_connection_set_password_callback(px_connection *restrict connection, PXPasswordCallback *callback, void* context)
{
    connection->password_callback = callback;
    connection->password_callback_context = context;
}

px_connection_attempt_result px_connection_open(px_connection *restrict connection)
{
    if (connection->connection_status != px_connection_status_closed)
    {
        return px_connection_attempt_result_not_closed;
    }
    
    if (!px_connection_open_socket(connection))
    {
        connection->connection_status = px_connection_status_failed;
        return px_connection_attempt_result_invalid_host;
    }
    
    const px_connection_attempt_result startUpResult = px_connection_startup(connection);
    if (startUpResult != px_connection_attempt_result_success)
    {
        if (startUpResult != px_connection_attempt_result_authentication_needed)
        {
            px_connection_close(connection);
            connection->connection_status = px_connection_status_failed;
        }
    }
    
    return startUpResult;
}

void px_connection_close(px_connection *restrict connection)
{
    switch (connection->connection_status)
    {
        case px_connection_status_open:
        case px_connection_status_authentication_pending:
            px_connection_send_terminate_message(connection);
        case px_connection_status_opening:
            close(connection->socket_number);
        case px_connection_status_closed:
        case px_connection_status_failed:
            break;
    }
    
    connection->connection_status = px_connection_status_closed;
}

static bool px_connection_open_socket(px_connection *restrict connection)
{
    px_sockaddr_with_length socket_address = px_socket_address_new(connection->connection_params);
    if (socket_address.sockaddr == NULL)
    {
        return false;
    }
    
    const int socket_number = socket(socket_address.sockaddr->sa_family, SOCK_STREAM, 0);
    if (socket_number == -1)
    {
        px_socket_address_delete(socket_address);
        return false;
    }
    connection->socket_number = socket_number;
    
    const int connect_result = connect(socket_number, socket_address.sockaddr, socket_address.length);
    if (connect_result == -1)
    {
        close(socket_number);
        px_socket_address_delete(socket_address);
        return false;
    }
    
    px_socket_address_delete(socket_address);
    connection->connection_status = px_connection_status_opening;
    return true;
}

static px_connection_attempt_result px_connection_startup(px_connection *restrict connection)
{
    if (!px_connection_send_startup_message(connection))
        return px_connection_attempt_result_cannot_send_startup_message;
    
    return px_connection_authenticate(connection);
}

px_connection_attempt_result px_connection_authenticate(px_connection *restrict connection)
{
    while (!px_connection_read_authentication_response(connection))
    {
        if (connection->connection_status != px_connection_status_authentication_pending)
            return px_connection_attempt_result_authentication_failed;
        
        if (px_authentication_method_needs_password(connection->authentication_method) &&
            connection->connection_params->password == NULL)
        {
            if (connection->password_callback != NULL)
            {
                if (!connection->password_callback(connection, connection->password_callback_context))
                    return px_connection_attempt_result_authentication_needed;
            }
            else
            {
                return px_connection_attempt_result_authentication_needed;
            }
        }
        
        px_connection_send_authentication(connection);
    }
    
    return px_connection_wait_for_server_startup(connection);
}

static bool px_connection_read_authentication_response(px_connection *restrict connection)
{
    connection->connection_status = px_connection_status_opening;
    bool success = false;
        
    px_response *response = px_response_read_with_timeout(connection, 5 * 1000);
    if (response == NULL) return false;
    
    switch (response->message_type)
    {
        case px_message_type_authentication_ok:
            success = true;
            break;
        case px_message_type_authentication_md5_password:
            connection->connection_status = px_connection_status_authentication_pending;
            connection->authentication_method = PXAuthenticationMethodMD5;
            memcpy(connection->authentication_details.md5.salt,
                   response->response_data.authentication_md5_password.salt,
                   4);
            success = false;
            break;
        case px_message_type_error:
            success = false;
            break;
        default:
            success = false;
            break;
    }
    px_response_delete(response);
    
    return success;
}

static bool px_authentication_method_needs_password(px_authentication_method method)
{
    switch (method)
    {
        case PXAuthenticationMethodMD5:
            return true;
        case PXAuthenticationMethodNone:
        default:
            return false;
    }
}

static void px_connection_send_authentication(px_connection *restrict connection)
{
    switch (connection->authentication_method)
    {
        case PXAuthenticationMethodMD5:
            px_connection_send_password_message_md5(connection);
            break;
        default:
            break;
    }
}

static px_connection_attempt_result px_connection_wait_for_server_startup(px_connection *restrict connection)
{
    while (connection->connection_status == px_connection_status_opening)
    {        
        px_response *response = px_response_read_with_timeout(connection, 15 * 1000);
        if (response == NULL) return px_connection_attempt_result_unrecognized_server_message;
        
        switch (response->message_type)
        {
            case px_message_type_backend_key_data:
                connection->backend_process_id = response->response_data.backend_key_data.process_id;
                connection->backend_secret_key = response->response_data.backend_key_data.secret_key;
                break;
            case px_message_type_parameter_status:
                px_connection_add_runtime_parameter(connection,
                                                response->response_data.runtime_parameter_status.param_name,
                                                response->response_data.runtime_parameter_status.param_value);
                break;
            case px_message_type_ready_for_query:
                connection->connection_status = px_connection_status_open;
                break;
            case px_message_type_error:
                px_response_delete(response);
                return px_connection_attempt_result_server_error;
            default:
                break;
        }
        
        px_response_delete(response);
    }
    
    return px_connection_attempt_result_success;
}

void px_connection_add_runtime_parameter(px_connection *restrict connection, const char *restrict param_name, const char *restrict param_value)
{
    const size_t name_length = strlen(param_name);
    const size_t value_length = strlen(param_value);
    
    char *name_copy = malloc(name_length + value_length + 2);
    char *value_copy = stpcpy(name_copy, param_name) + 1;
    stpcpy(value_copy, param_value);
    
    if (connection->runtime_params.capacity == 0)
    {
        connection->runtime_params.capacity = 16;
        connection->runtime_params.params = calloc(connection->runtime_params.capacity, sizeof(px_connection_runtime_parameter_entry));
    }
    else if (connection->runtime_params.count + 1 >= connection->runtime_params.capacity)
    {
        connection->runtime_params.capacity *= 2;
        connection->runtime_params.params = realloc(connection->runtime_params.params,
                                                    connection->runtime_params.capacity * sizeof(px_connection_runtime_parameter_entry));
    }
    
    connection->runtime_params.params[connection->runtime_params.count++] = (px_connection_runtime_parameter_entry)
    {
        .name = name_copy,
        .value = value_copy
    };
}

static px_sockaddr_with_length px_socket_address_new(const px_connection_params *restrict connection_params)
{
    struct addrinfo* address_info = NULL;
    char port_string[16];
    sprintf(port_string, "%u", connection_params->port);
    
    const int address_error = getaddrinfo(connection_params->hostname, port_string, NULL, &address_info);
    
    if (address_error != 0)
    {
        fprintf(stderr, "%s\n", gai_strerror(address_error));
        if (address_info != NULL) freeaddrinfo(address_info);
        return (px_sockaddr_with_length){ .length = 0, .sockaddr = NULL };
    }
    
    struct sockaddr *copy = malloc(address_info->ai_addrlen);
    memcpy(copy, address_info->ai_addr, address_info->ai_addrlen);

    px_sockaddr_with_length result = (px_sockaddr_with_length)
    {
        .length = address_info->ai_addrlen,
        .sockaddr = copy
    };
    
    freeaddrinfo(address_info);
    
    return result;
}

static void px_socket_address_delete(px_sockaddr_with_length socketAddress)
{
    free(socketAddress.sockaddr);
}

static bool px_connection_send_startup_message(const px_connection *restrict connection)
{
    static const char *user_key = "user";
    const char *user_value = connection->connection_params->username;
    static const char *database_key = "database";
    const char *database_value = connection->connection_params->database;
    static const char *application_name_key = "application_name";
    const char *application_name_value = connection->connection_params->application_name == NULL ? "libpx" : connection->connection_params->application_name;
    
    px_message *message = px_message_new("0Tissssssc",
                                         px_connection_protocol_version,
                                         user_key,
                                         user_value,
                                         database_key,
                                         database_value,
                                         application_name_key,
                                         application_name_value,
                                         0);
    const bool success = px_message_send(message, connection->socket_number);
    px_message_delete(message);
    
    return success;
}

static bool px_connection_send_terminate_message(const px_connection *restrict connection)
{
    px_message *message = px_message_new("XT");
    const bool success = px_message_send(message, connection->socket_number);
    px_message_delete(message);
    return success;
}

static bool px_connection_send_password_message(const px_connection *restrict connection, const char *restrict password)
{
    px_message *message = px_message_new("pTs", password);
    const bool success = px_message_send(message, connection->socket_number);
    px_message_delete(message);
    return success;
}

static bool px_connection_send_sync_message(const px_connection *restrict connection)
{
    return px_message_send_sync(connection->socket_number);
}

bool px_connection_sync(px_connection *restrict connection, const bool read_response)
{
    if (!px_connection_send_sync_message(connection)) return false;
    if (read_response)
    {
        px_response *response = px_response_read_with_timeout(connection, -1);
        const bool ready_for_query = response->message_type == px_message_type_ready_for_query;
        px_response_delete(response);
        return ready_for_query;
    }
    else
    {
        return true;
    }
}

static bool px_connection_send_password_message_md5(const px_connection *restrict connection)
{
    unsigned char md5_0[16];
    unsigned char md5_1[16];
    char intermediate[64];
    char final_hash[64];
    char md5_response[64];
    
    const char *password = connection->connection_params->password;
    const char *salt = connection->authentication_details.md5.salt;
    
    if (password == NULL || salt == NULL)
        return false;
    
    char *password_with_username = malloc(strlen(password) + strlen(connection->connection_params->username) + 1);
    sprintf(password_with_username, "%s%s", password, connection->connection_params->username);
    px_security_md5_to_buffer(password_with_username, strlen(password_with_username), md5_0);
    free(password_with_username);
    
    px_security_print_md5(md5_0, intermediate);
    memcpy(intermediate + 32, salt, 4);
    
    px_security_md5_to_buffer(intermediate, 36, md5_1);
    px_security_print_md5(md5_1, final_hash);
    sprintf(md5_response, "md5%s", final_hash);
    
    return px_connection_send_password_message(connection, md5_response);
}

bool px_connection_poll(const px_connection *restrict connection, const int timeout)
{
    struct pollfd fd = (struct pollfd)
    {
        .fd = connection->socket_number,
        .events = POLLIN,
        .revents = 0
    };
    
    return poll(&fd, 1, timeout) == 1;
}

bool px_connection_has_incoming_data(const px_connection *restrict connection)
{
    return px_connection_poll(connection, 0);
}
