// ps2sdk include files
#include <debug.h>
#include <loadfile.h>
#include <smem.h>
#include <smod.h>
#include <sifcmd.h>
#include <sifrpc.h>
#include <iopheap.h>
#include <iopcontrol.h>
#include <kernel.h>
#include <sbv_patches.h>
#include <libcdvd-common.h>

// posix (newlib) include files
#include <unistd.h>
#include <time.h>
#include <fcntl.h>
#include <stdlib.h>

// This function is defined as weak in ps2sdkc, so how
// we are not using time zone, so we can safe some KB
void _ps2sdk_timezone_update() {}

DISABLE_PATCHED_FUNCTIONS(); // Disable the patched functionalities
// DISABLE_EXTRA_TIMERS_FUNCTIONS(); // Disable the extra functionalities for timers

//#define PRINTF printf
#define PRINTF scr_printf

// Blocks sizes to test
#define FILE_SIZE             (10 * 1024 * 1024)
#define FILE_BLOCK_SIZE_MIN   (2 * 1024)
#define FILE_BLOCK_SIZE_MAX   (256 * 1024)
#define STREAM_BLOCK_SIZE_MIN (2 * 1024)
#define STREAM_BLOCK_SIZE_MAX (32 * 1024)
#define STREAM_BUFMAX         80 // 80 sectors = 160KiB
#define STREAM_BANKMAX        5  // max 5 ringbuffers inside buffer ?

#define FILE_RANDOM "cdrom:\\RANDOM.BIN"
#define FILE_ZERO   "cdrom:\\ZERO.BIN"

//--------------------------------------------------------------
void print_speed(clock_t clk_start, clock_t clk_end, u32 fd_size, u32 buf_size)
{
    unsigned int msec = (int)((clk_end - clk_start) / (CLOCKS_PER_SEC / 1000));
    PRINTF("\t\t- Read %04dKiB in %04dms, blocksize=%06d, speed=%04dKB/s\n", fd_size / 1024, msec, buf_size, fd_size / msec);
}

//--------------------------------------------------------------
void test_read_file_1(const char *filename, unsigned int block_size, unsigned int total_size)
{
    int size_left;
    int fd;
    char *buffer = NULL;
    clock_t clk_start, clk_end;

    if ((fd = open(filename, O_RDONLY /*, 0644*/)) <= 0) {
        PRINTF("\t\t- Could not find '%s'\n", filename);
        return;
    }

    buffer = malloc(block_size);

    clk_start = clock();
    size_left = total_size;
    while (size_left > 0) {
        int read_size = (size_left > block_size) ? block_size : size_left;
        if (read(fd, buffer, read_size) != read_size) {
            PRINTF("\t\t- Failed to read file.\n");
            return;
        }
        size_left -= read_size;
    }
    clk_end = clock();

    print_speed(clk_start, clk_end, total_size - size_left, block_size);

    free(buffer);

    close(fd);
}

//--------------------------------------------------------------
void test_read_file(const char *filename)
{
    test_read_file_1(filename, 16*1024, FILE_SIZE);
}

//--------------------------------------------------------------
void test_read_stream_1(const char *filename, unsigned int block_size, unsigned int total_size)
{
    void *iopbuffer = SifAllocIopHeap(STREAM_BUFMAX * 2048);
    void *eebuffer = malloc(block_size);
    unsigned int sectors = block_size / 2048;
    unsigned int size_left = total_size;

    clock_t clk_start, clk_end;
    sceCdRMode mode = {1, SCECdSpinStm, SCECdSecS2048, 0};
    sceCdlFILE fp;
    u32 error;

    if (sceCdStInit(STREAM_BUFMAX, STREAM_BANKMAX, iopbuffer) == 0) {
        PRINTF("ERROR: sceCdStInit\n");
        return;
    }
    if (sceCdSearchFile(&fp, filename) == 0) {
        PRINTF("ERROR: sceCdSearchFile\n");
        return;
    }

    clk_start = clock();
    sceCdStStart(fp.lsn, &mode);
    while (size_left) {
        int rv = sceCdStRead(sectors, eebuffer, STMBLK, &error);
        if (rv != sectors) {
            PRINTF("\t\t- sceCdStRead = %d error = %d\n", rv, error);
            //    break;
        }
        if (error != SCECdErNO) {
            PRINTF("\t\t- ERROR %d\n", error);
            break;
        }
        size_left -= rv * 2048;
    }
    sceCdStStop();
    clk_end = clock();

    print_speed(clk_start, clk_end, total_size - size_left, block_size);

    free(eebuffer);
    SifFreeIopHeap(iopbuffer);
}

