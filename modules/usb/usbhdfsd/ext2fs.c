//File name:    ext2_driver.c
//https://github.com/lentinj/u-boot/blob/master/fs/ext2/dev.c
//https://github.com/lentinj/u-boot/blob/master/include/ext2fs.h
//https://github.com/lentinj/u-boot/blob/master/fs/ext2/ext2fs.c
//---------------------------------------------------------------------------
#include <stdio.h>
#include <errno.h>

#ifdef WIN32
#include <malloc.h>
#include <memory.h>
#include <string.h>
#else
#include <sysclib.h>
//#include <sys/stat.h>

#include <thbase.h>
#include <sysmem.h>
#define malloc(a)       AllocSysMemory(0,(a), NULL)
#define free(a)         FreeSysMemory((a))
#endif

#include "usbhd_common.h"
#include "scache.h"
#include "mass_stor.h"

#include "ext2fs.h"


static unsigned char *ext2_file_sectors;


uint16_t __le16_to_cpu(register uint16_t x) {
    unsigned char tmp[2];
    
    tmp[0]=x & 0xff;
    tmp[1]=(x >> 8);

    return getI16(tmp);
}


uint32_t __le32_to_cpu(register uint32_t x) {
    unsigned char tmp[4];

    tmp[0] = (x & 0x000000ff);
    tmp[1] = (x & 0x0000ff00) >> 8;
    tmp[2] = (x & 0x00ff0000) >> 16;
    tmp[3] = (x & 0xff000000) >> 24;

    return getI32(tmp);
}


int READ_SECTOR_INDIRECT(mass_dev* mass_device, unsigned int sector, unsigned char* buffer, int size) {
    int i, chunks = size / EXT2_SECTOR_SIZE;
    unsigned char *sbuf;
    
    for (i = 0; i < chunks; i++) {
        READ_SECTOR(mass_device, sector + i, sbuf);
        memcpy(&buffer[i * EXT2_SECTOR_SIZE], sbuf, EXT2_SECTOR_SIZE);
    }
    
    return chunks;
}


void ext2_get_super(mass_dev* dev, struct ext2_super_block *super, unsigned int start) {
    unsigned char sbuf[EXT2_SECTOR_SIZE];

    //printf("ext2_get_super devId: %d\n", dev->devId);

    READ_SECTOR_INDIRECT(dev, start + 2, sbuf, EXT2_SECTOR_SIZE);
    memcpy(super, sbuf, sizeof (struct ext2_super_block));

    super->s_inodes_count = __le32_to_cpu(super->s_inodes_count);
    super->s_blocks_count = __le32_to_cpu(super->s_blocks_count);
    super->s_r_blocks_count = __le32_to_cpu(super->s_r_blocks_count);
    super->s_free_blocks_count = __le32_to_cpu(super->s_free_blocks_count);
    super->s_free_inodes_count = __le32_to_cpu(super->s_free_inodes_count);
    super->s_first_data_block = __le32_to_cpu(super->s_first_data_block);
    super->s_log_block_size = __le32_to_cpu(super->s_log_block_size);
    super->s_log_frag_size = __le32_to_cpu(super->s_log_frag_size);
    super->s_blocks_per_group = __le32_to_cpu(super->s_blocks_per_group);
    super->s_frags_per_group = __le32_to_cpu(super->s_frags_per_group);
    super->s_inodes_per_group = __le32_to_cpu(super->s_inodes_per_group);
    super->s_mtime = __le32_to_cpu(super->s_mtime);
    super->s_wtime = __le32_to_cpu(super->s_wtime);
    super->s_mnt_count = __le16_to_cpu(super->s_mnt_count);
    super->s_max_mnt_count = __le16_to_cpu(super->s_max_mnt_count);
    super->s_magic = __le16_to_cpu(super->s_magic);
    super->s_state = __le16_to_cpu(super->s_state);
    super->s_errors = __le16_to_cpu(super->s_errors);
    super->s_minor_rev_level = __le16_to_cpu(super->s_minor_rev_level);
    super->s_lastcheck = __le32_to_cpu(super->s_lastcheck);
    super->s_checkinterval = __le32_to_cpu(super->s_checkinterval);
    super->s_creator_os = __le32_to_cpu(super->s_creator_os);
    super->s_rev_level = __le32_to_cpu(super->s_rev_level);
    super->s_def_resuid = __le16_to_cpu(super->s_def_resuid);
    super->s_def_resgid = __le16_to_cpu(super->s_def_resgid);
    super->s_first_ino = __le32_to_cpu(super->s_first_ino);
    super->s_inode_size = __le16_to_cpu(super->s_inode_size);
    super->s_block_group_nr = __le16_to_cpu(super->s_block_group_nr);
    super->s_feature_compat = __le32_to_cpu(super->s_feature_compat);
    super->s_feature_incompat = __le32_to_cpu(super->s_feature_incompat);
    super->s_feature_ro_compat = __le32_to_cpu(super->s_feature_ro_compat);
    super->s_algorithm_usage_bitmap =
            __le32_to_cpu(super->s_algorithm_usage_bitmap);
    super->s_journal_inum = __le32_to_cpu(super->s_journal_inum);
    super->s_journal_dev = __le32_to_cpu(super->s_journal_dev);
    super->s_last_orphan = __le32_to_cpu(super->s_last_orphan);
    super->s_hash_seed[0] = __le32_to_cpu(super->s_hash_seed[0]);
    super->s_hash_seed[1] = __le32_to_cpu(super->s_hash_seed[1]);
    super->s_hash_seed[2] = __le32_to_cpu(super->s_hash_seed[2]);
    super->s_hash_seed[3] = __le32_to_cpu(super->s_hash_seed[3]);
    super->s_default_mount_opts =
            __le32_to_cpu(super->s_default_mount_opts);
    super->s_first_meta_bg = __le32_to_cpu(super->s_first_meta_bg);
}


//void print_hex_memory(void *mem, int len) {
//  int i;
//  unsigned char *p = (unsigned char *)mem;
//  for (i=0;i<len;i++) {
//    printf("%02x", p[i]);
//  }
//  printf("\n");
//}


