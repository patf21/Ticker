#include <stdlib.h>

#include "ticker.h"
#include "store.h"
#include "cli.h"
#include "debug.h"
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
#include "struct.h"
#include <time.h>
#include <argo.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>

char *remove_gtn(char *str)
{
    char *new_str = (char *)malloc(strlen(str) + 1); // allocate memory for new string
    int j = 0;
    int gt_encountered = 0;
    for (int i = 0; i < strlen(str); i++)
    {
        if (str[i] == '>')
        {
            gt_encountered = 1; // set flag to indicate > encountered
        }
        if (gt_encountered && str[i] == '\n')
        {
            gt_encountered = 0; // reset flag after encountering newline
        }
        if (!gt_encountered)
        {
            new_str[j++] = str[i]; // copy character if flag not set
        }
    }
    new_str[j] = '\0'; // add null terminator to end of new string
    return new_str;
}
ARGO_VALUE *argo_read_value_from_buffer(char *buffer)
{
    FILE *stream = fmemopen(buffer, strlen(buffer), "r");
    ARGO_VALUE *value = argo_read_value(stream);
    fclose(stream);
    return value;
}
void kill_all_children(pid_t parent_pid)
{
    pid_t child_pid;
    int status;

    // Send a SIGTERM signal to all child processes
    while ((child_pid = waitpid(-1, &status, WNOHANG)) > 0)
    {
        if (getppid() == parent_pid)
        {
            kill(child_pid, SIGTERM);
        }
    }

    // Wait for all child processes to terminate
    while ((child_pid = waitpid(-1, &status, WNOHANG)) > 0)
    {
        // Do nothing - just wait for child processes to terminate
    }
}
WATCHER *bitstamp_watcher_start(WATCHER_TYPE *type, char *args[])
{
    WATCHER *watcher = malloc(sizeof(WATCHER));
    if (watcher == NULL)
    {
        perror("malloc failed");
        return NULL;
    }

    watcher->type = 1;

    watcher->tracing = 0;
    watcher->serial = 1;
    int read_pipe[2], write_pipe[2];
    if (pipe(read_pipe) < 0 || pipe(write_pipe) < 0)
    {
        perror("pipe failed");
        free(watcher);
        return NULL;
    }

    watcher->read_fd = read_pipe[0];
    watcher->write_fd = write_pipe[1];

    // Set the read end of the pipe to non-blocking mode
    int flags = fcntl(read_pipe[0], F_GETFL, 0);
    fcntl(read_pipe[0], F_SETFL, flags | O_NONBLOCK | O_ASYNC);

    pid_t child_pid = fork();
    watcher->pid=child_pid;
    if (child_pid < 0)
    {
        perror("fork failed");
        close(watcher->read_fd);
        close(watcher->write_fd);
        free(watcher);
        return NULL;
    }

    if (child_pid == 0)
    {
       
        /* Set up signal handler for SIGIO*/
        close(read_pipe[0]);
        close(write_pipe[1]);
        dup2(read_pipe[1], STDOUT_FILENO);
        dup2(write_pipe[0], STDIN_FILENO);
        if (execvp("uwsc", watcher_types[1].argv) == -1)
        {
            printf("ERROR");
        }

        exit(1);
    }
    // Parent process
    close(read_pipe[1]);
    close(write_pipe[0]);

    int flag = fcntl(watcher->read_fd, F_GETFL, 0);
    fcntl(watcher->read_fd, F_SETFL, flag | O_ASYNC | O_NONBLOCK);
    fcntl(watcher->read_fd, F_SETOWN, getpid());
    // Set up signal handler for child process input
    int flag2 = fcntl(watcher->write_fd, F_GETFL, 0);
    fcntl(watcher->write_fd, F_SETFL, flag2 | O_ASYNC | O_NONBLOCK);
    fcntl(watcher->write_fd, F_SETOWN, getpid());

    struct sigaction sa;
    sa.sa_sigaction = (void (*)(int, siginfo_t *, void *))sigio_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART | SA_SIGINFO;
    if (sigaction(SIGIO, &sa, NULL) < 0)
    {
        perror("sigaction() error");
        exit(1);
    }

    return watcher;
}

int bitstamp_watcher_stop(WATCHER *wp)
{
    kill_all_children(wp->pid);
    if (wp->name)
    {
        wp->name = NULL;
    }
    if (wp->shortname)
    {
        free(wp->shortname);
        wp->shortname = NULL;
    }

    kill(wp->pid, SIGKILL);
    free(wp);
    return 0;
}

int bitstamp_watcher_send(WATCHER *wp, void *arg)
{
    // TO BE IMPLEMENTED

    char message[256];
    snprintf(message, sizeof(message), "{ \"event\": \"bts:subscribe\", \"data\": { \"channel\": \"%s\" } }\n", wp->shortname);
    write(wp->write_fd, message, strlen(message));
    return 0;
}

