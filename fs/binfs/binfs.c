
#include <kernel/fs.h>
#include <kernel/ramfs.h>
#include <kernel/stat.h>

static struct file_operations binfs_dir_fops = {
	.readdir = ramfs_readdir,
};

static struct inode_operations binfs_dir_iops = {
	.lookup = ramfs_lookup,
	.default_file_ops = &binfs_dir_fops,
};

#define PROGRAM(name) int name(int argc, char *argv[])
extern PROGRAM(cat);
extern PROGRAM(date);
extern PROGRAM(echo);
extern PROGRAM(uls);
extern PROGRAM(mount_ramfs);
extern PROGRAM(uumount);
extern PROGRAM(umkdir);
extern PROGRAM(urmdir);
extern PROGRAM(ulink);
extern PROGRAM(uunlink);
extern PROGRAM(urename);
extern PROGRAM(ustat);

extern PROGRAM(consoletest);
extern PROGRAM(exectest);
extern PROGRAM(exntest);
extern PROGRAM(eventtest);
extern PROGRAM(jmptest);
extern PROGRAM(kbdtest);
extern PROGRAM(memtest);
extern PROGRAM(proctest);
extern PROGRAM(sigtest);
extern PROGRAM(strtest);
extern PROGRAM(multi);

extern PROGRAM(tsh);

typedef int(*funptr)(int, char**);
struct program {
	funptr fun;
	const char *name;
	size_t len;
};

#define BINENT(fun, name) { fun, name, sizeof(name) }
static struct program progtab[] = {
	BINENT(cat,         "cat"),
	BINENT(date,        "date"),
	BINENT(echo,        "echo"),
	BINENT(ulink,       "link"),
	BINENT(uls,         "ls"),
	BINENT(umkdir,      "mkdir"),
	BINENT(mount_ramfs, "mount-ramfs"),
	BINENT(urename,     "rename"),
	BINENT(urmdir,      "rmdir"),
	BINENT(ustat,       "stat"),
	BINENT(uumount,     "umount"),
	BINENT(uunlink,     "unlink"),

	BINENT(consoletest, "consoletest"),
	BINENT(exectest,    "exectest"),
	BINENT(exntest,     "exntest"),
	BINENT(eventtest,   "eventtest"),
	BINENT(jmptest,     "jmptest"),
	BINENT(kbdtest,     "kbdtest"),
	BINENT(memtest,     "memtest"),
	BINENT(proctest,    "proctest"),
	BINENT(sigtest,     "sigtest"),
	BINENT(strtest,     "strtest"),
	BINENT(multi,       "multi"),

	BINENT(tsh,         "tsh"),
};
#define NR_PROGRAMS (sizeof(progtab)/sizeof(*progtab))

static int mkbin(struct inode *dir, const char *name, int len, funptr fun)
{
	int error;
	struct inode *inode;

	error = ramfs_do_mknod(dir, name, len, &inode);
	if (error)
		return error;

	inode->i_rdev = 0;
	inode->i_mode = S_IFFUN;
	inode->i_nlink = 1;
	inode->i_op = NULL;
	inode->i_private = fun;
	return 0;
}

static struct super_block *binfs_read_super(struct super_block *sb, void *data,
		int silent)
{
	int error;
	static bool mounted = false;

	if (mounted)
		return NULL;
	else
		mounted = true;

	if (!(sb = ramfs_read_super(sb, data, silent)))
		return NULL;
	sb->s_mounted->i_op = &binfs_dir_iops;

	for (unsigned i = 0; i < NR_PROGRAMS; i++) {
		error = mkbin(sb->s_mounted, progtab[i].name, progtab[i].len,
				progtab[i].fun);
		if (error)
			return NULL;
	}
	return sb;
}

static void binfs_sysinit(void)
{
	register_filesystem(binfs_read_super, "binfs", 0);
}
EXPORT_KINIT(sysfs, SUB_VFS, binfs_sysinit);
