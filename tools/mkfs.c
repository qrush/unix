/*
 * Program to create a 1st Edition UNIX filesystem.
 * (c) 2008 Warren Toomey, GPL3.
 *
 * Other contributors, add your name above.
 *
 * TODO: add large file functionality
 *	 ensure that swap is not used on RF disk images.
 *	 deal with errors instead of exiting :-)
 *
 *	 in the long run, have the ability to read from/write to
 *	 existing images, a la an ftp client.
 *
 * $Revision: 1.21 $
 * $Date: 2008/05/06 09:22:07 $
 */

#include <sys/types.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>

#define BLKSIZE		 512	/* 512 bytes per block */
#define RF_SIZE		1024	/* Number of blocks on RF11 */
#define RF_NOSAWPSIZE    944	/* RF11 blocks excluding swap */
#define RF_INODES	 512	/* Cold UNIX sets 512 i-nodes on RF */
#define RK_SIZE		4864	/* Number of blocks on RK03, should be 4872 */
#define INODE_RATIO	   4	/* Disksize to i-node ratio */
#define ROOTDIR_INUM	  41	/* Special i-number for / */
#define NUMDIRECTBLKS      8	/* Only 8 direct blocks in an i-node */
#define BLKSPERINDIRECT  256	/* 256 block pointers in an indirect block */

struct v1inode {		/* Format of 1st edition i-node */
  uint16_t flags;
  uint8_t nlinks;
  uint8_t uid;
  uint16_t size;
  uint16_t block[NUMDIRECTBLKS];
  uint8_t ctime[4];		/* To ensure correct alignment */
  uint8_t mtime[4];		/* on 32-bit platforms */
  uint16_t unused;
};

#define INODEPERBLOCK	(BLKSIZE/sizeof(struct v1inode))

#define I_ALLOCATED	0100000
#define I_DIR		0040000
#define I_MODFILE	0020000
#define I_LARGEFILE	0010000
#define I_SETUID	0000040
#define I_EXEC		0000020
#define I_UREAD		0000010
#define I_UWRITE	0000004
#define I_OREAD		0000002
#define I_OWRITE	0000001

/*
 * We build directories with certain limitations: first, they are all
 * directories under /; second, they all occupy 4 contiguous disk blocks,
 * giving us up to 64 directory entries.
 */
#define DIRBLOCKS	4
#define DIRENTPERBLOCK	(BLKSIZE/sizeof(struct v1dirent))

struct v1dirent {		/* Format of 1st edition dir entry */
  uint16_t inode;
  char name[8];
};

struct directory {		/* Internal structure for each dir */
  uint16_t block;		/* Starting block */
  uint16_t inum;		/* I-node number used by this dir */
  int numentries;		/* Number of entries in entry[] */
  int nextfree;			/* Next free entry */
  struct v1dirent *entry;
};

#define PERMLISTSIZE 500
struct fileperm {		/* We read an external file to determine */
  char *name;			/* a list of struct fileperms which we can */
  uint16_t flags;		/* apply to the files which we add to the */
  uint8_t uid;			/* filesystem. */
  uint32_t mtime;
} permlist[PERMLISTSIZE];


unsigned char buf[BLKSIZE];	/* A block buffer */
uint16_t disksize;		/* Number of blocks on disk */
uint16_t icount=0;		/* Number of i-nodes created */
uint16_t nextfreeblock;		/* The next free block available */
uint8_t *freemap;		/* In-memory free-map */
uint8_t *inodemap;		/* In-memory inode bitmap */
struct v1inode *inodelist;	/* In-memory i-node list */

struct directory *rootdir;	/* The root directory */

FILE *diskfh;			/* Disk filehandle */
int debug=0;			/* Enable debugging output */