int bitstamp_watcher_recv(WATCHER *wp, char *buffer)
{
    ++(wp->serial);
    if (buffer[2] != 'S')
    {
        return 0;
    }
    if (buffer[4] != 'r')
    {
        return 0;
    }

    // determine length of input string
    int len = strlen(buffer);

    // allocate buffer for truncated string
    char newBuf2[len - 2 + 1]; // add 1 for null terminator

    // copy characters from input string, starting at index 2
    for (int i = 2; i < len; i++)
    {
        newBuf2[i - 2] = buffer[i];
    }

    // add null terminator to truncated string
    newBuf2[len - 4] = '\0';
    // regular expression pattern to match sequences of ">\n"
    char *newBuf;
    newBuf = remove_gtn(newBuf2);

    char *json_str = newBuf + strlen("Server message: '");

    ARGO_VALUE *value = argo_read_value_from_buffer(json_str);
    if (value == NULL)
    {
        fprintf(stderr, "failed to parse JSON: %s\n", json_str);
        return -1;
    }

    // Extract the "event" field
    ARGO_VALUE *event = argo_value_get_member(value, "event");
    if (event == NULL || strcmp(argo_value_get_chars(event), "trade") != 0)
    {
        argo_free_value(value);
        return 0;
    }

    // Extract the "data" object
    ARGO_VALUE *data = argo_value_get_member(value, "data");
    if (data == NULL)
    {
        fprintf(stderr, "failed to get data object\n");
        argo_free_value(value);
        return -1;
    }

    // Extract the currency pair
    char *channel = argo_value_get_chars(argo_value_get_member(value, "channel"));
    char *currency_pair = channel + strlen("live_trades_");

    // Extract the price and amount fields
    double amount;
    if (argo_value_get_double(argo_value_get_member(data, "amount"), &amount) != 0)
    {
        fprintf(stderr, "failed to get amount field\n");
        argo_free_value(value);
        return -1;
    }

    long price;
    if (argo_value_get_long(argo_value_get_member(data, "price"), &price) != 0)
    {
        fprintf(stderr, "failed to get price field\n");
        argo_free_value(value);
        return -1;
    }

    // Update the data store
    char price_key[128];
    snprintf(price_key, sizeof(price_key), "bitstamp.net:%s:price", currency_pair);
    struct store_value *price_value = store_get(price_key);
    if (price_value == NULL)
    {
        price_value = malloc(sizeof(struct store_value));
        price_value->type = STORE_LONG_TYPE;
        price_value->content.long_value = price;
    }
    else
    {
        price_value->content.long_value = price;
    }
    store_put(price_key, price_value);
    store_free_value(price_value);

    char volume_key[128];
    snprintf(volume_key, sizeof(volume_key), "bitstamp.net:%s:volume", currency_pair);
    struct store_value *volume_value = store_get(volume_key);
    double volume = volume_value != NULL ? volume_value->content.double_value : 0.0;
    if (volume_value == NULL)
    {
        volume_value = malloc(sizeof(struct store_value));
        volume_value->type = STORE_DOUBLE_TYPE;
    }
    volume_value->content.double_value = volume + amount;
    store_put(volume_key, volume_value);
    store_free_value(volume_value);

    argo_free_value(value);
    if (wp->tracing)
    {
        // get current time
        struct timespec current_time;
        clock_gettime(CLOCK_REALTIME, &current_time);
        char timestamp[30];
        snprintf(timestamp, sizeof(timestamp), "[%ld.%06ld]", current_time.tv_sec, current_time.tv_nsec / 1000);

        // create trace string
        char trace[256];
        int len = snprintf(trace, sizeof(trace), "%s[bitstamp.net]", timestamp);

        if (write(STDERR_FILENO, trace, len) == -1)
        {
            // handle write error
            perror("Error writing to stderr");
            return -1;
        }

        int num3 = wp->read_fd;
        int num4 = (wp->serial);
        char buf[10];

        // write num1 with 5 spaces between brackets
        snprintf(buf, sizeof(buf), "[%2d]", num3);
        write(STDERR_FILENO, buf, strlen(buf));
        char buf3[10];
        snprintf(buf3, sizeof(buf3), "[%5d]: ", num4);
        write(STDERR_FILENO, buf3, strlen(buf3));
        char buf2[1050];
        snprintf(buf2, sizeof(buf2), "%s\n", newBuf);
        write(STDERR_FILENO, buf2, strlen(buf2));
        // write to stderr
        free(newBuf);
    }
    else
    {

        free(newBuf);
    }
    argo_free_value(value);

    free(channel);
    free(event);
    return 0;
}

int bitstamp_watcher_trace(WATCHER *wp, int enable)
{
    wp->tracing = enable;
    // TO BE IMPLEMENTED
    return 0;
}
