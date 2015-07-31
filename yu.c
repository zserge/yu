#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <getopt.h>
#include <errno.h>

/* 
 * Try writing if file is a valid one, if write fails - close file and mark is
 * as invalid.
 */
static int xwrite(int *fd, void *buf, size_t bufsz) {
	int n;
	if (*fd > -1) {
		n = write(*fd, buf, bufsz);
		if (n != bufsz) {
			fprintf(stderr, "short write: %d, errno=%d\n", n, errno);
			close(*fd);
			*fd = -1;
		}
	}
}

int main(int argc, char *argv[]) {
	int infd = STDIN_FILENO;   /* input */
	int stdfd = STDOUT_FILENO; /* output */
	int outfd = -1;            /* rotating output */

	void *rotbuf = NULL;
	size_t logsz = 0;
	int c;

	int rotate = 2;
	size_t maxlogsz = 1024;
	char *logname = NULL;

	while ((c = getopt(argc, argv, "r:n:")) != -1) {
		switch (c) {
			case 'n':
				/* TODO: parse nM, nK as megabytes and kilobytes */
				maxlogsz = atoi(optarg);
				break;
			case 'r':
				/* TODO: check for valid number */
				rotate = atoi(optarg);
				break;
		}
	}

	if (optind != argc-1) {
		fprintf(stderr, "log name expected\n");
		return -1;
	}

	logname = argv[optind];
	
	rotbuf = malloc(maxlogsz);
	if (rotbuf == NULL) {
		fprintf(stderr, "malloc() failed\n");
		return -1;
	}

	/* Try opening file for append, but ignore all errors */
	outfd = open(logname, O_CREAT | O_RDWR | O_APPEND, 0644);
	if (outfd == -1) {
		fprintf(stderr, "open() failed: file=%s, errno=%d\n", logname, errno);
	}

	/* Go to the end of the file, on error - close the output */
	off_t offset = lseek(outfd, 0, SEEK_END);
	if (offset == -1) {
		fprintf(stderr, "lseek() failed: errno=%d\n", errno);
		close(outfd);
		outfd = -1;
	}
	logsz = (size_t) offset;

	for (;;) {
		char buf[BUFSIZ];
		size_t sz;

		/* Read from input, on error - terminate loop */
		sz = read(infd, buf, sizeof(buf));
		if (sz <= 0) {
			if (sz < 0) {
				fpritnf(stderr, "read failed: %d, errno=%d\n", sz, errno);
			}
			break;
		}

		/* If appending the buffer overflow the file - perform rotation */
		while (logsz + sz > maxlogsz) {
			int i;
			size_t tailsz;
			char oldname[BUFSIZ];
			char newname[BUFSIZ];

			/* rotate, print to stdout and close the current file */
			tailsz = logsz + sz - maxlogsz;
			xwrite(&outfd, buf, sz - tailsz);
			memmove(buf, buf + sz - tailsz, tailsz);
			sz = tailsz;
			close(outfd);

			for (i = rotate - 1; i >= 0; i--) {
				snprintf(newname, sizeof(newname)-1, "%s.%u", logname, i+1);
				if (i > 0) {
					snprintf(oldname, sizeof(oldname)-1, "%s.%u", logname, i);
				} else {
					snprintf(oldname, sizeof(oldname)-1, "%s", logname);
				}
				// FIXME check errors
				unlink(newname);
				rename(oldname, newname);
			}

			outfd = open(logname, O_CREAT | O_RDWR | O_TRUNC, 0644);
			if (outfd == -1) {
				fprintf(stderr, "open() failed: file=%s, errno=%d\n", logname, errno);
			}
			logsz = 0;
		}

		xwrite(&stdfd, buf, sz);
		xwrite(&outfd, buf, sz);
		fsync(outfd);
	}

	close(infd);
	close(outfd);

	return 0;
}