void ext2_read_block(register unsigned int fsblock) {
    register off_t physical_sector;
    register int i;
    
    ext2_volume->current_buffer = -1;
    
    //check if block is cached
    for (i = 0; i < ext2_volume->total_buffers; i++) {
        if (ext2_volume->buffer_blocks[i] == fsblock) {
            ext2_volume->current_buffer = i;
            return;
        }
    }
    
    //block not cached, find first free slot
    for (i = 0; i < ext2_volume->total_buffers; i++) {
        if (ext2_volume->buffer_blocks[i] == -1) {
            ext2_volume->current_buffer = i;
            break;
        }
    }

    if (ext2_volume->current_buffer == -1) {
        //block not cached and all buffers busy
        //force new buffer number
        if (fsblock == 0) {
            ext2_volume->current_buffer = 0;
        }
        else {
            ext2_volume->current_buffer = fsblock % ext2_volume->total_buffers;
        }
    }

    physical_sector = fsblock * (EXT2_BLOCK_SIZE(ext2_volume->super) / EXT2_SECTOR_SIZE);

    READ_SECTOR_INDIRECT(ext2_volume->dev, ext2_volume->start + physical_sector, &ext2_volume->buffer[ext2_volume->current_buffer * EXT2_BLOCK_SIZE(ext2_volume->super)], EXT2_BLOCK_SIZE(ext2_volume->super));

    ext2_volume->buffer_blocks[ext2_volume->current_buffer] = fsblock;
}


int ext2_volume_alloc_buffers(int total_buffers) {
    int i;

    if (ext2_volume->buffer_blocks) {
        free(ext2_volume->buffer_blocks);
        ext2_volume->buffer_blocks = 0;
    }
    if (ext2_volume->buffer) {
        free(ext2_volume->buffer);
        ext2_volume->buffer = 0;
    }

    ext2_volume->current_buffer = 0;
    ext2_volume->total_buffers = total_buffers;
    ext2_volume->buffer_blocks = malloc(sizeof(unsigned int) * ext2_volume->total_buffers);
    
    if (!ext2_volume->buffer_blocks) {
        return 0;
    }
    for (i = 0; i < ext2_volume->total_buffers; i++) {
        ext2_volume->buffer_blocks[i] = -1;
    }

    ext2_volume->buffer = (char*) malloc(ext2_volume->total_buffers * EXT2_BLOCK_SIZE(ext2_volume->super));
    if (ext2_volume->buffer == NULL) {
        free(ext2_volume->buffer_blocks);
        ext2_volume->buffer_blocks = 0;

        return 0;
    }
    
    return total_buffers;
}


int ext2_mount(mass_dev* dev, unsigned int start, unsigned int count) {
    struct ext2_super_block *super;
    int i;

    //printf("ext2_mount devId: %d, start: %d, count: %d\n", dev->devId, start, count);

    if (ext2_volume != NULL) {
        return -1;
    }

    super = (struct ext2_super_block*) malloc(sizeof (struct ext2_super_block));
    if (super == NULL)
        return -1;

    ext2_get_super(dev, super, start);
    if (super->s_magic != EXT2_SUPER_MAGIC) {
        free(super);
        return -1;
    }

    ext2_volume = (ext2_VOLUME*) malloc(sizeof (ext2_VOLUME));
    if (ext2_volume == NULL) {
        free(super);
        ext2_volume = NULL;
        return -1;
    }

    ext2_volume->super = super;

    if (!ext2_volume_alloc_buffers(1)) {
        free(super);
        free(ext2_volume);
        ext2_volume = NULL;
        return -1;
    }

    ext2_volume->dev = dev;
    ext2_volume->start = start;

    ext2_read_block(0);
    
    for (i = 0; i < EXT2_MAX_HANDLES; i++) {
        ext2_files[i] = NULL;
        ext2_dirs[i] = NULL;
    }

    return 1;
}


char *strdup (const char *s) {
    char *d = malloc (strlen (s) + 1);   // Space for length plus nul
    if (d == NULL) return NULL;          // No memory
    strcpy (d,s);                        // Copy the characters
    return d;                            // Return the new string
}


void ext2_get_group_desc(int group_id, struct ext2_group_desc *gdp)
{
	unsigned int block, offset;
	struct ext2_group_desc *le_gdp;
        
        //printf("ext2_get_group_desc devId: %d, group_id: %d\n", ext2_volume->dev->devId, group_id);

	block = 1 + ext2_volume->super->s_first_data_block;
	block += group_id / EXT2_DESC_PER_BLOCK(ext2_volume->super);
	ext2_read_block(block);

	offset = group_id % EXT2_DESC_PER_BLOCK(ext2_volume->super);
	offset *= sizeof(*gdp);

	le_gdp = (struct ext2_group_desc *)(&ext2_volume->buffer[ext2_volume->current_buffer * EXT2_BLOCK_SIZE(ext2_volume->super)] + offset);

	gdp->bg_block_bitmap = __le32_to_cpu(le_gdp->bg_block_bitmap);
	gdp->bg_inode_bitmap = __le32_to_cpu(le_gdp->bg_inode_bitmap);
	gdp->bg_inode_table = __le32_to_cpu(le_gdp->bg_inode_table);
	gdp->bg_free_blocks_count = __le16_to_cpu(le_gdp->bg_free_blocks_count);
	gdp->bg_free_inodes_count = __le16_to_cpu(le_gdp->bg_free_inodes_count);
	gdp->bg_used_dirs_count = __le16_to_cpu(le_gdp->bg_used_dirs_count);
}


unsigned int ext2_get_ino_block_sector(unsigned int ino)
{
	struct ext2_group_desc desc;
	unsigned int block;
	unsigned int group_id;

        //printf("ext2_get_ino_block devId: %d, ino: %d\n", ext2_volume->dev->devId, ino);

	ino--;

	group_id = ino / EXT2_INODES_PER_GROUP(ext2_volume->super);
	ext2_get_group_desc(group_id, &desc);

	ino %= EXT2_INODES_PER_GROUP(ext2_volume->super);

	block = desc.bg_inode_table;
	block += ino / (EXT2_BLOCK_SIZE(ext2_volume->super) /
			EXT2_INODE_SIZE(ext2_volume->super));
        
        block = block * (EXT2_BLOCK_SIZE(ext2_volume->super) / EXT2_SECTOR_SIZE);

        return ext2_volume->start + block;
}


unsigned int ext2_get_ino_block_offset(unsigned int ino)
{
	struct ext2_group_desc desc;
	unsigned int offset;
	unsigned int group_id;

        //printf("ext2_get_ino_block_offset devId: %d, ino: %d\n", ext2_volume->dev->devId, ino);

	ino--;

	group_id = ino / EXT2_INODES_PER_GROUP(ext2_volume->super);
	ext2_get_group_desc(group_id, &desc);

	ino %= EXT2_INODES_PER_GROUP(ext2_volume->super);
	offset = ino % (EXT2_BLOCK_SIZE(ext2_volume->super) /
			EXT2_INODE_SIZE(ext2_volume->super));
	offset *= EXT2_INODE_SIZE(ext2_volume->super);
        
        return offset;
}


