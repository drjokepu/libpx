//
//  message.c
//  libpx
//
//  Created by Tamas Czinege on 04/05/2012.
//  Copyright (c) 2012 Tamas Czinege. All rights reserved.
//

#include "message.h"
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h> 
#include <string.h>
#include <unistd.h>

typedef union
{
    unsigned int uint32;
    unsigned short uint16;
    char character;
} stack_values;

static void px_message_build_message_data(const char *restrict pattern, void** parameters, unsigned int *parameter_cursor, size_t *message_length, size_t *message_buffer_size, void **message, unsigned int *length_offset, bool is_sub_pattern);

static px_message *px_message_new_sync(void);

px_message *px_message_new(const char *restrict pattern, ...)
{
    const size_t pattern_length = strlen(pattern); 
    size_t message_length = 0;
    size_t message_buffer_size = 4096;
    void *message = malloc(message_buffer_size);
    unsigned int length_offset = 0;
    
    va_list parameters;
    va_start(parameters, pattern);
    for (unsigned int i = 0; i < pattern_length; i++)
    {
        stack_values values;
        void *data = NULL;
        size_t data_length = 0;
        
        if (i == 0) // type prefix
        {
            const char type = pattern[0];
            if (type == '0') continue;
            values.character = type;
            data = &(values.character);
            data_length = sizeof(char);
            length_offset = sizeof(char);
        }
        else if (i == 1) // message length
        {
            message_length += sizeof(unsigned int);
            continue;
        }
        else // message body
        {
            switch (pattern[i])
            {
                case 'c':
                {
                    values.character = (char)va_arg(parameters, int);
                    data = &(values.character);
                    data_length = sizeof(char);
                    break;
                }
                case 'i':
                {
                    values.uint32 = htonl(va_arg(parameters, unsigned int));
                    data = &(values.uint32);
                    data_length = sizeof(unsigned int);
                    break;
                }
                case 's':
                {
                    const char *string = va_arg(parameters, char*);
                    data_length = strlen(string) + 1;
                    data = (void*)string;
                    break;
                }
                case 'b':
                {
                    data = va_arg(parameters, void*);
                    data_length = va_arg(parameters, size_t);
                    break;
                }
                default:
                    break;
            }
        }
        
        if (message_length + data_length < message_buffer_size)
        {
            message_buffer_size *= 2;
            message = realloc(message, message_buffer_size);
        }
        memcpy(message + message_length, data, data_length);
        message_length += data_length;
    }
    va_end(parameters);
    
    *((int*)((char*)message + length_offset)) = htonl(message_length - length_offset);
    
    px_message *result = malloc(sizeof(px_message));
    result->messageBytes = message;
    result->messageLength = message_length;
    
    return result;
}

px_message *px_message_new_with_array(const char *restrict pattern, void** parameters)
{
    size_t message_length = 0;
    size_t message_buffer_size = 4096;
    void *message = malloc(message_buffer_size);
    unsigned int length_offset = 0;
    unsigned int parameter_cursor = 0;
    
    px_message_build_message_data(pattern, parameters, &parameter_cursor, &message_length, &message_buffer_size, &message, &length_offset, false);
    
    *((int*)((char*)message + length_offset)) = htonl(message_length - length_offset);
    
    px_message *result = malloc(sizeof(px_message));
    result->messageBytes = message;
    result->messageLength = message_length;
    
    return result;
}

