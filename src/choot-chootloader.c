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
#include <features.h>
#include <string.h>

#define UP 1
#define DOWN 2
#define RIGHT 3
#define LEFT 4
#define ENTER 0

static EFI_GUID BlockIoProtocolGUID = BLOCK_IO_PROTOCOL;
static EFI_GUID DevicePathGUID = DEVICE_PATH_PROTOCOL;


char n[1] = "0";
CHAR16 printable[93] = {' ','!','"','#', '$', '%', '&','\'','(',')','*','+',',','-','_','.', '/', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', ':', ';', '<', '=', '>', '?', '@','A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', '[', '~',']', '↑','←','@','a','b','c','d','e','f','g','h','i','j','k','l','m','n','o','p','q','r','s','t','u','v','w','x','y','z','{','}',};

struct file{
    EFI_FILE* root;
    EFI_FILE* file;
    EFI_STATUS status;
};

struct gummiboot_conf{
    char default_loader[200];
    unsigned int timeout;
    char editor_b;
    char auto_entries;
    char auto_firmware;
    char console_mode;
};

struct loader_entries{
    char title[200];
    char version[200];
    char machine_id[200];
    char efi[20][1024];
    char options[20][1024];
    int efi_index;
    int options_index;
} entries[100];


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

int mystrcpy(char* d, int index, CHAR16* s, int length){
    for(int i = 0; i < length; i++){
        d[index++] = s[i];
    }
    return index;
}

int map(int i, char* s, int index)
{
    if(i < 0){
        s[index] = '-';
        return index + 1;
    }
    if(i == 0){	
        s[index] = '0';
        return index + 1;
    }else if (i == 1){
        s[index] = '1';
        return index + 1;
    }else if (i == 2){
        s[index] = '2';
        return index + 1;
    }else if (i == 3){
        s[index] = '3';
        return index + 1;
    }else if (i == 4){
        s[index] = '4';
        return index + 1;
    }else if (i == 5){
        s[index] = '5';
        return index + 1;
    }else if (i == 6){
        s[index] = '6';
        return index + 1;
    }else if (i == 7){
        s[index] = '7';
        return index + 1;
    }else if (i == 8){
        s[index] = '8';
        return index + 1;
    }else if (i == 9){
        s[index] = '9';
        return index + 1;
    }else{
        index = map(i / 10, s, index);
        return map(i % 10, s, index);
    }
}

int cols = 0;
int rows = 0;
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
	//}else if(node->Type == HARDWARE_DEVICE_PATH && node->SubType == ) {

	}
    }
    Print(L"\r\n");
}

