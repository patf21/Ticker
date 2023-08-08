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


WATCHER *cli_watcher_start(WATCHER_TYPE *type, char *args[])
{
    WATCHER *watcher = malloc(sizeof(WATCHER));
    if (watcher == NULL)
    {
        perror("malloc failed");
        return NULL;
    }

    watcher->type = CLI_WATCHER_TYPE;
    watcher->pid = -1;
    watcher->read_fd = 0;
    watcher->write_fd = 1;

    watcher->pid = -1;
    return watcher;
}

int cli_watcher_stop(WATCHER *wp)
{
    free(wp);
    abort();
}

int cli_watcher_send(WATCHER *wp, void *arg)
{
    abort();
}
int cli_watcher_recv(WATCHER *wp, char *txt)
{

    if (wp->tracing)
    {
        // get current time
        struct timespec current_time;
        clock_gettime(CLOCK_REALTIME, &current_time);
        char timestamp[30];
        snprintf(timestamp, sizeof(timestamp), "[%ld.%06ld]", current_time.tv_sec, current_time.tv_nsec / 1000);

        // create trace string
        char trace[256];
        int len = snprintf(trace, sizeof(trace), "%s[CLI       ]", timestamp);

        if (write(STDERR_FILENO, trace, len) == -1)
        {
            // handle write error
            perror("Error writing to stderr");
            return -1;
        }

        int num3 = wp->read_fd;
        int num4 = ++(wp->serial);
        char buf[10];

        // write num1 with 5 spaces between brackets
        snprintf(buf, sizeof(buf), "[%2d]", num3);
        write(STDERR_FILENO, buf, strlen(buf));
        char buf3[10];
        snprintf(buf3, sizeof(buf3), "[%5d]: ", num4);
        write(STDERR_FILENO, buf3, strlen(buf3));
        char buf2[1028];
        snprintf(buf2, sizeof(buf2), "%s\n", txt);
        write(STDERR_FILENO, buf2, strlen(buf2));
        // write to stderr
    }
    else
    {
        ++(wp->serial);
    }
    return 0;
}
int cli_watcher_trace(WATCHER *wp, int enable)
{
    wp->tracing = enable;

    return 0;
}