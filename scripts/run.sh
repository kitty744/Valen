#!/bin/bash

# --- 1. DEFAULT SETTINGS ---
MEM="15G"
SMP="16,cores=16,threads=1"
SOUNDHW="pcspk"
AUDIODEV="sdl,id=audio0"
MACHINE="q35"
CPU="max"
VGA="std"
SERIAL="stdio"
DEBUG="guest_errors"

echo "[INFO]: CLEANING AND BUILDING"
make clean && make all

echo "[INFO]: LAUNCHING QEMU"

# --- 3. EXECUTION ---
qemu-system-x86_64 \
    -machine "$MACHINE,pcspk-audiodev=audio0" \
    -cpu "$CPU" \
    -smp "$SMP" \
    -m "$MEM" \
    -vga "$VGA" \
    -serial "$SERIAL" \
    -audiodev "$AUDIODEV" \
    -d "$DEBUG" \
    -cdrom bin/caneos.iso \
    $EXTRA_ARGS

# Cleanup after closing QEMU
make clean
clear
clear