/*
 * hig-itgio.c
 * din
 *
 * Kernel module to easily adapt the UltraCade ITGIO to the HID and lighting subsystem in Linux.
 *
 * Please note for this module to work, the following quirks
 * need to be passed to your kernel on boot or to usbhid on runtime:
 * HID_QUIRK_HIDDEV_FORCE | HID_QUIRK_NO_IGNORE | HID_QUIRK_ALWAYS_POLL = 0x40000410
 *
 * Example: modprobe -v usbhid "quirks=0x07c0:0x1584:0x40000410"
 * Example: usbhid.quirks=0x07c0:0x1584:0x40000410
 *
 * This module was based on Devin J. Pohly's PIUIO kernel module. Thank you for your work.
 *
 * Thank you for playing!
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation, version 2.
 */

#include <linux/device.h>
#include <linux/hid.h>
#include <linux/module.h>

//Constants for this device.
#define ITGIO_MAX_LEDS 16
#define ITGIO_REPORT_SIZE 5

#define ITGIO_VID 0x07C0

//Names to be found in /sys/class/leds/
static const char *led_names[] = {
    "itgio::output0",
    "itgio::output1",
    "itgio::output2",
    "itgio::output3",
    "itgio::output4",
    "itgio::output5",
    "itgio::output6",
    "itgio::output7",
    "itgio::output8",
    "itgio::output9",
    "itgio::output10",
    "itgio::output11",
    "itgio::output12",
    "itgio::output13",
    "itgio::output14",
    "itgio::output15",
};

//struct for keeing track of a single LED when called to update
struct itgio_led_device
{
        struct itgio_device *itgdev;
        struct led_classdev ldev;
        bool registered;
};

//The main struct/device state.
struct itgio_device
{
        struct hid_device *hdev;
        u8 *buf;
        struct mutex lock;
        struct itgio_led_device led[ITGIO_MAX_LEDS];
};

struct itgio_device *itgdev;

/*
 * Method that flips the bits of the ITGIO as they are active low instead of active high.
 *
 * The buttom type bytes are unused in this device, so they are shifted away so only the lower
 * two bytes contain the state of the buttons.
 */
static int itgio_raw_event(struct hid_device *hdev, struct hid_report *report,
                           u8 *data, int len)
{

        //hid_info(hdev, "read: %d %d-%d-%d-%d", len, data[0], data[1], data[2], data[3]);

        if (len >= 4)
        {
                //itgio will return a 4byte response when an event occurs
                //the first two bytes [0,1] are an echo of the current state of the lights (active high)
                //the last two [2,3] are representitive of the state of the gamepad (active low)

                //shift bits left so all of our inputs are on the bottom 16 bits.
                //itgio is active low, so invert it
                data[0] = ~data[2];
                data[1] = ~data[3];

                //these are meaningless as we are setting the
                //input ourselves, we know the lighting state.
                data[2] = 0;
                data[3] = 0;
        }

        return 0;
}

/*
 * Method that is called by the linux kernel on a change in
 * the state of the brightness file hook in /sys/class/leds
 */
static void itgio_led_set(struct led_classdev *dev, enum led_brightness b)
{
        struct itgio_led_device *led = container_of(dev, struct itgio_led_device, ldev);
        struct itgio_device *itgdev = led->itgdev;
        int index;
        int byte_num;
        int ret;

        index = led - itgdev->led;
        if (index > ITGIO_MAX_LEDS - 1)
        {
                hid_err(itgdev->hdev, "index %d invalid", index);
                return;
        }

        //hid_info(itgdev->hdev, "setting led #%d to %d", index, b);
        byte_num = (index / 8);
        index = index - (byte_num * 8);

        //data starts at index 1.
        byte_num++;

        //Lock the writing/reading in case multiple lights are changed in succession.
        mutex_lock(&itgdev->lock);

        //set or clear the bit.
        if (b)
        {
                itgdev->buf[byte_num] |= 1 << index;
        }
        else
        {
                itgdev->buf[byte_num] &= ~(1 << index);
        }

        //hid_info(itgdev->hdev, "write: b#%d-%d %d-%d-%d-%d-%d", byte_num, index, itgdev->buf[0], itgdev->buf[1], itgdev->buf[2], itgdev->buf[3], itgdev->buf[3]);

        //ensure we are talking to the lighting device and not the "gamepad" device.
        itgdev->buf[0] = 0x00;

        ret = hid_hw_raw_request(itgdev->hdev, itgdev->buf[0], itgdev->buf,
                                 ITGIO_REPORT_SIZE,
                                 HID_OUTPUT_REPORT,
                                 HID_REQ_SET_REPORT);

        mutex_unlock(&itgdev->lock);
}

/*
 * Method that is called to init the device state on powerup.
 */
