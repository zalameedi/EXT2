
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <time.h>
#include <libgen.h>
#include <sys/stat.h>
#include <ext2fs/ext2_fs.h>

// define shorter TYPES, save typing efforts
typedef struct ext2_group_desc  GD;
typedef struct ext2_super_block SUPER;
typedef struct ext2_inode       INODE;
typedef struct ext2_dir_entry_2 DIR;    // need this for new version of e2fs

#define BLKSIZE 1024
GD    *gp;
SUPER *sp;
INODE *ip;
DIR   *dp; 

/********** globals *************/
int fd;
int imap, bmap, indoes_start;  // IMAP and BMAP block number, inodes start block
int ninodes, nblocks;

int get_block(int fd, int blk, char buf[ ])
{
  lseek(fd, (long)blk*BLKSIZE, 0);
  read(fd, buf, BLKSIZE);
}

int put_block(int fd, int blk, char buf[ ])
{
  lseek(fd, (long)blk*BLKSIZE, 0);
  write(fd, buf, BLKSIZE);
}

int tst_bit(char *buf, int bit)
{
  int i, j;
  i = bit/8; j=bit%8;
  if (buf[i] & (1 << j))
     return 1;
  return 0;
}

int set_bit(char *buf, int bit)
{
  int i, j;
  i = bit/8; j=bit%8;
  buf[i] |= (1 << j);
}

int clr_bit(char *buf, int bit)
{
  int i, j;
  i = bit/8; j=bit%8;
  buf[i] &= ~(1 << j);
}

int ialloc(int dev)
{
  int  i;
  char buf[BLKSIZE];

  // read inode_bitmap block
  get_block(dev, imap, buf);

  for (i=0; i < ninodes; i++){
    if (tst_bit(buf, i)==0){
       set_bit(buf,i);
       put_block(dev, imap, buf);
       return i+1;
    }
  }
  return 0;
}

char *disk = "vdisk";

int main(int argc, char *argv[ ])
{
  int i, ino;
  char buf[BLKSIZE];

  if (argc > 1)
    disk = argv[1];

  fd = open(disk, O_RDWR);
  if (fd < 0){
    printf("open %s failed\n", disk);
    exit(1);
  }

  // read SUPER block
  get_block(fd, 1, buf);
  sp = (SUPER *)buf;

  ninodes = sp->s_inodes_count;
  nblocks = sp->s_blocks_count;

  printf("ninodes = %d nblocks = %d\n", ninodes, nblocks);

  // read Group Descriptor 0
  get_block(fd, 2, buf);
  gp = (GD *)buf;

  imap = gp->bg_inode_bitmap;
  printf("imap = %d\n", imap);

  getchar();

  for (i=0; i < 5; i++){  
    ino = ialloc(fd);
    printf("allocated ino = %d\n", ino);
    int x = balloc(fd);
    printf("%d\n", x);
  }
}


int balloc(int dev)
{
  int  i;
  char buf[BLKSIZE];

  // read inode_bitmap block
  get_block(dev, bmap, buf);

  for (i=0; i < nblocks; i++){
    if (tst_bit(buf, i)==0)
    {
       set_bit(buf,i);
       put_block(dev, bmap, buf);
        sp->s_free_blocks_count -= 1;
        gp->bg_free_blocks_count -= 1;
       return i+1;
    }
  }
  return 0;
}