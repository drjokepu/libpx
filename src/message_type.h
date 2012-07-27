//
//  message_type.h
//  libpx
//
//  Created by Tamas Czinege on 07/05/2012.
//  Copyright (c) 2012 Tamas Czinege. All rights reserved.
//

#ifndef libpx_message_type_h
#define libpx_message_type_h

typedef enum px_message_type
{
    px_message_type_authentication_ok,
    px_message_type_authentication_md5_password,
    px_message_type_backend_key_data,
    px_message_type_bind_complete,
    px_message_type_close_complete,
    px_message_type_command_complete,
    px_message_type_data_row,
    px_message_type_error,
    px_message_type_parameter_status,
    px_message_type_parse_complete,
    px_message_type_ready_for_query,
    px_message_type_row_description
} px_message_type;

typedef enum px_message_class
{
    px_message_class_undefined = 0,
    px_message_class_parse_complete = '1',
    px_message_class_bind_complete = '2',
    px_message_class_close_complete = '3',
    px_message_class_command_complete = 'C',
    px_message_class_data_row = 'D',
    px_message_class_error = 'E',
    px_message_class_cancellation_key_data = 'K',
    px_message_class_authentication_request = 'R',
    px_message_class_runtime_parameter_status_report = 'S',
    px_message_class_row_description = 'T',
    px_message_class_ready_for_query = 'Z'
} px_message_class;

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

#endif