/* Write the superblock out to the image */
void write_superblock(void)
{
  uint16_t freemapsize= disksize / 8;
  uint16_t inodemapmapsize= icount / 8;

  if (debug) printf("Writing freemap and inodemap from block 0\n");
  fseek(diskfh, 0, SEEK_SET);
  fwrite(&freemapsize, sizeof(freemapsize), 1, diskfh);
  fwrite(freemap, sizeof(uint8_t), (disksize / 8), diskfh);
  fwrite(&inodemapmapsize, sizeof(inodemapmapsize), 1, diskfh);
  fwrite(inodemap, sizeof(uint8_t), ((icount - ROOTDIR_INUM) / 8), diskfh);
  if (debug) {
    long posn= ftell(diskfh);
    long block= posn/BLKSIZE;
    long off= posn/BLKSIZE;
    printf("ending up at posn %ld, block %ld, offset %ld\n", posn, block, off);
    printf("Writing inodelist from block 2\n");
  }
  fseek(diskfh, 2 * BLKSIZE, SEEK_SET);
  if (debug)
    printf("Writing %d inodes, each size %d, total %d\n",
	icount, sizeof(struct v1inode), icount * sizeof(struct v1inode));
  fwrite(&inodelist[1], sizeof(struct v1inode), icount-1, diskfh);
  if (debug) {
    long posn= ftell(diskfh);
    long block= posn/BLKSIZE;
    long off= posn/BLKSIZE;
    printf("ending up at posn %ld, block %ld, offset %ld\n", posn, block, off);
  }
}

/* Mark block n as being used */
void block_inuse(int n)
{
  int bitmap[8] = {254, 253, 251, 247, 239, 223, 191, 127};
  int offset, bitmask;

  if (n >= disksize) {
    printf("Cannot mark block %d >= disk size %d\n", n, disksize); exit(1);
  }
  offset = n / 8;
  bitmask = bitmap[n % 8];
  freemap[offset] &= bitmask;
}

/* Mark i-node n as being in-use */
void inode_inuse(int n)
{
  int bitmap[8] = {1, 2, 4, 8, 16, 32, 64, 128};
  int offset, bitmask;

  if (n >= icount) {
    printf("Cannot mark inode %d >= icount %d\n", n, icount); exit(1);
  }
  inodelist[n].flags |= I_ALLOCATED;
  offset = (n - ROOTDIR_INUM) / 8;
  bitmask = bitmap[(n - ROOTDIR_INUM) % 8];
  inodemap[offset] |= bitmask;
}

/* Obtain an i-node. Returns the i-number. */
int alloc_inode(void)
{
  int i;
  for (i = 42; i < icount; i++) {
    if (inodelist[i].flags == 0) {
      inode_inuse(i);
      return (i);
    }
  }
  printf("Cannot allocate a new i-node\n"); exit(1);
}

/*
 * Allocate N contiguous blocks. Returns the first block number,
 * or dies if this is impossible.
 */
uint16_t alloc_blocks(int n)
{
  uint16_t i, firstblock = nextfreeblock;

  if (nextfreeblock + n >= disksize) {
    printf("Unable to allocate %d more blocks\n", n); exit(1);
  }
  for (i = nextfreeblock; i < n + nextfreeblock; i++)
    block_inuse(i);
  nextfreeblock += n;
  return (firstblock);
}

/* Allocate N contiguous blocks, and write a single indirect block
 * with those block numbers in it. Return the indirect block number;
 * the contiguous blocks come directly after the indirect block.
 */
int alloc_indirect_blocks(int n)
{
  uint16_t blkptr[BLKSPERINDIRECT];	/* Blk pointers in the indirect blk */
  uint16_t indirect_blknum;		/* Number of the indirect block */
  uint16_t i,b;

  if ( n > BLKSPERINDIRECT) {
    printf("Unable to allocate more than %d blocks in indirect\n", n); exit(1);
  }

  /* Obtain N+1 blocks, the first will be the indirect block */
  indirect_blknum= alloc_blocks(n+1);
  if (debug)
    printf("Allocated %d blocks, indirect is block %d\n", n+1, indirect_blknum);

  /* Fill the indirect block with the block numbers */
  memset(blkptr, 0, BLKSIZE);
  for (i=0,b=indirect_blknum+1; i< n; i++, b++)
    blkptr[i]= b;

  /* Write the indirect bock out to the image */
  fseek(diskfh, indirect_blknum * BLKSIZE, SEEK_SET);
  fwrite(blkptr, BLKSIZE, 1, diskfh);
  if (debug)
    printf("Wrote indirect block at %d\n", indirect_blknum);

  /* Return the indirect block number */
  return(indirect_blknum);
}