int ext2_get_inode(unsigned int ino, struct ext2_inode *inode)
{
	struct ext2_group_desc desc;
	unsigned int block;
	unsigned int group_id;
	unsigned int offset;
	struct ext2_inode *le_inode;
	int i, is_root;

        //printf("ext2_get_inode devId: %d, ino: %d\n", ext2_volume->dev->devId, ino);

        is_root = ino == EXT2_ROOT_INO;

	ino--;

	group_id = ino / EXT2_INODES_PER_GROUP(ext2_volume->super);
	ext2_get_group_desc(group_id, &desc);

	ino %= EXT2_INODES_PER_GROUP(ext2_volume->super);

	block = desc.bg_inode_table;
	block += ino / (EXT2_BLOCK_SIZE(ext2_volume->super) /
			EXT2_INODE_SIZE(ext2_volume->super));

	ext2_read_block(block);

	offset = ino % (EXT2_BLOCK_SIZE(ext2_volume->super) /
			EXT2_INODE_SIZE(ext2_volume->super));
	offset *= EXT2_INODE_SIZE(ext2_volume->super);

	le_inode = (struct ext2_inode *)(&ext2_volume->buffer[ext2_volume->current_buffer * EXT2_BLOCK_SIZE(ext2_volume->super)] + offset);

	inode->i_mode = __le16_to_cpu(le_inode->i_mode);
	inode->i_uid = __le16_to_cpu(le_inode->i_uid);
	inode->i_size = __le32_to_cpu(le_inode->i_size);
	inode->i_atime = __le32_to_cpu(le_inode->i_atime);
	inode->i_ctime = __le32_to_cpu(le_inode->i_ctime);
	inode->i_mtime = __le32_to_cpu(le_inode->i_mtime);
	inode->i_dtime = __le32_to_cpu(le_inode->i_dtime);
	inode->i_gid = __le16_to_cpu(le_inode->i_gid);
	inode->i_links_count = __le16_to_cpu(le_inode->i_links_count);
	inode->i_blocks = __le32_to_cpu(le_inode->i_blocks);
	inode->i_flags = __le32_to_cpu(le_inode->i_flags);

	if (S_ISLNK(inode->i_mode)) {
            //symlinks are not supported
            return -1;
	} else {
            for (i = 0; i < EXT2_N_BLOCKS; i++)
                    inode->i_block[i] = __le32_to_cpu(le_inode->i_block[i]);
        }

	inode->i_generation = __le32_to_cpu(le_inode->i_generation);
	inode->i_file_acl = __le32_to_cpu(le_inode->i_file_acl);
	inode->i_dir_acl = __le32_to_cpu(le_inode->i_dir_acl);
	inode->i_faddr = __le32_to_cpu(le_inode->i_faddr);
	inode->osd2.linux2.l_i_frag = le_inode->osd2.linux2.l_i_frag;
	inode->osd2.linux2.l_i_fsize = le_inode->osd2.linux2.l_i_fsize;
	inode->osd2.linux2.l_i_uid_high =
			__le16_to_cpu(le_inode->osd2.linux2.l_i_uid_high);
	inode->osd2.linux2.l_i_gid_high =
			__le16_to_cpu(le_inode->osd2.linux2.l_i_gid_high);
        
        if (is_root && inode->i_size == 0) {
            //printf("ext2_get_inode root inode has %d size, exiting\n", inode->i_size);
            return -1;
        }

	return 0;
}


unsigned int ext2_get_block_addr(struct ext2_inode *inode,
				 unsigned int logical)
{
	register unsigned int physical;
	register unsigned int addr_per_block;

        //printf("ext2_get_block_addr devId: %d, logical: %d\n", ext2_volume->dev->devId, logical);

	/* direct */
        
	if (logical < EXT2_NDIR_BLOCKS) {
		physical = inode->i_block[logical];
		return physical;
	}

	/* indirect */

	logical -= EXT2_NDIR_BLOCKS;

	addr_per_block = EXT2_ADDR_PER_BLOCK (ext2_volume->super);
	if (logical < addr_per_block) {
		ext2_read_block(inode->i_block[EXT2_IND_BLOCK]);
		physical = __le32_to_cpu(((unsigned int *)&ext2_volume->buffer[ext2_volume->current_buffer * EXT2_BLOCK_SIZE(ext2_volume->super)])[logical]);
		return physical;
	}

	/* double indirect */

	logical -=  addr_per_block;

	if (logical < addr_per_block * addr_per_block) {
		ext2_read_block(inode->i_block[EXT2_DIND_BLOCK]);
		physical = __le32_to_cpu(((unsigned int *)&ext2_volume->buffer[ext2_volume->current_buffer * EXT2_BLOCK_SIZE(ext2_volume->super)])
						[logical / addr_per_block]);
		ext2_read_block(physical);
		physical = __le32_to_cpu(((unsigned int *)&ext2_volume->buffer[ext2_volume->current_buffer * EXT2_BLOCK_SIZE(ext2_volume->super)])
						[logical % addr_per_block]);
		return physical;
	}

	/* triple indirect */

	logical -= addr_per_block * addr_per_block;
        ext2_read_block(inode->i_block[EXT2_TIND_BLOCK]);
	physical = __le32_to_cpu(((unsigned int *)&ext2_volume->buffer[ext2_volume->current_buffer * EXT2_BLOCK_SIZE(ext2_volume->super)])
				[logical / (addr_per_block * addr_per_block)]);
	ext2_read_block(physical);
	logical = logical % (addr_per_block * addr_per_block);
	physical = __le32_to_cpu(((unsigned int *)&ext2_volume->buffer[ext2_volume->current_buffer * EXT2_BLOCK_SIZE(ext2_volume->super)])[logical / addr_per_block]);
	ext2_read_block(physical);
	physical = __le32_to_cpu(((unsigned int *)&ext2_volume->buffer[ext2_volume->current_buffer * EXT2_BLOCK_SIZE(ext2_volume->super)])[logical % addr_per_block]);
	return physical;
}


/*
 * bitwise divide, useful when we do not want to link with lgcc
 * http://stackoverflow.com/questions/2776211/how-can-i-multiply-and-divide-using-only-bit-shifting-and-adding
 * 8 post
 * modified arguments
 */
