#include <efi.h>
#include <efilib.h>
#include "efibind.h"
#include "efidef.h"
#include "efidevp.h"
#include "eficon.h"
#include "efiapi.h"
#include "efierr.h"
#include "efiprot.h"
#include "sl.h"
#include <string.h>

#define UP 1
#define DOWN 2
#define RIGHT 3
#define LEFT 4
#define ENTER 0


static EFI_GUID BlockIoProtocolGUID = BLOCK_IO_PROTOCOL;
static EFI_GUID DevicePathGUID = DEVICE_PATH_PROTOCOL;

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

void map(int i, char* s)
{
    if(i == 0){	
        strcat(s, "0");
    }else if (i == 1){
        strcat(s, "1");
    }else if (i == 2){
        strcat(s, "2");
    }else if (i == 3){
        strcat(s, "3");
    }else if (i == 4){
        strcat(s, "4");
    }else if (i == 5){
        strcat(s, "5");
    }else if (i == 6){
        strcat(s, "6");
    }else if (i == 7){
        strcat(s, "7");
    }else if (i == 8){
        strcat(s, "8");
    }else if (i == 9){
        strcat(s, "9");
    }else{
        map(i / 10, s);
        map(i % 10, s);
    }
}

int cols = 0;
int oldy = 0;
int my_mvaddch(int y, int x, char c)
{
    if (x >= 0 && x < cols){
      uefi_call_wrapper(ST->ConOut->SetCursorPosition, 1, ST->ConOut, x, y);
      char s[2];
      s[0] = c;
      s[1] = '\0';
        Print(a2u(s));
    }
    if (oldy - y >= 5) {
        uefi_call_wrapper(ST->BootServices->Stall, 1, 40000);
//    uefi_call_wrapper(ST->BootServices->Stall, 1, 80000);
//        uefi_call_wrapper(ST->ConOut->ClearScreen, 1, ST->ConOut);
    }
    oldy = y;
    return 0;
}



void printInt(int value) {
	CHAR16 out[32];
	CHAR16 *ptr = out;
	if (value == 0) {
		Print(L"0");
		return;
	}

	ptr += 31;
	*--ptr = 0;
	int tmp = value;// >= 0 ? value : -value;

	while (tmp) {
		*--ptr = '0' + tmp % 10;
		tmp /= 10;
	}
	if (value < 0) *--ptr = '-';
	Print(ptr);
}

void printLabeledInt(CHAR16 *label, int value) {
  Print(label);
  printInt(value);
  Print(L"\r\n");
}

CHAR16 asChar(UINT8 nibble) {
  return nibble + (nibble < 10 ? '0' : '7');
}

void printUUID(UINT8 uuid[16]) {
  int charPos = 0;
  int i;
  CHAR16 *uuidStr= L"00000000-0000-0000-0000-000000000000";
  for(i = 3; i >= 0; i--) {
    uuidStr[charPos++] = asChar(uuid[i] >> 4);
    uuidStr[charPos++] = asChar(uuid[i] & 0xF);
  }
  charPos++;
  for(i = 5; i >= 4; i--) {
    uuidStr[charPos++] = asChar(uuid[i] >> 4);
    uuidStr[charPos++] = asChar(uuid[i] & 0xF);
  }
  charPos++;
  for(i = 7; i >= 6; i--) {
    uuidStr[charPos++] = asChar(uuid[i] >> 4);
    uuidStr[charPos++] = asChar(uuid[i] & 0xF);
  }
  charPos++;
  for(i = 8; i <= 9; i++) {
    uuidStr[charPos++] = asChar(uuid[i] >> 4);
    uuidStr[charPos++] = asChar(uuid[i] & 0xF);
  }

  for(i = 10; i < 16; i++) {
    if(i == 4 || i == 6 || i == 8 || i == 10) charPos++;
    uuidStr[charPos++] = asChar(uuid[i] >> 4);
    uuidStr[charPos++] = asChar(uuid[i] & 0xF);
  }
Print(L"\r\n");
Print(uuidStr);
}