/* Add a filename and i-number to the given directory */
void add_direntry(struct directory *d, uint16_t inum, char *name)
{
  if (d == NULL) {
    printf("I need a struct directory * please\n"); exit(1);
  }
#if 0
  /* Removed for /dev/creation */
  if (inum < ROOTDIR_INUM) {
    printf("Illegal inum %d in add_direntry\n", inum); exit(1);
  }
#endif
  if (name && strlen(name) > 8) {
    printf("Name %s too long\n", name); exit(1);
  }

  /* Find an entry in the directory which is empty */
  if (d->nextfree >= d->numentries) {
    printf("Unable to add directory entry\n"); exit(1);
  }
  d->entry[d->nextfree].inode = inum;
  strncpy(d->entry[d->nextfree].name, name, 8);
  d->nextfree++;
}

/* Search for an return a struct fileperm pointer, given a directory
 * and filename. If no match is found, NULL is returned.
 */
struct fileperm *search_fileperm(char *dir, char *file)
{
  int i;
  char name[BLKSIZE];

  snprintf(name, BLKSIZE, "%s/%s", dir, file);
  if (debug) printf("Searching permlist for %s\n", name);

  for (i=0; permlist[i].name; i++) {
    if (!strcmp(permlist[i].name, name)) {
      if (debug) printf("Found permlist entry for %s\n", name);
      return(&permlist[i]);
    }
  }
  return(NULL);
}

/*
 * Create a directory with the given name. Allocate an i-node for it, and a
 * set of blocks. Attach it to /. Return the struct allocated. If name is
 * "/", we are making the root directory itself.
 */
struct directory *create_dir(char *name, int numblocks)
{
  struct directory *d;
  uint16_t inum, blk;
  int i;

  if (numblocks > 8) {
    printf("Can't allocate >8 blocks per directory\n"); exit(1);
  }

  if (name && strlen(name) > 8) {
    printf("Name %s too long\n", name); exit(1);
  }

  d = (struct directory *)calloc(1, sizeof(struct directory));
  d->numentries= numblocks * DIRENTPERBLOCK;
  d->nextfree= 0;
  d->entry= (struct v1dirent *)calloc(d->numentries, sizeof(struct v1dirent));

  /* Allocate an i-node and some blocks for the directory */
  if (strcmp(name, "/")) inum = alloc_inode();
  else inum = ROOTDIR_INUM;
  d->block = alloc_blocks(numblocks);
  d->inum= inum;
  if (debug) printf("In create_dir, got back inum %d blk %d, %d config blks\n",
			inum, d->block, numblocks);

  /* Mark the i-node as a directory */
  inodelist[inum].flags |= I_DIR | I_UREAD | I_UWRITE | I_OREAD;
  inodelist[inum].nlinks= 2;	/* Size is determined at write-time */
  for (i=0, blk=d->block; i < numblocks; i++, blk++)
    inodelist[inum].block[i]= blk;

  /* If the directory is not /, attach it to rootdir */
  if (strcmp(name, "/")) {
    add_direntry(rootdir, inum, name);
    /* Update rootdir's nlinks in its inode */
    inodelist[ROOTDIR_INUM].nlinks++;
  }

  /* and add entries for . and .. */
  add_direntry(d, ROOTDIR_INUM, "..");
  add_direntry(d, inum, ".");

  if (debug)
    printf("Created dir for %s, inum %d\n", name, inum);
  return (d);
}

