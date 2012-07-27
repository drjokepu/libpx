//
//  px.c
//  libpx
//
//  Created by Tamas Czinege on 03/05/2012.
//  Copyright (c) 2012 Tamas Czinege. All rights reserved.
//

#include <err.h>
#include <getopt.h>
#include <pwd.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <editline/readline.h>
#include "px.h"

static bool use_colors = true;
static bool detailed_errors = true;

static void repl(px_connection *restrict connection);
static void eval(px_connection *restrict connection, const char *restrict command_text);
static void print_px_error(const px_error *restrict error);
static void print_result(const px_result *restrict result);
static void print_result_summary(const px_result *restrict result);

static unsigned int max_characters_in_column(const px_result *restrict result, const unsigned int column_number) __attribute__((pure));

static bool password_prompt(const px_connection *connection, void *context);

static void list_tables(px_connection *restrict connection);
static void describe_table(px_connection *restrict connection, const char *restrict table_name);
static bool describe_table_columns(px_connection *restrict connection, const char *restrict table_name);
static bool describe_table_constraints(px_connection *restrict connection, const char *restrict table_name);

int main(int argc, char *const argv[])
{
    px_connection_params *connectionParams = px_connection_params_new();
    
    while (1)
    {
        static struct option options[] =
        {
            { "username", required_argument, 0, 'u' },
            { "host", required_argument, 0, 'h' },
            { "database", required_argument, 0, 'd' }
        };
        
        int optionIndex = 0;
        const int shortOption = getopt_long(argc, argv, "u:h:d:", options, &optionIndex);
        
        if (shortOption == -1)
        {
            break;
        }
        
        switch (shortOption)
        {
            case 'u':
                px_connection_params_set_username(connectionParams, optarg);
                break;
            case 'h':
                px_connection_params_set_hostname(connectionParams, optarg);
                break;
            case 'd':
                px_connection_params_set_database(connectionParams, optarg);
                break;
            default:
                break;
        }
    }
    
    if (px_connection_params_get_username(connectionParams) == NULL)
    {
        uid_t uid = getuid();
        struct passwd *pwd = getpwuid(uid);
        if (pwd == NULL)
        {
            err(2, NULL);
            return 2;
        }
        px_connection_params_set_username(connectionParams, pwd->pw_name);
    }
    
    if (px_connection_params_get_database(connectionParams) == NULL)
    {
        px_connection_params_set_database(connectionParams, px_connection_params_get_username(connectionParams));
    }
    
    if (px_connection_params_get_hostname(connectionParams) == NULL)
    {
        px_connection_params_set_hostname(connectionParams, "localhost");
    }
    
    if (px_connection_params_get_port(connectionParams) == 0)
    {
        px_connection_params_set_port(connectionParams, 5432);
    }
    
    px_connection_params_set_application_name(connectionParams, "px");
    
    px_connection *connection = px_connection_new(connectionParams);
    px_connection_params_delete(connectionParams);
    
    // set up callbacks
    px_connection_set_password_callback(connection, password_prompt);
    
    const px_connection_attempt_result connectionAttemptResult = px_connection_open(connection);
    
    if (connectionAttemptResult != px_connection_attempt_result_success)
    {
        switch (connectionAttemptResult)
        {
            case px_connection_attempt_result_invalid_host:
                errx(1, "Cannot connect to host");
                px_connection_delete(connection);
                return 1;
            case px_connection_attempt_result_authentication_failed:
                errx(2, "Authentication failed");
                px_connection_close(connection);
                px_connection_delete(connection);
                return 2;
                
            default:
            {
                fprintf(stderr, "Error code: %i\n", connectionAttemptResult);
                
                const px_error *pxError = px_connection_get_last_error(connection);
                
                if (pxError == NULL)
                {
                    err(1, NULL);
                    return 1;
                }
                else
                {
                    print_px_error(pxError);
                    return 1;
                }
            }
        }
    }
    
    repl(connection);
    
    px_connection_close(connection);
    px_connection_delete(connection);
}

