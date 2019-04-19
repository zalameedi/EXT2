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
    dev = root->dev;printf("calculated\n");
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
        iput(pip);
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
    //printf("dp->name: %s\n", dp->name);
    dp->rec_len = 4 * ((8 + dp->name_len + 3) / 4);
    cp = cp + dp->rec_len;
    dp = (DIR*) cp;
    strcpy(dp->name, name);
    //printf("dp->name: %s\n", dp->name);
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
    //printf("dp->name: %s\n", dp->name);
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
  printf("CREAT ino: %d\n bno: %d\n name: %s\n ", ino, bno, name);

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

int myrmdir()
{

  char parent[64];
  char child[64];
  char temp[128];

  strcpy(temp, pathname);

  strcpy(parent, dirname(temp));
  strcpy(temp, pathname);
  strcpy(child, basename(temp));
  int pino = getino(dev, parent);
  MINODE *pip = iget(dev, pino);
  int ino = getino(dev, pathname);
  MINODE *mip;
  if (pathname[0] == 0)
  {
    printf("No such pathname.\n");
    return;
  }

  if(S_ISDIR(pip->INODE.i_mode))
  {
    if(search(pip, child) != 0)
    {
      int temp1 = search(pip, child);
      iput(pip);
      mip = iget(dev, temp1);
      if(mip->refCount > 1)
      {
        //printf("mip->refCount = %d\n", mip->refCount);
        printf("Busy directory\n");
        iput(mip);
        return;
      }

      if(mip->INODE.i_links_count > 2)
      {
        printf("Directory not empty.\n");
        return;
      }

      else 
      {
        for (int i=0; i<12; i++)
        {
          if (mip->INODE.i_block[i]==0)
             continue;
          bdalloc(mip->dev, mip->INODE.i_block[i]);
          idalloc(mip->dev, mip->ino);
          iput(mip); 
          pip=iget(mip->dev, pino);
          rm_child(pip, pathname);
          pip->INODE.i_links_count --;
          pip->INODE.i_atime = pip->INODE.i_mtime = time(0L);
          pip->dirty = 1;
          iput(pip);
          return 1;
          
        }
      }
    }
    
  }
  else 
  {
    printf("Not a directory.\n");
  }
}

int rm_child(MINODE *parent, char *name)
{
  char bufr[1024];
  char *cp;
  DIR *dp2 = NULL;
  int addrem;
  int start, end;
  for(int i = 0; i < 12; i++)
  {
    if (parent->INODE.i_block[i] == 0)
    {
      printf("couldnt find %s\n", name);
      return 0;	
    }
    get_block(dev, parent->INODE.i_block[i], bufr);
    cp = bufr;
    dp = (DIR*)cp;
    while (cp < bufr + BLKSIZE)
    {

      if (strcmp(name, dp->name) == 0)
      {
        printf("found %s\n", name);
        if (dp->rec_len == (4 * ((8 + dp->name_len + 3 ) / 4)))
        {
          //entry in the middle of block
		addrem = dp->rec_len;
		dp2 = (DIR*)cp;
		cp += dp->rec_len;	
		dp = (DIR*)cp;
		start = cp + dp2->rec_len;
		end = bufr + BLKSIZE;
		memcpy(dp2, dp, end - start);
		cp = bufr;
		dp = (DIR*)cp;
		while (cp + addrem < bufr + BLKSIZE)
		{
			start = dp->rec_len;
			cp += dp->rec_len;
			dp = (DIR*)cp;
		}
		cp -= start;
		dp = (DIR*)cp;
		dp->rec_len += addrem;

        }
        else
        {
          //entry at end of block
          dp2->rec_len += dp->rec_len;
        }
	put_block(dev, parent->INODE.i_block[i], bufr);
	//return 1 for success
        return 1;
      }
      dp2 = dp;
      cp += dp->rec_len;
      dp = (DIR*)cp;
    }
    
  }
  
  //return 0 for failure
  return 0;
}


