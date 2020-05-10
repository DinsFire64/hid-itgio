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

There are two ways of doing this, one is to add the quirks to your linux kernel parameters or add the quirks during runtime.

To load at runtime, just use modprobe to unload and reload the `usbhid` kernel module.

`rmmod usbhid; modprobe -v usbhid "quirks=0x07c0:0x1584:0x40000010"`

Or to always load, add the following to your boot parameters. This is usually done in the configs of your boot loader (such as grub in `/etc/default/grub`), but please check with your distro's documentation.

`usbhid.quirks=0x07c0:0x1584:0x40000010`

## Testing

The device when loaded will be a standard HID gamepad in the Linux system. To test input, any game (such as StepMania) or a program like `jstest` can be used.

To test output, `utils/hid-itgio-test.py` will cycle all 16 lights.

## Known Issues

This module in short testing has preformed well as a means to play StepMania. It is a work in progress, and taking PRs for improvement.

* Module can crash during unloading. Potential problem with unloading the LED submodule. Sometimes it works great, others not, I cannot pin down precisely what the issue is.

# Thank you for playing.