static void repl(px_connection *restrict connection)
{
    char prompt[256];
    bool exit = false;
    
    while (!exit)
    {
        sprintf(prompt, "%s# ", px_connection_params_get_database(px_connection_get_connection_params(connection)));
        const char *originalLine = readline(prompt);        
        const unsigned int length = strlen(originalLine);
        char *line = malloc(length + 1);
        memcpy(line, originalLine, length + 1);
        
        if (length > 0)
        {
            for (unsigned int i = length - 1; line > 0 && line[i] == '\n'; i--)
            {
                line[i] = 0;
            }
            
            if (strcmp(line, "\\q") == 0)
            {
                exit = true;
            }
            else if (strcmp(line, "\\error") == 0)
            {
                print_px_error(px_connection_get_last_error(connection));
            }
            else if (strcmp(line, "\\dt") == 0)
            {
                list_tables(connection);
            }
            else if (strncmp(line, "\\d ", 3) == 0)
            {
                const char *table_name = line + 3;
                if (*table_name == 0)
                {
                    list_tables(connection);
                }
                else
                {
                    describe_table(connection, table_name);
                }
            }
            else if (strncmp(line, "\\ ", 1) == 0)
            {
                fprintf(stderr, "Unrecognized command: %s\n", line);
            }
            else
            {
                eval(connection, line);
            }
        }
        
        add_history(line);
        free(line);
    }
}

static void eval(px_connection *restrict connection,const char *restrict commandText)
{
    px_query *query = px_query_new(commandText, connection);
    px_result_list *result_list = px_query_execute(query);
    px_query_delete(query);
    
    if (result_list == NULL || result_list->count == 0)
    {
        const px_error *pxError = px_connection_get_last_error(connection);
        
        if (pxError == NULL)
        {
            err(1, NULL);
            return;
        }
        else
        {
            print_px_error(pxError);
        }
    }
    else
    {
        if (result_list->count > 1)
        {
            printf("Received %u results.\n\n", result_list->count);
        }
        
        for (unsigned int i = 0; i < result_list->count; i++)
        {
            if (i > 0) printf("\n");
            px_result *result = result_list->results[i];
            print_result(result);
            print_result_summary(result);
        }
    }
    
    px_result_list_delete(result_list, false);
}

static void print_result(const px_result *restrict result)
{
    const unsigned int column_count = px_result_get_column_count(result);
    
    if (column_count == 0)
    {
        return;
    }
    
    unsigned int *column_lengths = malloc(column_count * sizeof(unsigned int));
    for (unsigned int i = 0; i < column_count; i++)
        column_lengths[i] = max_characters_in_column(result, i);
    
    
    // table header
    for (unsigned int i = 0; i < column_count; i++)
    {
        const unsigned int columnLength = column_lengths[i];
        
        if (i == 0)
            printf("┌─");
        else
            printf("┬─");
        
        for (unsigned int k = 0; k < columnLength + 1; k++)
        {
            printf("─");
        }
    }
    printf("┐\n");
    
    // column names
    for (unsigned int i = 0; i < column_count; i++)
    {
        const char *headerName = px_result_get_column_name(result, i);
        const unsigned int headerLength = px_utf8_strlen(headerName);
        const unsigned int columnLength = column_lengths[i];
        
        printf("│ ");
        
        if (use_colors) printf("\033[1;34;40m");
        printf("%s ", headerName);
        if (use_colors) printf("\033[0m");
        
        for (unsigned int k = 0; k < (columnLength - headerLength); k++)
        {
            printf(" ");
        }

    }
    printf("│\n");
    
    // data types
    for (unsigned int i = 0; i < column_count; i++)
    {
        char *data_type = px_result_copy_column_datatype_as_string(result, i);
        const unsigned int data_type_length = px_utf8_strlen(data_type);
        free(data_type);
        const unsigned int column_length = column_lengths[i];
        
        printf("│ ");
        
        if (use_colors) printf("\033[1;33;40;22m");
        printf("%s ", data_type);
        if (use_colors) printf("\033[0m");
        
        for (unsigned int k = 0; k < (column_length - data_type_length); k++)
        {
            printf(" ");
        }
        
    }
    printf("│\n");
    
    for (unsigned int i = 0; i < column_count; i++)
    {
        const unsigned int columnLength = column_lengths[i];
        
        if (i == 0)
            printf("├─");
        else
            printf("┼─");
        
        for (unsigned int k = 0; k < columnLength + 1; k++)
        {
            printf("─");
        }
    }
    printf("┤\n");
    
    // draw rows
    const unsigned int row_count = px_result_get_row_count(result);
    
    for (unsigned int i = 0; i < row_count; i++)
    {
        for (unsigned int j = 0; j < column_count; j++)
        {
            char *cell_value = px_result_copy_cell_value_as_string(result, j, i);
            const unsigned int cellLength = px_utf8_strlen(cell_value);
            const unsigned int columnLength = column_lengths[j];
            printf("│ ");
            printf("%s ", cell_value);
            free(cell_value);
            for (unsigned int k = 0; k < (columnLength - cellLength); k++)
            {
                printf(" ");
            }
        }
        printf("│\n");
    }
    
    // bottom border
    for (unsigned int i = 0; i < column_count; i++)
    {
        const unsigned int columnLength = column_lengths[i];
        
        if (i == 0)
            printf("└─");
        else
            printf("┴─");
        
        for (unsigned int k = 0; k < columnLength + 1; k++)
        {
            printf("─");
        }
    }
    printf("┘\n");
}

