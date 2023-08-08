#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <sys/wait.h>
#include "ticker.h"
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <ctype.h>
#include "cli.h"
#include "bitstamp.h"
#include <time.h>
#include <sys/ioctl.h>
#include <pthread.h>
#include "struct.h"
#define MAX_ID_LEN 10
#define MAX_FD_LEN 10
#include <pthread.h>
#include "store.h"
static pthread_mutex_t input_mutex = PTHREAD_MUTEX_INITIALIZER;

WATCHER *watcher_list[3000];
char *watcher_names[3000];
volatile sig_atomic_t termination_flag = 0;
volatile sig_atomic_t stdin_flag = 0;
volatile sig_atomic_t work = 0;
void print_argv(char **argv)
{
    int i = 0;
    char **argv_copy = argv; // create a copy of argv

    write(STDOUT_FILENO, " ", 1);
    while (argv_copy[i])
    {
        write(STDOUT_FILENO, argv_copy[i], strlen(argv_copy[i]));
        write(STDOUT_FILENO, " ", 1);
        i++;
    }
}

long async_strtol(const char *str, char **endptr, int base)
{
    long result = 0;
    char *tmp_endptr = NULL;

    // Call strtol with a non-NULL endptr to get the pointer to the
    // first invalid character in the input string.
    tmp_endptr = (endptr != NULL) ? *endptr : NULL;
    result = strtol(str, &tmp_endptr, base);

    // Check if a valid conversion was performed
    if (tmp_endptr == str)
    {
        return -1;
    }

    // Update the endptr if it is provided.
    if (endptr != NULL)
    {
        *endptr = tmp_endptr;
    }

    return result;
}
void my_process_input_line(char *line)
{

    // Remove leading and trailing whitespace
    char *start = line;
    char *end = line + strlen(line) - 1;
    while (isspace(*start))
    {
        start++;
    }
    while (isspace(*end))
    {
        end--;
    }
    *(end + 1) = '\0';

    // Check if the line is empty
    if (strlen(start) == 0)
    {
        return;
    }

    // Split the line into tokens using whitespace as a delimiter
    char *token = strtok(start, " \t");
    int count = 0;
    while (token != NULL)
    {

        // Process each token

        // Check if the token matches "quit" (ignoring case)

        if (!strncmp(token, "quit", 5))
        {
            // If this is the first token, set the termination flag and print the token
            if (count == 0)
            {
                token = strtok(NULL, " \t");
                if (!token)
                {
                    char *st = "ticker> ";
                    write(STDOUT_FILENO, st, strlen(st));
                    fflush(stdout);
                    termination_flag = 1;
                    kill(getpid(), SIGINT);
                }
                else
                {
                    printf("???\n");
                    return;
                }
            }
        }
        // If the token doesn't match "quit", print "???"
        else
        {
            pthread_mutex_t watcher_list_mutex = PTHREAD_MUTEX_INITIALIZER;

            if (!strncmp(token, "show", 4))
            {
                token = strtok(NULL, " \t");
                if (!strncmp(token, "bitstamp.net:live_trades_", 25))
                {
                    // Extract the two variables from the string
                    char *var1 = strtok(token + 26, ":");

                    char *var2 = strtok(NULL, ":");

                    // Check if both variables were successfully extracted
                    if (var1 != NULL && var2 != NULL)
                    {
                        char price_key[128];
                        snprintf(price_key, sizeof(price_key), "bitstamp.net:live_trades_%s:%s", var1, var2);
                        struct store_value *price_value = store_get(price_key);

                        if (price_value == NULL || price_value->type != STORE_LONG_TYPE)
                        {
                            printf("???\n");
                            return;
                        }
                        else
                        {
                        
                        }
                        store_free_value(price_value);
                        return;
                    }
                }

                // If the string does not match the expected format or the variables were not successfully extracted
                printf("???\n");
                return;
            }

            // If the current token matches the price field, extract the price

            if (!strncmp(token, "start", 6))
            {
                if (count == 0)
                {
                    token = strtok(NULL, " \t");
                    if (!token)
                    {
                        printf("???\n");
                        return;
                    }
                    if (!strncmp(token, "bitstamp.net", 13))
                    {
                        char following_words[1028] = "";
                        char first_word_after_bitstamp[256] = "";
                        token = strtok(NULL, " \t");
                        if (!token)
                        {
                            printf("???\n");
                            return;
                        }
                        strncpy(first_word_after_bitstamp, token, sizeof(first_word_after_bitstamp) - 1);
                        strncpy(following_words, token, sizeof(following_words) - 1);
                        token = strtok(NULL, " \t");
                        while (token)
                        {
                            strncat(following_words, " ", sizeof(following_words) - strlen(following_words) - 1);
                            strncat(following_words, token, sizeof(following_words) - strlen(following_words) - 1);
                            token = strtok(NULL, " \t");
                        }

                        // Lock the mutex before accessing the watcher_list array
                        pthread_mutex_lock(&watcher_list_mutex);

                        for (int i = 0; i < 3000; i++)
                        {
                            if (!(watcher_list[i]))
                            {
                                WATCHER *new_watcher = bitstamp_watcher_start(&watcher_types[1], watcher_types[1].argv);

                                new_watcher->name = malloc(strlen(following_words) + 1); // Allocate memory for the name string
                                strcpy(new_watcher->name, following_words);
                                new_watcher->shortname = malloc(strlen(first_word_after_bitstamp) + 1); // Allocate memory for the name string
                                strcpy(new_watcher->shortname, first_word_after_bitstamp);              // Copy the name string into the newly allocated memory
                                watcher_list[i] = new_watcher;

                                bitstamp_watcher_send(watcher_list[1], NULL);
                                break;
                            }
                        }

                        // Unlock the mutex after accessing the watcher_list array
                        pthread_mutex_unlock(&watcher_list_mutex);
                        return;
                    }
                    else
                    {
                        printf("???\n");
                        return;
                    }
                }
                else
                {
                    printf("???\n");
                    return;
                }
            }
            if (!strncmp(token, "watchers", 9))
            {

                token = strtok(NULL, " \t");
                if (!token)
                {

                    // CODE HERE
                    for (int i = 0; i < 3000; i++)
                    {
                        if (watcher_list[i])
                        {
                            if (((watcher_list[i]->type) == CLI_WATCHER_TYPE))
                            {

                                char id_str[MAX_ID_LEN + 1];
                                snprintf(id_str, MAX_ID_LEN + 1, "%d", i);
                                write(STDOUT_FILENO, id_str, strlen(id_str));

                                write(STDOUT_FILENO, "\t", 1);
                                write(STDOUT_FILENO, watcher_types[watcher_list[i]->type].name, strlen(watcher_types[watcher_list[i]->type].name));
                                write(STDOUT_FILENO, "(", 1);

                                char pid_str[MAX_ID_LEN + 1];
                                snprintf(pid_str, MAX_ID_LEN + 1, "%d", watcher_list[i]->pid);
                                write(STDOUT_FILENO, pid_str, strlen(pid_str));
                                write(STDOUT_FILENO, ",", 1);

                                char read_fd_str[MAX_FD_LEN + 1];
                                snprintf(read_fd_str, MAX_FD_LEN + 1, "%d", watcher_list[i]->read_fd);
                                write(STDOUT_FILENO, read_fd_str, strlen(read_fd_str));
                                write(STDOUT_FILENO, ",", 1);

                                char write_fd_str[MAX_FD_LEN + 1];
                                snprintf(write_fd_str, MAX_FD_LEN + 1, "%d", 1);
                                write(STDOUT_FILENO, write_fd_str, strlen(write_fd_str));
                                write(STDOUT_FILENO, ")\n", 2);
                            }
                            else
                            {

                                char id_str[MAX_ID_LEN + 1];
                                snprintf(id_str, MAX_ID_LEN + 1, "%d", i);
                                write(STDOUT_FILENO, id_str, strlen(id_str));

                                write(STDOUT_FILENO, "\t", 1);
                                write(STDOUT_FILENO, watcher_types[watcher_list[i]->type].name, strlen(watcher_types[watcher_list[i]->type].name));
                                write(STDOUT_FILENO, "(", 1);

                                char pid_str[MAX_ID_LEN + 1];
                                snprintf(pid_str, MAX_ID_LEN + 1, "%d", watcher_list[i]->pid);
                                write(STDOUT_FILENO, pid_str, strlen(pid_str));
                                write(STDOUT_FILENO, ",", 1);

                                char read_fd_str[MAX_FD_LEN + 1];
                                snprintf(read_fd_str, MAX_FD_LEN + 1, "%d", watcher_list[i]->read_fd);
                                write(STDOUT_FILENO, read_fd_str, strlen(read_fd_str));
                                write(STDOUT_FILENO, ",", 1);

                                char write_fd_str[MAX_FD_LEN + 1];
                                snprintf(write_fd_str, MAX_FD_LEN + 1, "%d", watcher_list[i]->write_fd);
                                write(STDOUT_FILENO, write_fd_str, strlen(write_fd_str));
                                write(STDOUT_FILENO, ") [", 3);

                                write(STDOUT_FILENO, watcher_list[i]->name, strlen(watcher_list[i]->name));
                                write(STDOUT_FILENO, "]\n", 2);
                            }
                        }
                    }
                    return;
                }
                else
                {
                    printf("???\n");
                    return;
                }
            }
            if (!strncmp(token, "trace", 6))
            {
                token = strtok(NULL, " \t");
                if (!token)
                {
                    printf("???\n");
                    return;
                }
                char *endptr = NULL;
                int base = 10;
                int result = (int)async_strtol(token, &endptr, base);
                if (result < 0)
                {
                    printf("???\n");
                    return;
                }
                token = strtok(NULL, " \t");
                if (token)
                {
                    printf("???\n");
                    return;
                }
                if ((result <= 3000) && watcher_list[result])
                {

                    if (result == 0)
                    {
                        cli_watcher_trace(watcher_list[result], 1);
                        return;
                    }
                    bitstamp_watcher_trace(watcher_list[result], 1);
                    return;
                }
                else
                {
                    printf("???\n");
                    return;
                }
            }
            if (!strncmp(token, "untrace", 8))
            {

                token = strtok(NULL, " \t");
                if (!token)
                {
                    printf("???\n");
                    return;
                }
                char *endptr = NULL;
                int base = 10;
                int result = (int)async_strtol(token, &endptr, base);
                if (result < 0)
                {
                    printf("???\n");
                    return;
                }
                token = strtok(NULL, " \t");
                if (token)
                {
                    printf("???\n");
                    return;
                }
                if ((result <= 3000) && watcher_list[result])
                {
                    if (result == 0)
                    {
                        cli_watcher_trace(watcher_list[result], 0);
                        return;
                    }
                    bitstamp_watcher_trace(watcher_list[result], 0);
                    return;
                }
                else
                {
                    printf("???\n");
                    return;
                }
            }
            if (!strncmp(token, "stop", 5))
            {
                token = strtok(NULL, " \t");
                if (!token)
                {
                    printf("???\n");
                    return;
                }
                char *endptr = NULL;
                int base = 10;
                int result = (int)async_strtol(token, &endptr, base);
                if (result <= 0)
                {
                    printf("???\n");
                    return;
                }
                token = strtok(NULL, " \t");
                if (token)
                {
                    printf("???\n");
                    return;
                }
                if ((result <= 3000) && watcher_list[result])
                {
                    int i = result;

                    // Send a SIGTERM signal to the child process
                    bitstamp_watcher_stop(watcher_list[i]);

                    // Update the watcher table to indicate that the ID is available
                    watcher_list[i] = NULL;
                    return;
                }
                else
                {
                    printf("???\n");
                    return;
                }
            }
            else
            {
                printf("???\n");
                return;
            }
            // Move to the next token and increment the count
            token = strtok(NULL, " \t");
            count++;
        }
    }
}