/* Write the directory out to image */
void write_dir(struct directory *d, char *name)
{
  if (d == NULL) {
    printf("I need a struct directory * please\n"); exit(1);
  }

  if (debug)
    printf("Writing dir %s, size %d, %d blks from block %d (offset 0x%x) on\n",
			name, d->numentries * sizeof(struct v1dirent),
			d->numentries / DIRENTPERBLOCK, d->block,
			d->block * BLKSIZE);

  /* Directory size is the # of bytes of the in-use directory entries */
  inodelist[d->inum].size= d->nextfree * sizeof(struct v1dirent);
  fseek(diskfh, d->block * BLKSIZE, SEEK_SET);
  fwrite(d->entry, d->numentries * sizeof(struct v1dirent), 1, diskfh);
}

/* Create a file in a given directory. Returns the first block number. */
/* Does not actually copy the file's bits onto the image. */
/* At present we cannot deal with very large files, i.e. > 1 indirect block */
int create_file(struct directory *d, char *name, int size, struct fileperm *p)
{
  uint16_t inum;
  uint16_t firstblock;
  uint32_t epoch=0;
  int i, blk, numblocks;

  if (d == NULL) {
    printf("I need a struct directory * please\n"); exit(1);
  }
  if (name == NULL) {
    printf("I need a filename please\n"); exit(1);
  }
  if (size > BLKSPERINDIRECT * BLKSIZE) {
    printf("File %s is a very large file, skipping", name); return (0);
  }

  /* Allocate an i-node and some blocks for the directory */
  inum = alloc_inode();
  numblocks = (size + BLKSIZE - 1) / BLKSIZE;

  /* Get either that many blocks, or an indirect block if a large file */
  /* and put the list of blocks in the i-node */
  if (numblocks > NUMDIRECTBLKS) {
    firstblock = alloc_indirect_blocks(numblocks);
    inodelist[inum].flags |= I_LARGEFILE;
    inodelist[inum].block[0] = firstblock;
    firstblock++;		/* So that we return the 1st data block */
  } else {
    firstblock = alloc_blocks(numblocks);
    for (i = 0, blk = firstblock; i < numblocks; i++, blk++)
      inodelist[inum].block[i] = blk;
  }

  /* Make the i-node reflect the file */
  inodelist[inum].nlinks = 1;
  inodelist[inum].size = size;

  /* If we have existing permissions, use them. Otherwise, just */
  /* make some default permissions and set uid to 0 */
  if (p) {
    inodelist[inum].uid = p->uid;
    memcpy(inodelist[inum].mtime, &p->mtime, sizeof(uint32_t));
    memcpy(inodelist[inum].ctime, &p->mtime, sizeof(uint32_t));
    inodelist[inum].flags |= I_MODFILE | p->flags;
  } else {
    inodelist[inum].uid = 0;
    memcpy(inodelist[inum].mtime, &epoch, sizeof(uint32_t));
    memcpy(inodelist[inum].ctime, &epoch, sizeof(uint32_t));
    inodelist[inum].flags |= I_MODFILE | I_EXEC | I_UREAD | I_UWRITE |
							 I_OREAD | I_OWRITE;
  }

  /* Add the i-node and filename to the directory */
  add_direntry(d, inum, name);
  if (debug)
    printf("Created file %s size %d inum %d\n", name, size, inum);
  return (firstblock);
}

/*
 * Open up the file given by the fullname, and copy
 * size bytes into the image starting at firstblock.
 */
void copy_file(char *fullname, int firstblock, int size)
{
  FILE *zin;
  int cnt;

  if (debug)
    printf("Copying %s size %d to block %d (offset 0x%x) on\n",
			fullname, size, firstblock, firstblock * BLKSIZE);
  if ((zin = fopen(fullname, "r")) == NULL) {
    printf("Unable to read %s to copy onto image\n", fullname); exit(1);
  }

  fseek(diskfh, firstblock * BLKSIZE, SEEK_SET);
  while ((cnt = fread(buf, 1, BLKSIZE, zin)) > 0)
    fwrite(buf, 1, cnt, diskfh);

  fclose(zin); fflush(diskfh);
}

/*
 * Make a directory /dir on the image. Add all the
 * files in basedir/dir into /dir on the image.
 */
