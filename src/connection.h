//
//  connection.h
//  libpx
//
//  Created by Tamas Czinege on 03/05/2012.
//  Copyright (c) 2012 Tamas Czinege. All rights reserved.
//

#ifndef libpx_connection_h
#define libpx_connection_h

#include <stdbool.h>
#include <sys/types.h>
#include <sys/socket.h>
#include "typedef.h"

typedef enum px_connection_status
{
    px_connection_status_failed = -1,
    px_connection_status_closed = 0,
    px_connection_status_opening = 1,
    px_connection_status_authentication_pending = 2,
    px_connection_status_open = 3
} px_connection_status;

typedef enum px_authentication_method
{
    PXAuthenticationMethodNone,
    PXAuthenticationMethodMD5
} px_authentication_method;

typedef enum px_connection_attempt_result
{
    px_connection_attempt_result_success = 0,
    px_connection_attempt_result_not_closed = 1,
    px_connection_attempt_result_invalid_host = 2,
    px_connection_attempt_result_authentication_needed = 3,
    px_connection_attempt_result_authentication_failed = 4,
    px_connection_attempt_result_cannot_send_startup_message = 5,
    px_connection_attempt_result_startup_timeout = 6,
    px_connection_attempt_result_unrecognized_server_message = 7,
    px_connection_attempt_result_server_error = 8
} px_connection_attempt_result;

typedef struct px_connection_runtime_parameter_entry
{
    char *name;
    char *value;
} px_connection_runtime_parameter_entry;

typedef struct px_connection_runtime_params
{
    size_t count;
    size_t capacity;
    px_connection_runtime_parameter_entry* params;
} px_connection_runtime_params;

typedef bool PXPasswordCallback(const px_connection* connection, void *context);

struct px_connection
{
    px_connection_params *connection_params;
    px_connection_status connection_status;
    px_authentication_method authentication_method;
    px_connection_runtime_params runtime_params;
    px_error *last_error;
    int socket_number;
    int backend_process_id;
    int backend_secret_key;
    PXPasswordCallback *password_callback;
    void *password_callback_context;
    
    union
    {
        struct
        {
            char salt[4];
        } md5;
    } authentication_details;
};

// creation & deletion
px_connection *px_connection_new(const px_connection_params *restrict connection_params);
void px_connection_delete(px_connection *connection);

// opening & closing a connection
px_connection_attempt_result px_connection_open(px_connection *restrict connection);
void px_connection_close(px_connection *restrict connection);

px_connection_attempt_result px_connection_authenticate(px_connection *restrict connection);

// getting information about a connection
const px_connection_status px_connection_get_status(const px_connection *restrict connection) __attribute__((pure));
px_connection_params *px_connection_get_connection_params(const px_connection *restrict connection) __attribute__((pure));
const px_error *px_connection_get_last_error(const px_connection *restrict connection) __attribute__((pure));

// connection callbacks
void px_connection_set_password_callback(px_connection *restrict connection, PXPasswordCallback *callback, void* context);

// changing connection properties
void px_connection_set_last_error(px_connection *restrict connection, px_error *error);

// sending various messages
bool px_connection_sync(px_connection *restrict connection, const bool read_response);

// investigate incoming data
bool px_connection_poll(const px_connection *restrict connection, const int timeout);
bool px_connection_has_incoming_data(const px_connection *restrict connection);

#endif