int No_divide(register unsigned long long ullDividend, register unsigned long long ullDivisor, int *nRemainder)
{
    register int nQuotient = 0;
    register int nPos = -1;

    while (ullDivisor <  ullDividend) 
    {
        ullDivisor <<= 1;
        nPos ++;
    }

    ullDivisor >>= 1;

    while (nPos > -1)
    {
        if (ullDividend >= ullDivisor)
        {
            nQuotient += (1 << nPos);                        
            ullDividend -= ullDivisor;  
        }

        ullDivisor >>= 1;
        nPos -= 1;
    }

    if (nRemainder) {
        *nRemainder = (int) ullDividend;
    }

    return nQuotient;
}


int ext2_read_data(struct ext2_inode *inode,
		   long long offset, char *buffer, register size_t length)
{
	register unsigned int logical, physical;
	int blocksize = EXT2_BLOCK_SIZE(ext2_volume->super);
	int shift;
	register size_t read;
        long long real_size;

        //printf("ext2_read_data devId: %d, offset: %lu, length: %d\n", ext2_volume->dev->devId, offset, length);
        
	if (offset == inode->i_size && S_ISDIR(inode->i_mode)) {    //TODO check
            return -1;
        }
        
        real_size = ((unsigned long long)inode->i_dir_acl << 32) | inode->i_size;
        if (offset >= real_size) {
            offset = real_size - 1;
        }
	if (offset + length >= real_size)
		length = real_size - offset;

	read = 0;
        logical = No_divide(offset, blocksize, &shift);

	if (shift) {
		physical = ext2_get_block_addr(inode, logical);
		ext2_read_block(physical);

		if (length < blocksize - shift) {
			memcpy(buffer, &ext2_volume->buffer[ext2_volume->current_buffer * EXT2_BLOCK_SIZE(ext2_volume->super)] + shift, length);
			return length;
		}
		read += blocksize - shift;
		memcpy(buffer, &ext2_volume->buffer[ext2_volume->current_buffer * EXT2_BLOCK_SIZE(ext2_volume->super)] + shift, read);

		buffer += read;
		length -= read;
		logical++;
	}

	while (length) {
		physical = ext2_get_block_addr(inode, logical);
		ext2_read_block(physical);

		if (length < blocksize) {
			memcpy(buffer, &ext2_volume->buffer[ext2_volume->current_buffer * EXT2_BLOCK_SIZE(ext2_volume->super)], length);
			read += length;
			return read;
		}
		memcpy(buffer, &ext2_volume->buffer[ext2_volume->current_buffer * EXT2_BLOCK_SIZE(ext2_volume->super)], blocksize);

		buffer += blocksize;
		length -= blocksize;
		read += blocksize;
		logical++;
	}

	return read;
}


off_t ext2_dir_entry(struct ext2_inode *inode,
		     off_t index, struct ext2_dir_entry_2 *entry)
{
	int ret;

        //printf("ext2_dir_entry devId: %d, index: %lu\n", ext2_volume->dev->devId, index);

	ret = ext2_read_data(inode, index,
			     (char*)entry, sizeof(*entry));
	if (ret == -1) {
		return -1;
        }

        entry->inode = __le32_to_cpu(entry->inode);
        entry->rec_len = __le16_to_cpu(entry->rec_len);
	return index + entry->rec_len;
}


unsigned int ext2_seek_name(const char *name)
{
	struct ext2_inode inode;
	int ret;
	unsigned int ino;
	off_t index;
	struct ext2_dir_entry_2 entry;

        //printf("ext2_seek_name devId: %d, name: %s\n", ext2_volume->dev->devId, name);
        
	ino = EXT2_ROOT_INO;
	while(1) {
		while (*name == '/')
			name++;
		if (!*name)
		    break;
		ret = ext2_get_inode(ino, &inode);
		if (ret == -1) {
			return 0;
                }

		index = 0;
		while (1) {
			index = ext2_dir_entry(&inode, index, &entry);
			if (index == -1){
				return 0;
                        }

			ret = strncmp(name, entry.name, entry.name_len);
			if (ret == 0  &&
			    (name[entry.name_len] == 0 ||
			     name[entry.name_len] == '/')) {
			     	ino = entry.inode;
				break;
			}
		}
		name += entry.name_len;
	}

	return ino;
}


int ext2_lookup_inode_data(const char *pathname, struct ext2_inode *inode) {
    int ino;
    int ret;

    ino = ext2_seek_name(pathname);
    if (ino == 0) {
        return -ENOENT;
    }

    ret = ext2_get_inode(ino, inode);
    if (ret == -1) {
        return -EIO;
    }
    
    return 1;
}


unsigned int ext2_get_inode_sectors_map(struct ext2_inode *inode, unsigned char *mapBuff, int mapBuffLen) {
    register unsigned int entries = 0;
    register unsigned int logical, physical, last_physical, change;
    register int blocksize = EXT2_BLOCK_SIZE(ext2_volume->super);
    register int length;
    register size_t read;
    register long long real_size, offset = 0;
    register int entry_addr = 0;
    unsigned int start_sector, end_sector, holds;
    int shift;

    memset(mapBuff, 0, mapBuffLen);

    last_physical = ext2_get_block_addr(inode, 0);
    start_sector = ext2_volume->start + (last_physical * (blocksize / EXT2_SECTOR_SIZE));
    memcpy(mapBuff + entry_addr, &start_sector, 4);   //copy start sector
    entry_addr += 4;
    entries++;

    real_size = ((unsigned long long)inode->i_dir_acl << 32) | inode->i_size;
    while (offset < real_size) {
        if (entry_addr > mapBuffLen) {
            return -1;
        }

        length = blocksize;

        if (offset >= real_size) {
            break;
        }
	if (offset + length >= real_size) {
            length = real_size - offset;
        }

        read = 0;
        logical = No_divide(offset, blocksize, &shift);

        if (shift) {
                physical = ext2_get_block_addr(inode, logical);
                if (physical != last_physical) {
                    change = physical - last_physical;
                    if (change > 1 || change < 0) {
                        end_sector = ext2_volume->start + (last_physical * (blocksize / EXT2_SECTOR_SIZE));
                        end_sector += blocksize / 512 - 1;
                        holds = end_sector - start_sector;
                        memcpy(mapBuff + entry_addr, &holds, 4);       //copy sectors to end
                        entry_addr += 4;

                        start_sector = ext2_volume->start + (physical * (blocksize / EXT2_SECTOR_SIZE));
                        memcpy(mapBuff + entry_addr, &start_sector, 4);       //copy start sector
                        entry_addr += 4;

                        entries++;
                    }
                    last_physical = physical;
                }

                if (length < blocksize - shift) {
                        end_sector = ext2_volume->start + (last_physical * (blocksize / EXT2_SECTOR_SIZE));
                        end_sector += blocksize / 512 - 1;
                        holds = end_sector - start_sector;
                        memcpy(mapBuff + entry_addr, &holds, 4);       //copy sectors to end

                        return entries;
                }
                read += blocksize - shift;

                length -= read;
                logical++;
        }

        while (length) {
                physical = ext2_get_block_addr(inode, logical);
                if (physical != last_physical) {
                    change = physical - last_physical;
                    if (change > 1 || change < 0) {
                        end_sector = ext2_volume->start + (last_physical * (blocksize / EXT2_SECTOR_SIZE));
                        end_sector += blocksize / 512 - 1;
                        holds = end_sector - start_sector;
                        memcpy(mapBuff + entry_addr, &holds, 4);       //copy sectors to end
                        entry_addr += 4;

                        start_sector = ext2_volume->start + (physical * (blocksize / EXT2_SECTOR_SIZE));
                        memcpy(mapBuff + entry_addr, &start_sector, 4);       //copy start sector
                        entry_addr += 4;

                        entries++;
                    }
                    last_physical = physical;
                }

                if (length < blocksize) {
                        end_sector = ext2_volume->start + (last_physical * (blocksize / EXT2_SECTOR_SIZE));
                        end_sector += blocksize / 512 - 1;
                        holds = end_sector - start_sector;
                        memcpy(mapBuff + entry_addr, &holds, 4);       //copy sectors to end

                        return entries;
                }

                length -= blocksize;
                read += blocksize;
                logical++;
        }


        offset += blocksize;
    }

    end_sector = ext2_volume->start + (last_physical * (blocksize / EXT2_SECTOR_SIZE));
    end_sector += blocksize / 512 - 1;
    holds = end_sector - start_sector;
    memcpy(mapBuff + entry_addr, &holds, 4);       //copy sectors to end

    return entries;
}


