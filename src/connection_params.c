//
//  connection_params.c
//  libpx
//
//  Created by Tamas Czinege on 03/05/2012.
//  Copyright (c) 2012 Tamas Czinege. All rights reserved.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "connection_params.h"
#include "utility.h"

px_connection_params *px_connection_params_new(void)
{
    return calloc(1, sizeof(px_connection_params));
}

px_connection_params *px_connection_params_copy(const px_connection_params *restrict old)
{
    px_connection_params *new = px_connection_params_new();
    px_connection_params_set_hostname(new, px_connection_params_get_hostname(old));
    px_connection_params_set_port(new, px_connection_params_get_port(old));
    px_connection_params_set_database(new, px_connection_params_get_database(old));
    px_connection_params_set_username(new, px_connection_params_get_username(old));
    px_connection_params_set_password(new, px_connection_params_get_password(old));
    px_connection_params_set_application_name(new, px_connection_params_get_application_name(old));
    
    return new;
}

void px_connection_params_delete(px_connection_params *connection_params)
{
    if (connection_params->hostname != NULL)
        free(connection_params->hostname);
    
    if (connection_params->database != NULL)
        free(connection_params->database);
    
    if (connection_params->username != NULL)
        free(connection_params->username);
    
    if (connection_params->password != NULL)
        free(connection_params->password);
    
    if (connection_params->application_name != NULL)
        free(connection_params->application_name);
    
    free(connection_params);
}

const char *px_connection_params_get_hostname(const px_connection_params *restrict connection_params)
{
    return connection_params->hostname;
}

void px_connection_params_set_hostname(px_connection_params *restrict connection_params, const char *restrict value)
{
    if (connection_params->hostname != NULL)
    {
        free(connection_params->hostname);
    }
    
    if (value == NULL)
    {
        connection_params->hostname = NULL;
    }
    else
    {
        connection_params->hostname = px_copy_string(value);
    }
}

const char *px_connection_params_get_username(const px_connection_params *restrict connection_params)
{
    return connection_params->username;
}

void px_connection_params_set_username(px_connection_params *restrict connection_params, const char *restrict value)
{
    if (connection_params->username != NULL)
    {
        free(connection_params->username);
    }
    
    if (value == NULL)
    {
        connection_params->username = NULL;
    }
    else
    {
        connection_params->username = px_copy_string(value);
    }
}

const char *px_connection_params_get_password(const px_connection_params *restrict connection_params)
{
    return connection_params->password;
}

void px_connection_params_set_password(px_connection_params *restrict connection_params, const char *restrict value)
{
    if (connection_params->password != NULL)
    {
        free(connection_params->password);
    }
    
    if (value == NULL)
    {
        connection_params->password = NULL;
    }
    else
    {
        connection_params->password = px_copy_string(value);
    }
}

const char *px_connection_params_get_database(const px_connection_params *restrict connection_params)
{
    return connection_params->database;
}

void px_connection_params_set_database(px_connection_params *restrict connection_params, const char *restrict value)
{
    if (connection_params->database != NULL)
    {
        free(connection_params->database);
    }
    
    if (value == NULL)
    {
        connection_params->database = NULL;
    }
    else
    {
        connection_params->database = px_copy_string(value);
    }
}

unsigned int px_connection_params_get_port(const px_connection_params *restrict connection_params)
{
    return connection_params->port;
}

void px_connection_params_set_port(px_connection_params *restrict connection_params, const unsigned int value)
{
    connection_params->port = value;
}

const char *px_connection_params_get_application_name(const px_connection_params *restrict connection_params)
{
    return connection_params->application_name;
}

void px_connection_params_set_application_name(px_connection_params *restrict connection_params, const char *restrict value)
{
    if (connection_params->application_name != NULL)
    {
        free(connection_params->application_name);
    }
    
    if (value == NULL)
    {
        connection_params->application_name = NULL;
    }
    else
    {
        connection_params->application_name = px_copy_string(value);
    }
}