void printDevicePath(EFI_DEVICE_PATH *devicePath) {
  EFI_DEVICE_PATH *node = devicePath;
  // https://github.com/vathpela/gnu-efi/blob/master/lib/dpath.c for printing device paths.
  //TODO: Create a file image with gpt partion and a haiku image.
  Print(L"Device type: ");
  Print(L"  ");

  for (; !IsDevicePathEnd(node); node = NextDevicePathNode(node)) {
    Print(L" \\ ");
    printInt(node->Type);
    Print(L".");
    printInt(node->SubType);
    
    if( node->Type == MEDIA_DEVICE_PATH && node->SubType == MEDIA_HARDDRIVE_DP ) {
      Print(L"\r\n");

      HARDDRIVE_DEVICE_PATH *hdPath = (HARDDRIVE_DEVICE_PATH *) node;
      printLabeledInt(L"  [HARDDRIVE_DEVICE_PATH] Partition nr: ",  hdPath->PartitionNumber);
      printLabeledInt(L"  [HARDDRIVE_DEVICE_PATH] Partition start: ",  hdPath->PartitionStart);
      printLabeledInt(L"  [HARDDRIVE_DEVICE_PATH] Partition size: ",  hdPath->PartitionSize);
      printLabeledInt(L"  [HARDDRIVE_DEVICE_PATH] MBR type: ",  hdPath->MBRType);
      printLabeledInt(L"  [HARDDRIVE_DEVICE_PATH] Signature type: ",  hdPath->SignatureType);

      printUUID(hdPath->Signature);
      Print(L"\r\n\r\n");
    }
  }
  Print(L"\r\n");
}


