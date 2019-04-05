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

char buff[BLKSIZE];

change_dir()
{
  char temp[256];
  MINODE *mip;


  if(pathname[0]=='/')
  {
    dev = root->dev;
  }
  
  if (pathname[0] == 0)
  {
	iput(running->cwd);
  	running->cwd = iget(root->dev, 2);
  	return;
  }
  else
  {
	int ino;
  	strcpy(temp, pathname);
	ino = getino(dev, temp);

  	printf("dev=%d ino=%d\n", dev, ino);

	mip = iget(dev, ino);
  	printf("mode=%4x  ", mip->INODE.i_mode);
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
     //printf("i_block[%d] = %d\n", i, mip->INODE.i_block[i]);
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

void print_stats(MINODE *ip)
{
  char sbuf[1024], temp[256];
  int i=0;
  char *cp;
  printf("iNode \tRec_len\t name_len\t name\n");
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
            printf("%3d   %4d     %4d     %s\n",
                dp->inode, dp->rec_len, dp->name_len, temp);        
            cp += dp->rec_len;
            dp = (DIR *)cp;
        }
    }
}


int pwd(MINODE *wd)
{
	if (wd == root) 
		printf("/\n");
	else
      rpwd(wd); 
      printf("\n");
}


int getpino(int dev, MINODE* mip)
{
    // Gets parent inode for a directory (doesn't work for files)
    char temp[256];
    DIR *dp = (DIR *) buff;
    char *cp = buff;

    get_block(dev, mip->INODE.i_block[0], buff);  // .. should be in block one

    while (cp < buff + BLKSIZE)
    {
        strncpy(temp, dp->name, dp->name_len);
        temp[dp->name_len] = '\0';  // Append NULL character
        
        if (!strcmp(temp, ".."))  // Found ye parent
        {
            return dp->inode;
        }
        
        cp += dp->rec_len;  // Move to next record
        dp = (DIR *) cp;
    }
    return 0;
}

rpwd(MINODE *wd)
{
  MINODE *pip;
  DIR *dp;
  char *cp;
  char temp[256];

  if(wd==root)
  {
    return;
  }

  pip = iget(dev, getpino(dev, wd)); // Extract the parent INODE
  for(int i=0; i<12; i++)
  {
    get_block(dev, pip->INODE.i_block[i], buff);
    dp = (DIR *) buff;
    cp = buff;

    while(cp < buff + BLKSIZE)
    {
      if(dp->inode == wd->ino) //Find your own ino in parent, grab name
      {
        strncpy(temp, dp->name, dp->name_len);
        temp[dp->name_len] = '\0'; //Don't append null character
        rpwd(pip);
        printf("/%s", temp);
        return;
      }
      cp += dp->rec_len;  // Move to next record
      dp = (DIR *) cp;
    }
  }
}

int dmkdir()
{
  MINODE *mip = running->cwd;
  if(pathname[0] == '/')
  {
    mip = root;
  }

  char parent[64];
  char child[64];
  char temp[128];

  strcpy(temp, pathname);

  strcpy(parent, dirname(temp));
  strcpy(temp, pathname);
  strcpy(child, basename(temp));

  int pino = getino(dev, parent);
  MINODE *pip = iget(dev, pino);

  if(S_ISDIR(pip->INODE.i_mode))
  {
    if(!search(pip, child))
    {
      mymkdir(pip, child);
    }
    else
    {
      printf("child already exists\n");
    }
    
  }
  else
  {
    printf("not a dir\n");
  }
  
}

int mymkdir(MINODE *pip, char *name)
{
  int ino = ialloc(dev);
  int bno = balloc(dev);
  char *cp;
  printf("MKDIR ino: %d\n bno: %d\n name: %s\n ", ino, bno, name);

  MINODE *mip = iget(dev, ino);
  mip->INODE.i_mode = 0x41ED;		// OR 040755: DIR type and permissions
  mip->INODE.i_uid  = running->uid;	// Owner uid 
  mip->INODE.i_gid  = pip->INODE.i_gid;	// Group Id
  mip->INODE.i_size = BLKSIZE;		// Size in bytes 
  mip->INODE.i_links_count = 2;	        // Links count=2 because of . and ..
  mip->INODE.i_atime = mip->INODE.i_ctime = mip->INODE.i_mtime = time(0L);  // set to current time
  mip->INODE.i_blocks = 2;                	// LINUX: Blocks count in 512-byte chunks 
  mip->INODE.i_block[0] = bno;             // new DIR has one data block   
  for (int i = 1; i < 15; i++)
    mip->INODE.i_block[i] = 0;
 
  mip->dirty = 1;               // mark minode dirty
  iput(mip);                    // write INODE to dis
  char buf[BLKSIZE];
  dp = (DIR*)buf;
  dp->inode = ino;
  dp->rec_len = 12;
  dp->name_len = 1;
  strcpy(dp->name, ".");
  cp = buf + dp->rec_len;
  dp = (DIR*)cp;
  dp->inode = pip->ino;
  dp->rec_len = 1012;
  dp->name_len = 2;
  strcpy(dp->name, "..");
  put_block(dev, bno, buf);
  enter_name(pip, ino, name);
  pip->INODE.i_links_count++;
  pip->INODE.i_atime =  time(0L);
  pip->dirty = 1;
  iput(pip);
}

