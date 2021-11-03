#include <stdio.h>
#include <fcntl.h>

#include <sys/stat.h>

int files_eq(int fd1, int fd2) {
    char null = '\0';

    struct statx stat1;
    int rc = statx(fd1, &null, AT_EMPTY_PATH, STATX_ALL, &stat1);
    if (rc != 0) {
        perror("SYSTEM ERROR: fstat failed");
        return -1;
    }

    struct statx stat2;
    rc = statx(fd2, &null, AT_EMPTY_PATH, STATX_ALL, &stat2);
    if (rc != 0) {
        perror("SYSTEM ERROR: fstat failed");
        return -1;
    }

    return stat1.stx_dev_major == stat2.stx_dev_major && stat1.stx_dev_minor == stat2.stx_dev_minor &&
           stat1.stx_ino == stat2.stx_ino;
}

int main() {
    // touch file
    int fd1 = open("file", O_RDONLY);
    // rm file
    // touch file
    int fd2 = open("file", O_RDONLY);

    printf("%d", files_eq(fd1, fd2));
    // 0

    /*
     * touch file
     * int fd1 = open("file", O_RDONLY);
     * rename file file0 file
     * touch file
     * int fd2 = open("file", O_RDONLY);
     */
    /*
     * touch file
     * int fd1 = open("file", O_RDONLY);
     * gdb -q
     * attach $PID
     * call (int) chdir("some_other_dir")
     * detach
     * quit
     * touch file
     * int fd2 = open("file", O_RDONLY);
     */
    /*
     * touch file
     * int fd1 = open("file", O_RDONLY);
     * mount /dev/cdrom $(PWD)
     * touch file
     * int fd2 = open("file", O_RDONLY);
     */
}