static void print_result_summary(const px_result *restrict result)
{
    const px_command_type command_type = px_result_get_command_type(result);
    const unsigned int affected_rows = px_result_get_affected_rows(result);
    
    if (use_colors) printf("\033[1;32;40;22m");
    
    switch (command_type)
    {
        case px_command_type_select:
            if (affected_rows == 1)
                printf("Selected 1 row.\n");
            else
                printf("Selected %u rows.\n", affected_rows);
            break;
        case px_command_type_insert:
            if (affected_rows == 1)
            {
                const unsigned int row_oid = px_result_get_row_oid(result);
                if (row_oid == 0)
                {
                    printf("Inserted 1 row.\n");
                }
                else
                {
                    printf("Inserted 1 row. (row oid = %u)\n", row_oid);
                }
            }
            else
            {
                printf("Inserted %u rows.\n", affected_rows);
            }
            break;
        case px_command_type_update:
            if (affected_rows == 1)
                printf("Updated 1 row.\n");
            else
                printf("Updated %u rows.\n", affected_rows);
            break;
        case px_command_type_delete:
            if (affected_rows == 1)
                printf("Deleted 1 row.\n");
            else
                printf("Deleted %u rows.\n", affected_rows);
            break;
        case px_command_type_unknown:
        default:
        {
            const char *command_tag = px_result_get_command_tag(result);
            if (command_type == px_command_type_unknown && command_tag == NULL)
                printf("Ok.\n");
            else
                printf("? %s\n", command_tag);
            break;
        }
    }
    
    if (use_colors) printf("\033[0m");
}

static unsigned int max_characters_in_column(const px_result *restrict result, const unsigned int column_number)
{
    unsigned int max = px_utf8_strlen(px_result_get_column_name(result, column_number));
    unsigned int row_count = px_result_get_row_count(result);
    
    char *data_type = px_result_copy_column_datatype_as_string(result, column_number);
    const unsigned int dataTypeLength = px_utf8_strlen(data_type);
    free(data_type);
    if (dataTypeLength > max) max = dataTypeLength;
    
    for (unsigned int i = 0; i < row_count; i++)
    {
        char *cell_value = px_result_copy_cell_value_as_string(result, column_number, i);
        const unsigned int cell_length = px_utf8_strlen(cell_value);
        free(cell_value);
        if (cell_length > max) max = cell_length;
    }
    
    return max;
}

static void print_px_error(const px_error *restrict error)
{
    if (error == NULL)
    {
        fprintf(stderr, "No error.\n");
    }
    else
    {
        if (use_colors)
        {
            if (detailed_errors)
            {
                fprintf(stderr,
                        " Severity: \033[1;31;40;22m%s\033[0m\n"
                        "SQL State: \033[1;31;40;22m%s\033[0m\n"
                        "  Message: \033[1;31;40;22m%s\033[0m\n"
                        "   Detail: \033[1;31;40;22m%s\033[0m\n"
                        "     Hint: \033[1;31;40;22m%s\033[0m\n"
                        " Position: \033[1;31;40;22m%s\033[0m\n"
                        "    Where: \033[1;31;40;22m%s\033[0m\n"
                        "     File: \033[1;31;40;22m%s\033[0m\n"
                        "     Line: \033[1;31;40;22m%s\033[0m\n"
                        "  Routine: \033[1;31;40;22m%s\033[0m\n",
                        px_error_get_severity(error),
                        px_error_get_sqlstate(error),
                        px_error_get_message(error),
                        px_error_get_detail(error),
                        px_error_get_hint(error),
                        px_error_get_position(error),
                        px_error_get_where(error),
                        px_error_get_file(error),
                        px_error_get_line(error),
                        px_error_get_routine(error));
            }
            else
            {
                fprintf(stderr,
                        "SQL State: \033[1;31;40;22m%s\033[0m\n  Message: \033[1;31;40;22m%s\033[0m\n",
                        px_error_get_sqlstate(error),
                        px_error_get_message(error));
            }
        }
        else
        {
            if (detailed_errors)
            {
                
            }
            else
            {
                fprintf(stderr,
                    "SQL State: %s\n  Message: %s\n",
                    px_error_get_sqlstate(error),
                    px_error_get_message(error));
            }
        }
    }
}

