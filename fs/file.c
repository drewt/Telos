
#include <kernel/fs.h>
#include <kernel/mm/slab.h>

static void fs_file_sysinit(void)
{
	file_cachep = slab_cache_create(sizeof(struct file));
	inode_cachep = slab_cache_create(sizeof(struct inode));
}
EXPORT_KINIT(fs_file, SUB_VFS, fs_file_sysinit);

