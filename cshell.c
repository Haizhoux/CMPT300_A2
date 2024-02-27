#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdbool.h>
#include <time.h>
//#include <process.h>

#define LINE_MAX 1024
#define MAX_ARG_LEN 256
#define LOG_MAX 2048
#define ENV_MAX 1024

typedef struct
{
    char *name;
    char *value;
} EnvVar;

typedef struct
{
    char *name;
    struct tm time;
    int return_value;
} Command;

// variable
int args_number;
char env_name[MAX_ARG_LEN];
char env_value[MAX_ARG_LEN];
EnvVar env_var_array[ENV_MAX];
int env_elements_number;
char trimed_line[LINE_MAX];
char program_name[LINE_MAX];
char args_line[LINE_MAX];
char *arg_list[MAX_ARG_LEN];
Command log_list[LOG_MAX];
int log_elements_number;
char red[LINE_MAX] = "\033[1;31m";
char blue[LINE_MAX] = "\033[1;34m";
char green[LINE_MAX] = "\033[0;32m";
char white[LINE_MAX] = "\e[0;37m";
char color_code[MAX_ARG_LEN] = "\e[0;37m";
char script_lines[LINE_MAX][LINE_MAX];

// MODE 1 FUNCTION
int get_built_in_command_mode1(char *line);
void print_2d_array_mode1(char **array, int row);
void get_program_mode1(char *after_trimed_line);
void print_EnvVar_mode1(const char *org_name);
void print_log_mode1();
void append2log_list_mode1(char *command_name, int value);

void print_log_mode1()
{
    for (int i = 0; i < log_elements_number; i++)
    {
        printf("%s%s", color_code, asctime(&log_list[i].time));
        printf("%s %s %d\n", color_code, log_list[i].name, log_list[i].return_value);
    }
}

void append2log_list_mode1(char *command_name, int value)
{
    time_t rawtime;
    struct tm *info;
    time(&rawtime);
    log_list[log_elements_number].name = strdup(command_name);
    log_list[log_elements_number].return_value = value;
    info = localtime(&rawtime);
    log_list[log_elements_number].time = *info;
    log_elements_number++;
}

void trim_line_mode1(char *line)
{
    int line_len = strlen(line);
    int count_start = 0;
    int count_end = 0;

    memset(trimed_line, 0, sizeof(trimed_line));

    if (line_len == 0)
    {
        trimed_line[0] = '\0';
    }

    for (int i = 0; i < strlen(line); i++)
    {
        if (line[i] != ' ')
        {
            count_start = i;
            break;
        }
    }

    for (int j = line_len - 1; j > count_start; j--)
    {
        if (line[j] != ' ' && line[j] != '\n')
        {
            count_end = j;
            break;
        }
    }

    int k = 0;
    for (int j = count_start; j <= count_end; j++)
    {
        trimed_line[k] = line[j];
        k++;
    }
    trimed_line[k] = '\0';
}

// built_in_command return value
// 0: error case
// 1: print
// 2: log
// 3: theme
// 4: exit
// 5: EnvVar
// -1: Non built in
int get_built_in_command_mode1(char *line)
{
    if (strcmp("print", program_name) == 0)
    {
        return 1;
    }
    if (strcmp("log", program_name) == 0)
    {
        return 2;
    }
    if (strcmp("theme", program_name) == 0)
    {
        return 3;
    }
    if (strcmp("exit", program_name) == 0)
    {
        return 4;
    }
    else if (line[0] == '$')
    {
        return 5;
    }
    return -1;
}

void get_name_mode1(char *line)
{
    int name_index = 0;
    int env_name_index = 0;
    memset(env_name, 0, sizeof(env_name));
    for (int i = 1; i < strlen(line); i++)
    {
        if (line[i] == '=')
        {
            name_index = i;
            break;
        }
    }

    for (int i = 1; i < name_index; i++)
    {
        env_name[env_name_index] = line[i];
        env_name_index++;
    }

    env_name[env_name_index] = '\0';
    return;
}

void get_value_mode1(char *line)
{
    int value_index = 0;
    int env_value_index = 0;
    memset(env_value, 0, sizeof(env_value));
    for (int i = 1; i < strlen(line); i++)
    {
        if (line[i] == '=')
        {
            value_index = i + 1;
            break;
        }
    }

    for (int i = value_index; i < strlen(line) - 1; i++)
    {
        env_value[env_value_index] = line[i];
        env_value_index++;
    }

    env_value[env_value_index] = '\0';
    return;
}