static bool password_prompt(const px_connection *connection, void *context)
{
    px_connection_params_set_password(px_connection_get_connection_params(connection), getpass("Password: "));
    return true;
}

static void list_tables(px_connection *restrict connection)
{
    static const char *command_text =
    "SELECT n.nspname as \"Schema\","
    "c.relname as \"Name\","
    "CASE c.relkind WHEN 'r' THEN 'table' WHEN 'v' THEN 'view' WHEN 'i' THEN 'index' WHEN 'S' THEN 'sequence' WHEN 's' THEN 'special' END as \"Type\","
    "pg_catalog.pg_get_userbyid(c.relowner) as \"Owner\" "
    "FROM pg_catalog.pg_class c "
    "LEFT JOIN pg_catalog.pg_namespace n ON n.oid = c.relnamespace "
    "WHERE c.relkind IN ('r','') "
    "AND n.nspname <> 'pg_catalog' "
    "AND n.nspname <> 'information_schema' "
    "AND n.nspname !~ '^pg_toast' "
    "AND pg_catalog.pg_table_is_visible(c.oid) "
    "ORDER BY 1,2;";
    
    eval(connection, command_text);
}

static void describe_table(px_connection *restrict connection, const char *restrict table_name)
{
    if (!describe_table_columns(connection, table_name)) return;
    if (!describe_table_constraints(connection, table_name)) return;
}

static bool describe_table_columns(px_connection *restrict connection, const char *restrict table_name)
{
    static const char *command_text =
    "select a.attname as name, pg_catalog.format_type(a.atttypid, a.atttypmod) as type, a.attnotnull as \"not null\", d.adsrc as \"default\" from pg_attribute a left outer join pg_attrdef d on (d.adrelid = a.attrelid and d.adnum = a.attnum)  where a.attrelid = $1::regclass and a.attnum > 0 and a.attisdropped = false;";
    
    px_parameter *table_name_parameter = px_parameter_new_string(table_name);
    px_query *query = px_query_new(command_text, connection);
    px_query_add_parameter(query, table_name_parameter);
    px_parameter_delete(table_name_parameter);
    
    px_result_list *result_list = px_query_execute(query);
    const bool success = result_list != NULL && result_list->count > 0;
    
    if (success)
    {
        printf("Table Columns:\n");
        print_result(result_list->results[0]);
    }
    else
    {
        print_px_error(px_connection_get_last_error(connection));
    }

    px_result_list_delete(result_list, false);
    
    return success;
}

static bool describe_table_constraints(px_connection *restrict connection, const char *restrict table_name)
{
    static const char *command_text = 
    "select c.conname as name, "
    "case when c.contype = 'c' then 'check' when c.contype = 'p' then 'primary key' when c.contype = 'u' then 'unique key' when c.contype = 't' then 'constraint trigger' when c.contype = 'x' then 'exclusion constraint' else 'unknown' end as type, "
    "c.confrelid::regclass as \"referenced table\" "
    "from pg_constraint c "
    "left outer join pg_attribute a on (a.attrelid = c.conrelid and a.attnum = ANY(c.conkey)) "
    "where c.conrelid = $1::regclass";
    
    px_parameter *table_name_parameter = px_parameter_new_string(table_name);
    px_query *query = px_query_new(command_text, connection);
    px_query_add_parameter(query, table_name_parameter);
    px_parameter_delete(table_name_parameter);
    
    px_result_list *result_list = px_query_execute(query);
    const bool success = result_list != NULL && result_list->count > 0;
    
    if (success)
    {
        printf("\nTable Constaints:\n");
        print_result(result_list->results[0]);
    }
    else
    {
        print_px_error(px_connection_get_last_error(connection));
    }   
    
    px_result_list_delete(result_list, false);
    
    return success;
}