int link(char *oldname, char *newname)
{
  int i;
  char parentdir[64];
  char temp[64], *cp;
  DIR *dp;
  int parent;
  MINODE *pip, *target_ptr;

  if (oldname[i] == '\0')
  {
  	printf("Error that file isn't valid.");
	return -1;
  }
  if (newname[0] == '\0')
  {
  	printf("Error that file isn't valid.");
	return -1;
  }

  cp = strrchr(newname, '/');
  if(cp == NULL)
  {
  parent = running->cwd->ino;
  strcpy(temp, newname);
 
  }
  else
  {
	*cp = '\0';
	parent = strcpy(parentdir, newname);
	pip = getino(dev, parentdir);
	strcpy(name, cp+1);
  }

  int target = getino(dev, oldname);
  if(target == 0)
  {
	  printf("Didn't function correctly.\n");
	  return -1;
  }
  if(parent == NULL)
  {
	  printf("Didn't function correctly.\n");
	  return -1;
  }
  pip = iget(dev, parent);
  if(search(&pip->INODE, parent) != 0)
	{
	printf("parent doesn't exist, won't work");		
	return -1;
	}
  else
  {
	  target_ptr = iget(dev, target);
	  //Check to see if it's a directory
	  if(!(S_ISDIR(target_ptr->INODE.i_mode)))
	  {
		  printf("Cannot link to a directory.\n");
		  iput(pip);
		  return -1;
  	  }

	  get_block(dev, pip->INODE.i_block[0], buff);
	  dp = (DIR *) buff;
	  cp = buff;
	  while(cp +dp->rec_len < BLKSIZE + buff)
	  {
		  cp += dp->rec_len;
		  dp = (DIR *) cp;
	  }

	  int need_len = ((4 * ((8 + dp->name_len + 3) / 4)));
	  int cmp = dp->rec_len - need_len;

	  if(cmp >= need_len)
	  {
		  //Must allocate another block
		  dp->rec_len = 4 * ((8+dp->name_len +3) / 4);
		  cp = cp + dp->rec_len;
		  dp = (DIR *) cp;
		  strcpy(dp->name, name);
		  dp->rec_len = cmp;
		  dp->inode = target_ptr->ino;
		  strncpy(dp->name, temp, dp->name_len);
		  put_block(fd, pip->INODE.i_block[0], buff);

	  }
	  else
	  { 
		  i=0;
		  while(cmp < need_len)
		  {
			  i++;
			if(pip->INODE.i_block[0] == 0)
			{
				pip->INODE.i_block[0] = balloc(dev);
				pip->refCount = 0;
				cmp = 1024;
				memset(buff, 0, 1024);
				cp = buff;
				dp = (DIR *) buff;
			}
			else
			{
				get_block(dev, pip->INODE.i_block[0], buff);
				cp = buff;
				dp = (DIR *) buff;
				while(cp + dp->rec_len < BLKSIZE + buff)
				{
					cp += dp->rec_len;
					dp = (DIR *) cp;
				}
				need_len = ((4* ((8 + dp->name_len + 3) /4)));
				cmp = dp->rec_len - need_len;
				if(cmp >= need_len)
				{
					dp->rec_len = need_len;
					cp = cp + dp->rec_len;
					dp = (DIR *) cp;


			}

		  }
	  }
		
	dp->rec_len = cmp;
	dp->name_len = strlen(temp);
	dp->inode = target_ptr->ino;
	strncpy(dp->name, temp, dp->name_len);
	put_block(fd, pip->INODE.i_block[0], buff);

}

	pip->dirty = 1;
	pip->refCount += 1;
	target_ptr->INODE.i_atime = time(0L);
	iput(target_ptr);
	return target_ptr -> ino;

}
}