int enter_name(MINODE *pip, int myino, char *name)
{
  int i;
  int ideal_len = 4 * ((8 + strlen(name) + 3) / 4);
  for (i = 0; i < 12; i++)
  {
    if (pip->INODE.i_block[i] == 0)
    {
      break;
    }
  }
  char buf[1024];
  i--;
  get_block(dev, pip->INODE.i_block[i], buf);
  dp = (DIR*)buf;
  char *cp = buf;

  printf("step to LAST entry in data block %d\n", pip->INODE.i_block[i]);
  while (cp + dp->rec_len < buf + BLKSIZE){

    /****** Technique for printing, compare, etc.******
    c = dp->name[dp->name_len];
    dp->name[dp->name_len] = 0;
    printf("%s ", dp->name);
    dp->name[dp->name_len] = c;
    **************************************************/

    cp += dp->rec_len;
    dp = (DIR *)cp;
  } 
  // dp NOW points at last entry in block
  int remain = dp->rec_len - (4 * ((8 + dp->name_len + 3 ) / 4));
  if (remain >= ideal_len)
  {
    printf("dp->name: %s\n", dp->name);
    dp->rec_len = 4 * ((8 + dp->name_len + 3) / 4);
    cp = cp + dp->rec_len;
    dp = (DIR*) cp;
    strcpy(dp->name, name);
    printf("dp->name: %s\n", dp->name);
    dp->name_len = strlen(name);
    dp->rec_len = remain;
    dp->inode = myino;
  }
  else
  {
    i++;
    get_block(dev, pip->INODE.i_block[i], buf);
    cp = buf;
    dp = (DIR*)cp;
    dp->rec_len = 1024;
    strcpy(dp->name, name);
    printf("dp->name: %s\n", dp->name);
    dp->name_len = strlen(name);
    dp->inode = myino;
  }
  put_block(dev, pip->INODE.i_block[i], buf);
}

int creat_file()
{
   MINODE *mip = running->cwd;
  if(pathname[0] == '/')
  {
    mip = root;
  }

  char parent[64];
  char child[64];
  char temp[128];

  strcpy(temp, pathname);

  strcpy(parent, dirname(temp));
  strcpy(temp, pathname);
  strcpy(child, basename(temp));

  int pino = getino(dev, parent);
  MINODE *pip = iget(dev, pino);

  if(S_ISDIR(pip->INODE.i_mode))
  {
    if(!search(pip, child))
    {
      my_creat(pip, child);
    }
    else
    {
      printf("child already exists\n");
    }
    
  }
  else
  {
    printf("not a dir\n");
  }
  
}


int my_creat(MINODE *pip, char *name)
{
  int ino = ialloc(dev);
  int bno = balloc(dev);
  char *cp;
  printf("MKDIR ino: %d\n bno: %d\n name: %s\n ", ino, bno, name);

  MINODE *mip = iget(dev, ino);
  mip->INODE.i_mode = 0x81A4;		// OR 040755: DIR type and permissions
  mip->INODE.i_uid  = running->uid;	// Owner uid 
  mip->INODE.i_gid  = pip->INODE.i_gid;	// Group Id
  mip->INODE.i_size = 0;		// Size in bytes 
  mip->INODE.i_links_count = 1;	        // Links count=2 because of . and ..
  mip->INODE.i_atime = mip->INODE.i_ctime = mip->INODE.i_mtime = time(0L);  // set to current time
  mip->INODE.i_blocks = 2;                	// LINUX: Blocks count in 512-byte chunks 
  mip->INODE.i_block[0] = bno;             // new DIR has one data block   
  for (int i = 1; i < 15; i++)
    mip->INODE.i_block[i] = 0;
 
  mip->dirty = 1;               // mark minode dirty
  iput(mip);                    // write INODE to dis
  char buf[BLKSIZE];
  dp = (DIR*)buf;
  dp->inode = ino;
  dp->rec_len = 12;
  dp->name_len = 1;
  strcpy(dp->name, ".");
  cp = buf + dp->rec_len;
  dp = (DIR*)cp;
  dp->inode = pip->ino;
  dp->rec_len = 1012;
  dp->name_len = 2;
  strcpy(dp->name, "..");
  put_block(dev, bno, buf);
  enter_name(pip, ino, name);
  pip->INODE.i_atime =  time(0L);
  pip->dirty = 1;
  iput(pip);
}





  