void dimensionSelection(){
	UINTN columns = 0, rowsl = 0;
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
        status = uefi_call_wrapper(ST->ConOut->QueryMode, 1, ST->ConOut, i, &columns, &rowsl);
        if (EFI_SUCCESS == status){
            int index = 0;
            index = map(i, resolution[i], index);
            index = mystrcpy(resolution[i], index, L": Mode Level x: ", 16);
            index = map(columns, resolution[i], index);
            index = mystrcpy(resolution[i], index, L", y: ", 5);
            index = map(rowsl, resolution[i], index);
            index = mystrcpy(resolution[i], index, L"\n\r", 2);
            int j = 0;
            while(j < 20 && index2mode[j] != -1){
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
    uefi_call_wrapper(ST->ConOut->QueryMode, 1, ST->ConOut, index2mode[selectedMode], &columns, &rowsl);
    cols = columns;
    rows = rowsl;
}

EFI_STATUS
EFIAPI
efi_main (EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable)
{
    InitializeLib(ImageHandle, SystemTable);
    SystemTable->BootServices->SetWatchdogTimer(0, 0, 0, NULL);
    uefi_call_wrapper(ST->ConOut->ClearScreen, 1, ST->ConOut);
    //CHAR16 *Str =  L"yehaw\n\r";
    //Print(Str);
    char* s[1];
    s[0] = ' ';
    //dimensionSelection();
    uefi_call_wrapper(ST->ConOut->ClearScreen, 1, ST->ConOut);
    uefi_call_wrapper(ST->ConOut->SetCursorPosition, 1, ST->ConOut, 0, 0);
    //main(0, s, &my_mvaddch, cols, rows);
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
    map(noOfHandles, handleout, 16);
    int index = mystrcpy(handleout, 15, L"\n\r", 4);
    //strcat(handleout, "\n\r");
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
        int index2 = mystrcpy(f, 0, L"Media ID: ", 10);
        //strcat(f, "Media ID:");
        map(blockIOProtocol->Media->MediaId, f, 10);
        index2 = mystrcpy(f, index2, L"\n\r", 4);
        //strcat(f, "\n\r");
        //Print(a2u(f));
    }
    Print(L"\r\n\r\nSimpleFileSystemProtocol\r\n");
    EFI_GUID sfspGuid = EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_GUID;
    EFI_GUID gEfiLoadedImageProtocolGuid = EFI_LOADED_IMAGE_PROTOCOL_GUID;
    EFI_LOADED_IMAGE_PROTOCOL *LoadedImage;
    EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *FileSystem;
    EFI_HANDLE* handless = NULL;   
    UINTN handleCount = 0;
//    EFI_STATUS efiStatus = uefi_call_wrapper(ST->BootServices->LocateHandleBuffer, 1, ByProtocol, &sfspGuid, NULL, &handleCount, &handless);
    EFI_STATUS efiStatus = uefi_call_wrapper(ST->BootServices->HandleProtocol, 1, ImageHandle, &gEfiLoadedImageProtocolGuid, (void**)&LoadedImage);

//    for (index = 0; index < (int)handleCount; ++ index)
//    {
            EFI_SIMPLE_FILE_SYSTEM_PROTOCOL* fs = NULL;
            EFI_DEVICE_PATH *Dp;
            //efiStatus = uefi_call_wrapper(ST->BootServices->HandleProtocol, 1, handless[index], &sfspGuid, (void**)&fs);
            efiStatus = uefi_call_wrapper(ST->BootServices->HandleProtocol, 1, LoadedImage->DeviceHandle, &sfspGuid, (void**)&fs);
            if (EFI_ERROR(efiStatus) || fs == NULL){
                Print(L"Skipped handle, Simple File System Error error!\r\n");
            }
  //          efiStatus = uefi_call_wrapper(ST->BootServices->HandleProtocol, 1, handles[i], &DevicePathGUID, (void *) &devicePath);
            if (EFI_ERROR(status2) || devicePath == NULL) {
                Print(L"Skipped handle, device path error!\r\n");
            }
            //printDevicePath(devicePath);
            Print(L"Try To Open File System\r\n");
            EFI_FILE* root = NULL;
            efiStatus = uefi_call_wrapper(fs->OpenVolume, 1, fs, &root);
            if (efiStatus == EFI_SUCCESS) {
                Print(L"Succesfully Opened\r\n");
            }
            if (efiStatus == EFI_UNSUPPORTED){
                Print(L"Unsupported\r\n");
            }
            if(efiStatus == EFI_NO_MEDIA){
                Print(L"NO MEDI\r\n");
            }
            if(efiStatus == EFI_DEVICE_ERROR){
                Print(L"Device Error\r\n");
            }
            if(efiStatus == EFI_VOLUME_CORRUPTED){
                Print(L"CorruptVolum\r\n");
            }
            if(efiStatus == EFI_ACCESS_DENIED){
                Print(L"ACCESS_DENIED\r\n");
            }
            if(efiStatus == EFI_OUT_OF_RESOURCES){
                Print(L"OUT_OF_RESOURCES\r\n");
            }
            if(efiStatus == EFI_MEDIA_CHANGED){
                Print(L"Media Changed\r\n");
            }
            Print(L"Something Else?\r\n");
            Print(L"Try to open File\r\n");
            EFI_FILE* token = NULL;
            efiStatus = uefi_call_wrapper(root->Open, 1, root, &token, L"EFI\\Boot\\test.txt", EFI_FILE_MODE_READ | EFI_FILE_MODE_WRITE, EFI_FILE_READ_ONLY);
//            efiStatus = root->Open(root, &token, L"/EFI/Boot/test.txt", EFI_FILE_MODE_READ, EFI_FILE_READ_ONLY);
            if(efiStatus == EFI_SUCCESS){
                Print(L"File Successfully Opened\r\n");
                char buffer[100] = "";
                bufferSize = 20;
                efiStatus = uefi_call_wrapper(root->Read, 1, token, &bufferSize, &buffer);
                char l[100];
                int index3 = map(bufferSize, l, 0);
                index3 = mystrcpy(l, index3, L" Bytes gelesen\r\n", 18);
                Print(a2u(l));
                Print(a2u(buffer));
                Print(L"\r\n");
                buffer[20] = "A";
                buffer[21] = "B";
                buffer[22] = "C";
                bufferSize = 22;
                efiStatus = uefi_call_wrapper(root->Write, 1, token, &bufferSize, &buffer);
                efiStatus = uefi_call_wrapper(root->Flush, 1, token);
                efiStatus = uefi_call_wrapper(root->Close, 1, root);
            }else {
                Print(L"Couldn't Open\r\n");
                if(efiStatus == EFI_NOT_FOUND){
                    Print(L"test.txt not found\r\n");
                }
                if(efiStatus == EFI_NO_MEDIA){
                    Print(L"No Media\r\n");
                }
                if (efiStatus == EFI_MEDIA_CHANGED){
                    Print(L"Media Changed\r\n");
                }
               if (efiStatus == EFI_DEVICE_ERROR){
                    Print(L"Device Error\r\n");
                }
               if (efiStatus == EFI_VOLUME_CORRUPTED){
                    Print(L"Volume Corrupted\r\n");
                }
               if (efiStatus == EFI_WRITE_PROTECTED){
                    Print(L"WriteProtected\r\n");
                }
               if (efiStatus == EFI_ACCESS_DENIED){
                    Print(L"Access Dienied\r\n");
                }
               if (efiStatus == EFI_OUT_OF_RESOURCES){
                    Print(L"Out of Resources\r\n");
                }
               if (efiStatus == EFI_VOLUME_FULL){
                    Print(L"Volume Full\r\n");
                }
               Print(L"No Info?\r\n");
           }
//        }
    return EFI_SUCCESS;
}
