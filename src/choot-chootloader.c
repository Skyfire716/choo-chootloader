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
ProcessFilesInDir (
    IN EFI_FILE_HANDLE Dir,
    IN EFI_DEVICE_PATH CONST *DirDp
)
{
    EFI_STATUS Status;
    EFI_FILE_INFO *FileInfo;
    CHAR16 *FileName;
    UINTN FileInfoSize;
    EFI_DEVICE_PATH *Dp;
    
    // big enough to hold EFI_FILE_INFO struct and
    // the whole file path
    FileInfo = AllocatePool (MAX_FILE_INFO_SIZE);
    if (FileInfo == NULL) {
        return EFI_OUT_OF_RESOURCES;
    }
    
    for (;;) {
        // get the next file's info. there's an internal position
        // that gets incremented when you read from a directory
        // so that subsequent reads gets the next file's info
        FileInfoSize = MAX_FILE_INFO_SIZE;
        //Status = Dir->Read (Dir, &FileInfoSize, (VOID *) FileInfo);
        Status = uefi_call_wrapper(Dir->Read, 1, Dir, &FileInfoSize, (VOID *) FileInfo);
        if (EFI_ERROR (Status) || FileInfoSize == 0) { //this is how we eventually exit this function when we run out of files
            if (Status == EFI_BUFFER_TOO_SMALL) {
                Print (L"EFI_FILE_INFO > MAX_FILE_INFO_SIZE. Increase the size\n");
            }
            FreePool (FileInfo);
            return Status;
        }
        
        // skip files named . or ..
        if (StrCmp (FileName, L".") == 0 || StrCmp (FileName, L"..") == 0) {
            continue;
        }
        
        // so we have absolute device path to child file/dir
        Dp = FileDevicePath (DirDp, FileName);
        if (Dp == NULL) {
            FreePool (FileInfo);
            return EFI_OUT_OF_RESOURCES;
        }
        
        // Do whatever processing on the file
        //        PerFileFunc (Dir, DirDp, FileInfo, Dp);
        Print (L"FileName = %s\n", FileInfo->FileName);
        /*
        if (FileInfo->Attribute & EFI_FILE_DIRECTORY) {
            //
            // recurse
            //
            
            EFI_FILE_HANDLE NewDir;
            
            //            Status = Dir->Open (Dir, &NewDir, FileName, EFI_FILE_MODE_READ, 0);
            Status = uefi_call_wrapper(Dir->Open, 1, Dir, &NewDir, FileName, EFI_FILE_MODE_READ, 0);
            if (Status != EFI_SUCCESS) {
                FreePool (FileInfo);
                FreePool (Dp);
                return Status;
            }
            NewDir->SetPosition (NewDir, 0);
            
            Status = ProcessFilesInDir (NewDir,Dp);
            //            Dir->Close (NewDir);
            Status = uefi_call_wrapper(Dir->Close, 1, NewDir);
            if (Status != EFI_SUCCESS) {
                FreePool (FileInfo);
                FreePool (Dp);
                return Status;
            }
        }
        */
        FreePool (Dp);
    }
}
/*
 * EFI_STATUS
 * EFIAPI PerFileFunc (
 *    IN EFI_FILE_HANDLE Dir,
 *    IN EFI_DEVICE_PATH *DirDp,
 *    IN EFI_FILE_INFO *FileInfo,
 *    IN EFI_DEVICE_PATH *Dp
 * )
 * {
 *    EFI_STATUS Status;
 *    EFI_FILE_HANDLE File;
 *    
 *    Print (L"Path = %s FileName = %s\n", ConvertDevicePathToText(DirDp, TRUE,
 *                                                                 TRUE), FileInfo->FileName);
 *    
 *    // read the file into a buffer
 *    Status = Dir->Open (Dir, &File, FileInfo->FileName, EFI_FILE_MODE_READ,
 *                        0);
 *    if (EFI_ERROR (Status)) {
 *        return Status;
 *    }
 *    
 *    // reset position just in case
 *    File->SetPosition (File, 0);
 *    
 *    // ****Do stuff on the file here****
 *    
 *    Dir->Close (File);
 *    
 *    return EFI_SUCCESS;
 * }
 */

