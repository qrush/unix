/*
 * Program to create a 1st Edition UNIX filesystem.
 * (c) 2008 Warren Toomey, GPL3.
 *
 * Other contributors, add your name above.
 *
 * TODO: work out how to add /dev and device files
 *	 ensure that swap is not used on RF disk images.
 *	 deal with errors instead of exiting :-)
 *
 * $Revision: 1.12 $
 * $Date: 2008/05/02 13:37:55 $
 */

int debug=1;

#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>

#ifdef linux
#include <stdint.h>
#endif

#define RF_SIZE		1024	/* Number of blocks on RF11 */
#define RK_SIZE		4872	/* Number of blocks on RK03 */
#define BLKSIZE		 512	/* 512 bytes per block */
#define INODE_RATIO	   4	/* Disksize to i-node ratio */
#define ROOTDIR_INUM	  41	/* Special i-number for / */

struct v1inode {		/* Format of 1st edition i-node */
  uint16_t flags;
  uint8_t nlinks;
  uint8_t uid;
  uint16_t size;
  uint16_t block[8];
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
#define NUMDIRENTRIES	DIRBLOCKS*DIRENTPERBLOCK

struct v1dirent {		/* Format of 1st edition dir entry */
  uint16_t inode;
  char name[8];
};

struct directory {		/* Internal structure for each dir */
  uint16_t block;		/* Starting block */
  struct v1dirent entry[NUMDIRENTRIES];
};

unsigned char buf[BLKSIZE];	/* A block buffer */
uint16_t disksize;		/* Number of blocks on disk */
uint16_t icount;		/* Number of i-nodes created */
uint16_t nextfreeblock;		/* The next free block available */
uint8_t *freemap;		/* In-memory free-map */
uint8_t *inodemap;		/* In-memory inode bitmap */
struct v1inode *inodelist;	/* In-memory i-node list */

struct directory *rootdir;	/* The root directory */

FILE *diskfh;			/* Disk filehandle */

/* Write the superblock out to the image */
void write_superblock(void)
{
  if (debug) printf("Writing freemap and inodemap from block 0\n");
  fseek(diskfh, 0, SEEK_SET);
  fwrite(&disksize, sizeof(disksize), 1, diskfh);
  fwrite(freemap, sizeof(uint8_t), (disksize / 8), diskfh);
  fwrite(&icount, sizeof(icount), 1, diskfh);
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
  fwrite(inodelist, sizeof(struct v1inode), icount, diskfh);
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
int alloc_blocks(int n)
{
  int i, firstblock = nextfreeblock;

  if (nextfreeblock + n >= disksize) {
    printf("Unable to allocate %d more blocks\n", n); exit(1);
  }
  for (i = nextfreeblock; i < n + nextfreeblock; i++)
    block_inuse(i);
  nextfreeblock += n;
  return (firstblock);
}

/* Add a filename and i-number to the given directory */
void add_direntry(struct directory *d, uint16_t inum, char *name)
{
  int i;

  if (d == NULL) {
    printf("I need a struct directory * please\n"); exit(1);
  }
  if (inum < ROOTDIR_INUM) {
    printf("Illegal inum %d in add_direntry\n", inum); exit(1);
  }
  if (name && strlen(name) > 8) {
    printf("Name %s too long\n", name); exit(1);
  }

  /* Find an entry in the directory which is empty */
  for (i = 0; i < NUMDIRENTRIES; i++) {
    if (d->entry[i].inode == 0) {
      d->entry[i].inode = inum;
      strncpy(d->entry[i].name, name, 8);
      return;
    }
  }

  printf("Unable to add directory entry\n"); exit(1);
}

/*
 * Create a directory with the given name. Allocate an i-node for it, and a
 * set of blocks. Attach it to /. Return the struct allocated. If name is
 * "/", we are making the root directory itself.
 */
struct directory *create_dir(char *name)
{
  struct directory *d;
  uint16_t inum;

  if (name && strlen(name) > 8) {
    printf("Name %s too long\n", name); exit(1);
  }

  d = (struct directory *)calloc(1, sizeof(struct directory));

  /* Allocate an i-node and some blocks for the directory */
  if (strcmp(name, "/")) inum = alloc_inode();
  else inum = ROOTDIR_INUM;
  d->block = alloc_blocks(DIRBLOCKS);

  /* Mark the i-node as a directory */
  inodelist[inum].flags |= I_DIR | I_UREAD | I_UWRITE | I_OREAD | I_OWRITE;

  /* If the directory is not /, attach it to rootdir */
  if (strcmp(name, "/")) add_direntry(rootdir, inum, name);

  /* and add entries for . and .. */
  add_direntry(d, inum, ".");
  add_direntry(d, ROOTDIR_INUM, "..");

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
    printf("Writing dir %s of %d blocks from block %d (offset 0x%x) on\n",
			name, DIRBLOCKS, d->block, d->block * BLKSIZE);

  fseek(diskfh, d->block * BLKSIZE, SEEK_SET);
  fwrite(&(d->entry), sizeof(d->entry), 1, diskfh);
}

/* Create a file in a given directory. Returns the first block number. */
/* Does not actually copy the file's bits onto the image. */
/* At present we cannot deal with large files, i.e. > 8 blocks */
int create_file(struct directory *d, char *name, int size)
{
  uint16_t inum;
  uint16_t firstblock;
  int i, blk, numblocks;

  if (d == NULL) {
    printf("I need a struct directory * please\n"); exit(1);
  }
  if (name == NULL) {
    printf("I need a filename please\n"); exit(1);
  }
  if (size > 8 * BLKSIZE) {
    printf("File %s is a large file, skipping", name); return (0);
  }

  /* Allocate an i-node and some blocks for the directory */
  inum = alloc_inode();
  numblocks = (size + BLKSIZE - 1) / BLKSIZE;
  firstblock = alloc_blocks(numblocks);

  /* Make the i-node reflect the file */
  inodelist[inum].flags |= I_EXEC | I_UREAD | I_UWRITE | I_OREAD | I_OWRITE;
  inodelist[inum].nlinks = 1;
  inodelist[inum].uid = 0;
  inodelist[inum].size = size;
  for (i = 0, blk = firstblock; i < numblocks; i++, blk++)
    inodelist[inum].block[i] = blk;

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

  /* Get the full external directory name */
  snprintf(fullname, BLKSIZE, "%s/%s", basedir, dir);

  /* Open the external directory */
  D = opendir(fullname);
  if (D == NULL) {
    printf("Cannot opendir %s\n", fullname); exit(1);
  }

  /* Create the image directory */
  d = create_dir(dir);

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
    if (sb.st_size > 8 * BLKSIZE) {
      printf("Skipping large file %s for now\n", fullname); continue;
    }

    /* Skip if the name is too long */
    if (strlen(dp->d_name) > 8) {
      printf("Skipping long filename %s for now\n", fullname); continue;
    }

    /* Create the file's i-node */
    /* and copy the file into the image */
    firstblock = create_file(d, dp->d_name, sb.st_size);
    copy_file(fullname, firstblock, sb.st_size);
  }
  closedir(D);

  /* And write the directory out */
  write_dir(d, dir);
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


void usage(void)
{
  printf("Usage: mkfs topdir image rk/rf\n");
  printf("\ttopdir is an existing dir which holds bin/, etc/, tmp/ ...\n");
  printf("\timage will be the image created\n");
  printf("\tlast argument is either 'rk' or 'rf'\n");
  exit(1);
}

int main(int argc, char *argv[])
{
  int i;
  int numiblocks;

  /* Get the image name and the type of disk */
  if (argc != 4) usage();
  if (strcmp(argv[3], "rk") && strcmp(argv[3], "rf")) usage();

  /* Set the disk size */
  if (argv[3][1] == 'k') disksize = RK_SIZE;
  else disksize = RF_SIZE;

  /* Create the image */
  diskfh = fopen(argv[2], "w+");
  if (diskfh == NULL) {
    printf("Unable to create image %s\n", argv[2]); exit(1);
  }

  /* Make the image full-sized */
  for (i = 0; i < disksize; i++)
    fwrite(buf, BLKSIZE, 1, diskfh);

  /* Create the free-map: fortunately RK_SIZE and RF_SIZE are divisible by 8 */
  /* Make all the blocks free to start with */
  freemap = (char *)malloc(disksize / 8);
  for (i = 0; i < disksize / 8; i++)
    freemap[i] = 0xff;

  /* Mark blocks 0 and 1 as in-use */
  block_inuse(0); block_inuse(1);

  /* Choose disksize/INODE_RATIO as the number of i-nodes to allocate */
  /* Make sure that it is divisible by 8 */
  icount = 8 * (disksize / INODE_RATIO / 8);

  /* Create the inode bitmap and inode list */
  inodemap = (char *)calloc(1, (icount - ROOTDIR_INUM) / 8);
  inodelist = (struct v1inode *)calloc(icount, sizeof(struct v1inode));

  /* Mark special i-nodes 0 to ROOTDIR_INUM-1 as allocated */
  for (i = 0; i < ROOTDIR_INUM; i++)
    inodelist[i].flags |= I_ALLOCATED;

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
  rootdir = create_dir("/");

  /* Walk the top directory argument, dealing with the subdirs */
  process_topdir(argv[1]);

  /* Write out the superblock and i-nodes */
  write_superblock();

  /* Write out the root directory */
  write_dir(rootdir, "/");

  fclose(diskfh);
  exit(0);
}
