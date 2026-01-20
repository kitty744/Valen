#!/bin/bash

# --- 1. Map Kconfig to QEMU (with fallbacks if .config is missing) ---
Q_MEM=${CONFIG_MEM_SIZE:-15G}
Q_SMP=${CONFIG_CPU_CORES:-16}
Q_ARCH=${CONFIG_MACHINE_TYPE:-q35}
Q_CPU=${CONFIG_CPU_TYPE:-max}

# --- 2. Handle Logic Toggles ---
Q_VGA="std"
if [ "$CONFIG_VGA_NONE" = "y" ]; then Q_VGA="none"; fi
if [ "$CONFIG_VGA_VIRTIO" = "y" ]; then Q_VGA="virtio"; fi

Q_AUDIO=""
if [ "$CONFIG_AUDIO_ENABLED" = "y" ]; then
    Q_AUDIO="-soundhw ${CONFIG_SOUNDHW:-pcspk} -audiodev sdl,id=audio0"
fi

echo "[INFO]: Launching with $Q_MEM RAM and $Q_SMP Cores..."

# --- 3. Run QEMU ---
qemu-system-x86_64 \
    -m "$Q_MEM" \
    -smp "$Q_SMP" \
    -machine "$Q_ARCH" \
    -cpu "$Q_CPU" \
    -vga "$Q_VGA" \
    $Q_AUDIO \
    -cdrom bin/caneos.iso \
    -serial stdio

make clean