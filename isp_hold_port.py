# Opening the serial port resets the Uno programmer, and its own bootloader
# then answers avrdude instead of the ArduinoISP sketch. Opening the port here
# first, and keeping it open, lets that reset finish before avrdude connects.
# Not needed if the Uno has a 10 uF capacitor between RESET and GND.

import os
import time

Import("env")


def hold_port_open(source, target, env):
    port = env.subst("$UPLOAD_PORT")
    env["ISP_PORT_FD"] = os.open(port, os.O_RDWR | os.O_NOCTTY | os.O_NONBLOCK)
    time.sleep(2)


env.AddPreAction("upload", hold_port_open)
