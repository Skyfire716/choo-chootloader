
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"
LOOPDEV=/dev/loop0
OVMFLOCATION=$DIR/EmulatoinFolders/run-ovmf/OVMF.fd
MODE="0"
PROGRAMM_NAME=""
POSITIONAL=()
EFIINCLOC=""
EFIINCx86_64LOC=""
EFIINCPROTOCOL=""
EFILIBDATA=""
while [[ $# -gt 0 ]]
do
key="$1"

case $key in
    -l|--loop)
    LOOPDEV=/dev/loop$2
    echo $LOOPDEV
    shift
    shift
    ;;
    --ovmf)
    OVMFLOCATION=$2
    echo $OVMFLOCATION
    shift
    shift
    ;;
    -m|--mode)
    MODE=$2
    echo $MODE
    shift
    shift
    ;;
    -p|--programm)
    PROGRAMM_NAME=$2
    echo $PROGRAMM_NAME
    shift
    shift
    ;;
    -e|--efi)
    EFIINCLOC=$2
    EFIINCx86_64LOC=$3
    EFIINCPROTOCOL=$4
    EFILIBDATA=$5
    shift
    shift
    shift
    shift
    shift
    echo "gnu-efi/inc=$EFIINCLOC"
    echo "gnu-efi/inc/x86_64=$EFIINCx86_64LOC"
    echo "gnu-efi/inc/protocol=$EFIINCPROTOCOL"
    echo "gnu-efi/lib/data.c=$EFILIBDATA"
    ;;
esac
done

function umountf {
    umount /mnt
    losetup -d $LOOPDEV
}
function mountf {
    rm $DIR/uefi.img
    cp $DIR/blank-uefi.img $DIR/uefi.img
    losetup --offset 1048576 --sizelimit 46934528 $LOOPDEV $DIR/uefi.img
    mkdosfs -F 32 /dev/loop0
    mount $LOOPDEV /mnt
}
function runf {
     qemu-system-x86_64 -cpu kvm64 -m 1024 -bios $OVMFLOCATION -drive file=$DIR/uefi.img,if=ide -net none
}
function umount_runf {
    umountf 
    runf
}

function compilef {
    clean
    echo Compile
    x86_64-w64-mingw32-gcc -ffreestanding -I$EFIINCLOC -I$EFIINCx86_64LOC -I$EFIINCPROTOCOL -c -o $DIR/build/$PROGRAMM_NAME.o $DIR/src/$PROGRAMM_NAME.c
    echo Finished
    echo Compile Data
    x86_64-w64-mingw32-gcc -ffreestanding -I$EFIINCLOC -I$EFIINCx86_64LOC -I$EFIINCPROTOCOL -c -o $DIR/build/data.o $EFILIBDATA/data.c
    echo Finished
    echo Link
    x86_64-w64-mingw32-gcc -nostdlib -Wl,-dll -shared -Wl,--subsystem,10 -e efi_main -o $DIR/build/$PROGRAMM_NAME.efi $DIR/build/$PROGRAMM_NAME.o -lgcc -lefi -I$EFIINCLOC -I$EFIINCx86_64LOC -I$EFIINCPROTOCOL
    echo Finished
}

function rundebugf {
    qemu-system-x86_64 -cpu qemu64 -bios $OVMFLOCATION -hda fat:rw:$DIR/EmulatoinFolders/hda-contents/ -net none -debugcon file:debug.log -global isa-debugcon.iobase=0x402 
}

function clean {
    rm -r $DIR/build/  
    mkdir $DIR/build/
}

if [ $MODE == "run" ]
then
    runf
elif [ $MODE == "mount" ]
then
    mountf
elif [ $MODE == "umount" ]
then
    umountf
elif [ $MODE == "run!" ]
then
    umount_runf
elif [ $MODE == "compile" ]
then   
    mkdir $DIR/build/
    compilef
elif [ $MODE == "all" ]
then
    mountf
    compilef
    cp $DIR/build/$PROGRAMM_NAME.efi /mnt/
    umountf
    runf
elif [ $MODE == "debug" ]
then 
    compilef
    cp $DIR/build/$PROGRAMM_NAME.efi $DIR/EmulatoinFolders/hda-contents/EFI/BOOT/BOOTX64.efi
    sleep 1
    rundebugf
elif [ $MODE == "make" ]
then
    mountf
    cp $DIR/build/$PROGRAMM_NAME.efi /mnt/
    umountf
    runf
fi

