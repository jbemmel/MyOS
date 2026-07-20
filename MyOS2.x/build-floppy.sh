#!/usr/bin/env bash
set -euo pipefail

cd "$(dirname "$0")"

build_dir=${BUILD_DIR:-build/debug}
floppy_image="$build_dir/floppy.img"
mount_dir=$(mktemp -d)
mounted=false

cleanup() {
  if $mounted; then sudo umount "$mount_dir"; fi
  rmdir "$mount_dir"
}
trap cleanup EXIT

test -f "$build_dir/kernel.bin" || {
  echo "Missing $build_dir/kernel.bin; run ./build-linux.sh first" >&2
  exit 1
}

truncate -s 1474560 "$floppy_image"
mkfs.fat -F 12 -n FREEDOS2000 "$floppy_image" >/dev/null
dd if=src/Init/bootsec of="$floppy_image" \
  bs=512 count=1 conv=notrunc status=none

sudo mount -o "loop,uid=$(id -u),gid=$(id -g)" "$floppy_image" "$mount_dir"
mounted=true
cp "$build_dir/kernel.bin" "$mount_dir/kernel.bin"
sync "$mount_dir/kernel.bin"
sudo umount "$mount_dir"
mounted=false

echo "Built MyOS2.x boot floppy: $floppy_image"
