/*
  Copyright (c) 2026 The LCDHost Revival contributors

  This file is part of LCDHost.

  LCDHost is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  LCDHost is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with LCDHost.  If not, see <http://www.gnu.org/licenses/>.
*/

/*
  A fake hidapi for Linux, loaded with LD_PRELOAD, that simulates Logitech
  devices so that the HID drivers of LCDHost can run without hardware:

  - a G15 v2 (LCD and keys in the vendor collection FF00:0000, listed after
    a consumer collection with the same path, as hidapi does on Linux),
    unplugged 7 s after the first enumeration;
  - the two HID interfaces of a G510: the keyboard, then the LCD one;
  - the HID interface of a G19 (G-keys and backlight);
  - a G13 that cannot be opened (EACCES, like a missing udev rule).

  It logs on stderr what the drivers do with them ("fake-hidapi: ...").
  Used by tests/hid/run-fake-devices.sh.
*/

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <wchar.h>

#include <hidapi.h>

struct fake_usage
{
    unsigned short page;
    unsigned short usage;
};

struct fake_device
{
    const char *path;
    unsigned short product_id;
    const wchar_t *product;
    struct fake_usage usages[2];
    int usage_count;
    int deny_open;
    int unplug_after; /* seconds after the first enumeration, 0: never */
    /* Input reports delivered 3 s (pressed) and 3.5 s (released) after the
       device is opened, and the reply to a feature report request. */
    unsigned char key_down[8];
    unsigned char key_up[8];
    int key_size;
    unsigned char feature[8];
    int feature_size;
};

static const struct fake_device devices[] = {
    { "fake/g15v2", 0xC227, L"G15 GamePanel LCD", { { 0x000C, 0x0001 }, { 0xFF00, 0x0000 } }, 2, 0, 7,
      { 2, 0x01, 0x02, 0, 0 }, { 2, 0, 0, 0, 0 }, 5,
      { 2, 1, 0x10, 0 }, 4 },
    { "fake/g510-if0", 0xC22D, L"G510 Gaming Keyboard", { { 0x0001, 0x0006 } }, 1, 0, 0,
      { 0 }, { 0 }, 0, { 0 }, 0 },
    { "fake/g510-if1", 0xC22D, L"G510 Gaming Keyboard", { { 0x000C, 0x0001 }, { 0xFF00, 0x0000 } }, 2, 0, 0,
      { 3, 0, 0, 0x10, 0x01 }, { 3, 0, 0, 0, 0 }, 5,
      { 5, 0, 64, 255 }, 4 },
    { "fake/g19-mi01", 0xC229, L"G19 Gaming Keyboard", { { 0xFF00, 0x0000 } }, 1, 0, 0,
      { 2, 0x01, 0x10, 0x48 }, { 2, 0, 0, 0x40 }, 4,
      { 7, 255, 128, 0 }, 4 },
    { "fake/g13-denied", 0xC21C, L"G13", { { 0xFF00, 0x0000 } }, 1, 1, 0,
      { 0 }, { 0 }, 0, { 0 }, 0 },
};

#define DEVICE_COUNT ((int) (sizeof(devices) / sizeof(devices[0])))

struct hid_device_
{
    const struct fake_device *fake;
    struct timespec opened;
    int stage; /* 0: nothing sent, 1: key down sent, 2: key up sent */
    int writes;
};

static struct timespec first_enumeration;

static double seconds_since(const struct timespec *t)
{
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    return (double) (now.tv_sec - t->tv_sec) + (now.tv_nsec - t->tv_nsec) / 1e9;
}

static int unplugged(const struct fake_device *fake)
{
    return fake->unplug_after > 0 && first_enumeration.tv_sec != 0 &&
            seconds_since(&first_enumeration) >= fake->unplug_after;
}

static void log_bytes(const char *what, const char *path, const unsigned char *data, size_t length)
{
    size_t i;
    fprintf(stderr, "fake-hidapi: %s %s %u bytes:", what, path, (unsigned) length);
    for(i = 0; i < length && i < 8; ++i)
        fprintf(stderr, " %02x", data[i]);
    fprintf(stderr, "\n");
}

int HID_API_EXPORT HID_API_CALL hid_init(void)
{
    return 0;
}

int HID_API_EXPORT HID_API_CALL hid_exit(void)
{
    return 0;
}