/*
 * set or release ext2_files/ext2_dirs slot
 */
int ext2_affect_slot(void **arr, void *ptr, int set) {
    int i, success = 0;

    //printf("* ext2_affect_slot arr=%p ptr=%p set=%d\n", arr, ptr, set);

    for (i = 0; i < EXT2_MAX_HANDLES; i++) {
        if (set && !arr[i]) {
            arr[i] = ptr;
            success = 1;
        }
        else {
            if (arr[i] == ptr) {
                arr[i] = 0;
                success = 1;
            }
        }            

        if (success) {
            //printf("* ext2_affect_slot success arr=%p ptr=%p set=%d\n", arr, ptr, set);
            return 1;
        }
    }

    return 0;
}


int ext2_umount() {
    ext2_FILE *file;
    ext2_DIR *dir;
    int i;

    if (ext2_volume == NULL)
        return -1;
    
//    //printf("* ext2_umount devId: %d\n", ext2_volume->dev->devId);
    
    _fs_lock();

    free(ext2_volume->super);
    ext2_volume->super = 0;
    
    free(ext2_volume->buffer);
    ext2_volume->buffer = 0;
    
    free(ext2_volume->buffer_blocks);
    ext2_volume->buffer_blocks = 0;

    free(ext2_volume);
    ext2_volume = 0;
    
    //free handles
    for (i = 0; i < EXT2_MAX_HANDLES; i++) {
        if (ext2_files[i]) {
            file = (ext2_FILE*) ext2_files[i];
            if (file->path) {
                free(file->path);
                file->path = 0;
            }
            if (file->inode) {
                free(file->inode);
                file->inode = 0;
            }
            free(file);
            ext2_files[i] = 0;
        }
        
        if (ext2_dirs[i]) {
            dir = (ext2_DIR*) ext2_dirs[i];
            if (dir->inode) {
                free(dir->inode);
                dir->inode = 0;
            }

            free(dir);
            ext2_dirs[i] = 0;
        }
    }

    _fs_unlock();
    return 0;
}


//7.
int ext2_fs_close(iop_file_t* fd) {
    ext2_FILE *file = fd->privdata;

    //printf("* ext2_fs_close\n");
    
    _fs_lock();
    
    if (!file) {
        _fs_unlock();
        return -EBADF;
    }
    
    free(file->inode);
    free(file->path);
    free(file);

    ext2_affect_slot(ext2_files, fd->privdata, 0);
    fd->privdata = 0;
    
    //printf("* ext2_fs_close success\n");
    
    _fs_unlock();
    return 0;
}


int ext2_fs_dummy(void) {
    return -EIO;
}


int ext2_fs_lseek(iop_file_t* fd, unsigned long long offset, int whence) {
    ext2_FILE *file = fd->privdata;
    long long new_offset, real_size;

    //printf("* ext2_fs_lseek offset: %lu, whence: %d\n", offset, whence);
    
    _fs_lock();
    
    if (!file) {
        _fs_unlock();
        return -EBADF;
    }

    real_size = ((unsigned long long)file->inode->i_dir_acl << 32) | file->inode->i_size;
    
    switch(whence)
    {
    case SEEK_SET:
            new_offset = offset;
            break;
    case SEEK_CUR:
            new_offset = file->offset + offset;
            break;
    case SEEK_END:
            new_offset = real_size + offset;
            break;
    default:
            _fs_unlock();
            return -EINVAL;
    }

    if ( (new_offset < 0) ||
         (new_offset > real_size) ) {
        _fs_unlock();
        return -ENXIO;
    }

    file->offset = new_offset;

    _fs_unlock();
    return new_offset;
}


//1.
int ext2_fs_open(iop_file_t* fd, const char *pathname, int mode) {
    ext2_FILE *file;
    struct ext2_inode *inode;
    int ret;

    //printf("* ext2_fs_open pathname: %s, mode: %d\n", pathname, mode);
    
    _fs_lock();

#ifndef WRITE_SUPPORT
    //read only support
    if (mode != 0 && mode != O_RDONLY) { //correct O_RDONLY  number?
//        XPRINTF("USBHDFSD: mode (%d) != O_RDONLY (%d) \n", mode, O_RDONLY);
        _fs_unlock();
        return -EROFS;
    }
#endif

    inode = (struct ext2_inode*) malloc(sizeof (struct ext2_inode));
    if (inode == NULL) {
        _fs_unlock();
        return -ENOMEM;
    }

    ret = ext2_lookup_inode_data(pathname, inode);
    if (ret < 0) {
        _fs_unlock();
        free(inode);
        return ret;
    }

    file = (ext2_FILE*) malloc(sizeof (ext2_FILE));
    if (file == NULL) {
        _fs_unlock();
        free(inode);
        return -ENOMEM;
    }
    
    if (!ext2_affect_slot(ext2_files, file, 1)) {
        _fs_unlock();
        free(inode);
        return -EMFILE;
    }

    file->volume = ext2_volume;
    file->inode = inode;
    file->offset = 0;
    file->path = strdup(pathname);
    
    fd->privdata = file;

    //printf("* ext2_fs_open success pathname: %s\n", pathname);

    _fs_unlock();
    return 1;
}