void add_files(char *basedir, char *dir)
{
  struct directory *d;
  DIR *D;
  struct dirent *dp;
  struct stat sb;
  char fullname[BLKSIZE];
  uint16_t firstblock;
  struct fileperm *perms;

  /* Get the full external directory name */
  snprintf(fullname, BLKSIZE, "%s/%s", basedir, dir);

  /* Open the external directory */
  D = opendir(fullname);
  if (D == NULL) {
    printf("Cannot opendir %s\n", fullname); exit(1);
  }

  /* Create the image directory */
  d = create_dir(dir, DIRBLOCKS);

  /* Walk the directory */
  while ((dp = readdir(D)) != NULL) {
    if (!strcmp(dp->d_name, ".")) continue;		/* Skip . and .. */
    if (!strcmp(dp->d_name, "..")) continue;

    /* Stat the entry found */
    snprintf(fullname, BLKSIZE, "%s/%s/%s", basedir, dir, dp->d_name);
    if (stat(fullname, &sb) < 0) {
      printf("Cannot stat %s\n", fullname); continue;
    }

    /* Skip if it is not a regular file */
    if (!S_ISREG(sb.st_mode)) continue;

    /* Skip if it's too big */
    if (sb.st_size > BLKSPERINDIRECT * BLKSIZE) {
      printf("Skipping very large file %s for now\n", fullname); continue;
    }

    /* Skip if the name is too long */
    if (strlen(dp->d_name) > 8) {
      printf("Skipping long filename %s for now\n", fullname); continue;
    }

    /* Search for any known permissions for this file */
    perms= search_fileperm(dir, dp->d_name);

    /* Create the file's i-node */
    /* and copy the file into the image */
    firstblock = create_file(d, dp->d_name, sb.st_size, perms);
    copy_file(fullname, firstblock, sb.st_size);
  }
  closedir(D);

  /* And write the directory out */
  write_dir(d, dir);
}

/* Following the cold UNIX rf output, make /dev. In /dev,
 * make devices with specific i-nums, which I guess act like
 * early major/minor device numbers.
 */
void add_devdir(void)
{
  struct dev {
    char *name;
    uint16_t inum;
  } devlist[] = {
	{ "tty", 1 },
	{ "ppt", 2 },
	{ "mem", 3 },
	{ "rf0", 4 },
	{ "rk0", 5 },
	{ "tap0", 6 },
	{ "tap1", 7 },
	{ "tap2", 8 },
	{ "tap3", 9 },
	{ "tap4", 10 },
	{ "tap5", 11 },
	{ "tap6", 12 },
	{ "tap7", 13 },
	{ "tty0", 14 },
	{ "tty1", 15 },
	{ "tty2", 16 },
	{ "tty3", 17 },
	{ "tty4", 18 },
	{ "tty5", 19 },
	{ "tty6", 20 },
	{ "tty7", 21 },
	{ "lpr",  22 },
	{ "tty8", 1 },
	{ NULL, 0 },
  };

  struct directory *d;
  int i;

  d = create_dir("dev", 1);	/* cold UNIX only uses 1 block */

  for (i=0; devlist[i].name !=NULL; i++) {
    add_direntry(d, devlist[i].inum, devlist[i].name);
  }

  /* And write the directory out */
  write_dir(d, "dev");
}

/*
 * Given a top-level directory, find all of the subdirectories,
 * and add them and their files into the image.
 */
