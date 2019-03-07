ARCH            = $(shell uname -m | sed s,i[3456789]86,ia32,)
LIB_PATH        = /usr/lib64
EFI_INCLUDE     = /usr/include/efi
EFI_INCLUDES    = -nostdinc -I$(EFI_INCLUDE) -I$(EFI_INCLUDE)/$(ARCH) -I$(EFI_INCLUDE)/protocol

EFI_PATH        = /usr/lib64/gnuefi
EFI_CRT_OBJS    = /home/jonas/gnu-efi-3.0.9/gnuefi/crt0-efi-$(ARCH).o
EFI_LDS         = /home/jonas/gnu-efi-3.0.9/gnuefi/elf_$(ARCH)_efi.lds

CFLAGS          = -fno-stack-protector -fpic -fshort-wchar -mno-red-zone $(EFI_INCLUDES)
ifeq ($(ARCH),x86_64)
        CFLAGS  += -DEFI_FUNCTION_WRAPPER
endif

LDFLAGS         = -nostdlib -znocombreloc -T $(EFI_LDS) -shared -Bsymbolic -L$(EFI_PATH) -L$(LIB_PATH) \
                  $(EFI_CRT_OBJS) -lefi -lgnuefi

TARGET  = test.efi
OBJS    = test.o
SOURCES = test.c

all: $(TARGET)

test.so: $(OBJS)
	$(LD) -o $@ $(LDFLAGS) $^ $(EFI_LIBS)

%.efi: %.so
	objcopy -j .text -j .sdata -j .data \
		-j .dynamic -j .dynsym  -j .rel \
                -j .rela -j .reloc -j .eh_frame \
                --target=efi-app-$(ARCH) $^ $@ 
