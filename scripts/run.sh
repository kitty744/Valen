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

# --- 2. ARGUMENT PARSING ---
while [[ $# -gt 0 ]]; do
    case "$1" in
        -m)         MEM="$2";        shift 2 ;;
        -smp)       SMP="$2";        shift 2 ;;
        -soundhw)   SOUNDHW="$2";    shift 2 ;;
        -audiodev)  AUDIODEV="$2";   shift 2 ;;
        -machine)   MACHINE="$2";    shift 2 ;;
        -cpu)       CPU="$2";        shift 2 ;;
        -vga)       VGA="$2";        shift 2 ;;
        -serial)    SERIAL="$2";     shift 2 ;;
        -d)         DEBUG="$2";      shift 2 ;;
        *)          EXTRA_ARGS+="$1 "; shift ;; # Capture anything else
    esac
done

echo "[INFO]: CLEANING AND BUILDING"
make clean && make all

echo "[INFO]: LAUNCHING QEMU"
echo "[INFO]: Config: CPU=$CPU, MEM=$MEM, SMP=$SMP, VGA=$VGA"

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