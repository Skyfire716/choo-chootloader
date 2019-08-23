DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"
OVMFLOCATION=$DIR/EmulatoinFolders/run-ovmf/OVMF.fd

echo Create Part Table
parted $DIR/uefi.img -s -a minimal mklabel gpt
echo Create Partition
parted $DIR/uefi.img -s -a minimal mkpart EFI FAT16 2048s 93716s
echo Make Bootable
parted $DIR/uefi.img -s -a minimal toggle 1 boot
echo Temp Img
dd if=/dev/zero of=/tmp/part.img bs=512 count=91669
echo Format Img
mformat -i /tmp/part.img -h 32 -t 32 -n 64 -c 1
echo Copy EFI
mcopy -i /tmp/part.img $DIR/choot-chootloader.efi ::
echo Write to Img
dd if=/tmp/part.img of=$DIR/uefi.img bs=512 count=91669 seek=2048 conv=notrunc
echo Run...
read -p "Press enter to continue"
qemu-system-x86_64 -cpu kvm64 -m 1024 -bios $OVMFLOCATION -drive file=$DIR/uefi.img,if=ide -net none
