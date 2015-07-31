# Ю  - it's like ⊢ but with file rotation

Yu (named after a cyrillic letter) is a tee-like tool that takes data from
stdin, prints it to stdout and also keeps it in the rotated files.

It's useful for simple log rotation in embedded environment.

	Usage:

		-r N   - how many rotated files to keep
		-n N   - maximum file size in bytes

	Example:

		# Rotate dmesg output in 4 files, 1MB each
		cat /proc/kmsg | yu -r 4 -n 1048576

Licensed under MIT license.
