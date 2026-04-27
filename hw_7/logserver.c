#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <signal.h>
#include <errno.h>

#define FIFO_PATH       "/tmp/logserver.fifo"
#define LOG_PATH        "/tmp/logserver.log"
#define ALARM_INTERVAL  5
#define BUF_SIZE        4096

volatile sig_atomic_t flag_sigterm = 0;
volatile sig_atomic_t flag_sigint  = 0;
volatile sig_atomic_t flag_sigalrm = 0;
volatile sig_atomic_t flag_sigusr1 = 0;
volatile sig_atomic_t flag_sighup  = 0;
volatile sig_atomic_t is_daemon    = 0;

typedef struct {
    long messages;
    long bytes;
    long alarms;
} Stats;

static const char *g_fifo_path;
static const char *g_log_path;
static int         g_alarm_interval;

static void sig_handler(int sig) {
    if      (sig == SIGTERM) flag_sigterm = 1;
    else if (sig == SIGINT)  flag_sigint  = 1;
    else if (sig == SIGALRM) flag_sigalrm = 1;
    else if (sig == SIGUSR1) flag_sigusr1 = 1;
    else if (sig == SIGHUP)  flag_sighup  = 1;
}

static void setup_signals(void) {
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = sig_handler;
    sigemptyset(&sa.sa_mask);

    if (sigaction(SIGTERM, &sa, NULL) < 0) { perror("sigaction SIGTERM"); exit(1); }
    if (sigaction(SIGINT,  &sa, NULL) < 0) { perror("sigaction SIGINT");  exit(1); }
    if (sigaction(SIGALRM, &sa, NULL) < 0) { perror("sigaction SIGALRM"); exit(1); }
    if (sigaction(SIGUSR1, &sa, NULL) < 0) { perror("sigaction SIGUSR1"); exit(1); }
    if (sigaction(SIGHUP,  &sa, NULL) < 0) { perror("sigaction SIGHUP");  exit(1); }

    struct sigaction sa_ign;
    memset(&sa_ign, 0, sizeof(sa_ign));
    sa_ign.sa_handler = SIG_IGN;
    sigemptyset(&sa_ign.sa_mask);
    if (sigaction(SIGQUIT, &sa_ign, NULL) < 0) { perror("sigaction SIGQUIT"); exit(1); }
}

static void print_stats(Stats *s) {
    printf("Stats: messages=%ld bytes=%ld alarms=%ld\n",
           s->messages, s->bytes, s->alarms);
    fflush(stdout);
}

static void do_daemonize(Stats *s, int by_signal) {
    int log_fd = open(g_log_path, O_WRONLY | O_CREAT | O_APPEND, 0644);
    if (log_fd < 0) { perror("open log"); exit(1); }

    pid_t pid = fork();
    if (pid < 0) { perror("fork"); exit(1); }
    if (pid > 0) _exit(0);

    if (setsid() < 0) { perror("setsid"); _exit(1); }

    pid = fork();
    if (pid < 0) { perror("fork2"); _exit(1); }
    if (pid > 0) _exit(0);

    if (chdir("/") < 0) { perror("chdir"); _exit(1); }

    int devnull = open("/dev/null", O_RDONLY);
    if (devnull < 0) { perror("open /dev/null"); _exit(1); }
    if (dup2(devnull, STDIN_FILENO)  < 0) { perror("dup2 stdin");  _exit(1); }
    if (dup2(log_fd, STDOUT_FILENO)  < 0) { perror("dup2 stdout"); _exit(1); }
    if (dup2(log_fd, STDERR_FILENO)  < 0) { perror("dup2 stderr"); _exit(1); }
    close(devnull);
    close(log_fd);

    is_daemon = 1;

    if (by_signal) {
        printf("Daemonized by SIGHUP\n");
        print_stats(s);
    }
}

static int setup_fifo(const char *path) {
    if (mkfifo(path, 0600) == 0) return 0;
    if (errno != EEXIST) { perror("mkfifo"); return -1; }

    struct stat st;
    if (stat(path, &st) < 0) { perror("stat fifo"); return -1; }
    if (!S_ISFIFO(st.st_mode)) {
        fprintf(stderr, "%s exists but is not a FIFO\n", path);
        return -1;
    }
    return 0;
}

static void handle_pending(Stats *s) {
    if (flag_sigalrm) {
        printf("wait for data\n");
        fflush(stdout);
        s->alarms++;
        flag_sigalrm = 0;
        alarm(g_alarm_interval);
    }
    if (flag_sigusr1) {
        print_stats(s);
        flag_sigusr1 = 0;
    }
    if (flag_sighup && !is_daemon) {
        do_daemonize(s, 1);
        flag_sighup = 0;
    }
}

static void run_loop(Stats *s) {
    while (!flag_sigterm && !flag_sigint) {
        handle_pending(s);

        int fd;
        while (1) {
            fd = open(g_fifo_path, O_RDONLY);
            if (fd >= 0) break;
            if (errno != EINTR) { perror("open fifo"); exit(1); }
            handle_pending(s);
            if (flag_sigterm || flag_sigint) break;
        }

        if (flag_sigterm || flag_sigint) break;

        char buf[BUF_SIZE];
        int last_newline = 1;
        int stop = 0;

        while (1) {
            ssize_t n = read(fd, buf, BUF_SIZE - 1);
            if (n > 0) {
                buf[n] = '\0';
                fputs(buf, stdout);
                fflush(stdout);
                s->bytes += n;
                last_newline = (buf[n - 1] == '\n');
            } else if (n == 0) {
                break;
            } else {
                if (errno != EINTR) { perror("read fifo"); exit(1); }
                handle_pending(s);
                if (flag_sigterm) { stop = 1; break; }
            }
        }

        if (!last_newline && !stop) {
            putchar('\n');
            fflush(stdout);
        }

        close(fd);
        s->messages++;

        if (stop) break;
    }
}

int main(int argc, char *argv[]) {
    g_fifo_path      = FIFO_PATH;
    g_log_path       = LOG_PATH;
    g_alarm_interval = ALARM_INTERVAL;
    int start_daemon = 0;

    int opt;
    while ((opt = getopt(argc, argv, "df:l:n:")) != -1) {
        switch (opt) {
        case 'd': start_daemon = 1; break;
        case 'f': g_fifo_path = optarg; break;
        case 'l': g_log_path  = optarg; break;
        case 'n': g_alarm_interval = atoi(optarg); break;
        default:
            fprintf(stderr, "Usage: %s [-d] [-f fifo] [-l log] [-n interval]\n", argv[0]);
            exit(1);
        }
    }

    if (setup_fifo(g_fifo_path) < 0) exit(1);

    setup_signals();

    Stats stats = {0};

    if (start_daemon) do_daemonize(&stats, 0);

    alarm(g_alarm_interval);

    run_loop(&stats);

    if (flag_sigterm) printf("Terminated by SIGTERM\n");
    if (flag_sigint)  printf("Terminated by SIGINT\n");

    print_stats(&stats);
    fflush(stdout);

    unlink(g_fifo_path);
    return 0;
}
