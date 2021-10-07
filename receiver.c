#include <assert.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <unistd.h>

#include <sys/stat.h>

static const char *const fifo_name = "fifo";
static const char *const rd_lock_name = "rd_lock";
static const char *const sync_name = "sync";

static char buf[PIPE_BUF];

int main()
{
  int rc;
  do {
    sleep(1);
    rc = open(rd_lock_name, O_CREAT | O_EXCL, S_IRUSR | S_IWUSR);
  } while (rc == -1);

  int fifo_fd = open(fifo_name, O_RDONLY);
  if (fifo_fd == -1) {
    perror("SYSTEM ERROR: open failed");
    goto rd_lock_unlink;
  }

  while (true) {
    ssize_t bytes_read_cnt = read(fifo_fd, buf, PIPE_BUF);
    if (bytes_read_cnt == -1) {
      perror("SYSTEM ERROR: read failed");
      break;
    }
    if (bytes_read_cnt == 0) {
      break;
    }

    ssize_t bytes_written_cnt = write(STDOUT_FILENO, buf, bytes_read_cnt);
    if (bytes_written_cnt == -1) {
      perror("SYSTEM ERROR: write failed");
      break;
    }
    assert(bytes_written_cnt == bytes_read_cnt);
  }
  rc = close(fifo_fd);
  if (rc != 0) {
    perror("SYSTEM ERROR: close failed");
  }

  rc = open(sync_name, O_RDONLY);
  if (rc == -1) {
    perror("SYSTEM ERROR: open failed");
  }

rd_lock_unlink:
  rc = unlink(rd_lock_name);
  if (rc != 0) {
    perror("SYSTEM ERROR: unlink failed");
    return EXIT_SUCCESS;
  }
}
