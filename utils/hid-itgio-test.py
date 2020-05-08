#!/usr/bin/env python

# =============================================================================
# Author: din
# Requirements: Linux and hid-itgio
# Description: Tests the output lights for the ITGIO.
# =============================================================================

import os
import time

ITGIO_OUTPUT_LOCATION = "/sys/class/leds/itgio::output{}/brightness"

light_order = [
    (0, "P1 Right"),
    (1, "P1 Left"),
    (2, "P1 Down"),
    (3, "P1 Up"),
    (4, "P2 Right"),
    (5, "P2 Left"),
    (6, "P2 Down"),
    (7, "P2 Up"),
    (8, "P1 Upper Left Marquee"),
    (9, "P1 Lower Left Marquee"),
    (10, "P2 Upper Right Marquee"),
    (11, "P2 Lower Right Marquee"),
    (12, "P2 Button"),
    (13, "P1 Button"),
    (14, "Coin Counter"),
    (15, "Neon"),
]

def set_light(light_num, state):
    light_loc = ITGIO_OUTPUT_LOCATION.format(light_num)
    #print(light_loc, " > ", state)

    try:
        os_file = os.open(light_loc,os.O_RDWR)
        os.write(os_file, b"%d\n" % int(state))
        os.close(os_file)
    except:
        #print("Could not update light state")
        pass

if __name__ == "__main__":
    try:
        print("Testing ITGIO Lights...\r\n")

        #clear the lights first.
        for light in light_order:
            set_light(light[0], 0)

        while 1:
            for light in light_order:
                print("Turning on: " + str(light[1]))

                set_light(light[0], 1)
                time.sleep(1)
                set_light(light[0], 0)

    except KeyboardInterrupt:
        quit()
