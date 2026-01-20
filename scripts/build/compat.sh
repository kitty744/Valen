#!/bin/bash

# MEANT FOR TESTING THE OPERATING SYSTEMS COMPATABILITY ON LOWER-END MACHINES.

# Exit on error's.
set -e

echo "[INFO]: CLEANING PREVIOUS BUILD"
make clean

echo "[INFO]: BUILDING"
make all

echo "[INFO]: LAUNCHING QEMU"
qemu-system-x86_64 \
    -cdrom bin/caneos.iso \
    -smp 1,cores=1,threads=1 \
    -m 128M \
    -vga std \
    -serial stdio \
    -rtc base=localtime \
    -d guest_errors \
    -audiodev sdl,id=audio0 \
    -machine pcspk-audiodev=audio0

make clean
clear
clear