void ext2_inode2stat(struct ext2_inode *inode, fio_stat_t *stat) {
    memset(stat, 0, sizeof (fio_stat_t));
    stat->mode = FIO_SO_IROTH | FIO_SO_IXOTH;

    if (S_ISDIR(inode->i_mode)) {
        stat->mode |= FIO_SO_IFDIR;
    }
    else {
        stat->mode |= FIO_SO_IFREG;
    }

    //size is 32bits, so we cannot pass long long real_size here
    stat->size = inode->i_size;

    //todo - convert timestamp to datetime
    memcpy(stat->ctime, &inode->i_ctime, sizeof(uint32_t));
    memcpy(&stat->ctime[3], &inode->i_ctime, sizeof(uint32_t));

    memcpy(stat->atime, &inode->i_atime, sizeof(uint32_t));
    memcpy(&stat->atime[3], &inode->i_atime, sizeof(uint32_t));

    memcpy(stat->mtime, &inode->i_mtime, sizeof(uint32_t));
    memcpy(&stat->mtime[3], &inode->i_mtime, sizeof(uint32_t));
}


//2.
int ext2_fs_getstat(iop_file_t *fd, const char *pathname, fio_stat_t *stat) {
    struct ext2_inode *inode;
    int ret;

    //printf("* ext2_fs_getstat pathname: %s\n", pathname);
    
    _fs_lock();

    inode = (struct ext2_inode*) malloc(sizeof (struct ext2_inode));
    if (inode == NULL) {
        _fs_unlock();
        return -ENOMEM;
    }

    ret = ext2_lookup_inode_data(pathname, inode);
    if (ret < 0) {
        _fs_unlock();
        free(inode);
        return ret;
    }

    ext2_inode2stat(inode, stat);
    free(inode);

    //printf("* ext2_fs_getstat success pathname: %s\n", pathname);
    
    _fs_unlock();
    return 0;
}


//3.
int ext2_fs_dopen(iop_file_t *fd, const char *name) {
    ext2_DIR* dir;
    struct ext2_inode *inode;
    int ret;

    //printf("* ext2_fs_dopen name: %s\n", name);
    
    _fs_lock();

    inode = (struct ext2_inode*)malloc(sizeof(struct ext2_inode));
    if (inode == NULL) {
        _fs_unlock();
        return -ENOMEM;
    }

    ret = ext2_lookup_inode_data(name, inode);
    if (ret < 0) {
        _fs_unlock();
        free(inode);
        return ret;
    }

    if (!S_ISDIR(inode->i_mode)) {
            _fs_unlock();
            free(inode);
            return -ENOTDIR;
    }

    dir = (ext2_DIR*)malloc(sizeof(ext2_DIR));
    if (dir == NULL) {
            _fs_unlock();
            free(inode);
            return -ENOMEM;
    }
    if (!ext2_affect_slot(ext2_dirs, dir, 1)) {
        _fs_unlock();
        free(inode);
        return -EMFILE;
    }

    dir->volume = (ext2_VOLUME*)ext2_volume;
    dir->inode = inode;
    dir->index = 0;

    fd->privdata = dir;
    
    //printf("* ext2_fs_dopen success name: %s\n", name);

    _fs_unlock();
    return 1;
}


//6.
int ext2_fs_read(iop_file_t* fd, void * buffer, int size) {
    ext2_FILE *file = fd->privdata;
    int ret;

    //printf("* ext2_fs_read size: %d\n", size);
    
    _fs_lock();
    
    if (!file) {
        _fs_unlock();
        return -EBADF;
    }
    
    ret = ext2_read_data(file->inode, file->offset, buffer, size);
    if (ret == -1) {
        _fs_unlock();
        return -EIO;
    }

    file->offset += ret;

    //printf("* ext2_fs_read success size: %d\n", size);
    
    _fs_unlock();
    
    return ret;
}


//4.
int ext2_fs_dread(iop_file_t *fd, fio_dirent_t *buffer) {
    ext2_DIR *dir = fd->privdata;
    struct ext2_dir_entry_2 entry;
    int ret;
    
    //printf("* ext2_fs_dread\n");
    
    _fs_lock();
    
    if (!dir) {
        _fs_unlock();
        //printf("* ext2_fs_dread dir closed?\n");
        return -EBADF;
    }

    ret = ext2_dir_entry(dir->inode, dir->index, &entry);
    if (ret == -1) {
        _fs_unlock();
        //printf("* ext2_fs_dread success end\n");
        return 0;
    }
    dir->index = ret;

    entry.name[entry.name_len] = 0;

    memset(buffer, 0, sizeof (fio_dirent_t));
    ext2_inode2stat(dir->inode, &buffer->stat);
    strcpy(buffer->name, (const char*) entry.name);

    //printf("* ext2_fs_dread success\n");
    
    _fs_unlock();
    return 1;
}


//5.
int ext2_fs_closedir(iop_file_t* fd) {
    ext2_DIR *dir = fd->privdata;
    
    //printf("* ext2_fs_closedir\n");

    _fs_lock();
    
    if (!dir) {
        _fs_unlock();
        //printf("* ext2_fs_closedir dir closed?\n");
        return -EBADF;
    }

    free(dir->inode);
    free(dir);

    ext2_affect_slot(ext2_dirs, fd->privdata, 0);
    fd->privdata = 0;
    
    //printf("* ext2_fs_closedir success\n");
    
    _fs_unlock();
    return 0;
}


/* rest */
int ext2_fs_write(iop_file_t* fd, void * buffer, int size) {
    //printf("* ext2_fs_write size: %d\n", size);
    return ext2_fs_dummy();
}


