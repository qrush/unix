/*
 * Quick hack to read old unix a.out files and make
 * a simh "load file".
 *
 * Brad Parker <brad@heeltoe.com> 5/2008
 */
#include <sys/types.h>
#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/uio.h>
#include <unistd.h>

/* Binary loader.

   Loader format consists of blocks, optionally preceded, separated, and
   followed by zeroes.  Each block consists of:

        001             ---
        xxx              |
        lo_count         |
        hi_count         |
        lo_origin        > count bytes
        hi_origin        |
        data byte        |
        :                |
        data byte       ---
        checksum

   If the byte count is exactly six, the block is the last on the tape, and
   there is no checksum.  If the origin is not 000001, then the origin is
   the PC at which to start the program.
*/

typedef unsigned short u16;

struct exec {
        u16   a_magic;
        u16   a_text;
        u16   a_data;
        u16   a_bss;
        u16   a_syms;
        u16   a_entry;
        u16   a_unused;
        u16   a_flag;
};

unsigned char buffer[1024];
int checksum;

int verbose;

int
mk_hdr(int fd, int count, int origin)
{
	int i, ret, hc;

	checksum = 0;
	hc = count + 6;

	buffer[0] = 0x01;
	buffer[1] = 0x00;
	buffer[2] = hc & 0xff;
	buffer[3] = hc >> 8;
	buffer[4] = origin & 0xff;
	buffer[5] = origin >> 8;

	ret = write(fd, buffer, 6);
	if (ret != 6)
		return -1;

	for (i = 0; i < 6; i++)
		checksum += buffer[i] & 0xff;

	return 0;
}

int
mk_post_data(int fd)
{
	int ret, final_checksum;

	/* final checksum must be zero */
	final_checksum = 256 - (checksum & 0xff);

	buffer[0] = final_checksum;
	if (verbose)
		printf("checksum %08x, final %02x\n", checksum, final_checksum);

	ret = write(fd, buffer, 1);
	if (ret != 1)
		return -1;

	return 0;
}

int
load(int fd, char *buf, int bufsize)
{
	int i, ret;

	printf("load: %d bytes\n", bufsize);

	if (bufsize < 65536) {
		mk_hdr(fd, bufsize, 0);

		ret = write(fd, buf, bufsize);
		if (ret != bufsize)
			return -1;

		for (i = 0; i < bufsize; i++)
			checksum += buf[i] & 0xff;

		mk_post_data(fd);
	}

	return 0;
}

void
finish(int fd, int origin)
{
	mk_hdr(fd, 0, origin);
}

int
process_aout(int ifd, int ofd, int origin)
{
	int ret, hdr_size;
	struct exec hdr;
	char *textblob, *datablob;

	ret = read(ifd, (char *)&hdr, sizeof(hdr));
	if (ret != sizeof(hdr)) {
		return -1;
	}

	if (1) {
		printf("magic 0%o\ntext %d, data %d, bss %d, syms %d\n",
		       hdr.a_magic & 0xffff,
		       hdr.a_text, hdr.a_data, hdr.a_bss, hdr.a_syms);
		printf("entry %o, flag %o\n",
		       hdr.a_entry,
		       hdr.a_flag);
	}

	textblob = malloc(hdr.a_text);
	datablob = malloc(hdr.a_data);

	hdr_size = 0;

	/* v7 header? if so, 0 is after header */
	switch (hdr.a_magic) {
	case 0407: hdr_size = 0; break;
	case 0405: hdr_size = 12; break;
	}

	if (hdr_size) {
		/* back up */
		lseek(ifd, (off_t)-hdr_size, SEEK_CUR);
		hdr.a_text += hdr_size;
	}

	ret = read(ifd, textblob, hdr.a_text);
	if (ret != hdr.a_text)
		return -1;

	ret = read(ifd, datablob, hdr.a_data);
	if (ret != hdr.a_data)
		return -1;

	load(ofd, textblob, hdr.a_text);
	load(ofd, datablob, hdr.a_data);

	finish(ofd, origin);

	return 0;

}

int
process(char *inf, char *outf)
{
	int ifd, ofd, ret;
	int origin;

	ofd = open(outf, O_WRONLY | O_CREAT, 0666);
	if (ofd < 0) {
		perror(outf);
		return -1;
	}

	ifd = open(inf, O_RDONLY);
	if (ifd < 0) {
		perror(inf);
		close(ofd);
		return -1;
	}

	ret = 0;
	origin = 0;

	if (process_aout(ifd, ofd, origin)) {
		ret = -1;
	}

	close(ifd);
	close(ofd);

	return ret;
}


int main(int argc, char *argv[])
{
	char *filename = "a.out";
	char *output_filename = "loadfile";

	verbose = 0;

	if (process(filename, output_filename)) {
		fprintf(stderr, "%s: failed?\n", filename);
	}

	exit(0);
}