int mystat(char *pathname)
{
	struct stat myst;
	int ino = getino(dev, pathname);
	MINODE *mip = iget(dev, ino);
	myst.st_dev = dev;
	myst.st_ino = ino;
	myst.st_mode = mip->INODE.i_mode;
	myst.st_nlink = mip->INODE.i_links_count;
	myst.st_uid = mip->INODE.i_uid;
	myst.st_gid = mip->INODE.i_gid;
	myst.st_size = mip->INODE.i_size;
	myst.st_blksize = BLKSIZE;
	myst.st_blocks = mip->INODE.i_blocks;
	myst.st_atime = mip->INODE.i_atime;
	myst.st_mtime = mip->INODE.i_mtime;
	myst.st_ctime = mip->INODE.i_ctime;
	iput(mip);
	printf("device ID: %d\n", myst.st_dev);
	printf("inode number: %d\n", myst.st_ino);
	printf("mode: %o\n", myst.st_mode);
	   
	printf("links: %d\n", myst.st_nlink);
	printf("uid: %d\n", myst.st_uid);
	printf("gid: %d\n", myst.st_gid);
	printf("size: %d\n", myst.st_size);
	printf("blocks: %d\n", myst.st_blocks);

	char *cs, css[32];
	char *ms, mss[32];
	char *as, ss[32];

        cs = (char *)ctime(&myst.st_ctime);
        strncpy(css, cs, 24);
        css[24] = 0;
	printf("create time: %s\n", css);
        ms = (char *)ctime(&myst.st_mtime);
        strncpy(mss, ms, 24); //seg fault here
        mss[24] = 0;
	printf("modify time: %s\n", mss);
        as = (char *)ctime(&myst.st_atime);
        strncpy(ss, as, 24);
        ss[24] = 0;
	printf("access time: %s\n", ss);
}

int mychmod(char *pathname, char *mode)
{
  int _mode = atoi(mode);
	int ino = getino(dev, pathname);
	MINODE *mip = iget(dev, ino);
	mip->INODE.i_mode |= _mode;
	mip->dirty = 1;
	iput(mip);
}

int utime(char *pathname)
{
	int ino = getino(dev, pathname);
	MINODE *mip = iget(dev, ino);
	mip->INODE.i_atime = time(0L);
	mip->dirty = 1;
	iput(mip);
}

int mylink(char *oldFileName, char *newFileName)
{
    int oino, nino;
    MINODE *omip, *pip;
    char oparent[64], ochild[64];
    char nparent[64], nchild[64];

    printf("link oldfile:%s newfile:%s\n", oldFileName, newFileName);
    dbname(oldFileName, oparent, ochild);
    printf("oparent:%s ochild:%s\n", oparent, ochild);

    oino = getino(dev, ochild);  // to chk if oldFileName is a DIR or not
    omip = iget(dev, oino);

    if (getino(dev, newFileName) != 0) {
        printf("%s already exists\n", newFileName);
        return 0;
    }

    if (omip->INODE.i_mode == FILE_MODE) {
        DIR *dp;    
        MINODE *nmip;
        dbname(newFileName, nparent, nchild);
        nino = getino(dev, nparent);
        nmip = iget(dev, nino);
        if (nmip->dev !=  omip->dev) {
            printf("dev ids don't match - can't link files\n");
            return 0;
        }
        if (nmip->INODE.i_mode != DIR_MODE) {
            printf("%s(newfile) destination not a dir", nparent);
            return 0;
        }
        
        omip->dirty = 1;
        // creat new file with same ino as old file
        enter_name(nmip, omip->ino, nchild);
        // increment links count for INODE - same for both files
        omip->INODE.i_links_count++;
        iput(omip);
        iput(nmip);
        printf("link done\n");
        return 1;
    }
    else if (omip->INODE.i_mode == DIR_MODE) {
        printf("%s is a DIR: can't link DIRs\n", oldFileName);
        iput(omip);
        return 0;
    }
    printf("not a FILE\n");
}

