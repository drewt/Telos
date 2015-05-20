/*  Copyright 2013-2015 Drew Thoreson
 *
 *  This file is part of Telos.
 *
 *  Telos is free software: you can redistribute it and/or modify it under the
 *  terms of the GNU General Public License as published by the Free Software
 *  Foundation, version 2 of the License.
 *
 *  Telos is distributed in the hope that it will be useful, but WITHOUT ANY
 *  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 *  FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 *  details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with Telos.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _FS_EXT2_EXT2_H_
#define _FS_EXT2_EXT2_H_

#include <kernel/fs.h>

struct ext2_superblock {
	uint32_t inodes_count;
	uint32_t blocks_count;
	uint32_t r_blocks_count;
	uint32_t free_blocks_count;
	uint32_t free_inodes_count;
	uint32_t first_data_block;
	uint32_t log_block_size;
	uint32_t log_frag_size;
	uint32_t blocks_per_group;
	uint32_t frags_per_group;
	uint32_t inodes_per_group;
	uint32_t mtime;
	uint32_t wtime;
	uint16_t mnt_count;
	uint16_t max_mnt_count;
	uint16_t magic;
	uint16_t state;
	uint16_t errors;
	uint16_t minor_rev_level;
	uint32_t lastcheck;
	uint32_t checkinterval;
	uint32_t creator_os;
	uint32_t rev_level;
	uint16_t def_resuid;
	uint16_t def_resgid;
	/* EXT2_DYNAMIC_REV specific */
	uint32_t first_ino;
	uint16_t inode_size;
	uint16_t block_group_nr;
	uint32_t feature_compat;
	uint32_t feature_incompat;
	uint32_t feature_ro_compat;
	uint8_t  uuid[16];
	uint8_t  volume_name[16];
	uint8_t  last_mounted[64];
	uint32_t algo_bitmap;
	/* performance hints */
	uint8_t  prealloc_blocks;
	uint8_t  prealloc_dir_blocks;
	uint16_t _ph_alignment;
	/* journaling support */
	uint8_t  journal_uuid[16];
	uint32_t journal_inum;
	uint32_t journal_dev;
	uint32_t last_orphan;
	/* directory indexing support */
	uint32_t hash_seed[4];
	uint8_t  def_hash_version;
	uint8_t  _dis_padding[3];
	/* other options */
	uint32_t default_mount_options;
	uint32_t first_meta_bg;
	uint8_t _reserved[760];
} __packed;

/* s.magic */
enum { EXT2_SUPER_MAGIC = 0xEF53 };

/* s.state */
enum {
	EXT2_VALID_FS = 1, // unmounted cleanly
	EXT2_ERROR_FS = 2  // errors detected
};

/* s.errors */
enum {
	EXT2_ERRORS_CONTINUE = 1, // continue as if nothing happened
	EXT2_ERRORS_RO       = 2, // remount read-only
	EXT2_ERRORS_PANIC    = 3  // cause a kernel panic
};

/* s.creator_os */
enum {
	EXT2_OS_LINUX   = 0,
	EXT2_OS_HURD    = 1,
	EXT2_OS_MASIX   = 2,
	EXT2_OS_FREEBSD = 3,
	EXT2_OS_LITES   = 4
};

/* s.rev_level */
enum {
	EXT2_GOOD_OLD_REV = 0,
	EXT2_DYNAMIC_REV = 1
};

enum {
	EXT2_DEF_RESUID = 0, // s.resuid
	EXT2_DEF_RESGID = 0  // s.resgid
};

/* rev 0 constants */
enum {
	EXT2_GOOD_OLD_FIRST_INO  = 11,
	EXT2_GOOD_OLD_INODE_SIZE = 128
};

/* s.feature_compat */
enum {
	EXT2_FEATURE_COMPAT_DIR_PREALLOC  = 1,
	EXT2_FEATURE_COMPAT_IMAGIC_INODES = 2,
	EXT2_FEATURE_COMPAT_HAS_JOURNAL   = 4,
	EXT2_FEATURE_COMPAT_EXT_ATTR      = 8,
	EXT2_FEATURE_COMPAT_RESIZE_INO    = 16,
	EXT2_FEATURE_COMPAT_DIR_INDEX     = 32
};

/* s.feature_incompat */
enum {
	EXT2_FEATURE_INCOMPAT_COMPRESSION = 1,
	EXT2_FEATURE_INCOMPAT_FILETYPE    = 2,
	EXT2_FEATURE_INCOMPAT_RECOVER     = 4,
	EXT2_FEATURE_INCOMPAT_JOURNAL_DEV = 8,
	EXT2_FEATURE_INCOMPAT_META_BG     = 16
};

/* s.feature_ro_compat */
enum {
	EXT2_FEATURE_RO_COMPAT_SPARSE_SUPER = 1,
	EXT2_FEATURE_RO_COMPAT_LARGE_FILE   = 2,
	EXT2_FEATURE_RO_COMPAT_BTREE_DIR    = 4
};

