#include <assert.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <unistd.h>

static const char *const fifo_name = "fifo";
static const char *const sync_name = "sync";
static const char *const wr_lock_name = "wr_lock";

char buf[PIPE_BUF];

int main(int argc, const char *const argv[])
{
  --argc;
  ++argv;

  if (argc != 1) {
    printf("USAGE: fifo-sender <path_to_file>\n");
    return EXIT_SUCCESS;
  }

  const char *file_name = argv[0];
  int file_fd = open(file_name, O_RDONLY);
  if (file_fd == -1) {
    perror("SYSTEM ERROR: open failed");
    return EXIT_SUCCESS;
  }

  int rc;
  do {
    sleep(1);
    rc = open(wr_lock_name, O_CREAT | O_EXCL, S_IRUSR | S_IWUSR);
  } while (rc == -1);

  int fifo_fd = open(fifo_name, O_WRONLY);
  if (fifo_fd == -1) {
    perror("SYSTEM ERROR: open failed");
    goto wr_lock_unlink;
  }

  while (true) {
    ssize_t bytes_read_cnt = read(file_fd, buf, PIPE_BUF);
    if (bytes_read_cnt == -1) {
      perror("SYSTEM ERROR: read failed");
      break;
    }
    if (bytes_read_cnt == 0) {
      break;
    }

    ssize_t bytes_written_cnt = write(fifo_fd, buf, bytes_read_cnt);
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

  rc = open(sync_name, O_WRONLY);
  if (rc == -1) {
    perror("SYSTEM ERROR: open failed");
  }

wr_lock_unlink:
  rc = unlink(wr_lock_name);
  if (rc != 0) {
    perror("SYSTEM ERROR: unlink failed");
  }
}