void my_symlink (char *old_file, char *new_file)
{
    int i;
	int oino, parent_ino;
	MINODE *omip; //mip will point to the child Inode
	MINODE *p_mip; //p_mip will point to the parent minode
	INODE *ip; //ip will point to the child Inode
	INODE *p_ip; //p_ip will point to the parent inode
	char temp[64];
	char child_name[64];

	char link_temp[64];
	char link_parent[64];
	char link_child[64];
	int link_pino, link_ino;
	MINODE *link_pmip, *link_mip;
	
	strcpy (temp, old_file);
	strcpy(child_name, basename(temp));

	oino =  getino(dev, old_file);
	omip = iget(dev, oino);

	if (strlen(old_file) >= 60)
	{
		printf("name too long");
		return;
	}

	if (!omip)
	{
		printf("File doesnt exist");
	}
	
	strcpy(link_temp, new_file);
	dbname(link_temp, link_parent, link_child);

	link_pino = getino (dev, link_parent);
	link_pmip = iget(dev, link_pino);

	if(!link_pmip)
	{
		printf("Error! Parent doesnt exist");
		return;
	}
	if(!(S_ISDIR(link_pmip->INODE.i_mode)))
	{
		printf("Error! parent is not a directory!");
		return;
	}
	if(getino(dev,link_child)> 0)
	{
		printf("child already exists");
		return;
	}
	creat_file(new_file);
	link_ino = getino(dev,new_file);
	link_mip = iget(dev, link_ino);
	link_mip -> INODE.i_mode = 0120777;
	link_mip -> INODE.i_size =2;
	link_mip -> dirty = 1;
	iput(link_mip);

	link_pmip -> INODE.i_atime = time(0L);
	link_pmip -> dirty =1;

	iput(link_pmip);

}

void my_unlink(char *path)
{
                int ino, i;
                int parent_ino;

                MINODE *mip;
                MINODE *parent_mip;
                INODE *ip;
                INODE *parent_ip;

                char temp1[64], temp2[64];
                char my_dirname[64];
                char my_basename[64];

		strcpy(temp1, path);
                strcpy(temp2, path);
                strcpy(my_dirname, dirname(temp1));
                strcpy(my_basename, basename(temp2));
		
                printf("Dir name is %s\nBasename is %s\n", my_dirname, my_basename);

                //checks
                if(!path) 
                {
                                printf("ERROR: No file name given!\n");
                                return;
                }

                ino = getino(running->cwd, path);

                if(ino == 0)
                {
                                printf("ERROR: File does not exist!\n");
                                return;
                }

                mip = iget(dev, ino);

                if(!mip)
                {
                                printf("ERROR: Missing minode!\n");
                                return;
                }

                //file
                if(S_ISDIR(mip->INODE.i_mode))
                {
                                printf("ERROR: Can't unlink a directory!\n");
                                return;
                }

                printf("Unlinking...\n");
                ip = &mip->INODE;


                
		ip->i_links_count--; 


                printf("Links: %d\n", ip->i_links_count);

                for(i = 0; i < 12 && ip->i_block[i] != 0; i++)
                {
                                bdalloc(dev, ip->i_block[i]); //deallocate blocks
                }


                idalloc(dev, ino);          

                //removes file from parent
                parent_ino = getino(running->cwd, my_dirname);
                parent_mip = iget(dev, parent_ino);
                parent_ip = &parent_mip->INODE;
	

                //removes the child
                printf("Removing %s from %s\n", my_basename, my_dirname);
                rm_child(parent_mip, my_basename);

                //update
		parent_ip->i_links_count--; //REMOVE IF DOESNT WORK
                parent_ip->i_atime = time(0L);
                parent_ip->i_mtime = time(0L);
                parent_mip->dirty = 1;
                iput(parent_mip);
                mip->dirty = 1;
                iput(mip);

                return;
}

int dbname(char *pathname, char *dname, char *bname)
{
    char temp[128]; // dirname(), basename() destroy original pathname
    strcpy(temp, pathname);
    strcpy(dname, dirname(temp));
    strcpy(temp, pathname);
    strcpy(bname, basename(temp));
}

