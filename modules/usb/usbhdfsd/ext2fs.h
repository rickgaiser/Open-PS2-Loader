#ifndef EXT2_DRIVER_H
#define	EXT2_DRIVER_H 1

//#include <io_common.h>
#include <ioman.h>
#include <sys/stat.h>
//#include <string.h>

typedef unsigned long int off_t;    //todo - check
typedef u8 uint8_t;
typedef u16 uint16_t;
typedef u32 uint32_t;


#define IOCTL_CKFREE 0xBABEC0DE  //dlanor: Ioctl request code => Check free space
#define IOCTL_RENAME 0xFEEDC0DE  //dlanor: Ioctl request code => Rename
#define IOCTL_GETCLUSTER 0xBEEFC0DE        //jimmikaelkael: Ioctl request code => get file start cluster
#define IOCTL_GETDEVSECTORSIZE 0xDEADC0DE  //jimmikaelkael: Ioctl request code => get mass storage device sector size
#define IOCTL_CHECKCHAIN 0xCAFEC0DE  //polo: Ioctl request code => Check cluster chain
#define IOCTL_SECTORS_MAP_START 0x13370000
#define IOCTL_DEVID 0x13370201


#define EXT2_SECTORS_BYTES 512
#define EXT2_TMP_SECTORS_BYTES 1024
#define EXT2_SECTOR_SIZE 512
#define READ_SECTOR(d, a, b)	scache_readSector((d)->cache, (a), (void **)&b)


/*
 * Macro-instructions used to manage several block sizes
 */
#define EXT2_MIN_BLOCK_SIZE		1024
#define	EXT2_MAX_BLOCK_SIZE		4096
#define EXT2_MIN_BLOCK_LOG_SIZE		  10
# define EXT2_BLOCK_SIZE(s)		(EXT2_MIN_BLOCK_SIZE << (s)->s_log_block_size)
#define	EXT2_ADDR_PER_BLOCK(s)		(EXT2_BLOCK_SIZE(s) / sizeof (uint32_t))
# define EXT2_BLOCK_SIZE_BITS(s)	((s)->s_log_block_size + 10)
#define EXT2_INODE_SIZE(s)	(((s)->s_rev_level == EXT2_GOOD_OLD_REV) ? \
				 EXT2_GOOD_OLD_INODE_SIZE : \
				 (s)->s_inode_size)
#define EXT2_FIRST_INO(s)	(((s)->s_rev_level == EXT2_GOOD_OLD_REV) ? \
				 EXT2_GOOD_OLD_FIRST_INO : \
				 (s)->s_first_ino)

#define EXT2_SUPER_MAGIC	0xEF53
//#define EXT2_SUPER_MAGIC	0x53EF


/*
 * Constants relative to the data blocks
 */
#define	EXT2_NDIR_BLOCKS		12
#define	EXT2_IND_BLOCK			EXT2_NDIR_BLOCKS
#define	EXT2_DIND_BLOCK			(EXT2_IND_BLOCK + 1)
#define	EXT2_TIND_BLOCK			(EXT2_DIND_BLOCK + 1)
#define	EXT2_N_BLOCKS			(EXT2_TIND_BLOCK + 1)


/*
 * Revision levels
 */
#define EXT2_GOOD_OLD_REV	0	/* The good old (original) format */
#define EXT2_DYNAMIC_REV	1 	/* V2 format w/ dynamic inode sizes */

#define EXT2_CURRENT_REV	EXT2_GOOD_OLD_REV
#define EXT2_MAX_SUPP_REV	EXT2_DYNAMIC_REV

#define EXT2_GOOD_OLD_INODE_SIZE 128


/*
 * Macro-instructions used to manage group descriptors
 */
# define EXT2_BLOCKS_PER_GROUP(s)	((s)->s_blocks_per_group)
# define EXT2_DESC_PER_BLOCK(s)		(EXT2_BLOCK_SIZE(s) / sizeof (struct ext2_group_desc))
# define EXT2_INODES_PER_GROUP(s)	((s)->s_inodes_per_group)


