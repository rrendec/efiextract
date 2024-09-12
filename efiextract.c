/* SPDX-License-Identifier: GPL-2.0 */

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <endian.h>
#include <sys/param.h>

/* The PE/COFF MS-DOS stub magic number */
#define EFI_PE_MSDOS_MAGIC        "MZ"

/*
 * The Linux header magic number for a EFI PE/COFF
 * image targetting an unspecified architecture.
 */
#define EFI_PE_LINUX_MAGIC        "\xcd\x23\x82\x81"

/*
 * Bootable Linux kernel images may be packaged as EFI zboot images, which are
 * self-decompressing executables when loaded via EFI. The compressed payload
 * can also be extracted from the image and decompressed by a non-EFI loader.
 *
 * The de facto specification for this format is at the following URL:
 *
 * https://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git/tree/drivers/firmware/efi/libstub/zboot-header.S
 *
 * This definition is based on Linux upstream commit 29636a5ce87beba.
 */
struct linux_efi_zboot_header {
	uint8_t		msdos_magic[2];		/* PE/COFF 'MZ' magic number */
	uint8_t		reserved0[2];
	uint8_t		zimg[4];		/* "zimg" for Linux EFI zboot images */
	uint32_t	payload_offset;		/* LE offset to compressed payload */
	uint32_t	payload_size;		/* LE size of the compressed payload */
	uint8_t		reserved1[8];
	char		compression_type[32];	/* Compression type, NUL terminated */
	uint8_t		linux_magic[4];		/* Linux header magic */
	uint32_t	pe_header_offset;	/* LE offset to the PE header */
};

static inline void fixendian(uint32_t *ptr)
{
	*ptr = le32toh(*ptr);
}

static void copy_data(FILE *fin, FILE *fout, long offset, size_t len)
{
	unsigned char buf[16384];

	if (fseek(fin, offset, SEEK_SET)) {
		fprintf(stderr, "Error seeking data: %s", strerror(errno));
		return;
	}

	while (len) {
		size_t size = MIN(len, sizeof(buf));

		if (fread(buf, size, 1, fin) != 1) {
			fprintf(stderr, "Error reading data\n");
			return;
		}

		if (fwrite(buf, size, 1, fout) != 1) {
			fprintf(stderr, "Error writing data\n");
			return;
		}

		len -= size;
	}
}

int main(int argc, char **argv)
{
	FILE *fin, *fout = NULL;
	struct linux_efi_zboot_header header;
	int len;

	if (argc < 2) {
		fprintf(stderr, "Usage: %s <input> [<output>]\n", argv[0]);
		return EXIT_FAILURE;
	}

	fin = fopen(argv[1], "r");
	if (!fin) {
		fprintf(stderr, "Error opening %s: %s\n", argv[1], strerror(errno));
		return EXIT_FAILURE;
	}

	if (argc > 2 && !(fout = fopen(argv[2], "w"))) {
		fclose(fin);
		fprintf(stderr, "Error opening %s: %s\n", argv[2], strerror(errno));
		return EXIT_FAILURE;
	}

	len = fread(&header, sizeof(header), 1, fin);
	if (len != 1) {
		fclose(fin);
		fprintf(stderr, "Error reading EFI zboot header\n");
		return EXIT_FAILURE;
	}

	if (memcmp(&header.msdos_magic, EFI_PE_MSDOS_MAGIC, 2) != 0 ||
	    memcmp(&header.zimg, "zimg", 4) != 0 ||
	    memcmp(&header.linux_magic, EFI_PE_LINUX_MAGIC, 4) != 0) {
		fprintf(stderr, "Error: input is not a kernel EFI image\n");
		return EXIT_FAILURE;
	}

	fixendian(&header.payload_offset);
	fixendian(&header.payload_size);
	fixendian(&header.pe_header_offset);

	printf("Compression:    %.*s\n"
	       "Payload offset: %u Bytes\n"
	       "Payload size:   %u Bytes\n",
	       (int)sizeof(header.compression_type), header.compression_type,
	       header.payload_offset, header.payload_size);

	if (fout) {
		copy_data(fin, fout, header.payload_offset, header.payload_size);
		fclose(fout);
	}

	fclose(fin);

	return EXIT_SUCCESS;
}
