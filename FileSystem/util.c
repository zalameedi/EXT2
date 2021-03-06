/*********** util.c file ****************/

/**** globals defined in main.c file ****/
extern MINODE minode[NMINODE];
extern MINODE *root;
extern PROC   proc[NPROC], *running;
extern char   gpath[256];
extern char   *name[64];
extern int    n;
extern int    fd, dev;
extern int    nblocks, ninodes, bmap, imap, inode_start;
extern char   line[256], cmd[32], pathname[256];

int get_block(int dev, int blk, char *buf)
{
   lseek(dev, (long)blk*BLKSIZE, 0);
   read(dev, buf, BLKSIZE);
}   

int put_block(int dev, int blk, char *buf)
{
   lseek(dev, (long)blk*BLKSIZE, 0);
   write(dev, buf, BLKSIZE);
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

int tokenize(char *pathname)
{
  n = 0;
    name[n] = strtok(pathname,"/0");
    while (name[n] != NULL)
    {
        n++;
        name[n] = strtok(NULL,"/0");
    }
return n;
}

// return minode pointer to loaded INODE
MINODE *iget(int dev, int ino)
{
  int i;
  MINODE *mip;
  char buf[BLKSIZE];
  int blk, disp;
  INODE *ip;

  for (i=0; i<NMINODE; i++){
    mip = &minode[i];
    if (mip->dev == dev && mip->ino == ino){
       mip->refCount++;
       //printf("found [%d %d] as minode[%d] in core\n", dev, ino, i);
       return mip;
    }
  }
    
  for (i=0; i<NMINODE; i++){
    mip = &minode[i];
    if (mip->refCount == 0){
      //printf("allocating NEW minode[%d] for [%d %d]\n", i, dev, ino);
       mip->refCount = 1;
       mip->dev = dev;
       mip->ino = ino;

       // get INODE of ino to buf    
       blk  = (ino-1) / 8 + inode_start;
       disp = (ino-1) % 8;

       //printf("iget: ino=%d blk=%d disp=%d\n", ino, blk, disp);

       get_block(dev, blk, buf);
       ip = (INODE *)buf + disp;
       // copy INODE to mp->INODE
       mip->INODE = *ip;

       return mip;
    }
  }   
  printf("PANIC: no more free minodes\n");
  return 0;
}

iput(MINODE *mip)
{
 int i, block, offset;
 char buf[BLKSIZE];
 INODE *ip;

 if (mip==0) 
     return;

 mip->refCount--;
 
 if (mip->refCount > 0) return;
 if (!mip->dirty)       return;
 
 /* write back */
 //printf("iput: dev=%d ino=%d\n", mip->dev, mip->ino); 

 block =  ((mip->ino - 1) / 8) + inode_start;
 offset =  (mip->ino - 1) % 8;

 /* first get the block containing this inode */
 get_block(mip->dev, block, buf);

 ip = (INODE *)buf + offset;
 *ip = mip->INODE;

 put_block(mip->dev, block, buf);

} 

int search(MINODE *ip, char *name)
{
    int i;
    char *cp, temp[256], sbuf[BLKSIZE];
    DIR *dp;
    for(i=0; i<12; i++)
    {
        if(ip->INODE.i_block[i] == 0)
        {
            return 0;
        }
        get_block(dev, ip->INODE.i_block[i], sbuf);
        dp = (DIR *)sbuf;
        cp = sbuf;
        while(cp < sbuf + BLKSIZE)
        {
            strncpy(temp, dp->name, dp->name_len);
            temp[dp->name_len] = 0;    
            if(strcmp(name, temp)==0)
            {
                return dp->inode;            
            }

            cp += dp->rec_len;
            dp = (DIR *)cp;
        }
    }
    return 0;
}

int getino(int dev, char *pathname)
{
  int i, ino, blk, disp;
  char buf[BLKSIZE];
  INODE *ip;
  MINODE *mip;

  printf("getino: pathname=%s\n", pathname);
  if (strcmp(pathname, "/")==0)
      return 2;

  if (pathname[0]=='/')
    mip = iget(dev, 2);
  else
    mip = iget(running->cwd->dev, running->cwd->ino);

  strcpy(buf, pathname);
  tokenize(buf);

  for (i=0; i<n; i++){
      printf("===========================================\n");
      printf("getino: i=%d name[%d]=%s\n", i, i, name[i]);
 
      ino = search(mip, name[i]);

      if (ino==0){
         iput(mip);
         printf("name %s does not exist\n", name[i]);
         return 0;
      }
      iput(mip);
      mip = iget(dev, ino);
   }
   iput(mip);
   return ino;
}



int incFreeInodes(int dev)
{
  sp->s_free_inodes_count++;
  put_block(dev, 1, (char *)sp);
  gp->bg_free_inodes_count++;
  put_block(dev, 2, (char *)gp);
}

int decFreeInodes(int dev)
{
  sp->s_free_inodes_count--;
  put_block(dev, 1, (char *)sp);
  gp->bg_free_inodes_count--;
  put_block(dev, 2, (char *)gp);
}

int idalloc(int dev, int ino)
{
  int i;
  char buf[BLKSIZE];
  if (ino > ninodes){ // niodes global
    printf("inumber %d out of range\n", ino);
    return;
  }
  // get inode bitmap block
  get_block(dev, imap, buf);
  clr_bit(buf, ino-1);
  // write buf back
  put_block(dev, imap, buf);
  // update free inode count in SUPER and GD
  incFreeInodes(dev);
}

int bdalloc(int dev, int ino)
{
  int i;
  char buf[BLKSIZE];
  // get inode bitmap block
  get_block(dev, bmap, buf);
  clr_bit(buf, ino-1);
  // write buf back
  put_block(dev, bmap, buf);
}


int incFreeblocks(int dev)
{
  sp->s_free_blocks_count++;
  put_block(dev, 1, (char *)sp);
  gp->bg_free_blocks_count++;
  put_block(dev, 2, (char*)gp);
}

int decFreeblocks(int dev)
{
  sp->s_free_blocks_count--;
  put_block(dev, 1, (char *)sp);
  gp->bg_free_blocks_count--;
  put_block(dev, 2, (char *)gp);
}

int truncate(INODE *ip, int ino)
{
	for (int i = 0; i < 12; i++)
	{
		if (ip->i_block[i] != 0)
			bdalloc(dev, ip->i_block[i]);
		else
			break;
	}
	int blockt[256];
	get_block(dev, ip->i_block[12], blockt);
	for (int i = 0; i < 256; i++)
	{
		if (blockt[i] != 0)
			bdalloc(dev, blockt[i]);
		else
			break;
	}
	int blockt2[256];
	get_block(dev, ip->i_block[13], blockt);
	for (int i = 0; i < 256; i++)
	{
		get_block(dev, blockt[i], blockt2);
		for (int j = 0; j < 256; j++)
		{
			if (blockt2[j] != 0)
      {
				bdalloc(dev, blockt2[j]);
      }
			else
      {
	      idalloc(dev, ino);
        return;
      }
		}
	}
}

