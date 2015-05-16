#include <fcntl.h>
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/major.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>

#define TTY_MODE (S_IFCHR | S_IRUSR | S_IWUSR | S_IWGRP)

static _Noreturn void die(const char *msg)
{
	printf("%s\n", msg);
	exit(1);
}

static void fs_init(void)
{
	/* /dev/ */
	if (mkdir("/dev", S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH))
		die("fs_init: mkdir failed");
	if (mknod("/dev/cons0", TTY_MODE, makedev(TTY_MAJOR, 0)))
		die("fs_init: mknod failed");
	if (mknod("/dev/cons1", TTY_MODE, makedev(TTY_MAJOR, 1)))
		die("fs_init: mknod failed");

	/* /bin/ */
	if (mkdir("/bin", S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH))
		die("fs_init: mkdir failed");
	if (mount(NULL, "/bin", "binfs", 0, NULL))
		die("fs_init: mount failed");

	/* /mod/ */
	if (mkdir("/mod", S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH))
		die("fs_init: mkdir failed");
	if (mount(NULL, "/mod", "modfs", 0, NULL))
		die("fs_init: mount failed");

	int fd;
	char buf[6];

	if ((fd = open("/file", O_RDWR | O_CREAT, S_IRWXU)) < 0)
		die("fs_init: open failed");
	if (close(fd))
		die("fs_init: close failed");
	if ((fd = open("/test", O_RDWR | O_CREAT, S_IRWXU)) < 0)
		die("fs_init: open failed");
	if (write(fd, "test\n", 5) != 5)
		die("fs_init: write failed");
	if (close(fd))
		die("fs_init: close failed");
	if ((fd = open("/test", O_RDONLY)) < 0)
		die("fs_init: open(2) failed");
	if (read(fd, buf, 7) != 5)
		die("fs_init: read failed");
	if (close(fd))
		die("fs_init: close failed");
	buf[5] = '\0';

	if (strcmp(buf, "test\n"))
		die("fs_init: read/write test failed\n");
}

int main(void)
{
	int status;
	sigset_t set;

	fs_init();

	open("/dev/cons0", O_RDONLY);
	open("/dev/cons0", O_WRONLY);
	open("/dev/cons0", O_WRONLY);

	sigfillset(&set);
	sigprocmask(SIG_SETMASK, &set, NULL);

	if (!fork()) {
		char *argv[] = { "/bin/tsh", NULL };
		char *envp[] = { NULL };
		sigprocmask(SIG_UNBLOCK, &set, NULL);
		execve("/bin/tsh", argv, envp);
	}

	for (;;)
		wait(&status);
}
