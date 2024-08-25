
#include <stdint.h>

#include "internal.h"

#include "device.h"
#include "ioplib_util.h"
#include "mmcedrv_config.h"

extern struct cdvdman_settings_mmce cdvdman_settings;

uint32_t (*fp_mmcedrv_get_size)();
int (*fp_mmcedrv_read_sector)(int type, unsigned int sector_start, unsigned int sector_count, void *buffer);
void (*fp_mmcedrv_config_set)(int setting, int value);

void DeviceInit(void)
{
    DPRINTF("%s\n", __func__);
}

void DeviceDeinit(void)
{
    DPRINTF("%s\n", __func__);
}

int DeviceReady(void)
{
    // DPRINTF("%s\n", __func__);
    return SCECdComplete;
}

void DeviceFSInit(void)
{
    uint64_t iso_size;

    // get modload export table
    modinfo_t info;
    getModInfo("mmcedrv\0", &info);

    //Get func ptrs
    fp_mmcedrv_get_size = (void *)info.exports[4];
    fp_mmcedrv_read_sector = (void *)info.exports[5];
    fp_mmcedrv_config_set = (void *)info.exports[6];

    //Set port and iso fd
    DPRINTF("Port: %i\n", cdvdman_settings.port);
    DPRINTF("ISO fd: %i\n", cdvdman_settings.iso_fd);

    fp_mmcedrv_config_set(MMCEDRV_SETTING_PORT, cdvdman_settings.port);
    fp_mmcedrv_config_set(MMCEDRV_SETTING_ISO_FD, cdvdman_settings.iso_fd);

    DPRINTF("Waiting for device...\n");

    while (1) {
        iso_size = fp_mmcedrv_get_size();
        if (iso_size > 0)
            break;
        DelayThread(100 * 1000); // 100ms
    }

    DPRINTF("Waiting for device...done! connected to %llu byte iso\n", iso_size);
}

void DeviceLock(void)
{
    DPRINTF("%s\n", __func__);
}

void DeviceUnmount(void)
{
    DPRINTF("%s\n", __func__);
}

void DeviceStop(void)
{
    DPRINTF("%s\n", __func__);
}

int DeviceReadSectors(u32 lsn, void *buffer, unsigned int sectors)
{
    int rv = SCECdErNO;
    int res = 0;
    int retries = 0;
    int dump = 0;

    DPRINTF("%s(%u, 0x%p, %u)\n", __func__, (unsigned int)lsn, buffer, sectors);

    do {
        res = fp_mmcedrv_read_sector(MMCEDRV_TYPE_ISO, lsn, sectors, buffer);
        retries++;
    } while (res != sectors && retries < 3);

    if (retries == 3) {
        DPRINTF("%s: Failed to read after 3 retires, sector: %u, count: %u, buffer: 0x%p\n", __func__, lsn, sectors, buffer);
        rv = SCECdErREAD;
    }

    return rv;
}

//TODO: For VMCs
void mmce_readSector(unsigned int lba, unsigned short int nsectors, unsigned char *buffer)
{
    DPRINTF("%s\n", __func__);
}

void mmce_writeSector(unsigned int lba, unsigned short int nsectors, const unsigned char *buffer)
{
    DPRINTF("%s\n", __func__);
}