struct gummiboot_conf parseGummibootConf(struct file file){
    struct gummiboot_conf conf;
    char buffer[1024] = "";
    UINTN bufferSize = 1024;
    file.status = uefi_call_wrapper(file.root->Read, 1, file.file, &bufferSize, &buffer);
    char line[100] = "";
    int bufferindex = 0;
    int index = 0;
    SCANLINES : index = 0;
    for(int i = 0; i < 100; i++){
            line[i] = ' ';
        }
    if(buffer[bufferindex] == '#'){
        while(buffer[bufferindex] != '\r' && buffer[bufferindex] != '\n'){
            bufferindex++;
        }
        if(buffer[bufferindex] == '\n'){
            bufferindex++;
        }
    }else{
        while(buffer[bufferindex] != '\r' && buffer[bufferindex] != '\n' && index < 98){
            line[index] = buffer[bufferindex];
            index++;
            bufferindex++;
        }
        if(buffer[bufferindex] == '\n'){
            bufferindex++;
        }
    }
        if(matchstring(line, "default", 7)){
            char name[93] = "";
            for(int i = 9; i < index;i++){
                name[i - 9] = line[i];
            }
            for(int i = 0; i < 93; i++){
                conf.default_loader[i] = name[i];
            }
        }
        if(matchstring(line, "timeout", 7)){
            char nummer[93] = "";
            for(int i = 7; i < index;i++){
                nummer[i-7]=line[i];
            }
            conf.timeout = string_to_int(nummer, 93);
        }
        if(matchstring(line, "editor", 6)){
            if(matchstring_i(line, "no", 2, 8)){
                conf.editor_b = 0;
            }else if(matchstring_i(line, "yes", 3, 8)){
                conf.editor_b = 1;
            }else{
                Print(L"Unknown Editor Config\r\n");
            }
        }
        if(matchstring(line, "auto-entries", 12)){
            char boolval[88] = "";
            for(int i = 0; i < index - 12; i++){
                boolval[i - 12] = line[i];
            }
            conf.auto_entries = string_to_int(boolval, 88);
        }
        if(matchstring(line, "auto-firmware", 13)){
            char boolval[87] = "";
            for(int i = 0; i < index - 12; i++){
                boolval[i - 12] = line[i];
            }
            conf.auto_firmware = string_to_int(boolval, 87);
        }
        if(matchstring(line, "console_mode", 12)){
            if(matchstring_i(line, "auto", 4, 15)){
                conf.console_mode = 0;
            }else if(matchstring_i(line, "max", 3, 15)){
                conf.console_mode = 0;
            }else if(matchstring_i(line, "keep", 4, 15)){
                conf.console_mode = 0;
            }else{
            char boolval[87] = "";
            for(int i = 0; i < index - 13; i++){
                boolval[i - 13] = line[i];
            }
            conf.console_mode = string_to_int(boolval, 87);
            }
        }
        if(bufferindex < bufferSize){
            goto SCANLINES;
        }
        return conf;
}

void closeFile(struct file file){
    file.status = uefi_call_wrapper(file.root->Close, 1, file.root);    
}

void platzhalter(struct file file){
    char buffer[100] = "";
    UINTN bufferSize = 20;
    file.status = uefi_call_wrapper(file.root->Read, 1, file.file, &bufferSize, &buffer);
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
    file.status = uefi_call_wrapper(file.root->Write, 1, file.file, &bufferSize, &buffer);
    file.status = uefi_call_wrapper(file.root->Flush, 1, file.file);    
}

