EFI_CERT = /usr/lib/crt0-efi-x86_64.o
EFI_LDS = /usr/lib/elf_x86_64_efi.lds
EFI_HEADER = /usr/include/efi
EFI_x86_64 = /usr/include/efi/x86_64
EFI_LIB = /usr/lib

choot-chootloader.efi: build choot-chootloader.so choot-chootloader.o
	objcopy -j .text -j .sdata -j .data -j .dynamic -j .dynsym -j .rel -j .rela -j .reloc --target=efi-app-x86_64 Build/choot-chootloader.so choot-chootloader.efi

choot-chootloader.so: choot-chootloader.o sl.o slh.o
	ld Build/choot-chootloader.o Build/sl.o  $(EFI_CERT) -nostdlib -znocombreloc -T $(EFI_LDS) -shared -Bsymbolic -L $(EFI_LIB) -l:libgnuefi.a -l:libefi.a -o Build/choot-chootloader.so

choot-chootloader.o: src/choot-chootloader.c
	gcc src/choot-chootloader.c -c -fno-stack-protector -fpic -fshort-wchar -mno-red-zone -I $(EFI_HEADER) -I $(EFI_x86_64) -DEFI_FUNCTION_WRAPPER -o Build/choot-chootloader.o

slh.o: src/sl.h
	gcc src/sl.h -c -fno-stack-protector -fpic -fshort-wchar -mno-red-zone -I $(EFI_HEADER) -I $(EFI_x86_64) -DEFI_FUNCTION_WRAPPER -o Build/slh.o

sl.o: src/sl.c
	gcc src/sl.c -c -fno-stack-protector -fpic -fshort-wchar -mno-red-zone -I $(EFI_HEADER) -I $(EFI_x86_64) -DEFI_FUNCTION_WRAPPER -o Build/sl.o

build: build
	mkdir -p Build

clean: 
	rm -f -r Build