int ext2_pack_inode_sectors_map(unsigned char *mapBuff, int mapBuffLen, unsigned char *targetMapBuff, int targetMapBuffLen) {
    unsigned int src_entry, src_holds, src_entry_addr = 0;
    unsigned int src_entry2, src_holds2, src_entry_addr2 = 0;
    unsigned int dst_entry_addr = 0;
    unsigned int dups, tmp;


    while (src_entry_addr < mapBuffLen) {
        memcpy(&src_entry, mapBuff + src_entry_addr, 4);
        memcpy(&src_holds, mapBuff + src_entry_addr + 4, 4);
        
        if (!src_entry || !src_holds) {
            break;
        }

        if (src_holds > 0xFFFFF) {
            //file is too much fragmented
            return -1;
        }

        src_entry_addr += 8;

        if (dst_entry_addr > targetMapBuffLen) {
            //file is too much fragmented
            return -1;
        }

        memcpy(targetMapBuff + dst_entry_addr, &src_entry, 4);
        dst_entry_addr += 4;

        //check if holds are duplicated
        dups = 0;
        src_entry_addr2 = src_entry_addr;
        while (src_entry_addr2 < mapBuffLen) {
            memcpy(&src_entry2, mapBuff + src_entry_addr2, 4);
            memcpy(&src_holds2, mapBuff + src_entry_addr2 + 4, 4);

            src_entry_addr2 += 8;
            
            if (src_holds2 != src_holds) {
                break;
            }

            dups++;
        }
        
        if (dups > 255) {
            //file too much fragmented
            return -1;
        }

        if (dups == 0) {
            //no duplicates, save holds in target array
            memcpy(targetMapBuff + dst_entry_addr, &src_holds, 4);
            dst_entry_addr += 4;
        }
        else {
            //there are duplicates, save holds struct

            /*
             * holds struct:
             * ABBCCCCC
             * A - flag (8 decimal == 1000 binary, or 0 is not holds struct)
             * B - count duplicates/dups
             * C - holds
             */
            tmp = 0x80000000 | dups << 20 | src_holds;
            memcpy(targetMapBuff + dst_entry_addr, &tmp, 4);
            dst_entry_addr += 4;

            //..and add each sector
            while (dups > 0) {
                memcpy(&src_entry, mapBuff + src_entry_addr, 4);
                memcpy(&src_holds, mapBuff + src_entry_addr + 4, 4);
                
                if (!src_entry || !src_holds) {
                    break;
                }

                if (src_holds > 0xFFFFF) {
                    //file is too much fragmented
                    return -1;
                }

                src_entry_addr += 8;

                memcpy(targetMapBuff + dst_entry_addr, &src_entry, 4);
                dst_entry_addr += 4;

                dups--;
            }

            if (!src_entry || !src_holds) {
                break;
            }
        }
    }

    if (!dst_entry_addr) {
        return 0;
    }

    return dst_entry_addr / 4;
}


//int ext2_read_file_sector_by_map(unsigned char *mapBuff, register int mapBuffLen, register unsigned int sector, unsigned char *buff) {
//    register int entry_addr = 0;
//    register unsigned int start_sector = 0, holds = 0, same_holds = 0;
//    register unsigned int passed = 0, past_passed = 0;
//    unsigned char *sbuf;
//    register int i;
//
//    while (entry_addr < mapBuffLen) {
//        start_sector = 
//            (mapBuff + entry_addr)[0] + 
//            ((mapBuff + entry_addr)[1] << 8) + 
//            ((mapBuff + entry_addr)[2] << 16) + 
//            ((mapBuff + entry_addr)[3] << 24);
//        entry_addr += 4;
//
//        if (same_holds == 0) {
//            holds = 
//                (mapBuff + entry_addr)[0] + 
//                ((mapBuff + entry_addr)[1] << 8) + 
//                ((mapBuff + entry_addr)[2] << 16) + 
//                ((mapBuff + entry_addr)[3] << 24);
//            entry_addr += 4;
//            
//            if (holds > 0 && (holds & 0x80000000) == 0 && holds % 2 != 0) {
//                holds++;
//            }
//        }
//        else {
//            same_holds--;
//        }
//
//        if (!start_sector || !holds) {
//            //should not get here if sector is correct
//            return -1;
//        }
//
//        if ((holds & 0x80000000) != 0) {
//            same_holds = (holds & 0x0FF00000) >> 20;
//            holds = (holds & 0x000FFFFF) + 1;
//        }
//
//        passed += holds;
//
//        if (sector < passed) {
//            READ_SECTOR(ext2_volume->dev, start_sector + sector - past_passed, sbuf);
//            for (i = 0; i < 512; i++) {
//                buff[i] = sbuf[i];
//            }
//
//            return 1;
//        }
//
//        past_passed += holds;
//    }
//
//    return 0;
//}