void print_EnvVar_mode1(const char *org_name)
{
    char name[MAX_ARG_LEN];
    strncpy(name, org_name + 1, strlen(org_name) - 1);
    for (int i = 0; i < env_elements_number; i++)
    {
        if (strcmp(env_var_array[i].name, name) == 0)
        {
            printf("%s%s\n", color_code, env_var_array[i].value);
            append2log_list_mode1("print", 0);
            return;
        }
    }

    printf("%sError: No Environment Variable %s found.\n", color_code, org_name);
    append2log_list_mode1("print", 1);
}

void EnvVar_handler_mode1(char *line)
{
    bool find_equal = false;

    for (int i = 0; i < strlen(trimed_line); i++)
    {
        if (trimed_line[i] == ' ')
        {
            printf("%sVariable value expected.\n", color_code);
            return;
        }
        if (trimed_line[i] == '=')
        {
            find_equal = true;
        }
    }

    if (find_equal == false)
    {
        printf("%sError: No name and/or variable found for setting up Environment Variables.\n", color_code);
        return;
    }

    // legal input, then store the value
    get_name_mode1(line);
    get_value_mode1(line);

    // if the variable name already in the array, update value then return
    for (int i = 0; i < env_elements_number; i++)
    {
        // printf("my name of the array and env is: %s %s\n", env_var_array[i].name, env_name);
        if (strcmp(env_var_array[i].name, env_name) == 0)
        {
            env_var_array[i].value = env_value;
            return;
        }
    }

    // new variable store into the array
    env_var_array[env_elements_number].name = strdup(env_name);
    env_var_array[env_elements_number].value = strdup(env_value);
    env_elements_number++;

    return;
}

void get_program_mode1(char *after_trimed_line)
{
    int i;
    int len = strlen(after_trimed_line);
    int program_name_len;
    memset(program_name, 0, sizeof(program_name));

    if (len == 0)
    {
        program_name[0] = '\0';
    }

    for (i = 0; i < len; i++)
    {
        program_name[i] = after_trimed_line[i];
        if (after_trimed_line[i] == ' ')
        {
            break;
        }
    }
    program_name[i] = '\0';
    // Remove program name from trimed_line and then store into args_line
    program_name_len = strlen(program_name);
    int k = 0;
    int start_index = 0;
    if (len != program_name_len)
    {
        for (int i = program_name_len; i < len; i++)
        {
            if (after_trimed_line[i] != ' ')
            {
                start_index = i;
                break;
            }
        }
        memset(args_line, 0, sizeof(args_line));
        for (int i = start_index; i < len; i++)
        {
            args_line[k] = after_trimed_line[i];
            k++;
        }
        args_line[k] = '\0';
    }
    else
    {
        memset(args_line, 0, sizeof(args_line));
    }
}

int get_args_mode1()
{
    int index_row = 0;
    int index_col = 0;
    int args_len = strlen(args_line);
    int flag = 0;
    if (args_len == 0)
    {
        arg_list[0] = NULL;
        return 1;
    }
    char temp[256] = {0};

    for (int j = 0; j < args_len; j++)
    {
        if (args_line[j] == ' ' && flag == 1)
        {
            temp[index_col] = '\0';
            arg_list[index_row] = (const char *)strdup(temp);
            memset(temp, 0, sizeof(temp));
            flag = 0;
            index_row++;
            index_col = 0;
        }
        if (args_line[j] != ' ')
        {
            if (flag == 0)
            {
                flag = 1;
                temp[index_col] = args_line[j];
                index_col++;
            }
            else
            {
                temp[index_col] = args_line[j];
                index_col++;
            }
        }
        if (j == args_len - 1)
        {
            if (flag == 1)
            {
                flag = 0;
                temp[index_col] = args_line[j];
                index_col++;
                temp[index_col - 1] = '\0';
                arg_list[index_row] = (const char *)strdup(temp);
                memset(temp, 0, sizeof(temp));
                index_row++;
                index_col = 0;
            }
        }
    }
    arg_list[index_row] = NULL;
    return index_row + 1;
}

void print_2d_array(char **array, int row)
{
    int i;
    printf("start printing array:\n");
    for (i = 0; i < row; i++)
    {
        printf("%d\t%s\n", i, array[i]);
    }
    printf("finished printing array\n");
}

