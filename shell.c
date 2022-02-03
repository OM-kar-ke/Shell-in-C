#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <errno.h>
#include <sys/wait.h>
#include <readline/readline.h>
#include <readline/history.h>

#define MAX_COMM_SIZE 1024    // maximum size of a single command.
#define MAX_WORDS_IN_COMM 100 //max. words in the command.

// Takes command as input (including spaces).
char *input_command(char *cwd)
{
    int index = 0;
    char *command_arr = (char *)malloc(sizeof(char) * MAX_COMM_SIZE);
    char temp[MAX_COMM_SIZE];

    if (command_arr == NULL)
    {
        printf("Memory not allocated for command input.\n");
        exit(0);
    }

    strcpy(temp, cwd);
    strcat(temp, "$");
    command_arr = readline(temp);

    //when pressed CTRL+D.
    if (command_arr == NULL)
    {
        printf("\nGOODBYE\n");
        exit(0);
    }

    if (command_arr && *command_arr)
    {
        add_history(command_arr);
    }
    else
    {
        return NULL;
    }

    return command_arr;
}

char **parse_comm(char *command)
{
    char **args = (char **)malloc(sizeof(char *) * MAX_WORDS_IN_COMM);
    int index = 0;
    char *token = strtok(command, " ");

    if (args == NULL)
    {
        printf("Memory allocation failed.\n");
        exit(0);
    }

    while (token != NULL)
    {

        args[index++] = token;
        token = strtok(NULL, " ");
    }

    args[index] = NULL;

    return args;
}

int exec_comm(char **arg)
{
    int pid = 0, ret;
    pid = fork();

    if (pid == 0)
    {

        ret = execvp(arg[0], arg);
        if (ret == -1)
        {
            perror("execution failed");
            exit(errno);
        }
    }
    else
    {
        wait(0);
    }

    return ret;
}

int execute_with_path(char **args, char *path)
{
    int pid = 0, ret, flag = 0;
    char *token = strtok(path, ":");
    char temp[MAX_COMM_SIZE];

    while (token != NULL)
    {
        strcpy(temp, token);
        strcat(temp, "/");
        strcat(temp, args[0]);

        // If the binary file is present in the specified path and has execute permission?
        if (access(temp, X_OK) != -1)
        {

            flag = 1;

            pid = fork();

            if (pid == 0)
            {
                ret = execv(temp, args);
                if (ret == -1)
                {

                    return -1;
                }
            }
            else
            {
                wait(0);
            }
        }

        token = strtok(NULL, ":");

        if (flag == 0 && token == NULL)
        {
            printf("Executable file path is not correct.\n");
        }
    }

    return ret;
}

int main()
{

    char *command, *path = NULL;
    char cwd[MAX_COMM_SIZE];
    char **args;
    int ret, new_path_set = 0;


    if (getcwd(cwd, sizeof(cwd)) == NULL)
    {
        perror("getcwd() error");
        exit(0);
    }

    while (1)
    {
        // Take command as a input
        command = input_command(cwd);

        if (command == NULL)
        {
            continue;
        }

        if (strcmp(command, "exit") == 0 || *command == EOF)
        {
            printf("GOODBYE\n");
            return 0;
        }

        // Check if command is PS1=...
        char *s = (char *)malloc(sizeof(char) * 3);
        s[0] = command[0];
        s[1] = command[1];
        s[2] = command[2];

        if (strcmp(s, "PS1") == 0)
        {

            // Get the desired string.
            char *token = strtok(command, "=");
            token = strtok(NULL, "=");

            char *result = token + 1;          // removes first character
            result[strlen(result) - 1] = '\0'; // removes last character

            // Check for PS1="\w$".
            if (strcmp(result, "\\w$") == 0)
            {
                if (getcwd(cwd, sizeof(cwd)) == NULL)
                {
                    perror("getcwd() error");
                    exit(0);
                }
                continue;
            }
            strcpy(cwd, result);
            continue;
        }

        // Check for PATH.
        char *s2 = (char *)malloc(sizeof(char) * 4);
        if (s2 == NULL)
        {
            continue;
        }

        s2[0] = command[0];
        s2[1] = command[1];
        s2[2] = command[2];
        s2[3] = command[3];

        if (strcmp(s2, "PATH") == 0)
        {

            char *token = strtok(command, "=");
            token = strtok(NULL, "=");

            path = token;
            new_path_set = 1;
            printf("New Path is : %s\n", path);
            continue;
        }

        // Parse the command
        args = parse_comm(command);

        if (strcmp(args[0], "cd") == 0)
        {

            if (access(args[1], F_OK) != -1)
            {

                strcpy(cwd, args[1]);
                chdir(args[1]);
                if (strcmp(args[1], "..") == 0)
                {

                    if (getcwd(cwd, sizeof(cwd)) == NULL)
                    {

                        perror("getcwd() error");
                        exit(0);
                    }
                }
            }

            else
            {

                perror("cd() error");
            }

            continue;
        }

        if (new_path_set == 0)
        {
            // Execute the command normally.
            ret = exec_comm(args);
            if (ret == -1)
            {
                perror("execution failed");
                exit(errno);
            }
        }

        else
        {
            // Execute the command by using the new path set.
            int r = execute_with_path(args, path);

            if (r == -1)
            {

                printf("Execution Failed!\n");
                exit(0);
            }
        }
    }

    return 0;
}
