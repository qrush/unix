/*
 * conv2.c
 *	Tape file converter for simh
 *
 * This utility is for converting files to and from the tape format
 * used by simh.
 *
 * 2004 Tim N.
 * This code is in the public domain.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int
testeof(FILE *fp)
{
	int ch;

	if(!feof(fp) && (ch = fgetc(fp)) != -1) {
		ungetc(ch, fp);
		return 0;
	}
	return 1;
}

void
xperror(char *msg)
{
    perror(msg);
    exit(1);
}

FILE *
xfopen(char *fname, char *mode)
{
	FILE *fp = fopen(fname, mode);

	if(!fp)
		xperror(fname);
	return fp;
}

void
optfclose(FILE *fp)
{
	if(fp)
		fclose(fp);
}

int
xreadint(FILE *fp)
{
	unsigned char buf[4];

	if(fread(buf, sizeof buf, 1, fp) != 1) {
		fprintf(stderr, "short read\n");
		exit(1);
	}
	return buf[0] | (buf[1]<<8) | (buf[2]<<16) | (buf[3]<<24);
}

void
xwriteint(FILE *fp, int x)
{
	unsigned char buf[4];

	buf[0] = x & 0xff;
	buf[1] = (x>>8) & 0xff;
	buf[2] = (x>>16) & 0xff;
	buf[3] = (x>>24) & 0xff;
	fwrite(buf, sizeof buf, 1, fp);
}

void
xcopy(FILE *infp, FILE *outfp, int cnt, int allowshort)
{
	char buf[512];
	int x, r;

	while(cnt > 0) {
		r = (cnt > sizeof buf) ? sizeof buf : cnt;
		if((x = fread(buf, 1, r, infp)) != r) {
			if(x && allowshort) {
				memset(buf + x, 0, (sizeof buf) - x);
			} else {
				fprintf(stderr, "short read\n");
				exit(1);
			}
		}
		fwrite(buf, 1, r, outfp);
		cnt -= r;
	}
}

int
convout(FILE *infp, char *fname, int vflag)
{
	FILE *outfp = 0;
	char buf[128], *p;
	unsigned int l, reclen, i, cnt = 0;
	int bsize = -1;

	p = strrchr(fname, '.');
	if(p)
		l = p - fname;
	else
		l = strlen(fname);
	if(l + 10 > sizeof buf) {
		fprintf(stderr, "use a smaller name\n");
		return -1;
	}

	i = 1;
	while(!testeof(infp)) {
		reclen = xreadint(infp);
		if(reclen == 0x00000000) {
			if(vflag) {
				if(cnt)
					fprintf(stderr, "%s %d@%d -- ", buf, cnt, bsize);
				fprintf(stderr, "EOF\n");
			}
			optfclose(outfp);
			outfp = 0;
			cnt = 0;

		} else if(reclen == 0xffffffff) {
			if(vflag) {
				if(cnt)
					fprintf(stderr, "%s %d@%d -- ", buf, cnt, bsize);
				fprintf(stderr, "EOT\n");
			}
			optfclose(outfp);
			break;

		} else {
			if(reclen > 0x10000) {
				/* XXX we dont handle high-bit == error block */
				fprintf(stderr, "Bad Format?  Got length %d\n", reclen);
				return -1;
			}
			if(cnt && reclen != bsize) {
				fprintf(stderr, "Expected %d bytes, got %d bytes\n", bsize, reclen);
				return -1;
			}
			cnt ++;
			bsize = reclen;

			if(!outfp) {
				sprintf(buf, "%.*s-%d.raw", l, fname, i++);
				outfp = xfopen(buf, "wb");
			}
			xcopy(infp, outfp, reclen, 0);
			if(xreadint(infp) != reclen) {
				fprintf(stderr, "format error\n");
				return -1;
			}
		}
	}
	return 0;
}

int
convin(FILE *ofile, char *fname, int bsize, int vflag)
{
	FILE *ifile = xfopen(fname, "rb");
	int cnt = 0;

	while(!testeof(ifile)) {
		xwriteint(ofile, bsize);
		xcopy(ifile, ofile, bsize, 1);
		xwriteint(ofile, bsize);
		cnt ++;
	}
	xwriteint(ofile, 0); /* EOF */
	if(vflag)
		fprintf(stderr, "%s: %d@%d\n", fname, cnt, bsize);
	return 0;
}

void
xusage(char *prog)
{
	fprintf(stderr, "usage:  %s [-v] [-i file.tap | -o file.tap file ...]\n"
				"\tWhen using -o, files may be interspersed with [-b bsize]\n"
				"\tto specify the block size to use for following files\n"
				"\tThe default block size is 512.\n", prog);
	exit(1);
}

int
main(int argc, char **argv)
{
	FILE *ifile = 0, *ofile = 0;
	char *prog;
	int i, ocnt = 0, bsize = 512, vflag = 0;
	char *ofname = 0, ch;

	prog = argv[0];
    for(i = 1; i < argc; i++) {
		if (*argv[i] == '-') {
			ch = argv[i][1];
    		if(ch == 'v') 
        		vflag ++;
			else if(ch == 'b') {
				i++;
				if(!ofile || !argv[i] || !(bsize = atoi(argv[i])))
					xusage(prog);

			} else if(ch == 'i') {
				if(ifile || ofile || !argv[++i])
					xusage(prog);
				ifile = xfopen(argv[i], "rb");
				if(convout(ifile, argv[i], vflag) == -1)
					exit(1);

			} else if(ch == 'o') {
				if(ifile || ofile || !argv[++i])
					xusage(prog);
				ofile = xfopen(argv[i], "wb");
				ofname = argv[i];

			} else
				xusage(prog);
			continue;
		}

		if(!ofile)
			xusage(prog);
		if(convin(ofile, argv[i], bsize, vflag) == -1)
			exit(1);
		ocnt ++;
    }
	if(ofile)
		xwriteint(ofile, 0); /*EOF*/ /* 0xffffffff); - EOT */

	return 0;
}

