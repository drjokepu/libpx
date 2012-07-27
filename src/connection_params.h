//
//  connection_params.h
//  libpx
//
//  Created by Tamas Czinege on 03/05/2012.
//  Copyright (c) 2012 Tamas Czinege. All rights reserved.
//

#ifndef libpx_connection_params_h
#define libpx_connection_params_h
#include "typedef.h"

struct px_connection_params
{
    char *hostname;
    unsigned int port;
    char *database;
    char *username;
    char *password;
    
    char *application_name;
};

// creation, copying & deletion of connection params
px_connection_params *px_connection_params_new(void);
px_connection_params *px_connection_params_copy(const px_connection_params *restrict old);
void px_connection_params_delete(px_connection_params *connection_params);

// getters & setters of connection params
const char *px_connection_params_get_hostname(const px_connection_params *restrict connection_params) __attribute__((pure));
void px_connection_params_set_hostname(px_connection_params *restrict connection_params, const char *restrict value);

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

#endif /* libpx_connection_params_h */
