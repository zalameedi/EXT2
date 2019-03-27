/************* cd_ls_pwd.c file **************/

/**** globals defined in main.c file ****/
extern MINODE minode[NMINODE];
extern MINODE *root;
extern PROC   proc[NPROC], *running;
extern char   gpath[256];
extern char   *name[64];
extern int    n;
extern int fd, dev;
extern int nblocks, ninodes, bmap, imap, inode_start;
extern char line[256], cmd[32], pathname[256];
#define OWNER  000700
#define GROUP  000070
#define OTHER  000007

change_dir(char *pathname)
{
  if (pathname == 0)
  {
	iput(running->cwd);
	running->cwd = root;
  }
  else
  {
	int ino;
	MINODE *mip;
	ino = getino(pathname);
	mip = iget(dev, ino);
	if (S_ISDIR(mip->INODE.i_mode))
	{
		iput(running->cwd);
		running->cwd = mip;
		printf("cd successful\n");
	}
	else
	{
		printf("not a directory\n");
	}
  }
  printf("chage_dir(): to be constructed\n");


}


int ls_file(MINODE *ip, char *name)
{
  int k;
  u16 mode, mask;
  char mydate[32], *s, *cp, ss[32];

  mode = ip->INODE.i_mode;
  if (S_ISDIR(mode))
      putchar('d');
  else if (S_ISLNK(mode))
      putchar('l');
  else
      putchar('-');

   mask = 000400;
   for (k=0; k<3; k++){
      if (mode & mask)
         putchar('r');
      else
         putchar('-');
      mask = mask >> 1;

     if (mode & mask)
        putchar('w');
     else
        putchar('-');
        mask = mask >> 1;

     if (mode & mask)
        putchar('x');
     else
        putchar('-');
        mask = mask >> 1;

     }
     printf("%4d", ip->INODE.i_links_count);
     printf("%4d", ip->INODE.i_uid);
     printf("%4d", ip->INODE.i_gid);
     printf("  ");

     s = mydate;
     s = (char *)ctime(&ip->INODE.i_mtime);
     s = s + 4;
     strncpy(ss, s, 12);
     ss[12] = 0;

     printf("%s", ss);
     printf("%8ld",   ip->INODE.i_size);

     printf("    %s", name);

     if (S_ISLNK(mode))
        printf(" -> %s", (char *)ip->INODE.i_block);
     printf("\n");
}


int ls_dir(MINODE *mip)
{
  int i;
  char sbuf[BLKSIZE], temp[256];
  DIR *dp;
  char *cp;
  MINODE *dip;

  for (i=0; i<12; i++){ /* search direct blocks only */
     printf("i_block[%d] = %d\n", i, mip->INODE.i_block[i]);
     if (mip->INODE.i_block[i] == 0) 
         return 0;

     get_block(mip->dev, mip->INODE.i_block[i], sbuf);
     dp = (DIR *)sbuf;
     cp = sbuf;
     while (cp < sbuf + BLKSIZE){
        strncpy(temp, dp->name, dp->name_len);
        temp[dp->name_len] = 0;
        dip = iget(dev, dp->inode); 
        ls_file(dip, temp);
        iput(dip);

        cp += dp->rec_len;
        dp = (DIR *)cp;
     }
  }
}

int list_file()
{
  MINODE *mip;
  u16 mode;
  int dev, ino;

  if (pathname[0] == 0)
    ls_dir(running->cwd);
  else{
    dev = root->dev;
    ino = getino(dev, pathname);
    if (ino==0){
      printf("no such file %s\n", pathname);
      return -1;
    }
    mip = iget(dev, ino);
    mode = mip->INODE.i_mode;
    if (!S_ISDIR(mode))
      ls_file(mip, (char *)basename(pathname));
    else
      ls_dir(mip);
    iput(mip);
  }
}


int pwd(MINODE *wd)
{
	if (wd == root) 
		printf("/\n");
	else
         rpwd(wd); 
}

rpwd(MINODE *wd)
   {
     if (wd==root) return;
     int parentino = wd->INODE.i_block[0];
     pip = iget(dev, parentino);
     int blk = pip->INODE.i_block[0];
     char buf[BLKSIZE], my_name[256];
     get_block(dev, blk, buf);
     DIR *dp = (DIR *)buf;
     strncpy(my_name, dp->name, dp->name_len);
     rpwd(pip);  // recursive call rpwd() with pip

     printf("/%s", my_name);
   }


