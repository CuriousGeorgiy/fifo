#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/stat.h>

static const char *const comm_fifo_name = "comm_fifo";

static char cwd[PATH_MAX];

static char conn_fifo_path[PATH_MAX];
static char msg_fifo_path[PATH_MAX];
static char fifos_path[PATH_MAX];
static_assert(PATH_MAX <= PIPE_BUF,
              "path to message fifo must be written atomically");

static char buf[PIPE_BUF];
static char dummy_buf[PIPE_BUF * 2];

int main(int argc, const char *const argv[]) {
    --argc;
    ++argv;

    if (argc != 1) {
        printf("USAGE: fifo-sender <path_to_file>\n");
        return EXIT_FAILURE;
    }

    const char *file_name = argv[0];
    int file_fd = open(file_name, O_RDONLY);
    if (file_fd == -1) {
        perror("SYSTEM ERROR: open failed");
        return EXIT_FAILURE;
    }

    char *res = getcwd(cwd, PATH_MAX);
    if (res == NULL) {
        perror("SYSTEM ERROR: getcwd failed");
        return EXIT_FAILURE;
    }

    pid_t pid = getpid();

    int rc = sprintf(msg_fifo_path, "%s/%d_msg_fifo", cwd, pid);
    if (rc < 0) {
        perror("SYSTEM ERROR: sprintf failed");
        return EXIT_FAILURE;
    }
    rc = sprintf(conn_fifo_path, "%s/%d_conn_fifo", cwd, pid);
    if (rc < 0) {
        perror("SYSTEM ERROR: sprintf failed");
        return EXIT_FAILURE;
    }
    rc = sprintf(fifos_path, "%s/%d_conn_fifo %s/%d_msg_fifo", cwd, pid, cwd, pid);
    if (rc < 0) {
        perror("SYSTEM ERROR: sprintf failed");
        return EXIT_FAILURE;
    }

    rc = mkfifo(msg_fifo_path, S_IRUSR | S_IWUSR);
    if (rc != 0) {
        if (errno != EEXIST) {
            perror("SYSTEM ERROR: mkfifo failed");
            return EXIT_FAILURE;
        }
    }
    rc = mkfifo(conn_fifo_path, S_IRUSR | S_IWUSR);
    if (rc != 0) {
        if (errno != EEXIST) {
            perror("SYSTEM ERROR: mkfifo failed");
            return EXIT_FAILURE;
        }
    }
    int conn_fifo_fd = open(conn_fifo_path, O_RDONLY | O_NONBLOCK);
    if (conn_fifo_fd == -1) {
        perror("SYSTEM ERROR: open failed");
        return EXIT_FAILURE;
    }
    rc = fcntl(conn_fifo_fd, F_SETFL, 0);
    if (rc == -1) {
        perror("SYSTEM ERROR: fcntl failed");
        return EXIT_FAILURE;
    }

    int comm_fifo_fd = open(comm_fifo_name, O_WRONLY);
    if (comm_fifo_fd == -1) {
        perror("SYSTEM ERROR: open failed");
        return EXIT_FAILURE;
    }

    rc = fcntl(comm_fifo_fd, F_SETFL, O_DIRECT);
    if (rc == -1) {
        perror("SYSTEM ERROR: fcntl failed");
        return EXIT_FAILURE;
    }

    ssize_t bytes_written_cnt = write(comm_fifo_fd, fifos_path, PATH_MAX);
    if (bytes_written_cnt == -1) {
        perror("SYSTEM ERROR: write failed");
        return EXIT_FAILURE;
    }
    assert(bytes_written_cnt == PATH_MAX);

    CRIT_SEC_BEGIN
    size_t attempts_cnt = 10;
    ssize_t bytes_read_cnt;
    int msg_fifo_fd = -1;
    do {
        if (msg_fifo_fd == -1) {
            msg_fifo_fd = open(msg_fifo_path, O_WRONLY | O_NONBLOCK);
            if (msg_fifo_fd == -1 && errno != ENXIO) {
                perror("SYSTEM ERROR: open failed");
                return EXIT_FAILURE;
            }
        }
        bytes_read_cnt = read(conn_fifo_fd, dummy_buf, PIPE_BUF * 2);
        if (bytes_read_cnt == -1 && errno != EAGAIN) {
            perror("SYSTEM ERROR: read failed");
            return EXIT_FAILURE;
        }
        sleep(1);
        --attempts_cnt;
    } while ((msg_fifo_fd == -1 || bytes_read_cnt == -1) && attempts_cnt > 0);
    if (msg_fifo_fd == -1 || bytes_read_cnt == -1) {
        printf("RECEIVER ERROR: failed to connect to receiver");
        return EXIT_FAILURE;
    }

    CRIT_SEC_BEGIN
    rc = fcntl(msg_fifo_fd, F_SETFL, 0);
    if (rc == -1) {
        perror("SYSTEM ERROR: fcntl failed");
        return EXIT_FAILURE;
    }

    sighandler_t sighandler = signal(SIGPIPE, SIG_IGN);
    if (sighandler == SIG_ERR) {
        perror("SYSTEM ERROR: signal failed");
        return EXIT_FAILURE;
    }

    while (true) {
        bytes_read_cnt = read(file_fd, buf, PIPE_BUF);
        if (bytes_read_cnt == -1) {
            perror("SYSTEM ERROR: read failed");
            return EXIT_FAILURE;
        }
        if (bytes_read_cnt == 0) {
            return EXIT_SUCCESS;
        }

        bytes_written_cnt = write(msg_fifo_fd, buf, bytes_read_cnt);
        if (bytes_written_cnt == -1) {
            if (errno == EPIPE) {
                printf("RECEIVER ERROR: receiver closed message fifo\n");
            } else {
                perror("SYSTEM ERROR: write failed");
            }
            return EXIT_FAILURE;
        }
        if (bytes_written_cnt < bytes_read_cnt) {
            printf("RECEIVER ERROR: receiver closed message fifo\n");
            return EXIT_FAILURE;
        }
    }
    CRIT_SEC_END
    CRIT_SEC_END
}