// MODE 2 FUNCTION
int get_built_in_command_mode2(char *line);
void print_2d_array_mode2(char **array, int row);
void get_program_mode2(char *after_trimed_line);
void print_EnvVar_mode2(const char *org_name);
void print_log_mode2();
void append2log_list_mode2(char *command_name, int value);

void print_log_mode2()
{
    for (int i = 0; i < log_elements_number; i++)
    {
        printf("%s%s", color_code, asctime(&log_list[i].time));
        printf("%s%s\t%d\n", color_code, log_list[i].name, log_list[i].return_value);
    }
}

void append2log_list_mode2(char *command_name, int value)
{
    time_t rawtime;
    struct tm *info;
    time(&rawtime);
    log_list[log_elements_number].name = strdup(command_name);
    log_list[log_elements_number].return_value = value;
    info = localtime(&rawtime);
    log_list[log_elements_number].time = *info;
    log_elements_number++;
}

void trim_line_mode2(char *line)
{
    int line_len = strlen(line);
    int count_start = 0;
    int count_end = 0;

    memset(trimed_line, 0, sizeof(trimed_line));

    if (line_len == 0)
    {
        trimed_line[0] = '\0';
    }

    for (int i = 0; i < strlen(line); i++)
    {
        if (line[i] != ' ')
        {
            count_start = i;
            break;
        }
    }

    for (int j = line_len - 1; j > count_start; j--)
    {
        if (line[j] != ' ' && line[j] != '\n')
        {
            count_end = j;
            break;
        }
    }

    int k = 0;
    for (int j = count_start; j <= count_end; j++)
    {
        trimed_line[k] = line[j];
        k++;
    }
    trimed_line[k] = '\0';
}

// built_in_command return value
// 0: error case
// 1: print
// 2: log
// 3: theme
// 4: exit
// 5: EnvVar
// -1: Non built in
int get_built_in_command_mode2(char *line)
{
    if (strncmp("print", program_name, 4) == 0)
    {
        return 1;
    }
    if (strncmp("log", program_name, 3) == 0)
    {
        return 2;
    }
    if (strncmp("theme", program_name, 5) == 0)
    {
        return 3;
    }
    if (strncmp("exit", program_name, 4) == 0)
    {
        return 4;
    }
    else if (line[0] == '$')
    {
        return 5;
    }
    return -1;
}

void get_name_mode2(char *line)
{
    int name_index = 0;
    int env_name_index = 0;
    memset(env_name, 0, sizeof(env_name));
    for (int i = 1; i < strlen(line); i++)
    {
        if (line[i] == '=')
        {
            name_index = i;
            break;
        }
    }

    for (int i = 1; i < name_index; i++)
    {
        env_name[env_name_index] = line[i];
        env_name_index++;
    }

    env_name[env_name_index] = '\0';
    return;
}

void get_value_mode2(char *line)
{
    int value_index = 0;
    int env_value_index = 0;
    memset(env_value, 0, sizeof(env_value));
    for (int i = 1; i < strlen(line); i++)
    {
        if (line[i] == '=')
        {
            value_index = i + 1;
            break;
        }
    }

    for (int i = value_index; i < strlen(line) - 1; i++)
    {
        env_value[env_value_index] = line[i];
        env_value_index++;
    }

    env_value[env_value_index] = '\0';
    return;
}

void print_EnvVar_mode2(const char *org_name)
{
    char name[MAX_ARG_LEN] = {0};
    strncpy(name, org_name + 1, strlen(org_name) - 1);
    for (int i = 0; i < env_elements_number; i++)
    {
        if (strncmp(env_var_array[i].name, name, strlen(name)-1) == 0)
        {
            printf("%s%s\n", color_code, env_var_array[i].value);
            append2log_list_mode2("print", 0);
            return;
        }
    }

    printf("%sError: No Environment Variable %s found.\n", color_code, org_name);
    append2log_list_mode2("print", 1);
}

void EnvVar_handler_mode2(char *line)
{
    bool find_equal = false;

    for (int i = 0; i < strlen(trimed_line); i++)
    {
        if (trimed_line[i] == ' ')
        {
            printf("%sVariable value expected.\n", color_code);
            return;
        }
        if (trimed_line[i] == '=')
        {
            find_equal = true;
        }
    }

    if (find_equal == false)
    {
        printf("%sError: No name and/or variable found for setting up Environment Variables.\n", color_code);
        return;
    }

    get_name_mode2(line);
    get_value_mode2(line);

    for (int i = 0; i < env_elements_number; i++)
    {
        if (strcmp(env_var_array[i].name, env_name) == 0)
        {
            env_var_array[i].value = env_value;
            return;
        }
    }

    env_var_array[env_elements_number].name = strdup(env_name);
    env_var_array[env_elements_number].value = strdup(env_value);
    env_elements_number++;

    
    return;
}

