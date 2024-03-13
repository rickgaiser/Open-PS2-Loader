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
void test_read_stream_1(uint32_t lsn, unsigned int block_size, unsigned int total_size)
{
    void *iopbuffer = SifAllocIopHeap(STREAM_BUFMAX * 2048);
    void *eebuffer = malloc(block_size);
    unsigned int sectors = block_size / 2048;
    unsigned int size_left = total_size;

    clock_t clk_start, clk_end;
    sceCdRMode mode = {1, 1, SCECdSecS2048, 0};
    u32 error;

    int rv = sceCdStInit(STREAM_BUFMAX, STREAM_BANKMAX, iopbuffer);
    if (rv == 0) {
        PRINTF("ERROR: sceCdStInit, rv=%d\n", rv);
        return;
    }

    clk_start = clock();
    sceCdStStart(lsn, &mode);
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
    int rv;

    init_scr();
    scr_clear();
    print_header();

    SifExitIopHeap();
    SifLoadFileExit();
    SifExitRpc();

    SifInitRpc(0);
    while (!SifIopReset("rom0:UDNL cdrom0:\\MODULES\\IOPRP271.IMG;1", 0))
    //while (!SifIopReset("", 0))
        ;
    while (!SifIopSync())
        ;

    SifInitRpc(0);
    SifLoadFileInit();
    SifInitIopHeap();

    // Load cdvdstm
    // NOTE: on OPL this module will not be loaded
    rv = SifLoadModule("cdrom:MODULES\\CDVDSTM.IRX", 0, NULL);
    if (rv < 0)
        PRINTF("\t\tcould not load %s, rv=%d\n", "cdrom:MODULES\\CDVDSTM.IRX", rv);

    sceCdInit(SCECdINIT);
    sceCdMmode(SCECdPS2DVD);

    // speed test random file
    PRINTF("\t\tStreaming from 10 files located at 0 to 100%% of DVD:\n");
    test_read_stream_1(0*262144, 16*1024, FILE_SIZE);
    test_read_stream_1(1*262144, 16*1024, FILE_SIZE); // 0.5GiB
    test_read_stream_1(2*262144, 16*1024, FILE_SIZE);
    test_read_stream_1(3*262144, 16*1024, FILE_SIZE);
    test_read_stream_1(4*262144, 16*1024, FILE_SIZE);
    test_read_stream_1(5*262144, 16*1024, FILE_SIZE);
    test_read_stream_1(6*262144, 16*1024, FILE_SIZE);
    test_read_stream_1(7*262144, 16*1024, FILE_SIZE);
    test_read_stream_1(8*262144, 16*1024, FILE_SIZE);
    //test_read_stream_1(9*262144, 16*1024, FILE_SIZE); // 4.5GiB
    print_done();

    while (1) {}
    return 0;
}