EFI_STATUS
EFIAPI
efi_main (EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable)
{
    InitializeLib(ImageHandle, SystemTable);
    SystemTable->BootServices->SetWatchdogTimer(0, 0, 0, NULL);
    uefi_call_wrapper(ST->ConOut->ClearScreen, 1, ST->ConOut);
    CHAR16 *Str =  L"yehaw\n\r";
    //Print(Str);
    char* s[1];
    s[0] = ' ';
    UINTN columns = 0;
    UINTN rows = 0;
    char resolution[20][200];
    for (int i = 0; i < 20; i++){
        for(int j = 0; j < 200; j++){
            resolution[i][j] = '\0';
        }
    }
    int index2mode[20];
    for (int i = 0; i < 20; i++){
        index2mode[i] = -1;
    }
    int maxMode = 0;
    for (int i = 0; i < 20; i++){
        EFI_STATUS status;
        status = uefi_call_wrapper(ST->ConOut->QueryMode, 1, ST->ConOut, i, &columns, &rows);
        if (EFI_SUCCESS == status){
            map(i, resolution[i]);
            strcat(resolution[i], ": Mode Level x: ");
            map(columns, resolution[i]);
            strcat(resolution[i], ", y: ");
            map(rows, resolution[i]);
            strcat(resolution[i], "\n\r");
            int j = 0;
            while(index2mode[j] != -1){
                j++;
            }
            index2mode[j] = i;
            maxMode++;
        }else if (EFI_DEVICE_ERROR == status){
            //Print(L"DEVICE_ERROR\n\r");
        }else {
            //Print(L"Unsupported\n\r");
        }
    }
    EFI_INPUT_KEY key;
    EFI_STATUS st;
    int selectedMode = 0;
    int run = 1;
    while(run) {
        Print(L"Welcome to the ChootChootLoader!\n\r");
        Print(L"Select the Console Dimensions\n\r");
        Print(L"\n\r");
        Print(L"\n\r");
        for(int i = 0; i < 20; i++){
            if(resolution[i][0] != '\0'){
                if(index2mode[selectedMode] == i){
                    Print(L"=>");
                    Print(a2u(resolution[i]));
                }else {
                    Print(L"  ");
                    Print(a2u(resolution[i]));
                }
            }
        }
        WaitForSingleEvent(ST->ConIn->WaitForKey, 0);
        uefi_call_wrapper(ST->ConIn->ReadKeyStroke, 1, ST->ConIn, &key);
        //Print(L"ScanCode: %x  UnicodeChar: %xh CallRtStatus: %x\n", key.ScanCode, key.UnicodeChar, ST);
        if(key.ScanCode == UP){
           // Print(L"HOCH\n");
            selectedMode--;
        }else if (key.ScanCode == DOWN){
            //Print(L"RUNTER\n");
            selectedMode++;
        }else if (key.ScanCode == RIGHT) {
            //Print(L"Rechts\n");
	    selectedMode++;
        }else if (key.ScanCode == LEFT){
            //Print(L"Links\n");
	    selectedMode--;
        }else if (key.ScanCode == ENTER){
            Print(L"Enter\n");
	    run = 0;
        }
        selectedMode %= maxMode;
        uefi_call_wrapper(ST->ConOut->ClearScreen, 1, ST->ConOut);
    }
    uefi_call_wrapper(ST->ConOut->SetMode, 1, ST->ConOut, index2mode[selectedMode]);
    uefi_call_wrapper(ST->ConOut->QueryMode, 1, ST->ConOut, index2mode[selectedMode], &columns, &rows);
    cols = columns;
    uefi_call_wrapper(ST->ConOut->ClearScreen, 1, ST->ConOut);
    uefi_call_wrapper(ST->ConOut->SetCursorPosition, 1, ST->ConOut, 0, 0);
   main(0, s, &my_mvaddch, columns, rows);
    uefi_call_wrapper(ST->ConOut->ClearScreen, 1, ST->ConOut);
    uefi_call_wrapper(ST->ConOut->SetCursorPosition, 1, ST->ConOut, 0, 0);
    Print(L"Check for GUID\n\r");
    EFI_HANDLE handles[100];
    EFI_DEVICE_PATH *devicePath;
    EFI_BLOCK_IO *blockIOProtocol;
    UINTN bufferSize = 100 * sizeof(EFI_HANDLE);
    int i, noOfHandles;
    
    EFI_STATUS status2 = uefi_call_wrapper(ST->BootServices->LocateHandle, 1,
        ByProtocol, 
        &BlockIoProtocolGUID, 
        NULL, /* Ignored for AllHandles or ByProtocol */
        &bufferSize, 
	handles);
    noOfHandles = bufferSize == 0 ? 0 : bufferSize / sizeof(EFI_HANDLE);
    char handleout[200] = "Found Handles: ";
    map(noOfHandles, handleout);
    strcat(handleout, "\n\r");
    Print(a2u(handleout));
if(status2 == EFI_NOT_FOUND){
	Print(L"No Matching Search\r\n");
	return status2;
}else if (status2 == EFI_BUFFER_TOO_SMALL){
	Print(L"BuffersizeToSmall\r\n");
	return status2;
}else if (status2 == EFI_INVALID_PARAMETER){
	Print(L"Parameter falsch\r\n");
	return status2;
}
if (EFI_ERROR(status2)) {
    Print(L"Failed to LocateHandles!\r\n");
    return status2;
  }
  for (i = 0; i < noOfHandles; i++) {
    status2 = uefi_call_wrapper(ST->BootServices->HandleProtocol, 1, handles[i], &DevicePathGUID, (void *) &devicePath);
    if (EFI_ERROR(status2) || devicePath == NULL) {
      Print(L"Skipped handle, device path error!\r\n");
      continue;
    }
    status2 = uefi_call_wrapper(ST->BootServices->HandleProtocol, 1, handles[i], &BlockIoProtocolGUID, (void *) &blockIOProtocol);
    if (EFI_ERROR(status2) || blockIOProtocol == NULL) {
      Print(L"Skipped handle, block io protocol error!\r\n");
      continue;
    }
    printDevicePath(devicePath);
    char f[200];
    strcat(f, "Media ID:");
    map(blockIOProtocol->Media->MediaId, f);
    strcat(f, "\n\r");
    Print(a2u(f));
}

    return EFI_SUCCESS;
}