/* from linux/stat.h */
/* same as in ps2sdk/common/include/sys/stat.h */

#ifndef S_IFMT
#define S_IFMT  00170000
#endif

#define S_IFSOCK 0140000
#define S_IFLNK	 0120000
#define S_IFREG  0100000
#define S_IFBLK  0060000
#define S_IFDIR  0040000
#define S_IFCHR  0020000
#define S_IFIFO  0010000
#define S_ISUID  0004000
#define S_ISGID  0002000
#define S_ISVTX  0001000

#ifndef S_ISLNK
#define S_ISLNK(m)	(((m) & S_IFMT) == S_IFLNK)
#endif

#ifndef S_ISREG
#define S_ISREG(m)	(((m) & S_IFMT) == S_IFREG)
#endif

#ifndef S_ISDIR
#define S_ISDIR(m)	(((m) & S_IFMT) == S_IFDIR)
#endif

#define S_ISCHR(m)	(((m) & S_IFMT) == S_IFCHR)
#define S_ISBLK(m)	(((m) & S_IFMT) == S_IFBLK)
#define S_ISFIFO(m)	(((m) & S_IFMT) == S_IFIFO)
#define S_ISSOCK(m)	(((m) & S_IFMT) == S_IFSOCK)


/*
 * Structure of the super block
 */
struct ext2_super_block {
	uint32_t	s_inodes_count;		/* Inodes count */
	uint32_t	s_blocks_count;		/* Blocks count */
	uint32_t	s_r_blocks_count;	/* Reserved blocks count */
	uint32_t	s_free_blocks_count;	/* Free blocks count */
	uint32_t	s_free_inodes_count;	/* Free inodes count */
	uint32_t	s_first_data_block;	/* First Data Block */
	uint32_t	s_log_block_size;	/* Block size */
	uint32_t	s_log_frag_size;	/* Fragment size */
	uint32_t	s_blocks_per_group;	/* # Blocks per group */
	uint32_t	s_frags_per_group;	/* # Fragments per group */
	uint32_t	s_inodes_per_group;	/* # Inodes per group */
	uint32_t	s_mtime;		/* Mount time */
	uint32_t	s_wtime;		/* Write time */
	uint16_t	s_mnt_count;		/* Mount count */
	uint16_t	s_max_mnt_count;	/* Maximal mount count */
	uint16_t	s_magic;		/* Magic signature */
	uint16_t	s_state;		/* File system state */
	uint16_t	s_errors;		/* Behaviour when detecting errors */
	uint16_t	s_minor_rev_level; 	/* minor revision level */
	uint32_t	s_lastcheck;		/* time of last check */
	uint32_t	s_checkinterval;	/* max. time between checks */
	uint32_t	s_creator_os;		/* OS */
	uint32_t	s_rev_level;		/* Revision level */
	uint16_t	s_def_resuid;		/* Default uid for reserved blocks */
	uint16_t	s_def_resgid;		/* Default gid for reserved blocks */
	/*
	 * These fields are for EXT2_DYNAMIC_REV superblocks only.
	 *
	 * Note: the difference between the compatible feature set and
	 * the incompatible feature set is that if there is a bit set
	 * in the incompatible feature set that the kernel doesn't
	 * know about, it should refuse to mount the filesystem.
	 *
	 * e2fsck's requirements are more strict; if it doesn't know
	 * about a feature in either the compatible or incompatible
	 * feature set, it must abort and not try to meddle with
	 * things it doesn't understand...
	 */
	uint32_t	s_first_ino; 		/* First non-reserved inode */
	uint16_t   s_inode_size; 		/* size of inode structure */
	uint16_t	s_block_group_nr; 	/* block group # of this superblock */
	uint32_t	s_feature_compat; 	/* compatible feature set */
	uint32_t	s_feature_incompat; 	/* incompatible feature set */
	uint32_t	s_feature_ro_compat; 	/* readonly-compatible feature set */
	uint8_t	s_uuid[16];		/* 128-bit uuid for volume */
	char	s_volume_name[16]; 	/* volume name */
	char	s_last_mounted[64]; 	/* directory where last mounted */
	uint32_t	s_algorithm_usage_bitmap; /* For compression */
	/*
	 * Performance hints.  Directory preallocation should only
	 * happen if the EXT2_COMPAT_PREALLOC flag is on.
	 */
	uint8_t	s_prealloc_blocks;	/* Nr of blocks to try to preallocate*/
	uint8_t	s_prealloc_dir_blocks;	/* Nr to preallocate for dirs */
	uint16_t	s_padding1;
	/*
	 * Journaling support valid if EXT3_FEATURE_COMPAT_HAS_JOURNAL set.
	 */
	uint8_t	s_journal_uuid[16];	/* uuid of journal superblock */
	uint32_t	s_journal_inum;		/* inode number of journal file */
	uint32_t	s_journal_dev;		/* device number of journal file */
	uint32_t	s_last_orphan;		/* start of list of inodes to delete */
	uint32_t	s_hash_seed[4];		/* HTREE hash seed */
	uint8_t	s_def_hash_version;	/* Default hash version to use */
	uint8_t	s_reserved_char_pad;
	uint16_t	s_reserved_word_pad;
	uint32_t	s_default_mount_opts;
 	uint32_t	s_first_meta_bg; 	/* First metablock block group */
	uint32_t	s_reserved[190];	/* Padding to the end of the block */
};


