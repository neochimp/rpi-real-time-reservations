#!/usr/bin/env bash
set -euo pipefail

# === Configurable stuff ===
CONFIG_FILE="cs596-rpi-4.9.80-config"
KERNEL_NAME="kernel7"
CROSS_COMPILE="arm-linux-gnueabihf-"
DEVICE="/dev/sdc"          # SD card device (no partition number)
MNT_BOOT="mnt/boot"
MNT_ROOT="mnt/root"
JOBS="$(nproc)"

# === Setup mounts ===
mkdir -p "$MNT_BOOT" "$MNT_ROOT"

echo "[*] Copying kernel config..."
cp "$CONFIG_FILE" .config

echo "[*] Running menuconfig (interactive)..."
make ARCH=arm CROSS_COMPILE="$CROSS_COMPILE" menuconfig

echo "[*] Building kernel, modules, and DTBs..."
make ARCH=arm CROSS_COMPILE="$CROSS_COMPILE" zImage modules dtbs -j"$JOBS"

echo "[*] Mounting partitions..."
sudo mount "${DEVICE}1" "$MNT_BOOT"
sudo mount "${DEVICE}2" "$MNT_ROOT"

echo "[*] Installing modules to rootfs..."
sudo env PATH="$PATH" make ARCH=arm CROSS_COMPILE="$CROSS_COMPILE" \
    #INSTALL_MOD_PATH="$MNT_ROOT" \
    #-j"$JOBS" \
    #modules_install

echo "[*] Backing up existing kernel on boot partition..."
if [ -f "$MNT_BOOT/$KERNEL_NAME.img" ]; then
    sudo cp "$MNT_BOOT/$KERNEL_NAME.img" \
            "$MNT_BOOT/$KERNEL_NAME-backup-$(date +%Y%m%d%H%M%S).img"
fi

echo "[*] Copying new kernel image..."
sudo cp arch/arm/boot/zImage "$MNT_BOOT/$KERNEL_NAME.img"

echo "[*] Copying DTBs..."
sudo cp arch/arm/boot/dts/*.dtb "$MNT_BOOT/"

echo "[*] Copying DTB overlays..."
sudo cp arch/arm/boot/dts/overlays/*dtb* "$MNT_BOOT/overlays/"
sudo cp arch/arm/boot/dts/overlays/README "$MNT_BOOT/overlays/"

echo "[*] Unmounting..."
sudo umount "$MNT_BOOT"
sudo umount "$MNT_ROOT"

echo "[✓] Done!"