void get_program_mode2(char *after_trimed_line)
{
    int i;
    int len = strlen(after_trimed_line);
    int program_name_len;
    memset(program_name, 0, sizeof(program_name));

    if (len == 0)
    {
        program_name[0] = '\0';
    }

    for (i = 0; i < len; i++)
    {
        program_name[i] = after_trimed_line[i];
        if (after_trimed_line[i] == ' ')
        {
            break;
        }
    }
    program_name[i] = '\0';
    program_name_len = strlen(program_name);
    int k = 0;
    int start_index = 0;
    if (len != program_name_len)
    {
        for (int i = program_name_len; i < len; i++)
        {
            if (after_trimed_line[i] != ' ')
            {
                start_index = i;
                break;
            }
        }
        memset(args_line, 0, sizeof(args_line));
        for (int i = start_index; i < len; i++)
        {
            args_line[k] = after_trimed_line[i];
            k++;
        }
        args_line[k] = '\0';
    }
    else
    {
        memset(args_line, 0, sizeof(args_line));
    }
}

int get_args_mode2()
{
    int index_row = 0;
    int args_len = strlen(args_line);
    if (args_len == 0)
    {
        arg_list[0] = NULL;
        return 1;
    }
    char temp[256] = {0};
    char delim[] = " ";
    char *ptr = strtok(args_line, delim);

    while (ptr != NULL)
    {
        strcpy(temp, ptr);
        arg_list[index_row] = (const char *)strdup(temp);
        memset(temp, 0, sizeof(temp));
        index_row++;
        ptr = strtok(NULL, delim);
    }
    arg_list[index_row] = NULL;
    return index_row + 1;
}