static int itgio_obj_init(struct hid_device *hdev)
{
        int i;

        itgdev = kzalloc(sizeof(struct itgio_device), GFP_KERNEL);
        if (!itgdev)
        {
                hid_err(hdev, "failed to allocate memory");
                return -ENOMEM;
        }

        itgdev->buf = devm_kmalloc(&hdev->dev, ITGIO_REPORT_SIZE, GFP_KERNEL);
        if (!itgdev->buf)
        {
                hid_err(hdev, "failed to allocate buffer memory.");
                return -ENOMEM;
        }

        //top two bytes need to be high in order to enable input
        //...I think that's the way it's worked for me so far...
        itgdev->buf[ITGIO_REPORT_SIZE - 1] = 0xFF;
        itgdev->buf[ITGIO_REPORT_SIZE - 2] = 0xFF;

        //ensure that we don't attempt to unregister a device that isn't registered.
        for (i = 0; i < ITGIO_MAX_LEDS; i++)
        {
                itgdev->led[i].registered = false;
        }

        itgdev->hdev = hdev;

        return 0;
}

/*
 * Method to setup and register each of the ITGIO_MAX_LEDS's
 */
static int itgio_leds_init(struct hid_device *hdev)
{
        int i;
        const struct attribute_group **ag;
        struct attribute **attr;
        int ret;

        for (i = 0; i < ITGIO_MAX_LEDS; i++)
        {
                itgdev->led[i].itgdev = itgdev;
                itgdev->led[i].ldev.name = led_names[i];
                itgdev->led[i].ldev.max_brightness = LED_ON; //lights are either on or off.
                itgdev->led[i].ldev.brightness_set = itgio_led_set;
                itgdev->led[i].registered = true;

                ret = led_classdev_register(&itgdev->hdev->dev, &itgdev->led[i].ldev);
                if (ret)
                        goto out_unregister;

                if ((*itgdev->led[i].ldev.dev->class->dev_groups)->attrs == NULL)
                {
                        hid_err(hdev, "dev null");
                }

                //allow all users to set the lights.
                for (ag = itgdev->led[i].ldev.dev->class->dev_groups; *ag; ag++)
                {
                        attr = (*itgdev->led[i].ldev.dev->class->dev_groups)->attrs;
                        ret = sysfs_chmod_file(&itgdev->led[i].ldev.dev->kobj, *attr, S_IRUGO | S_IWUGO);
                }
        }

        return 0;

out_unregister:
        for (--i; i >= 0; i--)
                led_classdev_unregister(&itgdev->led[i].ldev);
        return ret;
}

/*
 * Method that conforms the report descriptor to the HID standard.
 *
 * This allows the rest of the HID system to regard this device as a standard gamepad,
 * and remove the lights as we are taking care of that in this kernel module.
 */
static __u8 *itgio_report_fixup(struct hid_device *hdev, __u8 *rdesc,
                                unsigned int *rsize)
{
        //TODO: See if we can add QUIRK flags here or similar to reduce the manual loading of quirks by the user.

        //Use the REPORT_ID to discern the two interfaces.
        if (*rsize >= 15 && rdesc[14] == 0x85 && rdesc[15] == 0xff)
        {
                //we will not be using this device, so corrupt the report to make
                //the kernel ignore it.
                //TODO: make this more eliquant, this feels hacky (but it works...)
                rdesc[0] = 0x00;
        }
        else
        {
                //make this device a gamepad.
                rdesc[2] = 0x09;
                rdesc[3] = 0x05;

                //this is my output, so register the LEDs.
                itgio_obj_init(hdev);
                itgio_leds_init(hdev);

                //disable the input of this device by zeroing out the
                //input (device to computer) size.
                rdesc[28] = 0x75;
                rdesc[29] = 0x00;
        }

        return rdesc;
}

/*
 * Method to tear down all of the objects we setup.
 */
static void itgio_remove(struct hid_device *hdev)
{
        int i = 0;

        //TODO: This is fairly buggy and only the LEDS really get unregistered
        //This need a solid memory cleanup and dealloc as well.

        if (itgdev != NULL)
        {
                mutex_lock(&itgdev->lock);

                for (i = 0; i < ITGIO_MAX_LEDS; i++)
                {
                        if (itgdev->led[i].registered && !IS_ERR_OR_NULL(itgdev->led[i].ldev.dev))
                        {
                                //TODO: Figure out why this call throws a nullptr
                                //led_classdev_unregister(&itgdev->led[i].ldev);

                                device_unregister(itgdev->led[i].ldev.dev);
                                itgdev->led[i].registered = false;
                        }
                }

                mutex_unlock(&itgdev->lock);
        }
}

static const struct hid_device_id itgio_devices[] = {
    {HID_USB_DEVICE(ITGIO_VID, 0x1501)},
    {HID_USB_DEVICE(ITGIO_VID, 0x1582)},
    {HID_USB_DEVICE(ITGIO_VID, 0x1584)},

    {}};
MODULE_DEVICE_TABLE(hid, itgio_devices);

static struct hid_driver itgio_driver = {
    .name = "itgio",
    .id_table = itgio_devices,
    .report_fixup = itgio_report_fixup,
    .raw_event = itgio_raw_event,
    .remove = itgio_remove,
};

MODULE_LICENSE("GPL");
MODULE_AUTHOR("din <dinsfire64@gmail.com>");
MODULE_DESCRIPTION("HID Driver for UltraCade ITG-IO");
MODULE_VERSION("0.2");

module_hid_driver(itgio_driver);
