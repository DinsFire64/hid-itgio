# Kernel Module for ITGIO

This is a small kernel module to adapt the HID quirks of the UltraCade ITG-IO into a standard HID Gamepad in modern Linux installations.

The UltraCade ITG-IO was a JAMMA adapter that shipped inside of In the Groove upgrade kits from ROXOR Entertainment. It allows for the connection of 16 JAMMA inputs into a computer over USB and the output of up to 16 light controls.

This device was mainly used to control Dance Dance Revolution cabinets and needed a modern way of interfacing with it using programs such as StepMania.

Documentation for this device can be found at [this link on UltraCade's website](https://service.globalvr.com/downloads/ultracade/components/040-ITGIOMA-UCT_ITG-IO_User-Docv02B.pdf)

## Building and Installing

`cd mod/`

`make`

`insmod hid-itgio.ko`

## Using

In order to properly use this, the linux kernel needs to be informed of the HID quirks of this device.

There are two ways of doing this, one is to add the quirks to your linux kernel parameters or add the quirk during runtime.

To load at runtime, just use modprobe to unload and reload the `usbhid` kernel module.

`rmmod usbhid; modprobe -v usbhid "quirks=0x07c0:0x1584:0x40000410"`

Or to always load, add the following to your boot parameters. This is usually done in the configs of your boot loader (such as grub), but please check with your distro's documentation.

`usbhid.quirks=0x07c0:0x1584:0x40000410`

## Testing

The device when loaded will be a standard HID gamepad in the Linux system. To test input, any game (such as StepMania) or a program like `jstest` can be used.

To test output, `utils/hid-itgio-test.py` will cycle all 16 lights.

## Known Issues

This module is a work in progress and as such is not currently stable. It is being published for the sake of research and collaboration.

* Input can be stuck on momentarily. This may be due to Linux not polling the device any further. This is my current focus as even with an overwritten 1ms/1kHz polling the device may require a write before a read...
* Module can crash during unloading. Potential problem with unloading the LED submodule. Sometimes it works great, others not.

# Thank you for playing.
