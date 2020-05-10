#!/bin/bash
rmmod usbhid; modprobe -v usbhid "quirks=0x07c0:0x1584:0x40000010"