void process_topdir(char *topdir)
{
  DIR *D;
  struct dirent *dp;
  struct stat sb;
  char fullname[BLKSIZE];

  /* Open the top directory */
  D = opendir(topdir);
  if (D == NULL) {
    printf("Cannot opendir %s\n", topdir); exit(1);
  }

  /* Walk the directory */
  while ((dp = readdir(D)) != NULL) {
    if (!strcmp(dp->d_name, ".")) continue;		/* Skip . and .. */
    if (!strcmp(dp->d_name, "..")) continue;

    /* Stat the entry found */
    snprintf(fullname, BLKSIZE, "%s/%s", topdir, dp->d_name);
    if (stat(fullname, &sb) < 0) {
      printf("Cannot stat %s\n", fullname); continue;
    }

    /* Skip if it is not a directory */
    if (!S_ISDIR(sb.st_mode)) continue;

    /* Skip if the name is too long */
    if (strlen(dp->d_name) > 8) {
      printf("Skipping long dirname %s for now\n", fullname); continue;
    }

    /* Now process that directory and its files */
    add_files(topdir, dp->d_name);
  }
  closedir(D);
}

/* Open and parse the file to obtain a list of V1 file owner/perm/dates.
 * The file has a very specific format. Lines before '======' are ignored.
 * After that, each line represents one file, and has pcre line format:
 *
 *
 *   ^[-xu][-r][-w][-r][-w] +\d+.*\/\S+ *\d+
 *
 * First 5 chars represent x=executable, u=setuid, r=read, w=write.
 * Spaces separate the permissions and the decimal userid.
 * Then we look for a / which begins the full pathname.
 * Spaces seperate the full pathname and the timestamp which is in 1/60th
 * of a second since the beginning of the year.
 */
void read_permsfile(char *file)
{
  FILE *zin;
  char linebuf[BLKSIZE];
  char *cptr, *sptr;
  int posn=0;

  if (file==NULL) return;
  if ((zin= fopen(file, "r"))==NULL) return;

  /* Find line beginning with = */
  while (1) {
    if (fgets(linebuf, BLKSIZE-1, zin)==NULL) {
      fclose(zin); return;
    }
    if (linebuf[0]== '=') break;
  }

  /* Now read and parse each line */
  while (1) {
    if ((posn >= PERMLISTSIZE) || (fgets(linebuf, BLKSIZE-1, zin)==NULL)) {
      fclose(zin); return;
    }

    /* Skip if junk permissions */
    if (linebuf[0] != '-' && linebuf[0] != 'x' && linebuf[0] != 'u')
      continue;

    /* Get the permissions */
    permlist[posn].flags=0;
    if (linebuf[0] == 'x') permlist[posn].flags |= I_EXEC;
    if (linebuf[0] == 'u') permlist[posn].flags |= I_SETUID | I_EXEC;
    if (linebuf[1] == 'r') permlist[posn].flags |= I_UREAD;
    if (linebuf[2] == 'w') permlist[posn].flags |= I_UWRITE;
    if (linebuf[3] == 'r') permlist[posn].flags |= I_OREAD;
    if (linebuf[4] == 'w') permlist[posn].flags |= I_OWRITE;

    /* Get the userid */
    permlist[posn].uid= strtol(&linebuf[5], NULL , 10);
    
    /* Search for the full pathname, skip line if not found */
    if ((cptr= strchr(linebuf, '/'))==NULL) continue;

    /* Find the space after the full name, skip if not found */
    /* Null-terminate the filename */
    if ((sptr= strchr(cptr, ' '))==NULL) continue;
    *(sptr++)= '\0';

    /* Copy the pathname minus the leading slash */
    /* However, if it starts with "/usr/", skip "/usr/" */
    if (!strncmp(cptr, "/usr/", 5))
      cptr+= 5;
    else
      cptr++;
    permlist[posn].name= strdup(cptr);

    /* Finally get the timestamp */
    permlist[posn].mtime= strtol(sptr, NULL , 10);

    if (debug) {
      printf("0%2o %2d %10d %s\n", permlist[posn].flags, permlist[posn].uid,
			permlist[posn].mtime, permlist[posn].name);
    }
    posn++;
  }
}


void usage(void)
{
  printf("Usage: mkfs [-d] [-p permsfile] topdir image rk/rf\n");
  printf("\ttopdir is an existing dir which holds bin/, etc/, tmp/ ...\n");
  printf("\timage will be the image created\n");
  printf("\tlast argument is either 'rk' or 'rf'\n");
  printf("\t-d enables debugging output\n");
  printf("\t-p reads the named file to obtain a list of V1 file permissions\n");
  exit(1);
}