struct hid_device_info HID_API_EXPORT * HID_API_CALL hid_enumerate(unsigned short vendor_id, unsigned short product_id)
{
    struct hid_device_info *head = NULL, **tail = &head;
    int i, u;
    (void) vendor_id;
    (void) product_id;
    if(first_enumeration.tv_sec == 0)
        clock_gettime(CLOCK_MONOTONIC, &first_enumeration);
    for(i = 0; i < DEVICE_COUNT; ++i)
    {
        if(unplugged(&devices[i]))
            continue;
        for(u = 0; u < devices[i].usage_count; ++u)
        {
            struct hid_device_info *info = calloc(1, sizeof(*info));
            info->path = strdup(devices[i].path);
            info->vendor_id = 0x046D;
            info->product_id = devices[i].product_id;
            info->serial_number = wcsdup(L"");
            info->manufacturer_string = wcsdup(L"Logitech");
            info->product_string = wcsdup(devices[i].product);
            info->usage_page = devices[i].usages[u].page;
            info->usage = devices[i].usages[u].usage;
            info->interface_number = -1;
            *tail = info;
            tail = &info->next;
        }
    }
    return head;
}

void HID_API_EXPORT HID_API_CALL hid_free_enumeration(struct hid_device_info *devs)
{
    while(devs)
    {
        struct hid_device_info *next = devs->next;
        free(devs->path);
        free(devs->serial_number);
        free(devs->manufacturer_string);
        free(devs->product_string);
        free(devs);
        devs = next;
    }
}

HID_API_EXPORT hid_device * HID_API_CALL hid_open_path(const char *path)
{
    int i;
    for(i = 0; i < DEVICE_COUNT; ++i)
    {
        if(strcmp(devices[i].path, path) || unplugged(&devices[i]))
            continue;
        if(devices[i].deny_open)
        {
            errno = EACCES;
            return NULL;
        }
        hid_device *dev = calloc(1, sizeof(*dev));
        dev->fake = &devices[i];
        clock_gettime(CLOCK_MONOTONIC, &dev->opened);
        fprintf(stderr, "fake-hidapi: open %s\n", path);
        return dev;
    }
    errno = ENOENT;
    return NULL;
}

HID_API_EXPORT hid_device * HID_API_CALL hid_open(unsigned short vendor_id, unsigned short product_id, const wchar_t *serial_number)
{
    (void) vendor_id;
    (void) product_id;
    (void) serial_number;
    errno = ENOENT;
    return NULL;
}

void HID_API_EXPORT HID_API_CALL hid_close(hid_device *dev)
{
    if(dev)
        fprintf(stderr, "fake-hidapi: close %s after %d writes\n", dev->fake->path, dev->writes);
    free(dev);
}

int HID_API_EXPORT HID_API_CALL hid_set_nonblocking(hid_device *dev, int nonblock)
{
    (void) dev;
    (void) nonblock;
    return 0;
}

int HID_API_EXPORT HID_API_CALL hid_read_timeout(hid_device *dev, unsigned char *data, size_t length, int milliseconds)
{
    const struct fake_device *fake = dev->fake;
    const unsigned char *report = NULL;
    (void) milliseconds;
    if(unplugged(fake))
        return -1;
    if(fake->key_size == 0 || (size_t) fake->key_size > length)
        return 0;
    if(dev->stage == 0 && seconds_since(&dev->opened) >= 3.0)
        report = fake->key_down;
    else if(dev->stage == 1 && seconds_since(&dev->opened) >= 3.5)
        report = fake->key_up;
    if(!report)
        return 0;
    ++dev->stage;
    memcpy(data, report, fake->key_size);
    log_bytes("input report", fake->path, data, fake->key_size);
    return fake->key_size;
}

int HID_API_EXPORT HID_API_CALL hid_read(hid_device *dev, unsigned char *data, size_t length)
{
    return hid_read_timeout(dev, data, length, 0);
}

int HID_API_EXPORT HID_API_CALL hid_write(hid_device *dev, const unsigned char *data, size_t length)
{
    if(unplugged(dev->fake))
        return -1;
    if(dev->writes++ == 0)
        log_bytes("first output report", dev->fake->path, data, length);
    return (int) length;
}

int HID_API_EXPORT HID_API_CALL hid_send_feature_report(hid_device *dev, const unsigned char *data, size_t length)
{
    if(unplugged(dev->fake))
        return -1;
    log_bytes("set feature report", dev->fake->path, data, length);
    return (int) length;
}

int HID_API_EXPORT HID_API_CALL hid_get_feature_report(hid_device *dev, unsigned char *data, size_t length)
{
    const struct fake_device *fake = dev->fake;
    if(unplugged(fake) || fake->feature_size == 0 || length < 1 || data[0] != fake->feature[0] ||
            (size_t) fake->feature_size > length)
        return -1;
    memcpy(data, fake->feature, fake->feature_size);
    return fake->feature_size;
}

HID_API_EXPORT const wchar_t* HID_API_CALL hid_error(hid_device *dev)
{
    (void) dev;
    return L"fake-hidapi: failed";
}
