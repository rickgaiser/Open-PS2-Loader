#include <errno.h>
#include <iomanX.h>
#include <stdio.h>
#include <loadcore.h>
#include <bdm.h>
#include <sifcmd.h>

#include <irx.h>

#include "usb-ioctl.h"

IRX_ID("usbhdfsd_for_EE", 1, 1);

static int xmassInit(iop_device_t *device)
{
    return 0;
}

static int xmassUnsupported(void)
{
    return -1;
}

static int xmassDevctl(iop_file_t *fd, const char *name, int cmd, void *arg, unsigned int arglen, void *buf, unsigned int buflen)
{
	return 1;
}

static iop_device_ops_t xmass_ops = {
    &xmassInit,
    (void *)&xmassUnsupported,
    (void *)&xmassUnsupported,
    (void *)&xmassUnsupported,
    (void *)&xmassUnsupported,
    (void *)&xmassUnsupported,
    (void *)&xmassUnsupported,
    (void *)&xmassUnsupported,
    (void *)&xmassUnsupported,
    (void *)&xmassUnsupported,
    (void *)&xmassUnsupported,
    (void *)&xmassUnsupported,
    (void *)&xmassUnsupported,
    (void *)&xmassUnsupported,
    (void *)&xmassUnsupported,
    (void *)&xmassUnsupported,
    (void *)&xmassUnsupported,
    (void *)&xmassUnsupported,
    (void *)&xmassUnsupported,
    (void *)&xmassUnsupported,
    (void *)&xmassUnsupported,
    (void *)&xmassUnsupported,
    (void *)&xmassUnsupported,
    &xmassDevctl,
};

static iop_device_t xmassDevice = {
    "xmass",
    IOP_DT_BLOCK | IOP_DT_FSEXT,
    1,
    "XMASS",
    &xmass_ops};

static void bdm_callback(int cause)
{
    static SifCmdHeader_t EventCmdData;

    EventCmdData.opt = cause;
    sceSifSendCmd(0, &EventCmdData, sizeof(EventCmdData), NULL, NULL, 0);
}

int _start(int argc, char *argv[])
{
    if (AddDrv(&xmassDevice) == 0) {
        bdm_RegisterCallback(&bdm_callback);
        return MODULE_RESIDENT_END;
    } else {
        return MODULE_NO_RESIDENT_END;
    }
}