int main(int argc, char *argv[])
{
  int makedevflag=0;	/* Do we make the /dev directory? */
  int i,ch;
  int numiblocks;
  int fs_size;		/* Equal to disksize minus any swap */

  /* Get any optional arguments */
  while ((ch = getopt(argc, argv, "dp:")) != -1) {
    switch (ch) {
	case 'd': debug=1; break;
	case 'p': read_permsfile(optarg); break;
    }
  }

  argc -= optind;
  argv += optind;

  /* Get the image name and the type of disk */
  if (argc != 3) usage();
  if (strcmp(argv[2], "rk") && strcmp(argv[2], "rf")) usage();

  /* Set the disk size */
  if (argv[2][1] == 'k') {	/* RK device */
    /* XXX: I can't seem to go past 4864 to 4872 blocks, and I haven't */
    /* worked out why yet. */
    disksize = RK_SIZE;
    fs_size = RK_SIZE;
  } else {
    disksize = RF_SIZE;
    fs_size = RF_NOSAWPSIZE;
    makedevflag=1;
  }

  /* Create the image */
  diskfh = fopen(argv[1], "w+");
  if (diskfh == NULL) {
    printf("Unable to create image %s\n", argv[1]); exit(1);
  }

  /* Make the image full-sized */
  for (i = 0; i < fs_size; i++)
    fwrite(buf, BLKSIZE, 1, diskfh);

  /* Create the free-map: fortunately RK_SIZE and RF_SIZE are divisible by 8 */
  /* Make all the blocks free to start with */
  freemap = (uint8_t *)malloc(disksize / 8);
  for (i = 0; i < disksize / 8; i++)
    freemap[i] = 0xff;

  /* Mark blocks 0 and 1 as in-use */
  block_inuse(0); block_inuse(1);

  /* Choose disksize/INODE_RATIO as the number of i-nodes to allocate */
  /* if we haven't already given icount a value. */
  /* Make sure that it is divisible by 8 */
  if (icount==0)
    icount = 8 * (disksize / INODE_RATIO / 8);

  /* Create the inode bitmap and inode list */
  inodemap = (uint8_t *)calloc(1, (icount - ROOTDIR_INUM) / 8);
  inodelist = (struct v1inode *)calloc(icount, sizeof(struct v1inode));

  /* Mark special i-nodes 0 to ROOTDIR_INUM-1 as allocated */
  /* We follow the output of cold UNIX here. */
  for (i = 0; i < ROOTDIR_INUM; i++) {
    inodelist[i].flags |= I_ALLOCATED|I_UREAD|I_UWRITE|I_OREAD|I_OWRITE;
    inodelist[i].nlinks= 1;
    inodelist[i].uid= 1;
    inodelist[i].mtime[3]= 0x38;
  }

  /* INODEPERBLOCK i-nodes fit into a block, so work out how many blocks the */
  /* inodelist occupies. Round up to ensure a partial block => full block */
  /* Mark the blocks from 2 up as being in-use */
  numiblocks = (icount + INODEPERBLOCK -1) / INODEPERBLOCK;
  if (debug)
    printf("%d i-nodes, %d i-node blocks\n", icount, numiblocks);
  nextfreeblock = 2 + numiblocks;
  for (i = 2; i < nextfreeblock; i++)
    block_inuse(i);


  /* Mark the root directory i-node (ROOTDIR_INUM) as in-use */
  /* Create the root directory */
  inode_inuse(ROOTDIR_INUM);
  rootdir = create_dir("/", 1);	/* cold UNIX only uses 1 block */

  /* Now create the /dev directory by hand */
  if (makedevflag)
    add_devdir();

  /* Walk the top directory argument, dealing with the subdirs */
  process_topdir(argv[0]);

  /* Write out the root directory */
  write_dir(rootdir, "/");

  /* Write out the superblock and i-nodes */
  write_superblock();

  fclose(diskfh);
  exit(0);
}