void sigint_handler(int signum)
{

    termination_flag = 1;
    for (int i = 1; i < 3000; i++)
    {

        if (watcher_list[i])
        {
            free(watcher_list[i]->name);
        }
    }
    _exit(0);
}
void sigchld_handler(int sig)
{
    int olderrno = errno;
    pid_t pid;
    int status;
    while ((pid = waitpid(-1, &status, WNOHANG)) > 0)
    {
       
    }
    if (pid == -1 && errno != ECHILD)
        perror("wait error");
    errno = olderrno;
}

// Set non-blocking I/O on standard input
//  int flags = fcntl(STDIN_FILENO, F_GETFL, 0);

void sigio_handler(int signo, siginfo_t *info, void *context, char *buffer)
{

    for (int i = 1; i < 3000; i++)
    {
        if (watcher_list[i])
        {
            char buf[1024];
            int rd = 0;
            if ((rd = read(watcher_list[i]->read_fd, buf, 1024)) > 0)
            {

                bitstamp_watcher_recv(watcher_list[i], buf);
                return;
            }
        }
    }

    if (stdin_flag)
    {
        if (buffer[0] == '\0')
            return;
        pthread_mutex_lock(&input_mutex);

        cli_watcher_recv(watcher_list[0], buffer);
        stdin_flag = 0;

        if (buffer == NULL)
        {
            buffer = (char *)info->si_value.sival_ptr;
        }

        int nCount = 0;
        for (int i = 0; i < sizeof(buffer); i++)
        {
            if (buffer[i] == '\n')
            {
                nCount++;
            }
        }

        // Tokenize the input buffer by splitting it by newline characters

#define MAX_TOKEN_SIZE 1028
        // Use strtok_r to parse the input string
        char *line_save, *token_save; // Declare save pointers for strtok_r
        char *line = strtok_r(buffer, "\n", &line_save);
        while (line != NULL)
        {

            // Tokenize the line by whitespace characters and newline characters
            char *token = strtok_r(line, " \t\n", &token_save);
            while (token != NULL)
            {

                char token_copy[MAX_TOKEN_SIZE];
                strncpy(token_copy, token, MAX_TOKEN_SIZE); // Copy the token into the buffer
                my_process_input_line(token_copy);          // Process the copy of the token
                token = strtok_r(NULL, " \t\n", &token_save);
            }

            line = strtok_r(NULL, "\n", &line_save); // Get the next line

            char *st = "ticker> ";
            write(STDOUT_FILENO, st, strlen(st));
            fflush(stdout);

            pthread_mutex_unlock(&input_mutex);
        }

        return;
    }

    char *input_buffer;
    static size_t input_buffer_size = 0;
    static ssize_t input_buffer_len = 0;

    if (info->si_fd != STDIN_FILENO)
    {
  
        return;
    }

    // Allocate a temporary buffer to read the available input
    char tmp_buffer[256];
    ssize_t tmp_len;
    int readit = 0;
    while ((tmp_len = read(info->si_fd, tmp_buffer, sizeof(tmp_buffer))) > 0)
    {
        readit = 1;
        // Calculate the new size of the input buffer
        size_t new_size = input_buffer_len + tmp_len;
        if (new_size >= input_buffer_size)
        {
            // Allocate a new buffer that is large enough to hold the current buffer plus the data in the temporary buffer
            size_t new_buffer_size = new_size * 2;
            char *new_buffer = realloc(input_buffer, new_buffer_size);
            if (new_buffer == NULL)
            {

                // Error handling: failed to allocate memory
                free(input_buffer);
                input_buffer = NULL;
                input_buffer_size = 0;
                input_buffer_len = 0;
                printf("ticker> ");
                return;
            }
            input_buffer = new_buffer;
            input_buffer_size = new_buffer_size;
        }

        // Copy the current buffer to the new buffer using memcpy()
        if (input_buffer_len > 0)
        {
            memcpy(input_buffer + input_buffer_len, tmp_buffer, tmp_len);
        }
        else
        {
            memcpy(input_buffer, tmp_buffer, tmp_len);
        }
        input_buffer_len += tmp_len;
        // Ensure the input buffer is null-terminated
        if (input_buffer_len < input_buffer_size)
        {
            input_buffer[input_buffer_len] = '\0';
        }
        else
        {
            // Resize the input buffer to accommodate the null character
            char *new_buffer = realloc(input_buffer, input_buffer_size + 1);
            if (new_buffer == NULL)
            {
                // Error handling: failed to allocate memory
                free(input_buffer);
                input_buffer = NULL;
                input_buffer_size = 0;
                input_buffer_len = 0;

                return;
            }
            input_buffer = new_buffer;
            input_buffer_size += 1;
            input_buffer[input_buffer_len] = '\0';
        }

        // Check for "Server message:" in the input buffer and return if found

        // Check if a complete line is available in the input buffer
        char *end_of_line;

        cli_watcher_recv(watcher_list[0], input_buffer);

        while ((end_of_line = memchr(input_buffer, '\n', input_buffer_len)) != NULL)
        {
            *end_of_line = '\0'; // Terminate the string at the end of the line
            // Replace process_input_line() with your own function

            my_process_input_line(input_buffer); // Process the input line

            size_t remaining_len = input_buffer_len - (end_of_line - input_buffer) - 1;
            memmove(input_buffer, end_of_line + 1, remaining_len); // Move the remaining data to the beginning of the buffer
            input_buffer_len = remaining_len;
            // Ensure the input buffer is null-terminated
            if (input_buffer_len < input_buffer_size)
            {
                input_buffer[input_buffer_len] = '\0';
            }
            else
            {
                // Resize the input buffer to accommodate the null character
                char *new_buffer = realloc(input_buffer, input_buffer_size + 1);
                if (new_buffer == NULL)
                {
                    // Error handling: failed to allocate memory
                    free(input_buffer);
                    input_buffer = NULL;
                    input_buffer_size = 0;
                    input_buffer_len = 0;
                    printf("ticker> ");
                    return;
                }
                input_buffer = new_buffer;
                input_buffer_size += 1;
                input_buffer[input_buffer_len] = '\0';
            }
        }
    }
    if (!readit)
    {
        return;
    }
    if (tmp_len == -1 && errno == EWOULDBLOCK)
    {
        free(input_buffer); // Free the memory allocated for input_buffer
        input_buffer = NULL;
        input_buffer_size = 0;
        input_buffer_len = 0;
        printf("ticker> ");
        return;
    }
    else
    {
        // There was an error or the other end of the pipe was closed
        free(input_buffer); // Free the memory allocated for input_buffer
        input_buffer = NULL;
        input_buffer_size = 0;
        input_buffer_len = 0;
        printf("ticker> ");
        return;
    }
}