typedef struct ext2_VOLUME {
    mass_dev* dev;
    struct ext2_super_block *super;
    unsigned int start;
    unsigned int current_buffer;
    unsigned int total_buffers;
    unsigned int *buffer_blocks;
    char *buffer;
} ext2_VOLUME;


typedef struct ext2_DIR {
        ext2_VOLUME *volume;
	struct ext2_inode *inode;
	off_t index;
} ext2_DIR;


/*
 * Structure of an inode on the disk
 */
struct ext2_inode {
	uint16_t	i_mode;		/* File mode */
	uint16_t	i_uid;		/* Low 16 bits of Owner Uid */
	uint32_t	i_size;		/* Size in bytes */
	uint32_t	i_atime;	/* Access time */
	uint32_t	i_ctime;	/* Creation time */
	uint32_t	i_mtime;	/* Modification time */
	uint32_t	i_dtime;	/* Deletion Time */
	uint16_t	i_gid;		/* Low 16 bits of Group Id */
	uint16_t	i_links_count;	/* Links count */
	uint32_t	i_blocks;	/* Blocks count */
	uint32_t	i_flags;	/* File flags */
	union {
		struct {
			uint32_t  l_i_reserved1;
		} linux1;
		struct {
			uint32_t  h_i_translator;
		} hurd1;
		struct {
			uint32_t  m_i_reserved1;
		} masix1;
	} osd1;				/* OS dependent 1 */
	uint32_t	i_block[EXT2_N_BLOCKS];/* Pointers to blocks */
	uint32_t	i_generation;	/* File version (for NFS) */
	uint32_t	i_file_acl;	/* File ACL */
	uint32_t	i_dir_acl;	/* Directory ACL */
	uint32_t	i_faddr;	/* Fragment address */
	union {
		struct {
			uint8_t	l_i_frag;	/* Fragment number */
			uint8_t	l_i_fsize;	/* Fragment size */
			uint16_t	i_pad1;
			uint16_t	l_i_uid_high;	/* these 2 fields    */
			uint16_t	l_i_gid_high;	/* were reserved2[0] */
			uint32_t	l_i_reserved2;
		} linux2;
		struct {
			uint8_t	h_i_frag;	/* Fragment number */
			uint8_t	h_i_fsize;	/* Fragment size */
			uint16_t	h_i_mode_high;
			uint16_t	h_i_uid_high;
			uint16_t	h_i_gid_high;
			uint32_t	h_i_author;
		} hurd2;
		struct {
			uint8_t	m_i_frag;	/* Fragment number */
			uint8_t	m_i_fsize;	/* Fragment size */
			uint16_t	m_pad1;
			uint32_t	m_i_reserved2[2];
		} masix2;
	} osd2;				/* OS dependent 2 */
};