int ext2_fs_ioctl(iop_file_t *fd, unsigned long request, void *data) {
    struct ext2_inode *inode;
    unsigned int physical;
    int ret = ext2_fs_dummy();
    unsigned int entry_addr = 0;
    int entries;
    unsigned char *ext2_tmp_file_sectors;
//    long long offset = 0, real_size;
//    unsigned char secBuff1[512];
//    unsigned char secBuff2[512];
//    unsigned int last_start_sector = -1, start_sector;
//    unsigned int end_sector, max, holds;
//    unsigned int secNo = 0;

    //printf("* ext2_fs_ioctl request: %lu\n", request);

    _fs_lock();
    
    if (!fd->privdata) {
        _fs_unlock();
        return -EBADF;
    }

    switch (request) {
        case IOCTL_CKFREE: //Request to calculate free space (ignore file/folder selected)
            //printf("* ext2_fs_ioctl IOCTL_CKFREE\n");
            ret = ext2_fs_dummy();
            break;
        case IOCTL_RENAME: //Request to rename opened file/folder
            //printf("* ext2_fs_ioctl IOCTL_RENAME\n");
            ret = ext2_fs_dummy();
            break;
        case IOCTL_GETCLUSTER:
            //printf("* ext2_fs_ioctl IOCTL_GETCLUSTER: %s\n", (char *) data);
            
            inode = (struct ext2_inode*)malloc(sizeof(struct ext2_inode));
            if (inode == NULL) {
                _fs_unlock();
                return -ENOMEM;
            }

            ret = ext2_lookup_inode_data((char *) data, inode);
            if (ret < 0) {
                _fs_unlock();
                free(inode);
                return ret;
            }

            physical = ext2_get_block_addr(inode, 0);
            ret = ext2_volume->start + (physical * (EXT2_BLOCK_SIZE(ext2_volume->super) / EXT2_SECTOR_SIZE));

            free(inode);

            break;
        case IOCTL_GETDEVSECTORSIZE:
            ret = mass_stor_sectorsize(ext2_volume->dev);
            break;
        case 0x1337C0DE:
            ret = 0x83;
            break;
        case IOCTL_CHECKCHAIN:
            //printf("* ext2_fs_ioctl IOCTL_CHECKCHAIN: %s\n", (char *) data);
            ret = 1;    //by default not fragmented 
            //because ext2 driver will handle fragmentation
            break;
        case IOCTL_DEVID:
            ret = ext2_volume->dev->devId;
            break;
    }

    if (ret < 0 && request >= IOCTL_SECTORS_MAP_START && request <= (IOCTL_SECTORS_MAP_START | EXT2_SECTORS_BYTES)) {
        if (request == IOCTL_SECTORS_MAP_START) {
            //first time - read sectors, this will take a while
            inode = (struct ext2_inode*)malloc(sizeof(struct ext2_inode));
            if (inode == NULL) {
                _fs_unlock();
                return -ENOMEM;
            }

            ret = ext2_lookup_inode_data((char *) data, inode);
            if (ret < 0) {
                _fs_unlock();
                free(inode);
                return ret;
            }

            if (!ext2_volume_alloc_buffers(20)) {
                ext2_umount();
                return -1;
            }

            ext2_tmp_file_sectors = malloc(EXT2_TMP_SECTORS_BYTES);
            if (!ext2_tmp_file_sectors) {
                _fs_unlock();
                free(inode);
                return 0xffffffff;
            }
            memset(ext2_tmp_file_sectors, 0, EXT2_TMP_SECTORS_BYTES);
            
            if ((entries = ext2_get_inode_sectors_map(inode, ext2_tmp_file_sectors, EXT2_TMP_SECTORS_BYTES)) < 0) {
                //file is too much fragmented - error
                _fs_unlock();
                free(inode);
                return 0xffffffff;
            }

            if (!ext2_volume_alloc_buffers(1)) {
                ext2_umount();
                return -1;
            }


//            entry_addr = 0;
//            while (entries > 0) {
//                memcpy(&start_sector, ext2_tmp_file_sectors + entry_addr, 4);
//                memcpy(&end_sector, ext2_tmp_file_sectors + entry_addr + 4, 4);
//                printf("%d - %d\n", start_sector, end_sector);
//
//                entry_addr += 8;
//                entries--;
//            }
//
//            printf("\n");
            
            ext2_file_sectors = malloc(EXT2_SECTORS_BYTES);
            if (!ext2_file_sectors) {
                _fs_unlock();
                free(inode);
                free(ext2_tmp_file_sectors);
                return 0xffffffff;
            }
            memset(ext2_file_sectors, 0, EXT2_SECTORS_BYTES);


            if ((entries = ext2_pack_inode_sectors_map(ext2_tmp_file_sectors, EXT2_TMP_SECTORS_BYTES, ext2_file_sectors, EXT2_SECTORS_BYTES)) < 0) {
                //file is too much fragmented - error
                _fs_unlock();
                free(inode);
                free(ext2_tmp_file_sectors);
                free(ext2_file_sectors);
                return 0xffffffff;
            }

            free(ext2_tmp_file_sectors);

            if (entries < 0) {
                _fs_unlock();
                free(inode);
                free(ext2_file_sectors);
                return 0xffffffff;
            }

//            printf("entries: %d\n", entries);
//            entry_addr = 0;
//            while (entries > 0) {
//                memcpy(&start_sector, ext2_file_sectors + entry_addr, 4);
//                
//                if ((start_sector & 0x80000000) != 0) {
//                    //holds struct
//                    printf("0x%X\n", start_sector);
//                }
//                else {
//                    printf("%d\n", start_sector);
//                }
//
//                entry_addr += 4;
//                entries--;
//            }
//            printf("\n");

            
            
//            if ((entries = ext2_pack_inode_sectors_map(inode, ext2_file_sectors, EXT2_SECTORS_BYTES, packed_sectors)) < 0) {
//                //file is too much fragmented - error
//                _fs_unlock();
//                free(inode);
//                return 0xffffffff;
//            }


//            while(1){}

//            real_size = ((unsigned long long)inode->i_dir_acl << 32) | inode->i_size;
//            secNo = 0;
//            while (offset < real_size) {
//                printf("%d\r", secNo);
//
//                if (ext2_read_data(inode, secNo * 512, secBuff1, 512) < 512) {
//                    break;
//                }
//                ext2_read_file_sector_by_packed_map(ext2_file_sectors, EXT2_SECTORS_BYTES, secNo, secBuff2);
//
//                if (memcmp(secBuff1, secBuff2, 512)) {
//                    printf("\n%d err\n", secNo);
//                    print_hex_memory(secBuff1, 512);
//                    print_hex_memory(secBuff2, 512);
//                    break;
//                }
//                
////                if (secNo >= 1000000) {
////                    printf("limit\n");
////                    break;
////                }
//                
//                secNo++;
//                offset = secNo * 512;
//            }
//            printf("\nok\n");
//            while(1){}

//            real_size = ((unsigned long long)inode->i_dir_acl << 32) | inode->i_size;
//            while (offset < real_size) {
//                printf("%d\r", secNo);
//
//                if (ext2_read_data(inode, secNo * 512, secBuff1, 512) < 512) {
//                    break;
//                }
//                ext2_read_file_sector_by_map(ext2_file_sectors, EXT2_SECTORS_BYTES, secNo, secBuff2);
//
//                if (memcmp(secBuff1, secBuff2, 512)) {
//                    printf("\n%d err\n", secNo);
//                    print_hex_memory(secBuff1, 512);
//                    print_hex_memory(secBuff2, 512);
//                    break;
//                }
//                
//                if (secNo >= 1000000) {
//                    printf("limit\n");
//                    break;
//                }
//
//                offset += 512;
//                secNo++;
//            }
            free(inode);
        }

        entry_addr = request & 0x0000FFFF;
        memcpy(&ret, ext2_file_sectors + entry_addr, 4);

        if (entry_addr + 4 >= EXT2_SECTORS_BYTES) {
            //last hit, release memory
            free(ext2_file_sectors);
        }
    }

    //printf("* ext2_fs_ioctl success request: %lu\n", request);

    _fs_unlock();
    return ret;
}


int ext2_fs_remove(iop_file_t *fd, const char *name) {
    //printf("* ext2_fs_remove name: %s\n", name);
    return ext2_fs_dummy();
}


int ext2_fs_mkdir(iop_file_t *fd, const char *name) {
    //printf("* ext2_fs_mkdir name: %s\n", name);
    return ext2_fs_dummy();
}


int ext2_fs_rmdir(iop_file_t *fd, const char *name) {
    //printf("* ext2_fs_rmdir name: %s\n", name);
    return ext2_fs_dummy();
}


int ext2_fs_chstat(iop_file_t *fd, const char *name, fio_stat_t *stat, unsigned int a) {
    //printf("* ext2_fs_chstat name: %s, a: %d\n", name, a);
    return ext2_fs_dummy();
}