struct ext2_bg_descriptor {
	uint32_t block_bitmap;
	uint32_t inode_bitmap;
	uint32_t inode_table;
	uint16_t free_blocks_count;
	uint16_t free_inodes_count;
	uint16_t used_dirs_count;
	uint16_t _pad;
	uint8_t _reserved[12];
} __packed;

struct ext2_inode {
	uint16_t mode;
	uint16_t uid;
	uint32_t size;
	uint32_t atime;
	uint32_t ctime;
	uint32_t mtime;
	uint32_t dtime;
	uint16_t gid;
	uint16_t links_count;
	uint32_t blocks;
	uint32_t flags;
	uint32_t osd1;
	uint32_t block[15];
	uint32_t generation;
	uint32_t file_acl;
	uint32_t dir_acl;
	uint32_t faddr;
	uint8_t osd2[12];
} __packed;

/* reserved inodes */
enum {
	EXT2_BAD_INO         = 1, // bad blocks inode
	EXT2_ROOT_INO        = 2, // root directory inode
	EXT2_ACL_IDX_INO     = 3, // ACL index node (deprecated?)
	EXT2_ACL_DATA_INO    = 4, // ACL data inode (deprecated?)
	EXT2_BOOT_LOADER_INO = 5, // boot loader inode
	EXT2_UNDEL_DIR_INO   = 6  // undelete directory inode
};

/* i.mode */
enum {
	/* file format */
	EXT2_S_IFSOCK = 0xC000, // socket
	EXT2_S_IFLNK  = 0xA000, // symbolic link
	EXT2_S_IFREG  = 0x8000, // regular file
	EXT2_S_IFBLK  = 0x6000, // block device
	EXT2_S_IFDIR  = 0x4000, // directory
	EXT2_S_IFCHR  = 0x2000, // character device
	EXT2_S_IFIFO  = 0x1000, // fifo
	/* process execution user/group override */
	EXT2_S_ISUID  = 0x0800, // set process user ID
	EXT2_S_ISGID  = 0x0400, // set process group ID
	EXT2_S_ISVTX  = 0x0200, // sticky bit
	/* access rights */
	EXT2_S_IRUSR  = 0x0100, // user read
	EXT2_S_IWUSR  = 0x0080, // user write
	EXT2_S_IXUSR  = 0x0040, // user execute
	EXT2_S_IRGRP  = 0x0020, // group read
	EXT2_S_IWGRP  = 0x0010, // group write
	EXT2_S_IXGRP  = 0x0008, // group execute
	EXT2_S_IROTH  = 0x0004, // other read
	EXT2_S_IWOTH  = 0x0002, // other write
	EXT2_S_IXOTH  = 0x0001  // other execute
};

/* i.flags */
enum {
	EXT2_SECRM_FL        = 0x00000001, // secure deletion
	EXT2_UNRM_FL         = 0x00000002, // record for undelete
	EXT2_COMPR_FL        = 0x00000004, // compressed file
	EXT2_SYNC_FL         = 0x00000008, // synchronous updates
	EXT2_IMMUTABLE_FL    = 0x00000010, // immutable file
	EXT2_APPEND_FL       = 0x00000020, // append only
	EXT2_NODUMP_FL       = 0x00000040, // do not dump/delete file
	EXT2_NOATIME_FL      = 0x00000080, // do not update atime
	/* reserved for compression usage */
	EXT2_DIRTY_FL        = 0x00000100, // dirty (modified)
	EXT2_COMPRBLK_FL     = 0x00000200, // compressed blocks
	EXT2_NOCOMPR_FL      = 0x00000400, // access raw compressed data
	EXT2_ECOMPR_FL       = 0x00000800, // compression error
	/* end of compression flags */
	EXT2_BTREE_FL        = 0x00001000, // b-tree format directory
	EXT2_INDEX_FL        = 0x00001000, // hash indexed directory
	EXT2_IMAGIC_FL       = 0x00002000, // AFS directory
	EXT2_JOURNAL_DATA_FL = 0x00004000, // journal file data
	EXT2_RESERVED_FL     = 0x80000000  // reserved for ext2 library
};

static inline uint32_t ext2_inode_blocks(struct ext2_superblock *sb,
		struct ext2_inode *inode)
{
	return inode->blocks / (2 << sb->log_block_size);
}

struct ext2_dirent {
	uint32_t ino;
	uint16_t rec_len;
	uint8_t name_len;
	uint8_t file_type;
	char name[256];
} __packed;

struct ext2_inode *ext2_inode(struct inode *vnode);
struct ext2_superblock *ext2_superblock(struct super_block *sb);

void ext2_read_inode(struct inode *vnode);
void ext2_put_inode(struct inode *vnode);
struct ext2_inode *ext2_iget(struct super_block *vsb, ino_t ino,
		struct buffer **buf);
struct bio_vec *ext2_inode_to_bio_vec(struct inode *vnode);

extern struct file_operations ext2_reg_fops;
extern struct file_operations ext2_dir_fops;
extern struct inode_operations ext2_reg_iops;
extern struct inode_operations ext2_dir_iops;

#endif
