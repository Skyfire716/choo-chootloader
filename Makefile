EFI_CERT = /usr/lib/crt0-efi-x86_64.o
EFI_LDS = /usr/lib/elf_x86_64_efi.lds
EFI_HEADER = /usr/include/efi
EFI_x86_64 = /usr/include/efi/x86_64
EFI_LIB = /usr/lib

choot-chootloader.efi: choot-chootloader.so choot-chootloader.o
	objcopy -j .text -j .sdata -j .data -j .dynamic -j .dynsym -j .rel -j .rela -j .reloc --target=efi-app-x86_64 choot-chootloader.so choot-chootloader.efi

choot-chootloader.so: choot-chootloader.o sl.o slh.o
	ld choot-chootloader.o sl.o  $(EFI_CERT) -nostdlib -znocombreloc -T $(EFI_LDS) -shared -Bsymbolic -L $(EFI_LIB) -l:libgnuefi.a -l:libefi.a -o choot-chootloader.so

choot-chootloader.o: choot-chootloader.c
	gcc choot-chootloader.c -c -fno-stack-protector -fpic -fshort-wchar -mno-red-zone -I $(EFI_HEADER) -I $(EFI_x86_64) -DEFI_FUNCTION_WRAPPER -o choot-chootloader.o

slh.o: sl.h
	gcc sl.h -c -fno-stack-protector -fpic -fshort-wchar -mno-red-zone -I $(EFI_HEADER) -I $(EFI_x86_64) -DEFI_FUNCTION_WRAPPER -o slh.o

sl.o: sl.c
	gcc sl.c -c -fno-stack-protector -fpic -fshort-wchar -mno-red-zone -I $(EFI_HEADER) -I $(EFI_x86_64) -DEFI_FUNCTION_WRAPPER -o sl.o

clean: 
	rm -f choot-chootloader.o choot-chootloader.so sl.o slh.o