typedef struct ext2_FILE {
        ext2_VOLUME *volume;
	struct ext2_inode *inode;
	//off_t offset;
        long long offset;
	char *path;
} ext2_FILE;


/*
 * Structure of a blocks group descriptor
 */
struct ext2_group_desc
{
	uint32_t	bg_block_bitmap;		/* Blocks bitmap block */
	uint32_t	bg_inode_bitmap;		/* Inodes bitmap block */
	uint32_t	bg_inode_table;		/* Inodes table block */
	uint16_t	bg_free_blocks_count;	/* Free blocks count */
	uint16_t	bg_free_inodes_count;	/* Free inodes count */
	uint16_t	bg_used_dirs_count;	/* Directories count */
	uint16_t	bg_pad;
	uint32_t	bg_reserved[3];
};


/*
 * Special inode numbers
 */
#define	EXT2_BAD_INO		 1	/* Bad blocks inode */
#define EXT2_ROOT_INO		 2	/* Root inode */
#define EXT2_BOOT_LOADER_INO	 5	/* Boot loader inode */
#define EXT2_UNDEL_DIR_INO	 6	/* Undelete directory inode */


/*
 * Structure of a directory entry
 */
#define EXT2_NAME_LEN 255


#define EXT2_MAX_HANDLES 16

/* open files and directories */
void *ext2_files[EXT2_MAX_HANDLES];
void *ext2_dirs[EXT2_MAX_HANDLES];


/*
 * The new version of the directory entry.  Since EXT2 structures are
 * stored in intel byte order, and the name_len field could never be
 * bigger than 255 chars, it's safe to reclaim the extra byte for the
 * file_type field.
 */
struct ext2_dir_entry_2 {
	uint32_t	inode;			/* Inode number */
	uint16_t	rec_len;		/* Directory entry length */
	uint8_t	name_len;		/* Name length */
	uint8_t	file_type;
	char	name[EXT2_NAME_LEN];	/* File name */
};

extern int _fs_lock(void);
extern int _fs_unlock(void);

int ext2_mount(mass_dev* dev, unsigned int start, unsigned int count);
int ext2_umount();
int ext2_fs_close(iop_file_t* fd);
int ext2_fs_closedir(iop_file_t* fd);
int ext2_fs_lseek(iop_file_t* fd, unsigned long long offset, int whence);
int ext2_fs_open(iop_file_t* fd, const char *name, int mode);
int ext2_fs_dopen(iop_file_t *fd, const char *name);
int ext2_fs_read(iop_file_t* fd, void * buffer, int size);
int ext2_fs_dread(iop_file_t *fd, fio_dirent_t *buffer);
int ext2_fs_write(iop_file_t* fd, void * buffer, int size);
int ext2_fs_ioctl(iop_file_t *fd, unsigned long request, void *data);
int ext2_fs_remove(iop_file_t *fd, const char *name);
int ext2_fs_mkdir(iop_file_t *fd, const char *name);
int ext2_fs_rmdir(iop_file_t *fd, const char *name);
int ext2_fs_getstat(iop_file_t *fd, const char *name, fio_stat_t *stat);
int ext2_fs_chstat(iop_file_t *fd, const char *name, fio_stat_t *stat, unsigned int a);

/* mounted ext2 volume */
ext2_VOLUME *ext2_volume;


#endif	/* EXT2_DRIVER_H */
