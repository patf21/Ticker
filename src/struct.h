typedef struct watcher
{
    int type;
    int pid;
    int read_fd;
    int write_fd;
    int tracing;
    int serial;
    char *name;
    char *shortname;
} WATCHER;
int bitstamp_watcher_recv(WATCHER *wp, char *txt);
void sigio_handler(int signo, siginfo_t *info, void *context, char *buffer);