//--------------------------------------------------------------
void test_read_stream(const char *filename)
{
    test_read_stream_1(filename, 16*1024, FILE_SIZE);
}

//--------------------------------------------------------------
void print_header()
{
    PRINTF("\n\n\n");
    PRINTF("\t\tOPL Accurate Read tester v1.0\n");
}

//--------------------------------------------------------------
void print_done()
{
    int i;

    PRINTF("\t\tDone. Next test in ");
    for (i = 3; i > 0; i--) {
        PRINTF("%d ", i);
        sleep(1);
    }
}

//--------------------------------------------------------------
int main()
{
    init_scr();

    SifExitIopHeap();
    SifLoadFileExit();
    SifExitRpc();

    SifInitRpc(0);
    // while (!SifIopReset("rom0:UDNL cdrom:\\IOPRP300.IMG", 0))
    while (!SifIopReset("", 0))
        ;
    while (!SifIopSync())
        ;
    SifInitRpc(0);
    SifLoadFileInit();
    SifInitIopHeap();

    // Enable loading iop modules from EE memory
    sbv_patch_enable_lmb();

    // Load cdvdman
    // NOTE: on OPL this module will not be loaded
    if (SifLoadModule("rom0:CDVDMAN", 0, 0) < 0)
        PRINTF("\t\tcould not load %s\n", "rom0:CDVDMAN");

    // Load cdvdfsv
    // NOTE: on OPL this module will not be loaded
    if (SifLoadModule("rom0:CDVDFSV", 0, 0) < 0)
        PRINTF("\t\tcould not load %s\n", "rom0:CDVDFSV");

    sceCdInit(SCECdINIT);
    sceCdMmode(SCECdPS2DVD);

    // speed test random file
    scr_clear();
    print_header();
    PRINTF("\t\tReading from 10 files located at 0 to 100%% of CD:\n");
    test_read_file("cdrom:\\FILE0A.BIN;1");
    test_read_file("cdrom:\\FILE1A.BIN;1");
    test_read_file("cdrom:\\FILE2A.BIN;1");
    test_read_file("cdrom:\\FILE3A.BIN;1");
    test_read_file("cdrom:\\FILE4A.BIN;1");
    test_read_file("cdrom:\\FILE5A.BIN;1");
    test_read_file("cdrom:\\FILE6A.BIN;1");
    test_read_file("cdrom:\\FILE7A.BIN;1");
    test_read_file("cdrom:\\FILE8A.BIN;1");
    test_read_file("cdrom:\\FILE9A.BIN;1");
    PRINTF("\t\tStreaming from 10 files located at 0 to 100%% of CD:\n");
    test_read_stream("\\FILE0A.BIN;1");
    test_read_stream("\\FILE1A.BIN;1");
    test_read_stream("\\FILE2A.BIN;1");
    test_read_stream("\\FILE3A.BIN;1");
    test_read_stream("\\FILE4A.BIN;1");
    test_read_stream("\\FILE5A.BIN;1");
    test_read_stream("\\FILE6A.BIN;1");
    test_read_stream("\\FILE7A.BIN;1");
    test_read_stream("\\FILE8A.BIN;1");
    test_read_stream("\\FILE9A.BIN;1");
    print_done();

    while (1) {}
    return 0;
}
