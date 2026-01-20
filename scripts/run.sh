#!/bin/bash

# --- 1. DEPENDENCY CHECK & AUTO-INSTALLER ---
check_and_install() {
    # Core binaries needed for CaneOS
    DEPS=("qemu-system-x86_64" "nasm" "x86_64-elf-gcc" "grub-mkrescue" "xorriso" "kconfig-mconf")
    MISSING=()

    for tool in "${DEPS[@]}"; do
        if ! command -v "$tool" &> /dev/null; then
            MISSING+=("$tool")
        fi
    done

    if [ ${#MISSING[@]} -ne 0 ]; then
        echo "[!] Missing dependencies: ${MISSING[*]}"
        echo "[?] Attempting to identify OS and install..."

        if [[ "$OSTYPE" == "linux-gnu"* ]]; then
            if command -v apt &> /dev/null; then
                echo "Detected Ubuntu/Debian. Installing..."
                sudo apt update && sudo apt install -y gcc-x86-64-elf-binutils nasm grub-common grub-pc-bin xorriso mtools qemu-system-x86 kconfig-frontends
            elif command -v pacman &> /dev/null; then
                echo "Detected Arch Linux. Installing..."
                sudo pacman -S --needed gcc-x86_64-elf nasm grub libisoburn mtools qemu-full
                yay -S kconfig-frontends
            fi
        elif [[ "$OSTYPE" == "darwin"* ]]; then
            echo "Detected macOS. Installing via Homebrew..."
            brew install x86_64-elf-gcc nasm qemu xorriso mtools
            brew tap osx-cross/arm && brew install kconfig-frontends
        else
            echo "[ERROR] Unknown OS. Please install dependencies manually: ${MISSING[*]}"
            exit 1
        fi
    fi
}

# Run the check
check_and_install

# --- 2. MAP KCONFIG TO QEMU ---
# If .config exists, Make will have exported these. If not, we use defaults.
Q_MEM=$(echo ${CONFIG_MEM_SIZE:-2G} | tr -d '"')
Q_SMP=$(echo ${CONFIG_CPU_CORES:-5} | tr -d '"')
Q_ARCH=$(echo ${CONFIG_MACHINE_TYPE:-q35} | tr -d '"')
Q_CPU=$(echo ${CONFIG_CPU_TYPE:-qemu64} | tr -d '"')
Q_DEBUG=$(echo ${CONFIG_DEBUG_FLAGS:-guest_errors} | tr -d '"')

# Handle VGA Choice logic
Q_VGA="std"
if [ "$CONFIG_VGA_NONE" = "y" ]; then Q_VGA="none"; fi
if [ "$CONFIG_VGA_VIRTIO" = "y" ]; then Q_VGA="virtio"; fi

# Handle Audio logic
Q_AUDIO=""
if [ "$CONFIG_AUDIO_ENABLED" = "y" ]; then
    # Note: Modern QEMU prefers -audiodev. pcspk-audiodev connects the speaker to the backend.
    Q_AUDIO="-machine $Q_ARCH,pcspk-audiodev=audio0 -audiodev sdl,id=audio0 -soundhw ${CONFIG_SOUNDHW:-pcspk}"
fi

# --- 3. BUILD & RUN ---
echo "[INFO]: Cleaning and Building CaneOS..."
make clean && make all

echo "[INFO]: Launching QEMU ($Q_ARCH | $Q_CPU | $Q_MEM RAM)"

qemu-system-x86_64 \
    -m $Q_MEM \
    -smp $Q_SMP \
    -machine $Q_ARCH \
    -cpu $Q_CPU \
    -vga $Q_VGA \
    -d $Q_DEBUG \
    -serial stdio \
    -cdrom bin/caneos.iso \
    $Q_AUDIO \
    $EXTRA_ARGS

# Cleanup after closing QEMU
make clean