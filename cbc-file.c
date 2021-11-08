/*
 * This is an implementation that intends to provide a simple executable to
 * encrypt and decrypt an arbitrary file.
 *
 * The file is encrypted with ChaCha20+Poly1305 and the key is derived from the
 * password input and a salt.
 *
 * File format:
 * <encryption type: 1 byte> <salt: 32 bytes> <iv: 32 bytes> <aad: 32 bytes> <tag: 16 bytes> <file>
 */

#define _DEFAULT_SOURCE /* getentropy() */
#define _XOPEN_SOURCE 600 /* fdopen() */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>

#include <bearssl.h>
#include <argon2.h>

#define KEY_LENGTH 32
#define SALT_LENGTH 32
#define IV_LENGTH 32
#define AAD_LENGTH 32
#define TAG_LENGTH 16
#define FINAL_LENGTH (1 + SALT_LENGTH + IV_LENGTH + AAD_LENGTH + TAG_LENGTH)

#define TIME_COST 32 /* 32 computations */
#define MEM_COST (1 << 16) /* 2^16 bytes */
#define PARALLEL 4 /* number of threads/lanes */
#define PASS_LENGTH 32


enum encryption_type {
CHACHA20POLY1305 = 1,
};
const enum encryption_type default_enctype = CHACHA20POLY1305;

/* constant time */
static int ct_memcmp(const void *va, const void *vb, size_t l)
{
	const unsigned char *a = va, *b = vb;
	unsigned char rv = 0;
	for (size_t i=0; i<l; i++) rv |= a[i] ^ b[i];
	return rv;
}

static void usage(void)
{
	fputs("Usage: cbd-file lock|unlock <file>", stderr);
	exit(1);
}

static int read_password(uint8_t *key, const uint8_t *salt)
{
	int rv = -1;

	fprintf(stderr, "input password (up to %d): ", KEY_LENGTH);

	struct termios old, t;
	tcgetattr(STDIN_FILENO, &old);
	t = old;
	// only disable echo, keep other attributes
	t.c_lflag = old.c_lflag & ~(ECHO);
	tcsetattr(STDIN_FILENO, TCSANOW, &t);

	char pass[PASS_LENGTH];
	if (fgets(pass, PASS_LENGTH, stdin) == NULL) {
		// echo should always be turned off at exit
		goto end;
	}
	rv = 0;

	argon2id_hash_raw(
		TIME_COST, MEM_COST, PARALLEL,
		pass, strlen(pass), salt, SALT_LENGTH,
		key, KEY_LENGTH);

end:
	// restore previous attributes
	tcsetattr(STDIN_FILENO, TCSANOW, &old);

	return rv;
}

static off_t file_size(int fd)
{
	struct stat s = { 0 };
	int rv = fstat(fd, &s);
	if (rv != 0) {
		perror("fstat()");
		exit(100);
	}
	return s.st_size;
}

static ssize_t open_file_for_read(char *name, uint8_t **buffer)
{
	int f = open(name, O_RDONLY | O_CLOEXEC);
	if (f < 0) {
		perror("open()");
		return -1;
	}

	off_t size = file_size(f);
	*buffer = calloc(1, size);
	if (buffer == NULL) {
		perror("calloc()");
		return -1;
	}
	FILE *file = fdopen(f, "r");
	if (file == NULL) {
		perror("fdopen()");
		return -1;
	}

	if (fread(*buffer, 1, size, file) != (size_t)size) {
		fputs("mismatched file size and bytes read!\n", stderr);
		return -1;
	}
	fclose(file);

	return size;
}

int main(int argc, char **argv)
{
	if (argc < 3) {
		usage();
	}

	uint8_t *buffer;
	ssize_t bytes;

	uint8_t enctype;
	uint8_t key[KEY_LENGTH] = { 0 };
	uint8_t iv[IV_LENGTH];
	uint8_t aad[AAD_LENGTH];
	uint8_t salt[SALT_LENGTH];
	uint8_t tag[TAG_LENGTH];

	if (strcmp(argv[1], "lock") == 0) {
		// locking code
		bytes = open_file_for_read(argv[2], &buffer);
		if (bytes < 0) {
			return 1;
		}

		if (getentropy(iv, IV_LENGTH) < 0
			|| getentropy(aad, AAD_LENGTH) < 0
			|| getentropy(salt, SALT_LENGTH) < 0) {
			perror("getentropy()");
			return 1;
		}
		if (read_password(key, salt) < 0) {
			return 1;
		}

		br_poly1305_ctmul_run(key, iv, buffer, bytes, aad, AAD_LENGTH, tag, br_chacha20_ct_run, 1);

		enctype = default_enctype;
		fwrite(&enctype, 1, 1, stdout);

		fwrite(salt, 1, SALT_LENGTH, stdout);
		fwrite(iv, 1, IV_LENGTH, stdout);
		fwrite(aad, 1, AAD_LENGTH, stdout);
		fwrite(tag, 1, TAG_LENGTH, stdout);

		fwrite(buffer, 1, bytes, stdout);
	} else if (strcmp(argv[1], "unlock") == 0) {
		// unlocking code
		bytes = open_file_for_read(argv[2], &buffer);
		if (bytes < FINAL_LENGTH) {
			return 1;
		}

		memcpy(&enctype, buffer, 1);
		buffer += 1;
		if (enctype != default_enctype) {
			fputs("wrong encryption mode!\n", stderr);
			return 1;
		}
		memcpy(salt, buffer, SALT_LENGTH);
		buffer += SALT_LENGTH;
		memcpy(iv, buffer, IV_LENGTH);
		buffer += IV_LENGTH;
		memcpy(aad, buffer, AAD_LENGTH);
		buffer += AAD_LENGTH;
		memcpy(tag, buffer, TAG_LENGTH);
		buffer += TAG_LENGTH;

		bytes -= FINAL_LENGTH;

		if (read_password(key, salt) < 0) {
			return 1;
		}

		uint8_t new_tag[TAG_LENGTH];
		br_poly1305_ctmul_run(key, iv, buffer, bytes, aad, AAD_LENGTH, new_tag, br_chacha20_ct_run, 0);
		if (ct_memcmp(tag, new_tag, TAG_LENGTH)) {
			fputs("bad tag!\n", stderr);
			return 1;
		}

		fwrite(buffer, 1, bytes, stdout);
	} else {
		usage();
	}

	return 0;
}
