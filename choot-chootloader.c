#include <efi.h>
#include <efilib.h>
#include "sl.h"

static CHAR16* a2u(char *str)
{
	static CHAR16 mem[2048];
	int i;
	for (i = 0; str[i]; ++i)
	{
		mem[i] = (CHAR16) str[i];
	}
	mem[i] = 0;
	return mem;
}

void map(int i)
{
  if(i == 0)
  {	
  	Print(L"0");
  }else if (i == 1){
  	Print(L"1");
  }else if (i == 2){
  	Print(L"2");
  }else if (i == 3){
  	Print(L"3");
  }else if (i == 4){
  	Print(L"4");
  }else if (i == 5){
  	Print(L"5");
  }else if (i == 6){
  	Print(L"6");
  }else if (i == 7){
  	Print(L"7");
  }else if (i == 8){
  	Print(L"8");
  }else if (i == 9){
  	Print(L"9");
  }else{
  	map(i / 10);
	map(i % 10);
  }
}

int mcaddch(int y, int x, char c)
{
	uefi_call_wrapper(ST->ConOut->SetCursorPosition, 1, ST->ConOut, y, x);
	char s[2];
	s[0] = c;
	s[1] = '\0';	
	Print(a2u(s));
	return 0;
}

EFI_STATUS
EFIAPI
efi_main (EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable)
{
	InitializeLib(ImageHandle, SystemTable);
	SystemTable->BootServices->SetWatchdogTimer(0, 0, 0, NULL);
	uefi_call_wrapper(ST->ConOut->ClearScreen, 1, ST->ConOut);
	Print(L"Hallo, World!\n\r");
	CHAR16 *Str =  L"yehaw\n\r";
	Print(Str);
	char* s[1];
	s[0] = ' ';
	Print(L"%p\n\r", mcaddch);
	map(25);
	Print(L"\n\r");
	UINTN columns = 0;
	UINTN rows = 0;
	for (int i = 0; i < 10; i++){
		Print(a2u("Mode Level "));
		map(i);
		Print(L"\n\r");
		EFI_STATUS status;
		status = uefi_call_wrapper(ST->ConOut->QueryMode, 1, ST->ConOut, i, &columns, &rows);
		if (EFI_SUCCESS == status){
			map(columns);
			Print(L", ");
			map(rows);
			Print(L"\n\r");
		}else if (EFI_DEVICE_ERROR == status){
			Print(L"DEVICE_ERROR\n\r");
		}else {
			Print(L"Unsupported\n\r");
			if (i > 2){
				i = 10;
			}
		}
	}
	uefi_call_wrapper(ST->BootServices->Stall, 1, 2000000);
	uefi_call_wrapper(ST->ConOut->ClearScreen, 1, ST->ConOut);
	uefi_call_wrapper(ST->ConOut->SetCursorPosition, 1, ST->ConOut, 0, 0);
	main(0, s, mcaddch);
	uefi_call_wrapper(ST->BootServices->Stall, 1, 20000000);
	Print(L"Ende\n\r");
	return EFI_SUCCESS;
}