static void px_message_build_message_data(const char *restrict pattern, void** parameters, unsigned int *parameter_cursor, size_t *message_length, size_t *message_buffer_size, void **message, unsigned int *length_offset, bool is_sub_pattern)
{
    const size_t pattern_length = strlen(pattern); 

    for (unsigned int i = 0; i < pattern_length; i++)
    {
        stack_values values;
        void *data = NULL;
        size_t data_length = 0;
        
        if (!is_sub_pattern && i == 0) // type prefix
        {
            const char type = pattern[0];
            if (type == '0') continue;
            values.character = type;
            data = &(values.character);
            data_length = sizeof(char);
            *length_offset = sizeof(char);
        }
        else if (!is_sub_pattern && i == 1) // message length
        {
            *message_length += sizeof(unsigned int);
            continue;
        }
        else // message body
        {
            switch (pattern[i])
            {
                case 'c':
                {
                    values.character = (char)parameters[(*parameter_cursor)++];
                    data = &(values.character);
                    data_length = sizeof(char);
                    break;
                }
                case 'i':
                {
                    values.uint32 = htonl((unsigned int)parameters[(*parameter_cursor)++]);
                    data = &(values.uint32);
                    data_length = sizeof(unsigned int);
                    break;
                }
                case 'w':
                    values.uint16 = htons((unsigned short)parameters[(*parameter_cursor)++]);
                    data = &(values.uint16);
                    data_length = sizeof(unsigned short);
                    break;
                case 's':
                {
                    const char *string = (char*)parameters[(*parameter_cursor)++];
                    data = (void*)string;
                    data_length = strlen(string) + 1;
                    break;
                }
                case 'S':
                {
                    const char *string = (char*)parameters[(*parameter_cursor)++];
                    data = (void*)string;
                    data_length = strlen(string);
                    break;
                }
                case 'b':
                {
                    data = parameters[(*parameter_cursor)++];
                    data_length = (size_t)parameters[(*parameter_cursor)++];
                    break;
                }
                case '(':
                {
                    const unsigned int subpattern_count = (unsigned int)parameters[(*parameter_cursor)++];
                    unsigned int parenthesis_depth = 0;
                    unsigned int closing_parenthesis_position = i;
                    
                    do
                    {
                        switch (pattern[closing_parenthesis_position])
                        {
                            case '(':
                                parenthesis_depth++;
                                break;
                            case ')':
                                parenthesis_depth--;
                                break;
                            default:
                                break;
                        }
                        if (parenthesis_depth > 0) closing_parenthesis_position++;
                    }
                    while (closing_parenthesis_position < pattern_length && parenthesis_depth > 0);
                    
                    if (parenthesis_depth == 0)
                    {
                        if (closing_parenthesis_position - i <= 1)
                        {
                            i = closing_parenthesis_position;
                            break;
                        }
                        
                        const unsigned int sub_pattern_length = closing_parenthesis_position - i + 1 - 2;
                        char *sub_pattern = malloc(sub_pattern_length * sizeof(char));
                        memcpy(sub_pattern, pattern + i + 1, sub_pattern_length);
                        
                        for (unsigned int j = 0; j < subpattern_count; j++)
                        {
                            px_message_build_message_data(sub_pattern, parameters, parameter_cursor, message_length, message_buffer_size, message, length_offset, true);
                        }
                        free(sub_pattern);
                        i = closing_parenthesis_position;
                    }
                    else
                    {
                        fprintf(stderr, "Invalid px_message pattern\n");
                        return;
                    }
                }
                default:
                    break;
            }
        }
        
        if (message_length + data_length < message_buffer_size)
        {
            *message_buffer_size *= 2;
            *message = realloc(*message, *message_buffer_size);
        }
        memcpy(*message + *message_length, data, data_length);
        *message_length += data_length;
    }
}

void px_message_delete(px_message *message)
{
    free(message->messageBytes);
    free(message);
}

bool px_message_send(const px_message *restrict message, const int file_descriptor)
{
    const ssize_t bytes_written = write(file_descriptor, message->messageBytes, message->messageLength);
    return bytes_written != -1 && bytes_written == message->messageLength;
}

static px_message *px_message_new_sync(void)
{
    return px_message_new("ST");
}

bool px_message_send_sync(const int file_descriptor)
{
    px_message *message = px_message_new_sync();
    const bool success = px_message_send(message, file_descriptor);
    px_message_delete(message);
    return success;
}

