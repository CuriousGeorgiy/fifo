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

#define CRIT_SEC_BEGIN
#define CRIT_SEC_END

static const char *const comm_fifo_name = "comm_fifo";

static char fifo_paths[PATH_MAX];
static_assert(PATH_MAX <= PIPE_BUF,
              "path to message fifo must be written atomically");

static char buf[PIPE_BUF];
static char dummy_buf[PIPE_BUF * 2];

int main() {
    int comm_fifo_fd = open(comm_fifo_name, O_RDONLY);
    if (comm_fifo_fd == -1) {
        perror("SYSTEM ERROR: open failed");
        return EXIT_FAILURE;
    }

    int rc = fcntl(comm_fifo_fd, F_SETFL, O_DIRECT);
    if (rc == -1) {
        perror("SYSTEM ERROR: fcntl failed");
        return EXIT_FAILURE;
    }

    ssize_t bytes_read_cnt = 0;
    while (bytes_read_cnt == 0) {
        bytes_read_cnt = read(comm_fifo_fd, fifo_paths, PATH_MAX);
        if (bytes_read_cnt == -1) {
            perror("SYSTEM ERROR: read failed");
            return EXIT_FAILURE;
        }
        if (bytes_read_cnt == 0) {
            rc = close(comm_fifo_fd);
            if (rc != 0) {
                perror("SYSTEM ERROR: close failed");
                return EXIT_FAILURE;
            }
            comm_fifo_fd = open(comm_fifo_name, O_RDONLY);
            if (comm_fifo_fd == -1) {
                perror("SYSTEM ERROR: open failed");
                return EXIT_FAILURE;
            }

            rc = fcntl(comm_fifo_fd, F_SETFL, O_DIRECT);
            if (rc != 0) {
                perror("SYSTEM ERROR: fcntl failed");
                return EXIT_FAILURE;
            }
        }
    }
    assert(bytes_read_cnt == PATH_MAX);

    CRIT_SEC_BEGIN
    char *conn_fifo_path = strtok(fifo_paths, " ");
    if (conn_fifo_path == NULL) {
        printf("SENDER ERROR: client passed invalid fifo paths\n");
        return EXIT_FAILURE;
    }
    char *msg_fifo_path = strtok(NULL, "");
    if (msg_fifo_path == NULL) {
        printf("SENDER ERROR: sender passed invalid fifo paths\n");
        return EXIT_FAILURE;
    }

    int msg_fifo_fd = open(msg_fifo_path, O_RDONLY | O_NONBLOCK);
    if (msg_fifo_fd == -1) {
        perror("SYSTEM ERROR: open failed");
        return EXIT_FAILURE;
    }
    rc = fcntl(msg_fifo_fd, F_SETFL, 0);
    if (rc != 0) {
        perror("SYSTEM ERROR: fcntl failed");
        return EXIT_FAILURE;
    }

    int conn_fifo_fd = open(conn_fifo_path, O_WRONLY | O_NONBLOCK);
    if (conn_fifo_fd == -1) {
        if (errno == ENXIO) {
            printf("SENDER ERROR: sender closed connection fifo\n");
        } else {
            perror("SYSTEM ERROR: open failed");
        }
        return EXIT_FAILURE;
    }
    rc = fcntl(conn_fifo_fd, F_SETFL, 0);
    if (rc == -1) {
        perror("SYSTEM ERROR: fcntl failed");
        return EXIT_FAILURE;
    }

    rc = fcntl(conn_fifo_fd, F_SETPIPE_SZ, PIPE_BUF);
    if (rc == -1) {
        perror("SYSTEM ERROR: fcntl failed");
        return EXIT_FAILURE;
    }

    sighandler_t sighandler = signal(SIGPIPE, SIG_IGN);
    if (sighandler == SIG_ERR) {
        perror("SYSTEM ERROR: signal failed");
        return EXIT_FAILURE;
    }

    ssize_t bytes_written = write(conn_fifo_fd, dummy_buf, PIPE_BUF * 2);
    if (bytes_written == -1) {
        if (errno == EPIPE) {
            printf("SENDER ERROR: sender closed connection fifo\n");
        } else {
            perror("SYSTEM ERROR: write failed");
        }
        return EXIT_FAILURE;
    }

    CRIT_SEC_BEGIN
    while (true) {
        bytes_read_cnt = read(msg_fifo_fd, buf, PIPE_BUF);
        if (bytes_read_cnt == -1) {
            perror("SYSTEM ERROR: read failed");
            return EXIT_FAILURE;
        }
        if (bytes_read_cnt == 0) {
            return EXIT_SUCCESS;
        }

        ssize_t bytes_written_cnt = write(STDOUT_FILENO, buf, bytes_read_cnt);
        if (bytes_written_cnt == -1) {
            perror("SYSTEM ERROR: write failed");
            return EXIT_FAILURE;
        }
        assert(bytes_written_cnt == bytes_read_cnt);
    }
    CRIT_SEC_END
    CRIT_SEC_END
}
