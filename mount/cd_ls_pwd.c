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
	if (S_ISDIR(mip->i_mode))
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


int list_file()
{
  printf("list_file(): under construction\n");
}


int pwd(MINODE *wd)
{
  
}