struct file openFile(EFI_SIMPLE_FILE_SYSTEM_PROTOCOL* fs, CHAR16* name){
    Print(name);
    Print(L"\n\r"); 
    EFI_STATUS efiStatus;
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
        Print(L"EFI_ACCESS_DENIED\r\n");
    }
    if(efiStatus == EFI_OUT_OF_RESOURCES){
        Print(L"OUT_OF_RESOURCES\r\n");
    }
    if(efiStatus == EFI_MEDIA_CHANGED){
        Print(L"Media Changed\r\n");
    }
    Print(L"Something Else?\r\n");
    Print(L"Try to open File\r\n");
    //EFI_FILE* loaderEntries = NULL;
    //efiStatus = uefi_call_wrapper(root->Open, 1, root, &loaderEntries, L"loader\\entries", EFI_FILE_MODE_READ | //EFI_FILE_MODE_WRITE, EFI_FILE_READ_ONLY);
    //efiStatus = ProcessFilesInDir(loaderEntries, Dp);
    
    
    EFI_FILE* token = NULL;
    efiStatus = uefi_call_wrapper(root->Open, 1, root, &token, name, EFI_FILE_MODE_READ | EFI_FILE_MODE_WRITE, EFI_FILE_READ_ONLY);
    if(efiStatus == EFI_SUCCESS){
        Print(L"File Successfully Opened\r\n");
        /*
         *        char buffer[100] = "";
         *        UINTN bufferSize = 20;
         *        efiStatus = uefi_call_wrapper(root->Read, 1, token, &bufferSize, &buffer);
         *        char l[100];
         *        int index3 = map(bufferSize, l, 0);
         *        index3 = mystrcpy(l, index3, L" Bytes gelesen\r\n", 18);
         *        Print(a2u(l));
         *        Print(a2u(buffer));
         *        Print(L"\r\n");
         *        buffer[20] = "A";
         *        buffer[21] = "B";
         *        buffer[22] = "C";
         *        bufferSize = 22;
         *        efiStatus = uefi_call_wrapper(root->Write, 1, token, &bufferSize, &buffer);
         *        efiStatus = uefi_call_wrapper(root->Flush, 1, token);
         */
    }else {
        Print(L"Couldn't Open\r\n");
        if(efiStatus == EFI_NOT_FOUND){
            Print(name);
            Print(L" not found\r\n");
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
    struct file loaded_file;
    loaded_file.root = root;
    loaded_file.file = token;
    loaded_file.status = efiStatus;
    return loaded_file;
}

int isPrintable(CHAR16 c){
   for(int i = 0; i < 93; i++){
        if(c == printable[i]){
            return 1;   
        }
   }
   return 0;
}

int stringLen(CHAR16* string){
    int length = 0;
    while(isPrintable(string[length])){
        length++;
    }
    return length;
}

void stringAppend(CHAR16* a, CHAR16* b, CHAR16* dst){
    for(int i = 0; i < stringLen(a); i++){
        dst[i] = a[i];
    }
    dst[stringLen(a)] = '\\';
    for(int i = 1; i < stringLen(b) + 1; i++){
        dst[stringLen(a) + i] = b[i-1];
    }
}

int parseEntries(struct loader_entries loaders[], int loaderIndex, EFI_SIMPLE_FILE_SYSTEM_PROTOCOL* fs, CHAR16* filename){
    if(((char)filename[0]) == '.'){
        return 0;
    }
    loaders[loaderIndex].efi_index = 0;
    loaders[loaderIndex].options_index = 0;
    CHAR16 name[100] = L"loader\\entries\\";
    for(int i = 0; i < stringLen(filename); i++){
        name[i + 15] = filename[i];
    }
    struct file file = openFile(fs, name);
    char buffer[1024] = "";
    UINTN bufferSize = 1024;
    file.status = uefi_call_wrapper(file.root->Read, 1, file.file, &bufferSize, &buffer);
    char line[1024] = "";
    int bufferindex = 0;
    int index = 0;
    SCANLINES : index = 0;
    for(int i = 0; i < 1024; i++){
            line[i] = ' ';
        }
    if(buffer[bufferindex] == '#'){
        while(buffer[bufferindex] != '\r' && buffer[bufferindex] != '\n'){
            bufferindex++;
        }
        if(buffer[bufferindex] == '\n'){
            bufferindex++;
        }
    }else{
        while(buffer[bufferindex] != '\r' && buffer[bufferindex] != '\n' && index < 98){
            line[index] = buffer[bufferindex];
            index++;
            bufferindex++;
        }
        if(buffer[bufferindex] == '\n'){
            bufferindex++;
        }
    }
    line[index++] = '\r';
    line[index++] = '\n';
    if(matchstring(line, "title", 5)){
        for(int i = 6; i < index; i++){
            loaders[loaderIndex].title[i-6] = line[i];
        }
    }
    if(matchstring(line, "version", 7)){
        for(int i = 8; i < index; i++){
            loaders[loaderIndex].version[i-8] = line[i];
        }
    }
    if(matchstring(line, "machine-id", 10)){
        for(int i = 11; i < index; i++){
            loaders[loaderIndex].machine_id[i-11] = line[i];
        }
    }
    if(matchstring(line, "efi", 3) || matchstring(line, "linux", 5) || matchstring(line, "initrd", 6)){
        int start = 0;
        while(line[start] != ' ' && line[start] != '\t'){
            start++;
        }
        start++;
        while(line[start] == ' ' || line[start] == '\t'){
            start++;
        }
        for(int i = start; i < index; i++){
            loaders[loaderIndex].efi[loaders[loaderIndex].efi_index][i-start] = line[i];
        }
        loaders[loaderIndex].efi_index++;
    }
    if(matchstring(line, "options", 7)){
        for(int i = 8; i < index; i++){
            loaders[loaderIndex].options[loaders[loaderIndex].options_index][i-8] = line[i];
        }
        loaders[loaderIndex].options_index++;
    }
        if(bufferindex < bufferSize){
            goto SCANLINES;
        }
    closeFile(file);
    return 1;
}

int scanEntries(struct loader_entries loaders[], EFI_SIMPLE_FILE_SYSTEM_PROTOCOL* fs, IN EFI_FILE_HANDLE Dir, IN EFI_DEVICE_PATH CONST *DirDp){
    EFI_STATUS Status;
    EFI_FILE_INFO *FileInfo;
    CHAR16 *FileName;
    UINTN FileInfoSize;
    EFI_DEVICE_PATH *Dp;
    
    FileInfo = AllocatePool (MAX_FILE_INFO_SIZE);
    if (FileInfo == NULL) {
        return EFI_OUT_OF_RESOURCES;
    }
    int index = 0;
    for (;;) {
        FileInfoSize = MAX_FILE_INFO_SIZE;
        Status = uefi_call_wrapper(Dir->Read, 1, Dir, &FileInfoSize, (VOID *) FileInfo);
        if (EFI_ERROR (Status) || FileInfoSize == 0) { //this is how we eventually exit this function when we run out of files
            if (Status == EFI_BUFFER_TOO_SMALL) {
                Print (L"EFI_FILE_INFO > MAX_FILE_INFO_SIZE. Increase the size\n");
            }
            FreePool (FileInfo);
            return index;
            //return Status;
        }
        
        // so we have absolute device path to child file/dir
        Dp = FileDevicePath (DirDp, FileName);
        if (Dp == NULL) {
            FreePool (FileInfo);
            return index;
            //return EFI_OUT_OF_RESOURCES;
        }
        
        // Do whatever processing on the file
        //        PerFileFunc (Dir, DirDp, FileInfo, Dp);
        index += parseEntries(loaders, index, fs, FileInfo->FileName);
        FreePool (Dp);
    }
    return index;
}

STATIC EFI_STATUS
AppendFilePath(CONST CHAR16 *Path, CHAR16 **FilePath)
{
	if (!Path || !FilePath){
        Print(L"Invalid Parametr\r\n");
		return EFI_INVALID_PARAMETER;
    }  
	CHAR16 *FilePathPrefix = L"EFI\\";
	if (!FilePathPrefix)
		Print(L"Error AppendFilePath\r\n");

    Print(L"StrAppend\r\n");
	stringAppend(FilePathPrefix, Path, FilePath);
    Print(L"DONe\r\n");
	return EFI_SUCCESS;
}


EFI_STATUS
EfiDevicePathCreate(CONST CHAR16 *Path, CHAR16 **FilePath)
{
	if (!Path || !FilePath)
		return EFI_INVALID_PARAMETER;

	EFI_STATUS Status = EFI_OUT_OF_RESOURCES;

	if (Path[0] != L'\\') {
        Print(L"Appendfile\r\n");
		Status = AppendFilePath(Path, FilePath);
        Print(L"Appendfile Done\r\n");
		if (EFI_ERROR(Status))
			return Status;
	} else{
        Print(L"StrDup\r\n");
		*FilePath = StrDup(Path);
    }
	if (!*FilePath)
		return Status;
    Print(L"Finish\r\n");
	return EFI_SUCCESS;
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
