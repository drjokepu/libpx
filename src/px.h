//
//  px.h
//  libpx
//
//  Created by Tamas Czinege on 03/05/2012.
//  Copyright (c) 2012 Tamas Czinege. All rights reserved.
//

#ifndef libpx_px_h
#define libpx_px_h

#include <stdbool.h>

// structs
typedef struct px_connection px_connection;
typedef struct px_connection_params px_connection_params;
typedef struct px_error px_error;
typedef struct px_parameter px_parameter;
typedef struct px_query px_query;
typedef struct px_result px_result;

typedef struct px_result_list
{
    unsigned int capacity;
    unsigned int count;
    px_result **results;
} px_result_list;

typedef enum px_connection_status
{
    px_connection_status_failed = -1,
    px_connection_status_closed = 0,
    px_connection_status_opening = 1,
    px_connection_status_authentication_pending = 2,
    px_connection_status_open = 3
} px_connection_status;

typedef enum px_connection_attempt_result
{
    px_connection_attempt_result_success = 0,
    px_connection_attempt_result_not_closed = 1,
    px_connection_attempt_result_invalid_host = 2,
    px_connection_attempt_result_authentication_needed = 3,
    px_connection_attempt_result_authentication_failed = 4,
    px_connection_attempt_result_cannot_send_startup_message = 5,
    px_connection_attempt_result_startup_timeout = 6,
    px_connection_attempt_result_unrecognized_server_message = 7
} px_connection_attempt_result;

typedef enum px_command_type
{
    px_command_type_unknown = 0,
    px_command_type_insert,
    px_command_type_delete,
    px_command_type_update,
    px_command_type_select,
    px_command_type_move,
    px_command_type_fetch,
    px_command_type_copy
} px_command_type;

typedef unsigned int px_datatype;

// function pointer types
typedef bool PXPasswordCallback(const px_connection* connection, void *context);

// creation & deletion of connection params
px_connection_params *px_connection_params_new(void);
px_connection_params *px_connection_params_copy(const px_connection_params *restrict old);
void px_connection_params_delete(px_connection_params *connection_params);

// getters & setters of connection params
const char *px_connection_params_get_hostname(const px_connection_params *restrict connection_params) __attribute__((pure));
void px_connection_params_set_hostname(px_connection_params *restrict connectionParams, const char *restrict value);

const char *px_connection_params_get_username(const px_connection_params *restrict connection_params) __attribute__((pure));
void px_connection_params_set_username(px_connection_params *restrict connection_params, const char *restrict value);

const char *px_connection_params_get_password(const px_connection_params *restrict connection_params) __attribute__((pure));
void px_connection_params_set_password(px_connection_params *restrict connection_params, const char *restrict value);

const char *px_connection_params_get_database(const px_connection_params *restrict connection_params) __attribute__((pure));
void px_connection_params_set_database(px_connection_params *restrict connection_params, const char *restrict value);

unsigned int px_connection_params_get_port(const px_connection_params *restrict connection_params) __attribute__((pure));
void px_connection_params_set_port(px_connection_params *restrict connection_params, const unsigned int value);

const char *px_connection_params_get_application_name(const px_connection_params *restrict connection_params) __attribute__((pure));
void px_connection_params_set_application_name(px_connection_params *restrict connection_params, const char *restrict value);

// creation & deletion of connections
px_connection *px_connection_new(px_connection_params *connection_params);
void px_connection_delete(px_connection *connection);

// opening & closing a connection
px_connection_attempt_result px_connection_open(px_connection *connection);
void px_connection_close(px_connection *restrict connection);

px_connection_attempt_result px_connection_authenticate(px_connection *restrict connection);

// getting information about a connection
const px_connection_status px_connection_get_status(const px_connection *restrict connection) __attribute__((pure));
px_connection_params *px_connection_get_connection_params(const px_connection *restrict connection) __attribute__((pure));
const px_error *px_connection_get_last_error(const px_connection *restrict connection) __attribute__((pure));

// connection callbacks
void px_connection_set_password_callback(px_connection *restrict connection, PXPasswordCallback *callback);

// getting information about an error
const char *px_error_get_severity(const px_error *restrict error) __attribute__((pure));
const char *px_error_get_sqlstate(const px_error *restrict error) __attribute__((pure));
const char *px_error_get_message(const px_error *restrict error) __attribute__((pure));
const char *px_error_get_detail(const px_error *restrict error) __attribute__((pure));
const char *px_error_get_hint(const px_error *restrict error) __attribute__((pure));
const char *px_error_get_position(const px_error *restrict error) __attribute__((pure));
const char *px_error_get_internal_position(const px_error *restrict error) __attribute__((pure));
const char *px_error_get_internal_query(const px_error *restrict error) __attribute__((pure));
const char *px_error_get_where(const px_error *restrict error) __attribute__((pure));
const char *px_error_get_file(const px_error *restrict error) __attribute__((pure));
const char *px_error_get_line(const px_error *restrict error) __attribute__((pure));
const char *px_error_get_routine(const px_error *restrict error) __attribute__((pure));

// query parameters
px_parameter *px_parameter_new(void);
px_parameter *px_parameter_new_string(const char *restrict str);
px_parameter *px_parameter_new_oid(const unsigned int oid);

void px_parameter_delete(px_parameter *parameter);
void px_parameter_delete_members(px_parameter *parameter);

void px_parameter_bind(px_parameter *restrict parameter, const char *restrict text_value, const px_datatype datatype);
void px_parameter_bind_null(px_parameter *restrict parameter);
void px_parameter_bind_string(px_parameter *restrict parameter, const char *restrict str);

// queries
px_query *px_query_new(const char *restrict commandText, px_connection *restrict connection);
void px_query_delete(px_query *query);
void px_query_add_parameter(px_query *restrict query, const px_parameter *restrict parameter);
bool px_query_prepare(const px_query *restrict query);
bool px_query_bind(const px_query *restrict query);
px_result_list *px_query_execute(const px_query *restrict query);

// results
void px_result_delete(px_result *result);
void px_result_list_delete(px_result_list *result_list, bool keepElements);

const unsigned int px_result_get_column_count(const px_result *restrict result) __attribute__((pure));
const unsigned int px_result_get_row_count(const px_result *restrict result) __attribute__((pure));

const char* px_result_get_command_tag(const px_result *restrict result) __attribute__((pure));
const px_command_type px_result_get_command_type(const px_result *restrict result) __attribute__((pure));
const unsigned int px_result_get_affected_rows(const px_result *restrict result) __attribute__((pure));
const unsigned int px_result_get_row_oid(const px_result *restrict result) __attribute__((pure));

bool px_result_is_db_null(const px_result *restrict result, const unsigned int column, const unsigned int row) __attribute__((pure));

const char *px_result_get_column_name(const px_result *restrict result, const unsigned int index) __attribute__((pure));
const unsigned int px_result_get_column_datatype(const px_result *restrict result, const unsigned int index) __attribute__((pure));
char *px_result_copy_column_datatype_as_string(const px_result *restrict result, const unsigned int index);

char *px_result_copy_cell_value_as_string(const px_result *restrict result, const unsigned int column, const unsigned int row);

// utility functions
const size_t px_utf8_strlen(const char *str) __attribute__((const));

#endif
