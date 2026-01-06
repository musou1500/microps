#include <stdio.h>
#include <stddef.h>
#include <stdint.h>

#include "platform.h"

#include "util.h"
#include "net.h"

/*
 * NOTE: if you want to add/delete the entries after net_run(),
 *       you need to protect these lists with a lock.
 */
static struct net_device *devices;

struct net_device *
net_device_alloc(void)
{
    struct net_device *dev;

    dev = (struct net_device *)memory_alloc(sizeof(struct net_device));
    if (!dev)
    {
        errorf("memory_alloc() failure");
        return NULL;
    }

    return dev;
}

/*
 * NOTE: must not be call after net_run()
 */
int net_device_register(struct net_device *dev)
{
    static unsigned int index = 0;
    dev->index = index++;
    snprintf(dev->name, sizeof(dev->name), "net%d", dev->index);
    devices = dev;
    infof("success, device=%s, type=0x%04x", dev->name, dev->type);
    return 0;
}

static int
net_device_open(struct net_device *dev)
{
    infof("dev=%s", dev->name);
    if (NET_DEVICE_IS_UP(dev))
    {
        infof("device %s is already UP", dev->name);
        return -1;
    }
    dev->flags |= NET_DEVICE_FLAG_UP;
    return 0;
}

static int
net_device_close(struct net_device *dev)
{
    infof("dev=%s", dev->name);
    if (!NET_DEVICE_IS_UP(dev))
    {
        infof("device %s is already DOWN", dev->name);
        return -1;
    }
    dev->flags &= ~NET_DEVICE_FLAG_UP;
    return 0;
}

int net_device_output(struct net_device *dev, uint16_t type, const uint8_t *data, size_t len, const void *dst)
{
    debugf("dev=%s, type=0x%04x, len=%zu", dev->name, type, len);
    debugdump(data, len);
    if (!NET_DEVICE_IS_UP(dev))
    {
        errorf("device %s is DOWN", dev->name);
        return -1;
    }

    if (dev->mtu < len)
    {
        errorf("packet size %zu exceeds MTU %u", len, dev->mtu);
        return -1;
    }
    return 0;
}

int net_init(void)
{
    infof("initialize...");
    if (platform_init() == -1)
    {
        errorf("platform initialization failed");
        return -1;
    }

    infof("success");
    return 0;
}

int net_run(void)
{
    struct net_device *dev;

    infof("startup...");
    if (platform_run() == -1)
    {
        errorf("platform_run() failure");
        return -1;
    }

    for (dev = devices; dev != NULL; dev = dev->next)
    {
        if (net_device_open(dev) == -1)
        {
            errorf("net_device_open() failure");
            return -1;
        }
    }

    infof("success");
    return 0;
}

int net_shutdown(void)
{
    struct net_device *dev;
    infof("shutdown...");
    if (platform_shutdown() == -1)
    {
        errorf("platform_shutdown() failure");
        return -1;
    }

    for (dev = devices; dev != NULL; dev = dev->next)
    {
        if (net_device_close(dev) == -1)
        {
            errorf("net_device_close() failure");
            return -1;
        }
    }

    infof("success");
    return 0;
}