int main(int argc, char **argv)
{
    char line[LINE_MAX];
    pid_t pid;
    int built_in_command_type;
    env_elements_number = 0;
    log_elements_number = 0;

    if (argc == 1)
    {
        // interactive mode
        // printf("Enter interactive mode\n");
        while (1)
        {
            // TODO: theme color
            //  color = "%RED"
            //  printf("%scshell$:", color);
            printf("%scshell$%s:", color_code, white);
            fgets(line, LINE_MAX, stdin);
            trim_line_mode1(line);
            get_program_mode1(trimed_line);
            int number_args;
            // check if built-in command
            built_in_command_type = get_built_in_command_mode1(line);
            switch (built_in_command_type)
            {
            case -1:
                number_args = get_args_mode1();
                pid = fork();
                if (pid == 0)
                {
                    if (number_args == 1)
                    {
                        execlp(program_name, program_name, NULL);
                    }
                    else
                    {
                        execvp(program_name, arg_list);
                    }
                }
                else
                {
                    append2log_list_mode1(program_name, 0);
                    wait(NULL);
                }
                break;
            case 1:
                // print
                number_args = get_args_mode1();

                if (strcmp(trimed_line, "print") == 0)
                {
                    append2log_list_mode1(program_name, -1);
                    printf("%sNo arguments to print.\n", color_code);
                    break;
                }

                if (number_args == 2 && arg_list[0][0] == '$')
                {
                    print_EnvVar_mode1(arg_list[0]);
                }
                else
                {
                    for (int i = 0; i < number_args - 1; i++)
                    {
                        printf("%s%s ", color_code, arg_list[i]);
                    }
                    append2log_list_mode1(program_name, 0);
                    printf("\n");
                }
                break;
            case 2:
                // log
                append2log_list_mode1(program_name, 0);
                print_log_mode1();
                break;
            case 3:
                // theme
                trim_line_mode1(line);
                number_args = get_args_mode1();

                if (strcmp(trimed_line, "theme") == 0)
                {
                    printf("%s", color_code);
                    printf("%sunsupported theme.\n", color_code);
                    append2log_list_mode1(program_name, -1);
                }
                else if (strcmp(arg_list[0], "red") == 0 && number_args == 2)
                {
                    printf("%s", red);
                    strcpy(color_code, red);
                    append2log_list_mode1(program_name, 0);
                }
                else if (strcmp(arg_list[0], "blue") == 0 && number_args == 2)
                {
                    printf("%s", blue);
                    strcpy(color_code, blue);
                    append2log_list_mode1(program_name, 0);
                }
                else if (strcmp(arg_list[0], "green") == 0 && number_args == 2)
                {
                    printf("%s", green);
                    strcpy(color_code, green);
                    append2log_list_mode1(program_name, 0);
                }
                else
                {
                    printf("%s", color_code);
                    printf("%sunsupported theme.\n", color_code);
                    append2log_list_mode1(program_name, -1);
                }
                break;
            case 4:
                // exit
                printf("%sBye!\n", color_code);
                exit(0);
                break;
            case 5:
                EnvVar_handler_mode1(line);
                break;
            }
        }
    }
    else if (argc == 2)
    {
        char buffer[LINE_MAX] = {0};
        FILE *file;
        file = fopen(argv[1], "r");
        int number_args;
        int script_line_index = 0;
        int i;

        if (file == NULL)
        {
            printf("Unable to read script file: %s\n", argv[1]);
            return 1;
        }

        while (fgets(buffer, sizeof(buffer), file))
        {
            strcpy(script_lines[script_line_index], buffer);
            script_line_index++;
        }

        for (int j = 0; j < script_line_index + 1; j++)
        {
            trim_line_mode2(script_lines[j]);
            get_program_mode2(trimed_line);
            number_args = get_args_mode2();
            built_in_command_type = get_built_in_command_mode2(trimed_line);

            switch (built_in_command_type)
            {
            case -1:
                program_name[strlen(program_name)] = 0;
                program_name[strlen(program_name) - 1] = '\0';
                pid = fork();
                if (pid == 0)
                {
                    if (number_args == 1)
                    {
                        execlp(program_name, program_name, NULL);
                    }
                    else
                    {
                        execvp(program_name, arg_list);
                    }
                }
                else
                {
                    append2log_list_mode2(program_name, 0);
                    wait(NULL);
                }
                break;
            case 1:
                if (strcmp(trimed_line, "print") == 0)
                {
                    append2log_list_mode2(program_name, -1);
                    printf("%sNo arguments to print.\n", color_code);
                    break;
                }
                if (number_args == 2 && arg_list[0][0] == '$')
                {
                    print_EnvVar_mode2(arg_list[0]);
                }
                else
                {
                    for (i = 0; i < number_args - 1; i++)
                    {
                        printf("%s%s\t", color_code, arg_list[i]);
                    }
                    append2log_list_mode2(program_name, 0);
                    printf("\n");
                }
                break;
            case 2:
                append2log_list_mode2(program_name, 0);
                print_log_mode2();
                break;
            case 3:
                trim_line_mode2(buffer);
                number_args = get_args_mode2();
                if (strcmp(trimed_line, "theme") == 0)
                {
                    printf("%s", color_code);
                    printf("%sunsupported theme.\n", color_code);
                    append2log_list_mode2(program_name, -1);
                }
                else if (strncmp(arg_list[0], "red", 3) == 0 && number_args == 2)
                {
                    printf("%s", red);
                    strcpy(color_code, red);
                    append2log_list_mode2(program_name, 0);
                }
                else if (strncmp(arg_list[0], "blue", 4) == 0 && number_args == 2)
                {
                    printf("%s", blue);
                    strcpy(color_code, blue);
                    append2log_list_mode2(program_name, 0);
                }
                else if (strncmp(arg_list[0], "green", 5) == 0 && number_args == 2)
                {
                    printf("%s", green);
                    strcpy(color_code, green);
                    append2log_list_mode2(program_name, 0);
                }
                else
                {
                    printf("%s", color_code);
                    printf("%sunsupported theme.\n", color_code);
                    append2log_list_mode2(program_name, -1);
                }
                break;
            case 4:
                printf("%sBye!\n", color_code);
                exit(0);
                break;
            case 5:
                EnvVar_handler_mode2(trimed_line);
                break;
            }
            memset(buffer, 0, sizeof(buffer));
        }
        fclose(file);
    }
    else
    {
        printf("Error, too many arguments.\n");
        printf("Usage:\nInteractive mode: ./cshell\n");
        printf("Script mode: ./cshell [script file]\n");
        exit(0);
    }
    return 0;
}