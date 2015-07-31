#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <getopt.h>
#include <errno.h>

int main(int argc, char *argv[]) {
	int input = 0;
	int output = -1;
	void *rotbuf = NULL;
	size_t logsz = 0;
	int c;

	int rotate = 2;
	size_t maxlogsz = 1024;
	char *logname = "logcat.txt";

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

	output = open(logname, O_CREAT | O_RDWR | O_APPEND, 0644);
	if (output == -1) {
		fprintf(stderr, "failed to create output file: %d\n", errno);
		return -1;
	}

	off_t offset = lseek(output, 0, SEEK_END);
	if (offset == -1) {
		fprintf(stderr, "lseek() failed: %d\n", errno);
		return -1;
	}
	logsz = (size_t) offset;

	for (;;) {
		char buf[BUFSIZ];
		size_t sz;

		sz = read(input, buf, sizeof(buf));
		if (sz < 0) {
			fpritnf(stderr, "read failed: %d, errno=%d\n", sz, errno);
		} else if (sz == 0) {
			break;
		}
		while (logsz + sz >= maxlogsz) {
			int i;
			size_t tailsz;
			char oldname[BUFSIZ];
			char newname[BUFSIZ]; // TODO: PATH_MAX?

			/* rotate */
			tailsz = logsz + sz - maxlogsz;
			/*fprintf(stderr, "rotate %d %d %d %d\n", sz, logsz, sz - tailsz, maxlogsz);*/
			write(output, buf, sz - tailsz);
			memmove(buf, buf + sz - tailsz, tailsz);
			sz = tailsz;
			close(output);

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

			output = open(logname, O_CREAT | O_RDWR | O_TRUNC, 0644);
			if (output == -1) {
				fprintf(stderr, "failed to create output file: %d\n", errno);
				return -1;
			}
			logsz = 0;
		}

		write(1, buf, sz);
		write(output, buf, sz);
		fsync(output);
	}

	close(input);
	close(output);

	return 0;
}