int ticker(void)
{
    for (int i = 0; i > 3000; i++)
    {
        watcher_list[i] = NULL;
    }
    // struct timespec delay = {0, 100000000}; // 0.1 seconds
    // nanosleep(&delay, NULL);

    sigset_t mask;

    watcher_list[0] = cli_watcher_start(&watcher_types[CLI_WATCHER_TYPE], NULL);
    watcher_list[0]->name = "";
    // Install signal handler for SIGIO
    struct sigaction saIO;
    sigemptyset(&saIO.sa_mask);
    saIO.sa_sigaction = (void (*)(int, siginfo_t *, void *))sigio_handler;
    saIO.sa_flags = SA_SIGINFO;
    if (sigaction(SIGIO, &saIO, NULL) == -1)
    {

        exit(1);
    }
    int fd = fileno(stdin);        // get file descriptor for standard input
    fcntl(fd, F_SETOWN, getpid()); // set owner to current process
    fcntl(fd, F_GETFL);            // get current file status flags
    fcntl(fd, F_SETFL, fcntl(fd, F_GETFL) | O_ASYNC | O_NONBLOCK);
    fcntl(fd, F_SETSIG, SIGIO);

    // Set up signal handlers
    struct sigaction sigint_act, sigchld_act;
    sigemptyset(&sigint_act.sa_mask);
    sigint_act.sa_flags = 0;
    sigint_act.sa_handler = sigint_handler;
    if (sigaction(SIGINT, &sigint_act, NULL) == -1)
    {

        exit(1);
    }

    sigemptyset(&sigchld_act.sa_mask);
    sigchld_act.sa_flags = SA_RESTART | SA_NOCLDSTOP | SA_SIGINFO;
    sigchld_act.sa_handler = sigchld_handler;
    if (sigaction(SIGCHLD, &sigchld_act, NULL) == -1)
    {

        exit(1);
    }
    // If there's data available, read it and invoke sigio_handler() with the buffer

    // Main loop

    // If there's data available, read it and send the SIGIO signal
    // Check if there's data available in  the stdin buffer
    char buffer[2056];
    int bytes_available;
    ioctl(STDIN_FILENO, FIONREAD, &bytes_available);

    if (bytes_available > 0)
    {

        char *st = "ticker> ";
        write(STDOUT_FILENO, st, strlen(st));
        fflush(stdout);
        char buffer[2056];

        ssize_t n = read(STDIN_FILENO, buffer, sizeof(buffer));
        if (n > 0)
        {
            buffer[n] = '\0';

            stdin_flag = 1;
            // Call sigio_handler() with the buffer
            sigio_handler(SIGIO, NULL, NULL, buffer);
        }
    }
    else
    {
        fopen("stdin", "r");
        ssize_t n = read(STDIN_FILENO, buffer, sizeof(buffer));
        if (n == 0)
            return 0;
        if (n < 0)
        {
            // remember to return 0
        }
        else
        {

            char *st = "ticker> ";
            write(STDOUT_FILENO, st, strlen(st));
            fflush(stdout);

            char buffer[2056];

            if (n > 0)
            {
                buffer[n] = '\0';

                stdin_flag = 1;
                // Call sigio_handler() with the buffer
                sigio_handler(SIGIO, NULL, NULL, buffer);
            }
        }
    }

    if (!stdin_flag)
        printf("ticker> ");

    while (!termination_flag)
    {
        // Do work here

        sigfillset(&mask);
        sigdelset(&mask, SIGINT);
        sigdelset(&mask, SIGIO);
        // Suspend program until a signal is received
        // Do work here

        fflush(stdout);

        sigsuspend(&mask);

        sigdelset(&mask, SIGCHLD);
    }
    // Cleanup program here
    return 0;
}