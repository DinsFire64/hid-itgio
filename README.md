# Kernel Module for ITGIO

This is a small kernel module to adapt the HID quirks of the UltraCade ITG-IO into a standard HID Gamepad in modern Linux installations.

The UltraCade ITG-IO was a JAMMA adapter that shipped inside of In the Groove upgrade kits from ROXOR Entertainment. It allows for the connection of 16 JAMMA inputs into a computer over USB and the output of up to 16 light controls.

This device was mainly used to control Dance Dance Revolution cabinets and needed a modern way of interfacing with it using programs such as StepMania.

Documentation for this device can be found at [this link on UltraCade's website](https://service.globalvr.com/downloads/ultracade/components/040-ITGIOMA-UCT_ITG-IO_User-Docv02B.pdf)

## Building and Installing

`make`

`insmod hid-itgio.ko`

## Testing

The device when loaded will be a standard HID gamepad in the Linux system. For input any game (such as StepMania) or a program like `jstest` can be used to test input.

To test output, there is a script in `utils/` that can be run.

## Known Issues

This module is a work in progress and as such is not currently stable. It is being published for the sake of research and collaboration.

* Input can be stuck on or off. This may be due to Linux not polling the device any further
* Module can crash during unloading. Potential problem with unloading the LED submodule. Sometimes it works great, others not.
* My hardware is flaky so I have yet to really put this to the test on the cabinet, so I look toward my peers for testing and help.

# Thank you for playing.