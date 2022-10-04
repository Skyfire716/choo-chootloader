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
#include "mersennetwister.h"
#include <features.h>
#include <string.h>

#define EDITOR 1
#define AUTO_ENTRIES 2
#define AUTO_FIRMWARE 4
#define CONSOLE_MODE 8
#define UP 1
#define DOWN 2
#define RIGHT 3
#define LEFT 4
#define ENTER 0
#define MAX_FILE_INFO_SIZE 1024

#define DEBUG 1


#define STATION1  "             =======                                "
#define STATION2  "              |   |                                 "
#define STATION3  "              |   |                                 "
#define STATION4  "         _________________________________          "
#define STATION5  "        /                                 \         "
#define STATION6  "       /           TRAINSTATION            \        "
#define STATION7  "      /+-----------------------------------+\       "
#define STATION8  "     / |   +-----+    +-----+    +-----+   | \      "
#define STATION9  "       |   |     |    |     |    |     |   |        "
#define STATION10 "       |   |     |    |     |    |     |   |        "
#define STATION11 "       |   +-----+    +-----+    +-----+   |        "
#define STATION12 "       |          ______________           |        "
#define STATION13 "       |##        ______________           |        "
#define STATION14 "       |####       |          |            |        "
#define STATION15 "+------+-----------------------------------+-------+"
#define STATION16 "| O | O | O | O | O |   |##|   | O | O | O | O | O |"
#define STATION17 "+--------------------------------------------------+"

static EFI_GUID BlockIoProtocolGUID = BLOCK_IO_PROTOCOL;
static EFI_GUID DevicePathGUID = DEVICE_PATH_PROTOCOL;
static EFI_GUID myLoadedImageProtocolGUID = EFI_LOADED_IMAGE_PROTOCOL_GUID;

int fieldWidth = 12;
int fieldHeight = 18;
char D51State = 0;
char LogoState = 0;
char C51State = 0;
int screenWidth = 0;
int screenHeight = 0;
uint32_t background = 0;

char n[1] = "0";
CHAR16 printable[94] = {' ','!','"','#', '$', '%', '&','\'','(',')','*','+',',','-','_','.', '/', '\\', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', ':', ';', '<', '=', '>', '?', '@','A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', '[', '~',']', '↑','←','@','a','b','c','d','e','f','g','h','i','j','k','l','m','n','o','p','q','r','s','t','u','v','w','x','y','z','{','}',};

struct vector{
    float x;
    float y;
};

struct particle{
    struct vector *location;
    struct vector *acceleration;
    struct vector *velocity;
    float lifespan;
};

struct ParticleSystem{
    int numberOfParticels;
    int lifespan;
    int spawned;
    float emissionRate;
    uint8_t lastSpawn;
    struct vector *startPos;
    struct vector *startForce;
    struct vector *velocityX;
    struct vector *velocityY;
    struct particle **particle_array;
};

struct file{
    EFI_FILE* root;
    EFI_FILE* file;
    EFI_STATUS status;
    EFI_FILE_INFO *file_info;
};

struct choot_conf{
    char *mode;
    int modeLen;
    char *train;
    int trainLen;
};

struct gummiboot_conf{
    char *default_loader;
    int default_loaderLen;
    unsigned int timeout;
    char settings;
};

struct loader_entries{
    char *title;
    int titleLen;
    char *version;
    int versionLen;
    char *machine_id;
    int machine_idLen;
    char *efi;
    int efiLen;
    char *efilinux;
    int efilinuxLen;
    char *options;
    int optionsLen;
} entries[100];

struct train{
    int startPosX;
    int baselineY;
    int endPosX;
    int drawPosX;
    int drawPosY;
    void (*draw) (EFI_GRAPHICS_OUTPUT_PROTOCOL *gop, struct train* Coal, int x, int y, uint32_t pixel);
    struct train* next;
};

float mapValues(float x, float in_min, float in_max, float out_min, float out_max){
    return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_max;
}

void addVectors(struct vector *v1, struct vector *v2){
    if(v1 && v2){
        v1->x = v1->x + v2->x;
        v1->y = v1->y + v2->y;
    }
}

void updateParticle(struct particle *p) {
    if(p){
        addVectors(p->velocity, p->acceleration);
        addVectors(p->location, p->velocity);
        p->lifespan -= 2;
    }
}

void applyForceVec(struct particle *p, struct vector *accel){
    FreePool(p->acceleration);
    p->acceleration = accel;
}

void applyForce(struct particle *p, float x, float y) {
    p->acceleration->x = x;
    p->acceleration->y = y;
}

void applyVelocity(struct particle *p, float x, float y){
    p->velocity->x = x;
    p->velocity->y = y;
}

char isDead(struct particle *p){
    if(p->lifespan < 0.0){
        return 1;
    }else{
        return 0;
    }
}

EFI_STATUS EfiMemoryAllocate(IN UINTN Size, OUT VOID **AllocatedBuffer){
    return uefi_call_wrapper(ST->BootServices->AllocatePool, 3, EfiLoaderData, Size, AllocatedBuffer);
}

EFI_STATUS EfiMemoryFree(VOID *Buffer){
    return uefi_call_wrapper(ST->BootServices->FreePool, 1, Buffer);
}

CHAR16* char_to_wchar(char *str, int len){
    CHAR16 *dst = AllocateZeroPool((len + 1) * 2);
    for(int i = 0; i < len; i ++){
        dst[i] = (CHAR16) str[i];
    }
    dst[len + 1] = (CHAR16) '\n';
    return dst;
}

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

int trimstring(char* a, int length){
    int counter = 0; 
    while(a[counter] == ' ' || a[counter] == '\t') {
        counter++;
    }
    return counter;
}

int matchstring(char* a, char* b, int length){
    return matchstring_i(a, b, length, 0);
}

int matchstring_i(char* a, char* b, int length, int index){
    for(int i = index; i < length; i++){
        if (a[i] != b[i]){
            return 0;
        }
    }
    return 1;
}

void print_char_string(char* s, int length){
    for(int i = 0; i < length; i++){
        Print(L"%c", s[i]);
    }
}

int string_to_int(char* s, int length){
    int number = 0;
    for(int i = 0; i < length; i++){
        if(!(s[i] == ' ' && number == 0)){
        number = number * 10;
    if(s[i] == '0'){
        
    }else if(s[i] == '1'){
        number += 1;
    }else if(s[i] == '2'){
        number += 2;
    }else if(s[i] == '3'){
        number += 3;
    }else if(s[i] == '4'){
        number += 4;
    }else if(s[i] == '5'){
        number += 5;
    }else if(s[i] == '6'){
        number += 6;
    }else if(s[i] == '7'){
        number += 7;
    }else if(s[i] == '8'){
        number += 8;
    }else if(s[i] == '9'){
        number += 9;
    }else{
        return number/10;
    }
    }}
    return number;
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

void printLoaderEntry(struct loader_entries loader){
    if(loader.titleLen){
        Print(L"Loader ");
        print_char_string(loader.title, loader.titleLen);
        Print(L"\r\n");
    }
    if(loader.versionLen){
        Print(L"Version: ");
        print_char_string(loader.version, loader.versionLen);
        Print(L"\r\n");
    }
    if(loader.machine_idLen){
        Print(L"MachineID: ");
        print_char_string(loader.machine_id, loader.machine_idLen);
        Print(L"\r\n");
    }
    if(loader.efiLen){
        Print(L"EFI: \r\n");
        print_char_string(loader.efi, loader.efiLen);
        Print(L"\r\n");
    }
    if(loader.efilinuxLen){
        Print(L"Linux: \r\n");
        print_char_string(loader.efilinux, loader.efilinuxLen);
        Print(L"\r\n");
    }
    if(loader.optionsLen){
        Print(L"Options: \r\n");
        print_char_string(loader.options, loader.optionsLen);
        Print(L"\r\n");
    }
}

void printGummiBootConf(struct gummiboot_conf conf){
    Print(L"Gummibootconf\r\n");
    Print(L"Default: ");
    print_char_string(conf.default_loader, conf.default_loaderLen);
    Print(L"\r\n");
    Print(L"Timeout: %u\r\n", conf.timeout);
    Print(L"Editor: %u\r\n", conf.settings & EDITOR);
    Print(L"Auto-Entries: %u\r\n", conf.settings & AUTO_ENTRIES);
    Print(L"Auto-Firmware: %u\r\n", conf.settings & AUTO_FIRMWARE);
    Print(L"Console_mode: %u\r\n", conf.settings & CONSOLE_MODE);
}

void printChootConf(struct choot_conf conf){
    Print(L"parseChootChootLoaderConf\r\n");
    Print(L"Mode: ");
    print_char_string(conf.mode, conf.modeLen);
    Print(L"\r\n");
    Print(L"Train: ");
    print_char_string(conf.train, conf.trainLen);
    Print(L"\r\n");
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

void printFileInfo(EFI_FILE_INFO *fileinfo){
        Print(L"Size %u\r\n", fileinfo->Size);
        Print(L"FileSize %u\r\n", fileinfo->FileSize);
        Print(L"PhysicalSize %u\r\n", fileinfo->PhysicalSize);
        Print(L"Attribute %X\r\n", fileinfo->Attribute);
        Print(L"Filename %s\r\n", fileinfo->FileName);
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
        if(key.ScanCode == UP){
            selectedMode--;
        }else if (key.ScanCode == DOWN){
            selectedMode++;
        }else if (key.ScanCode == RIGHT) {
            selectedMode++;
        }else if (key.ScanCode == LEFT){
            selectedMode--;
        }else if (key.ScanCode == ENTER){
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
        if(DEBUG){
            Print (L"FileName = %s\n", FileInfo->FileName);
        }
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

void closeFile(struct file file){
    FreePool (file.file_info);
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

struct file openFile(EFI_SIMPLE_FILE_SYSTEM_PROTOCOL* fs, CHAR16* name, UINT64 mode, UINT64 attributes){
    Print(name);
    Print(L"\n\r"); 
    EFI_STATUS efiStatus;
    EFI_FILE* root = NULL;
    EFI_FILE_INFO *FileInfo;
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
    if(DEBUG){
        Print(L"Something Else?\r\n");
        Print(L"Try to open File\r\n");
    }
    //EFI_FILE* loaderEntries = NULL;
    //efiStatus = uefi_call_wrapper(root->Open, 1, root, &loaderEntries, L"loader\\entries", EFI_FILE_MODE_READ | //EFI_FILE_MODE_WRITE, EFI_FILE_READ_ONLY);
    //efiStatus = ProcessFilesInDir(loaderEntries, Dp);
    
    
    EFI_FILE* token = NULL;
    //efiStatus = uefi_call_wrapper(root->Open, 1, root, &token, name, EFI_FILE_MODE_READ | EFI_FILE_MODE_WRITE, EFI_FILE_READ_ONLY);
    efiStatus = uefi_call_wrapper(root->Open, 1, root, &token, name, mode, attributes);
    if(efiStatus == EFI_SUCCESS){
        if(DEBUG){
            Print(L"File Successfully Opened\r\n");
        }
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
        FileInfo = AllocatePool (MAX_FILE_INFO_SIZE);
        UINTN FileInfoSize = MAX_FILE_INFO_SIZE;
        //token->GetInfo(&token, &EFI_FILE_INFO_ID, &FileInfoSize, (VOID *) FileInfo);
        //efiStatus =  uefi_call_wrapper(root->Read, 1, root, &FileInfoSize, (VOID *) FileInfo);
        efiStatus = uefi_call_wrapper(token->GetInfo, 1, token, &gEfiFileInfoGuid, &FileInfoSize, (VOID *) FileInfo);
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
    loaded_file.file_info = FileInfo;
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
    int lenA = stringLen(a);
    int lenB = stringLen(b);
    for(int i = 0; i < lenA; i++){
        dst[i] = a[i];
    }
    for(int i = 0; i < lenB; i++){
        dst[lenA + i] = b[i];
    }
    if(DEBUG){
        Print(L"Index %u\r\n", lenA + lenB );
    }
    dst[lenA + lenB] = L"\0";
}

struct choot_conf parseChootChootLoaderConf(EFI_SIMPLE_FILE_SYSTEM_PROTOCOL* fs, CHAR16 *dst){
    struct choot_conf conf;
    conf.mode = NULL;
    conf.modeLen = 0;
    conf.train = NULL;
    conf.trainLen = 0;
    struct file file = openFile(fs, dst, EFI_FILE_MODE_READ, EFI_FILE_READ_ONLY);
    CHAR16 *buffer = AllocatePool(file.file_info->FileSize);
    UINTN bufferSize = file.file_info->FileSize;
    file.status = uefi_call_wrapper(file.root->Read, 1, file.file, &bufferSize, buffer);
    int start;
    int end;
    start = 0;
    end = 0;
    while(start < bufferSize){
        for(int i = start; i < bufferSize; i++){
            if(((char*) buffer)[i] == '\n'){
                end = i;
                i = bufferSize;
            }
        }
        if(((char)((char*)buffer[start])) == '#'){
            Print(L"Comment SkipLine\r\n");
        }else{
            while(((char*)buffer)[start] == ' '){
                start++;
            }
            if(matchstring(((char*)buffer) + start, "mode", 4)){
                start += 4;
                while(((char*)buffer)[start] == ' ' && ((char*)buffer)[start] == '\t'){
                    start++;
                }
                int modeStringLen = end - start;
                char *modeString = AllocatePool(modeStringLen);
                memcpy(modeString, ((char*)buffer) + start, modeStringLen);
                int offset = trimstring(modeString, modeStringLen);
                conf.mode = modeString + offset;
                conf.modeLen = modeStringLen - offset;
                if(DEBUG){
                    Print(L"Mode: ");
                    print_char_string(modeString, modeStringLen);
                    Print(L"\r\n");
                }
            }
            if(matchstring(((char*)buffer) + start, "train", 5)){
                start += 5;
                while(((char*)buffer)[start] == ' ' && ((char*)buffer)[start] == '\t'){
                    start++;
                }
                int trainStringLen = end - start;
                char *trainString = AllocatePool(trainStringLen);
                memcpy(trainString, ((char*)buffer) + start, trainStringLen);
                int offset = trimstring(trainString, trainStringLen);
                conf.train = trainString + offset;
                conf.trainLen = trainStringLen - offset;
                if(DEBUG){
                    Print(L"Train: ");
                    print_char_string(trainString, trainStringLen);
                    Print(L"\r\n");
                }
            }
        }
        start = end + 1;
        end = 0;
    }
    closeFile(file);
    FreePool(buffer);
    return conf;
}

struct gummiboot_conf parseGummibootConf(EFI_SIMPLE_FILE_SYSTEM_PROTOCOL* fs, CHAR16 *dst){
    struct gummiboot_conf conf;
    conf.default_loader = NULL;
    conf.default_loaderLen = 0;
    conf.timeout = 0;
    conf.settings = 0;
    struct file file = openFile(fs, dst, EFI_FILE_MODE_READ, EFI_FILE_READ_ONLY);
    CHAR16 *buffer = AllocatePool(file.file_info->FileSize);
    UINTN bufferSize = file.file_info->FileSize;
    file.status = uefi_call_wrapper(file.root->Read, 1, file.file, &bufferSize, buffer);
    int start;
    int end;
    start = 0;
    end = 0;
    while(start < bufferSize){
        for(int i = start; i < bufferSize; i++){
            if(((char*) buffer)[i] == '\n'){
                end = i;
                i = bufferSize;
            }
        }
        if(((char)((char*)buffer[start])) == '#'){
            Print(L"Comment SkipLine\r\n");
        }else{
            while(((char*)buffer)[start] == ' '){
                start++;
            }
            if(matchstring(((char*)buffer) + start, "default", 7)){
                start += 7;
                while(((char*)buffer)[start] == ' ' && ((char*)buffer)[start] == '\t'){
                    start++;
                }
                int defaultStringLen = end - start;
                char *defaultString = AllocatePool(defaultStringLen);
                memcpy(defaultString, ((char*)buffer) + start, defaultStringLen);
                int offset = trimstring(defaultString, defaultStringLen);
                conf.default_loader = defaultString + offset;
                conf.default_loaderLen = defaultStringLen - offset;
                if(DEBUG){
                    Print(L"Default: ");
                    print_char_string(defaultString, defaultStringLen);
                    Print(L"\r\n");
                }
            }
            if(matchstring(((char*)buffer) + start, "timeout", 7)){
                start += 7;
                while(((char*)buffer)[start] == ' ' && ((char*)buffer)[start] == '\t'){
                    start++;
                }
                int timeoutStringLen = end - start;
                char *timeoutString = AllocatePool(timeoutStringLen);
                memcpy(timeoutString, ((char*)buffer) + start, timeoutStringLen);
                conf.timeout = string_to_int(timeoutString, timeoutStringLen);
                if(DEBUG){
                    Print(L"Timeout: ");
                    print_char_string(timeoutString, timeoutStringLen);
                    Print(L"\r\n");
                }
            }
            if(matchstring(((char*)buffer) + start, "editor", 6)){
                start += 6;
                while(((char*)buffer)[start] == ' ' && ((char*)buffer)[start] == '\t'){
                    start++;
                }
                int editorStringLen = end - start;
                char *editorString = AllocatePool(editorStringLen);
                memcpy(editorString, ((char*)buffer) + start, editorStringLen);
                if(matchstring(editorString, " yes", 4)) {
                    conf.settings = conf.settings | EDITOR;
                }
                if(DEBUG){
                    Print(L"Editor: ");
                    print_char_string(editorString, editorStringLen);
                    Print(L"\r\n");
                }
            }
            if(matchstring(((char*)buffer) + start, "auto-entries", 12)){
                start += 12;
                while(((char*)buffer)[start] == ' ' && ((char*)buffer)[start] == '\t'){
                    start++;
                }
                int autoEntriesStringLen = end - start;
                char *autoEntriesString = AllocatePool(autoEntriesStringLen);
                memcpy(autoEntriesString, ((char*)buffer) + start, autoEntriesStringLen);
                if(string_to_int(autoEntriesString, autoEntriesStringLen)) {
                    conf.settings = conf.settings | AUTO_ENTRIES;
                }
                if(DEBUG){
                    Print(L"AutoEntries: ");
                    print_char_string(autoEntriesString, autoEntriesStringLen);
                    Print(L"\r\n");
                }
            }
            if(matchstring(((char*)buffer) + start, "auto-firmware", 13)){
                start += 13;
                while(((char*)buffer)[start] == ' ' && ((char*)buffer)[start] == '\t'){
                    start++;
                }
                int autoEntriesStringLen = end - start;
                char *autoEntriesString = AllocatePool(autoEntriesStringLen);
                memcpy(autoEntriesString, ((char*)buffer) + start, autoEntriesStringLen);
                if(string_to_int(autoEntriesString, autoEntriesStringLen)) {
                    conf.settings = conf.settings | AUTO_FIRMWARE;
                }
                if(DEBUG){
                    Print(L"AutoFirmware: ");
                    print_char_string(autoEntriesString, autoEntriesStringLen);
                    Print(L"\r\n");
                }
            }
            if(matchstring(((char*)buffer) + start, "console-mode", 12)){
                start += 12;
                while(((char*)buffer)[start] == ' ' && ((char*)buffer)[start] == '\t'){
                    start++;
                }
                int autoEntriesStringLen = end - start;
                char *autoEntriesString = AllocatePool(autoEntriesStringLen);
                memcpy(autoEntriesString, ((char*)buffer) + start, autoEntriesStringLen);
                /*
                if(string_to_int(autoEntriesString, autoEntriesStringLen)) {
                    conf.settings = conf.settings | CONSOLE_MODE;
                }
                */
                if(DEBUG){
                    Print(L"ConsoleMode: ");
                    print_char_string(autoEntriesString, autoEntriesStringLen);
                    Print(L"\r\n");
                }
            }
        }
        start = end + 1;
        end = 0;
    }
    closeFile(file);
    FreePool(buffer);
    return conf;
}

int parseEntries(struct loader_entries loaders[], int loaderIndex, EFI_SIMPLE_FILE_SYSTEM_PROTOCOL* fs, CHAR16* filename){
    if(((char)filename[0]) == '.'){
        return 0;
    }
    loaders[loaderIndex].title = NULL;
    loaders[loaderIndex].titleLen = 0;
    loaders[loaderIndex].version = NULL;
    loaders[loaderIndex].versionLen = 0;
    loaders[loaderIndex].machine_id = NULL;
    loaders[loaderIndex].machine_idLen = 0;
    loaders[loaderIndex].efi = NULL;
    loaders[loaderIndex].efiLen = 0;
    loaders[loaderIndex].efilinux = NULL;
    loaders[loaderIndex].efilinuxLen = 0;
    loaders[loaderIndex].options = NULL;
    loaders[loaderIndex].optionsLen = 0;
    CHAR16 *location = L"loader\\entries\\";
    CHAR16 *dst = AllocateZeroPool(2 * (stringLen(location) + stringLen(filename) + 1));
    memcpy(dst, location, 2 * stringLen(location));
    memcpy(dst + stringLen(location), filename, 2 * stringLen(filename));
    if(DEBUG){
        Print(L"Location: %s %u\r\n", location, stringLen(location));
        Print(L"Filename: %s %u\r\n", filename, stringLen(filename));
        Print(L"Destination: %s %u\r\n", dst, stringLen(dst));
    }
    struct file file = openFile(fs, dst, EFI_FILE_MODE_READ, EFI_FILE_READ_ONLY);
    CHAR16 *buffer = AllocatePool(file.file_info->FileSize);
    UINTN bufferSize = file.file_info->FileSize;
    file.status = uefi_call_wrapper(file.root->Read, 1, file.file, &bufferSize, buffer);
    int start;
    int end;
    start = 0;
    end = 0;
    int optionsLen = loaders[loaderIndex].optionsLen;
    char* options = loaders[loaderIndex].options;
    while(start < bufferSize){
        for(int i = start; i < bufferSize; i++){
            if(((char*) buffer)[i] == '\n'){
                end = i;
                i = bufferSize;
            }
        }
        if(((char*)buffer[start]) == '#' && DEBUG){
            Print(L"Comment SkipLine\r\n");
        }else{
            while(((char*)buffer)[start] == ' '){
                start++;
            }
            if(matchstring(((char*)buffer) + start, "title", 5)){
                start += 5;
                while(((char*)buffer)[start] == ' ' && ((char*)buffer)[start] == '\t'){
                    start++;
                }
                int titleStringLen = end - start;
                char *titleString = AllocatePool(titleStringLen);
                memcpy(titleString, ((char*)buffer) + start, titleStringLen);
                int offset = trimstring(titleString, titleStringLen);
                loaders[loaderIndex].title = titleString + offset;
                loaders[loaderIndex].titleLen = titleStringLen - offset;
                if(DEBUG){
                    Print(L"Title: ");
                    print_char_string(titleString, titleStringLen);
                    Print(L"\r\n");
                }
            }
            if(matchstring(((char*)buffer) + start, "version", 7)){
                start += 7;
                while(((char*)buffer)[start] == ' ' && ((char*)buffer)[start] == '\t'){
                    start++;
                }
                int versionStringLen = end - start;
                char *versionString = AllocatePool(versionStringLen);
                memcpy(versionString, ((char*)buffer) + start, versionStringLen);
                int offset = trimstring(versionString, versionStringLen);
                loaders[loaderIndex].version = versionString + offset;
                loaders[loaderIndex].versionLen = versionStringLen - offset;
                if(DEBUG){
                    Print(L"Version: ");
                    print_char_string(versionString, versionStringLen);
                    Print(L"\r\n");
                }
            }
            if(matchstring(((char*)buffer) + start, "machine-id", 10)){
                start += 10;
                while(((char*)buffer)[start] == ' ' && ((char*)buffer)[start] == '\t'){
                    start++;
                }
                int machineStringLen = end - start;
                char *machineString = AllocatePool(machineStringLen);
                memcpy(machineString, ((char*)buffer) + start, machineStringLen);
                int offset = trimstring(machineString, machineStringLen);
                loaders[loaderIndex].machine_id = machineString + offset;
                loaders[loaderIndex].machine_idLen = machineStringLen - offset;
                if(DEBUG){
                    Print(L"Machine-ID: ");
                    print_char_string(machineString, machineStringLen);
                    Print(L"\r\n");
                }
            }
            if(matchstring(((char*)buffer) + start, "efi", 3)){
                start += 3;
                while(((char*)buffer)[start] == ' ' && ((char*)buffer)[start] == '\t'){
                    start++;
                }
                int efiStringLen = end - start;
                char *efiString = AllocatePool(efiStringLen);
                memcpy(efiString, ((char*)buffer) + start, efiStringLen);
                int offset = trimstring(efiString, efiStringLen);
                loaders[loaderIndex].efi = efiString + offset;
                loaders[loaderIndex].efiLen = efiStringLen - offset;
                if(DEBUG){
                    Print(L"EFI: ");
                    print_char_string(efiString, efiStringLen);
                    Print(L"\r\n");
                }
            }
            if(matchstring(((char*)buffer) + start, "linux", 5)){
                start += 5;
                while(((char*)buffer)[start] == ' ' && ((char*)buffer)[start] == '\t'){
                    start++;
                }
                int linuxStringLen = end - start;
                char *linuxString = AllocatePool(linuxStringLen);
                memcpy(linuxString, ((char*)buffer) + start, linuxStringLen);
                int offset = trimstring(linuxString, linuxStringLen);
                loaders[loaderIndex].efilinux = linuxString + offset;
                loaders[loaderIndex].efilinuxLen = linuxStringLen - offset;
                if(DEBUG){
                    Print(L"Linux: ");
                    print_char_string(linuxString, linuxStringLen);
                    Print(L"\r\n");
                }
            }
            if(matchstring(((char*)buffer) + start, "options", 7)){
                start += 7;
                while(((char*)buffer)[start] == ' ' && ((char*)buffer)[start] == '\t'){
                    start++;
                }
                char *newoptions = AllocatePool(end - start + optionsLen + 1);
                if(options){
                    memcpy(newoptions, options, optionsLen);
                }
                FreePool(options);
                newoptions[optionsLen] = ' ';
                memcpy(newoptions + optionsLen + 1, ((char*)buffer) + start, end - start);
                optionsLen += (end - start);
                options = newoptions;
                if(DEBUG){
                    Print(L"Options: ");
                    print_char_string(options, optionsLen);
                    Print(L"\r\n");
                }
            }
            if(matchstring(((char*)buffer) + start, "initrd", 6)){
                start += 6;
                while(((char*)buffer)[start] == ' ' && ((char*)buffer)[start] == '\t'){
                    start++;
                }
                int initrdStringLen = 8;
                char *initrdString = " initrd=";
                char *newoptions = AllocatePool(initrdStringLen + end - start + optionsLen);
                if(options){
                    memcpy(newoptions, options, optionsLen);
                }
                FreePool(options);
                memcpy(newoptions + optionsLen, initrdString, initrdStringLen);
                memcpy(newoptions + optionsLen + initrdStringLen, ((char*)buffer) + start, end - start);
                optionsLen += initrdStringLen;
                optionsLen += (end - start);
                options = newoptions;
                if(DEBUG){
                    Print(L"Initrd: ");
                    print_char_string(options, optionsLen);
                    Print(L"\r\n");
                }
            }
        }
        start = end + 1;
        end = 0;
    }
    loaders[loaderIndex].options = options;
    loaders[loaderIndex].optionsLen = optionsLen;
    closeFile(file);
    FreePool(dst);
    FreePool(buffer);
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
        if(((char)FileInfo->FileName[0]) != '.'){
            index += parseEntries(loaders, index, fs, FileInfo->FileName);
        }
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

EFI_STATUS executeImage(EFI_HANDLE parent, EFI_HANDLE agent, CHAR16 *name, VOID *sourcebuffer, UINTN sourcebufferSize, VOID *optionsBuffer, UINTN optionsSize){
    if(DEBUG){
        Print(L"File %s\r\n", name);
    }
    EFI_STATUS efiStatus;
    EFI_HANDLE imageHandle;
    EFI_DEVICE_PATH *imagePath = FileDevicePath(agent, name);
    efiStatus = uefi_call_wrapper(ST->BootServices->LoadImage, 1, FALSE, parent, imagePath, sourcebuffer, sourcebufferSize, &imageHandle);
    if(efiStatus == EFI_SUCCESS && DEBUG){
        Print(L"Success\r\n");
    }else if(efiStatus == EFI_NOT_FOUND){
        Print(L"NotFound\r\n");
    }else if(efiStatus == EFI_INVALID_PARAMETER){
        Print(L"InvalidParametre\r\n");
    }else if(efiStatus == EFI_UNSUPPORTED){
        Print(L"Unsupported\r\n");
    }else if(efiStatus == EFI_OUT_OF_RESOURCES){
        Print(L"OUT of Resources\r\n");
    }else if(efiStatus == EFI_LOAD_ERROR){
        Print(L"Load Error\r\n");
    }else if(efiStatus == EFI_DEVICE_ERROR){
        Print(L"Device Error\r\n");
    }else if(efiStatus == EFI_ACCESS_DENIED){
        Print(L"Access Denied\r\n");
    }else if(efiStatus == EFI_SECURITY_VIOLATION){
        Print(L"SECUTRIY VIOLATION\r\n");
    }
    if(sourcebuffer){
        if(DEBUG){
            Print(L"Free sourcebuffer\r\n");
        }
        efiStatus = EfiMemoryFree(sourcebuffer);
        if(efiStatus != EFI_SUCCESS){
            Print(L"Couldn't free sourcebuffer\r\n");
        }
    }
    if(optionsBuffer){
        if(DEBUG){
            Print(L"Found Load Options\r\n");
        }
        EFI_LOADED_IMAGE_PROTOCOL *loadedProtocol;
        efiStatus = uefi_call_wrapper(ST->BootServices->OpenProtocol, 1, imageHandle, &myLoadedImageProtocolGUID, (void**)&loadedProtocol, parent, NULL, EFI_OPEN_PROTOCOL_BY_HANDLE_PROTOCOL);
        if(efiStatus != EFI_SUCCESS){
            Print(L"Error Loading Protocol\r\n");
        }else{
            if(DEBUG){
                Print(L"Options: %s\r\n", loadedProtocol->LoadOptions);
                Print(L"CodeType: %u\r\n", loadedProtocol->ImageCodeType);
                Print(L"DataType: %u\r\n", loadedProtocol->ImageDataType);
            }
            loadedProtocol->LoadOptionsSize = optionsSize;
            loadedProtocol->LoadOptions = optionsBuffer;
            if(DEBUG){
                Print(L"Options: %s\r\n", loadedProtocol->LoadOptions);
                Print(L"CodeType: %u\r\n", loadedProtocol->ImageCodeType);
                Print(L"DataType: %u\r\n", loadedProtocol->ImageDataType);
            }
        }
        efiStatus = uefi_call_wrapper(ST->BootServices->CloseProtocol, 1, imageHandle, &myLoadedImageProtocolGUID, parent, NULL);
        if(efiStatus != EFI_SUCCESS){
            Print(L"Error closing Protocol\r\n");
        }
    }
    if(DEBUG){
        Print(L"Start Image\r\n");
    }
    UINTN returnDataSize;
    CHAR16* returnData;
    efiStatus = uefi_call_wrapper(ST->BootServices->StartImage, 1, imageHandle, returnDataSize, returnData);
    if(efiStatus  == EFI_INVALID_PARAMETER){
        Print(L"Invalid Parameter\r\n");
    }else if(efiStatus == EFI_SECURITY_VIOLATION){
        Print(L"Security Violation\r\n");
    }else{
        Print(L"Returned Code %u\r\n", efiStatus);
    }
    if(DEBUG){
        Print(L"Unload Image\r\n");
    }
    efiStatus = uefi_call_wrapper(ST->BootServices->UnloadImage, 1, imageHandle);
    if(efiStatus == EFI_SUCCESS){
        Print(L"Success\r\n");
    }else if(efiStatus == EFI_UNSUPPORTED){
        Print(L"Unsupported\r\n");
    }else if(efiStatus == EFI_INVALID_PARAMETER){
        Print(L"Invalid Parameter\r\n");
    }else{
        Print(L"Return Code: %u\r\n", efiStatus);
    }
    return efiStatus;
}

EFI_STATUS LoadImg(EFI_SIMPLE_FILE_SYSTEM_PROTOCOL* fs, CHAR16 *Path, VOID** sourcebuffer, UINTN *sourcebufferSize){
    if(DEBUG){
        Print(L"Load Data to sourcebuffer\r\n");
    }
    EFI_STATUS efiStatus = EFI_SUCCESS;
    struct file initramfs = openFile(fs, Path, EFI_FILE_MODE_READ, EFI_FILE_READ_ONLY);
    if(initramfs.status != EFI_SUCCESS){
        return EFI_INVALID_PARAMETER;
    }
    printFileInfo(initramfs.file_info);
    VOID *FileBuffer = NULL;
    UINTN FileSize = (UINTN)initramfs.file_info->FileSize;
    if(DEBUG){
        Print(L"IMG Size: %u\r\n", sourcebufferSize);
    }
    if(FileSize){
        if(!*sourcebuffer){
            if(*sourcebufferSize && *sourcebufferSize < FileSize){
                FileSize = *sourcebufferSize;
            }
            efiStatus = EfiMemoryAllocate(FileSize, &FileBuffer);
            if(efiStatus == EFI_OUT_OF_RESOURCES){
                Print(L"Out of Memory\r\n");
            }else if(efiStatus == EFI_INVALID_PARAMETER){
                Print(L"Invalid Parameter\r\n");
            }
            if(EFI_ERROR(efiStatus)){
                Print(L"Error Alloc\r\n");
            }
        }else{
            FileBuffer = *sourcebuffer;
        }
        efiStatus = uefi_call_wrapper(initramfs.root->Read, 1, initramfs.file, &FileSize, FileBuffer);
        if(efiStatus != EFI_SUCCESS){
            Print(L"Read Error\r\n");
            if(!*sourcebuffer){
                efiStatus = EfiMemoryFree(FileBuffer);
                
            }
        }
    }
    *sourcebuffer = FileBuffer;
    *sourcebufferSize = FileSize;
    closeFile(initramfs);
    if(DEBUG){
        Print(L"Data Loaded\r\n");
    }
    return efiStatus;
}

void freeEntries(){
    int index = 0;
    while(entries[index].titleLen){
        struct loader_entries entry = entries[index];
        if(entry.titleLen){
            FreePool(entry.title);
        }
        if(entry.versionLen){
            FreePool(entry.version);
        }
        if(entry.machine_idLen){
            FreePool(entry.machine_id);
        }
        if(entry.efiLen){
            FreePool(entry.efi);
        }
        if(entry.efilinuxLen){
            FreePool(entry.efilinux);
        }
        if(entry.optionsLen){
            FreePool(entry.options);
        }
        index++;
    }
}

void swapSlash(char* s, int len){
    for(int i = 0; i < len; i++){
        if(s[i] == '/'){
            s[i] = '\\';
        }
    }
}

void simpleBoot(EFI_HANDLE parent, EFI_HANDLE agent){
    uefi_call_wrapper(ST->ConOut->ClearScreen, 1, ST->ConOut);
    EFI_INPUT_KEY key;
    EFI_STATUS st;
    int selectedMode = 0;
    int index = 0;
    int run = 1;
    while(run) {
        while(entries[index].titleLen){
            if(index == selectedMode){
                Print(L"=>");
            }else{
                Print(L"  ");
            }
            print_char_string(entries[index].title, entries[index].titleLen);
            Print(L"\r\n");
            index++;
        }
        WaitForSingleEvent(ST->ConIn->WaitForKey, 0);
        uefi_call_wrapper(ST->ConIn->ReadKeyStroke, 1, ST->ConIn, &key);
        if(key.ScanCode == UP){
            selectedMode--;
        }else if (key.ScanCode == DOWN){
            selectedMode++;
        }else if (key.ScanCode == RIGHT) {
            selectedMode++;
        }else if (key.ScanCode == LEFT){
            selectedMode--;
        }else if (key.ScanCode == ENTER){
            run = 0;
        }
        selectedMode %= index;
        index = 0;
        uefi_call_wrapper(ST->ConOut->ClearScreen, 1, ST->ConOut);
    }
    struct loader_entries entry = entries[selectedMode];
    if(DEBUG){
        Print(L"Boot: ");
        print_char_string(entry.title, entry.titleLen);
        Print(L"\r\n");
    }
    swapSlash(entry.efi, entry.efiLen);
    swapSlash(entry.efilinux, entry.efilinuxLen);
    if(entry.efiLen){
        if(DEBUG){
            Print(L"File: ");
            print_char_string(entry.efi, entry.efiLen);
            Print(L"\r\n");
            Print(L"File: %s\r\n", char_to_wchar(entry.efi, entry.efiLen));
        }
        executeImage(parent, agent, char_to_wchar(entry.efi, entry.efiLen) + 1, NULL, 0, char_to_wchar(entry.options, entry.optionsLen), entry.optionsLen * 2);
    }
    if(entry.efilinuxLen){
        if(DEBUG){
            Print(L"File: ");
            print_char_string(entry.efilinux, entry.efilinuxLen);
            Print(L"\r\n");
            Print(L"File: %s\r\n", char_to_wchar(entry.efilinux, entry.efilinuxLen));
            Print(L"Options: %s\r\n", char_to_wchar(entry.options, entry.optionsLen));
        }
        executeImage(parent, agent, char_to_wchar(entry.efilinux, entry.efilinuxLen) + 1, NULL, 0, char_to_wchar(entry.options, entry.optionsLen), entry.optionsLen * 2);
    }
}


static inline void PlotPixel_32bpp(EFI_GRAPHICS_OUTPUT_PROTOCOL *gop, int x, int y, uint32_t pixel)
{
    if(x >= 0 && y >= 0 && x < screenWidth && y < screenHeight){
        *((uint32_t*)(gop->Mode->FrameBufferBase + 4 * gop->Mode->Info->PixelsPerScanLine * y + 4 * x)) = pixel;
    }
}

static inline void PlotLine_32bpp(EFI_GRAPHICS_OUTPUT_PROTOCOL *gop, int x0, int y0, int x1, int y1, uint32_t pixel){
  int dx =  abs(x1-x0), sx = x0<x1 ? 1 : -1;
  int dy = -abs(y1-y0), sy = y0<y1 ? 1 : -1;
  int err = dx+dy, e2; /* error value e_xy */
  while (1) {
    //setPixel(x0,y0);
    PlotPixel_32bpp(gop, x0, y0, pixel);
    if (x0==x1 && y0==y1) break;
    e2 = 2*err;
    if (e2 > dy) { err += dy; x0 += sx; } /* e_xy+e_x > 0 */
    if (e2 < dx) { err += dx; y0 += sy; } /* e_xy+e_y < 0 */
  }   
}

static inline void PlotRect_32bpp(EFI_GRAPHICS_OUTPUT_PROTOCOL *gop, int x0, int y0, int width, int height, uint32_t pixel){
    PlotLine_32bpp(gop, x0, y0, x0 + width, y0, pixel);
    PlotLine_32bpp(gop, x0, y0, x0, y0 + height, pixel);
    PlotLine_32bpp(gop, x0, y0 + height, x0 + width, y0 + height, pixel);
    PlotLine_32bpp(gop, x0 + width, y0, x0 + width, y0 + height, pixel);
}

static inline void PlotFilledRect_32bpp(EFI_GRAPHICS_OUTPUT_PROTOCOL *gop, int x, int y, int width, int height, uint32_t pixel){
    for(int i = y; i < y + height; i++){
        for(int j = x; j < x + width; j++){
            PlotPixel_32bpp(gop, j, i, pixel);
        }
    }
}

static inline void PlotCircle_32bpp(EFI_GRAPHICS_OUTPUT_PROTOCOL *gop, int x0, int y0, int radius, uint32_t pixel)
  {
    int f = 1 - radius;
    int ddF_x = 0;
    int ddF_y = -2 * radius;
    int x = 0;
    int y = radius;

    PlotPixel_32bpp(gop, x0, y0 + radius, pixel);
    PlotPixel_32bpp(gop, x0, y0 - radius, pixel);
    PlotPixel_32bpp(gop, x0 + radius, y0, pixel);
    PlotPixel_32bpp(gop, x0 - radius, y0, pixel);

    while(x < y)
    {
      if(f >= 0)
      {
        y--;
        ddF_y += 2;
        f += ddF_y;
      }
      x++;
      ddF_x += 2;
      f += ddF_x + 1;

      PlotPixel_32bpp(gop, x0 + x, y0 + y, pixel);
      PlotPixel_32bpp(gop, x0 - x, y0 + y, pixel);
      PlotPixel_32bpp(gop, x0 + x, y0 - y, pixel);
      PlotPixel_32bpp(gop, x0 - x, y0 - y, pixel);
      PlotPixel_32bpp(gop, x0 + y, y0 + x, pixel);
      PlotPixel_32bpp(gop, x0 - y, y0 + x, pixel);
      PlotPixel_32bpp(gop, x0 + y, y0 - x, pixel);
      PlotPixel_32bpp(gop, x0 - y, y0 - x, pixel);
    }
  }
  
static inline void PlotEllipse_32bpp(EFI_GRAPHICS_OUTPUT_PROTOCOL *gop, int xm, int ym, int a, int b, uint32_t pixel)
{
   int dx = 0, dy = b; /* im I. Quadranten von links oben nach rechts unten */
   long a2 = a*a, b2 = b*b;
   long err = b2-(2*b-1)*a2, e2; /* Fehler im 1. Schritt */

   do {
       PlotPixel_32bpp(gop, xm+dx, ym+dy, pixel); /* I. Quadrant */
       PlotPixel_32bpp(gop, xm-dx, ym+dy, pixel); /* II. Quadrant */
       PlotPixel_32bpp(gop, xm-dx, ym-dy, pixel); /* III. Quadrant */
       PlotPixel_32bpp(gop, xm+dx, ym-dy, pixel); /* IV. Quadrant */

       e2 = 2*err;
       if (e2 <  (2*dx+1)*b2) { dx++; err += (2*dx+1)*b2; }
       if (e2 > -(2*dy-1)*a2) { dy--; err -= (2*dy-1)*a2; }
   } while (dy >= 0);

   while (dx++ < a) { /* fehlerhafter Abbruch bei flachen Ellipsen (b=1) */
       PlotPixel_32bpp(gop, xm+dx, ym, pixel); /* -> Spitze der Ellipse vollenden */
       PlotPixel_32bpp(gop, xm-dx, ym, pixel);
   }
}

static inline void PlotEllipseHalfe_32bpp(EFI_GRAPHICS_OUTPUT_PROTOCOL *gop, int xm, int ym, int a, int b, uint32_t pixel, char side)
{
   int dx = 0, dy = b; /* im I. Quadranten von links oben nach rechts unten */
   long a2 = a*a, b2 = b*b;
   long err = b2-(2*b-1)*a2, e2; /* Fehler im 1. Schritt */
   do {
       if(side){
        PlotPixel_32bpp(gop, xm+dx, ym+dy, pixel); /* I. Quadrant */
        PlotPixel_32bpp(gop, xm+dx, ym-dy, pixel); /* IV. Quadrant */
       }else {
        PlotPixel_32bpp(gop, xm-dx, ym+dy, pixel); /* II. Quadrant */
        PlotPixel_32bpp(gop, xm-dx, ym-dy, pixel); /* III. Quadrant */
       }
       

       e2 = 2*err;
       if (e2 <  (2*dx+1)*b2) { dx++; err += (2*dx+1)*b2; }
       if (e2 > -(2*dy-1)*a2) { dy--; err -= (2*dy-1)*a2; }
   } while (dy >= 0);

   while (dx++ < a) { /* fehlerhafter Abbruch bei flachen Ellipsen (b=1) */
       if(side){
       PlotPixel_32bpp(gop, xm+dx, ym, pixel); /* -> Spitze der Ellipse vollenden */
       }else{
       PlotPixel_32bpp(gop, xm-dx, ym, pixel);
       }
   }
}

static inline void Plot_UnderScore(EFI_GRAPHICS_OUTPUT_PROTOCOL *gop, int x, int y, uint32_t pixel){
    PlotLine_32bpp(gop, x, y + fieldHeight * 0.4375, x + fieldWidth * 0.8333333333, y + fieldHeight * 0.4375, pixel);
}

static inline void Plot_Equal(EFI_GRAPHICS_OUTPUT_PROTOCOL *gop, int x, int y, uint32_t pixel){
    PlotLine_32bpp(gop, x, y - fieldHeight * 0.0625, x + fieldWidth * 0.8333333333, y - fieldHeight * 0.0625, pixel);
    PlotLine_32bpp(gop, x, y + fieldHeight * 0.125, x + fieldWidth * 0.8333333333, y + fieldHeight * 0.125, pixel);
}

static inline void Plot_Storke(EFI_GRAPHICS_OUTPUT_PROTOCOL *gop, int x, int y, uint32_t pixel){
    PlotLine_32bpp(gop, x + fieldWidth * 0.6666666, y + fieldHeight * 0.5, x + fieldWidth * 0.6666666, y - fieldHeight * 0.375, pixel);
}

static inline void Plot_Slash(EFI_GRAPHICS_OUTPUT_PROTOCOL *gop, int x, int y, uint32_t pixel){
    PlotLine_32bpp(gop, x, y + fieldHeight * 0.4375, x + fieldWidth * 0.8333333333, y - fieldHeight * 0.375, pixel);
}

static inline void Plot_BackSlash(EFI_GRAPHICS_OUTPUT_PROTOCOL *gop, int x, int y, uint32_t pixel){
    PlotLine_32bpp(gop, x, y - fieldHeight * 0.375, x + fieldWidth * 0.8333333333, y + fieldHeight * 0.4375, pixel);       
}

static inline void Plot_Plus(EFI_GRAPHICS_OUTPUT_PROTOCOL *gop, int x, int y, uint32_t pixel){
    PlotLine_32bpp(gop, x, y + fieldHeight * 0.125, x + fieldWidth * 0.8333333333, y + fieldHeight * 0.125, pixel);
    PlotLine_32bpp(gop, x + 0.5 * fieldWidth, y + 0.25 * fieldHeight, x + 0.5 * fieldWidth, y + 0.4735 * fieldHeight, pixel);
}

static inline void Plot_Y(EFI_GRAPHICS_OUTPUT_PROTOCOL *gop, int x, int y, uint32_t pixel){
    
}

static inline void Plot_At(EFI_GRAPHICS_OUTPUT_PROTOCOL *gop, int x, int y, uint32_t pixel){
    
}

static inline void Plot_DoubleDot(EFI_GRAPHICS_OUTPUT_PROTOCOL *gop, int x, int y, uint32_t pixel){
    
}

static inline void Plot_i(EFI_GRAPHICS_OUTPUT_PROTOCOL *gop, int x, int y, uint32_t pixel){
    
}

static inline void Plot_Dot(EFI_GRAPHICS_OUTPUT_PROTOCOL *gop, int x, int y, uint32_t pixel){
    
}

static inline void Plot_Apos(EFI_GRAPHICS_OUTPUT_PROTOCOL *gop, int x, int y, uint32_t pixel){
    
}

static inline void Plot_BackApos(EFI_GRAPHICS_OUTPUT_PROTOCOL *gop, int x, int y, uint32_t pixel){
    
}

static inline void Plot_O(EFI_GRAPHICS_OUTPUT_PROTOCOL *gop, int x, int y, uint32_t pixel){
    PlotEllipse_32bpp(gop, x + fieldWidth * 0.5, y + fieldHeight * 0.111111111, fieldWidth * 0.333333333, fieldHeight * 0.388888, pixel);
}

static inline void Plot_Os(EFI_GRAPHICS_OUTPUT_PROTOCOL *gop, int x, int y, uint32_t pixel){
    PlotEllipse_32bpp(gop, x, y, fieldWidth * 0.5, fieldHeight * 0.375, pixel);
}

static inline void Plot_I(EFI_GRAPHICS_OUTPUT_PROTOCOL *gop, int x, int y, uint32_t pixel){
    PlotLine_32bpp(gop, x + fieldWidth * 0.5, y + fieldHeight * 0.4375, x + fieldWidth * 0.5, y - fieldHeight * 0.25, pixel);
    PlotLine_32bpp(gop, x + fieldWidth * 0.25, y + fieldHeight * 0.4375, x + fieldWidth * 0.75, y + fieldHeight * 0.4375, pixel);
    PlotLine_32bpp(gop, x + fieldWidth * 0.25, y - fieldHeight * 0.25, x + fieldWidth * 0.75, y - fieldHeight * 0.25, pixel);
}

static inline void Plot_Score(EFI_GRAPHICS_OUTPUT_PROTOCOL *gop, int x, int y, uint32_t pixel){
    PlotLine_32bpp(gop, x, y + fieldHeight * 0.125, x + fieldWidth * 0.8333333333, y + fieldHeight * 0.125, pixel);
}

static inline void Plot_Bracket(EFI_GRAPHICS_OUTPUT_PROTOCOL *gop, int x, int y, uint32_t pixel){
    PlotEllipseHalfe_32bpp(gop, x + fieldWidth * 0.5, y + fieldHeight * 0.125, fieldWidth * 0.5, fieldHeight * 0.375, pixel, 0);
}

static inline void Plot_BracketClose(EFI_GRAPHICS_OUTPUT_PROTOCOL *gop, int x, int y, uint32_t pixel){
    PlotEllipseHalfe_32bpp(gop, x + fieldWidth * 0.0833333333, y + fieldHeight * 0.125, fieldWidth * 0.5, fieldHeight * 0.375, pixel, 1);
}

static inline void Plot_CornerBracket(EFI_GRAPHICS_OUTPUT_PROTOCOL *gop, int x, int y, uint32_t pixel){
    PlotLine_32bpp(gop, x + fieldWidth * 0.333333333, y + fieldHeight * 0.5, x + fieldWidth * 0.333333333, y - fieldHeight * 0.375, pixel);
    PlotLine_32bpp(gop, x + fieldWidth * 0.333333333, y + fieldHeight * 0.5, x + fieldWidth * 0.75, y + fieldHeight * 0.5, pixel);
    PlotLine_32bpp(gop, x + fieldWidth * 0.333333333, y - fieldHeight * 0.375, x + fieldWidth * 0.75, y - fieldHeight * 0.375, pixel);
}

static inline void Plot_CornerBracketClose(EFI_GRAPHICS_OUTPUT_PROTOCOL *gop, int x, int y, uint32_t pixel){
    PlotLine_32bpp(gop, x + fieldWidth * 0.6666666, y + fieldHeight * 0.5, x + fieldWidth * 0.6666666, y - fieldHeight * 0.375, pixel);
    PlotLine_32bpp(gop, x + fieldWidth * 0.25, y + fieldHeight * 0.5, x + fieldWidth * 0.6666666, y + fieldHeight * 0.5, pixel);
    PlotLine_32bpp(gop, x + fieldWidth * 0.25, y - fieldHeight * 0.375, x + fieldWidth * 0.6666666, y - fieldHeight * 0.375, pixel);
}

static inline void Plot_H(EFI_GRAPHICS_OUTPUT_PROTOCOL *gop, int x, int y, uint32_t pixel){
    PlotLine_32bpp(gop, x + fieldWidth * 0.25, y + fieldHeight * 0.4375, x + fieldWidth * 0.25, y - fieldHeight * 0.25, pixel);
    PlotLine_32bpp(gop, x + fieldWidth * 0.75, y + fieldHeight * 0.4375, x + fieldWidth * 0.75, y - fieldHeight * 0.25, pixel);
    PlotLine_32bpp(gop, x + fieldWidth * 0.25, y + fieldHeight * 0.0625, x + fieldWidth * 0.75, y + fieldHeight * 0.0625, pixel);
}

static inline void Plot_A(EFI_GRAPHICS_OUTPUT_PROTOCOL *gop, int x, int y, uint32_t pixel){
    PlotLine_32bpp(gop, x + fieldWidth * 0.25, y + fieldHeight * 0.4375, x + fieldWidth * 0.5, y - fieldHeight * 0.25, pixel);
    PlotLine_32bpp(gop, x + fieldWidth * 0.75, y + fieldHeight * 0.4375, x + fieldWidth * 0.5, y - fieldHeight * 0.25, pixel);
    PlotLine_32bpp(gop, x + fieldWidth * 0.4166666, y + fieldHeight * 0.0625, x + fieldWidth * 0.6666666, y + fieldHeight * 0.0625, pixel);
}

static inline void Plot_B(EFI_GRAPHICS_OUTPUT_PROTOCOL *gop, int x, int y, uint32_t pixel){
    x += 0.25 * fieldWidth;
    PlotLine_32bpp(gop, x + fieldWidth * 0.0833333333, y + fieldHeight * 0.4375, x + fieldWidth * 0.0833333333, y - fieldHeight * 0.25, pixel);
    PlotEllipseHalfe_32bpp(gop, x + fieldWidth * 0.0833333333, y + fieldHeight * 0.333333333, fieldWidth * 0.5, fieldHeight * 0.2222222, pixel, 1);
    PlotEllipseHalfe_32bpp(gop, x + fieldWidth * 0.0833333333, y, fieldWidth * 0.5, fieldHeight * 0.2222222, pixel, 1);
}

static inline void Plot_C(EFI_GRAPHICS_OUTPUT_PROTOCOL *gop, int x, int y, uint32_t pixel){
    x += 0.25 * fieldWidth;
    PlotEllipseHalfe_32bpp(gop, x + fieldWidth * 0.5, y + fieldHeight * 0.125, fieldWidth * 0.5, fieldHeight * 0.375, pixel, 0);
}

static inline void Plot_D(EFI_GRAPHICS_OUTPUT_PROTOCOL *gop, int x, int y, uint32_t pixel){
    x += 0.25 * fieldWidth;
    PlotLine_32bpp(gop, x + fieldWidth * 0.0833333333, y + fieldHeight * 0.4375, x + fieldWidth * 0.0833333333, y - fieldHeight * 0.25, pixel);
    PlotEllipseHalfe_32bpp(gop, x + fieldWidth * 0.0833333333, y + fieldHeight * 0.125, fieldWidth * 0.5, fieldHeight * 0.375, pixel, 1);
}

static inline void Plot_Tilde(EFI_GRAPHICS_OUTPUT_PROTOCOL *gop, int x, int y, uint32_t pixel){
    PlotPixel_32bpp(gop, x + 7, y, pixel);
    PlotPixel_32bpp(gop, x + 7, y + 1, pixel);
    PlotPixel_32bpp(gop, x + 6, y + 2, pixel);
    PlotPixel_32bpp(gop, x + 5, y + 2, pixel);
    PlotPixel_32bpp(gop, x + 4, y + 1, pixel);
    PlotPixel_32bpp(gop, x + 4, y, pixel);
    PlotPixel_32bpp(gop, x + 3, y - 1, pixel);
    PlotPixel_32bpp(gop, x + 2, y - 2, pixel);
    PlotPixel_32bpp(gop, x + 1, y - 2, pixel);
    PlotPixel_32bpp(gop, x, y - 1, pixel);
    PlotPixel_32bpp(gop, x, y, pixel);
}

static inline void Plot_Char(EFI_GRAPHICS_OUTPUT_PROTOCOL *gop, int x, int y, char letter, uint32_t pixel){
    if(letter == ' '){
        return;
    }
    switch(letter){
        case '_':
            Plot_UnderScore(gop, x, y, pixel);
            break;
        case '-':
            Plot_Score(gop, x, y, pixel);
            break;
        case '=':
            Plot_Equal(gop, x, y, pixel);
            break;
        case 'D':
            Plot_D(gop, x, y, pixel);
            break;
        case '/':
            Plot_Slash(gop, x, y, pixel);
            break;
        case '\\':
            Plot_BackSlash(gop, x, y, pixel);
            break;
        case '(':
            Plot_Bracket(gop, x, y, pixel);
            break;
        case ')':
            Plot_BracketClose(gop, x, y, pixel);
            break;
        case '[':
            Plot_CornerBracket(gop, x, y, pixel);
            break;
        case ']':
            Plot_CornerBracketClose(gop, x, y, pixel);
            break;
        case 'I':
            Plot_I(gop, x, y, pixel);
            break;
        case 'i':
            Plot_i(gop, x, y, pixel);
            break;
        case '@':
            Plot_At(gop, x, y, pixel);
            break;
        case 'A':
            Plot_A(gop, x, y, pixel);
            break;
        case 'B':
            Plot_B(gop, x, y, pixel);
            break;
        case 'C':
            Plot_C(gop, x, y, pixel);
            break;
        case '`':
            Plot_BackApos(gop, x, y, pixel);
            break;
        case 'H':
            Plot_H(gop, x, y, pixel);
            break;
        case '|':
            Plot_Storke(gop, x, y, pixel);
            break;
        case '~':
            Plot_Tilde(gop, x, y, pixel);
            break;
        case 'O':
            Plot_O(gop, x, y, pixel);
            break;
        case '.':
            Plot_Dot(gop, x, y, pixel);
            break;
        case ':':
            Plot_DoubleDot(gop, x, y, pixel);
            break;
        case 'Y':
            Plot_Y(gop, x, y, pixel);
            break;
        case '+':
            Plot_Plus(gop, x, y, pixel);
            break;
        case 'o':
            Plot_Os(gop, x, y, pixel);
            break;
        case '\'':
            Plot_Apos(gop, x, y, pixel);
            break;
        default:
            break;
    }
}

static inline void Plot_String(EFI_GRAPHICS_OUTPUT_PROTOCOL *gop, int x, int y, char *str, int length, uint32_t pixel){
    int i = 0;
    if(x < 0){
        i = (-1) * x / fieldWidth;
        x += ((-1) * x / fieldWidth) * fieldWidth;
    }
    if((screenWidth - x) / fieldWidth < length){
        length = (screenWidth - x + fieldWidth) / fieldWidth;
    }
    for(i; i < length; i++){
        //PlotFilledRect_32bpp(gop, x, y, fieldWidth, fieldHeight, 255);
        Plot_Char(gop, x, y, str[i], pixel);
        x += fieldWidth;
        x += 1;
    }
}

static inline void Plot_Station(EFI_GRAPHICS_OUTPUT_PROTOCOL *gop, int x, int y, uint32_t pixel){
    Plot_String(gop, x, y, STATION1, 52, pixel);
    y += fieldHeight;
    Plot_String(gop, x, y, STATION2, 52, pixel);
    y += fieldHeight;
    Plot_String(gop, x, y, STATION3, 52, pixel);
    y += fieldHeight;
    Plot_String(gop, x, y, STATION4, 52, pixel);
    y += fieldHeight;
    Plot_String(gop, x, y, STATION5, 52, pixel);
    y += fieldHeight;
    Plot_String(gop, x, y, STATION6, 52, pixel);
    y += fieldHeight;
    Plot_String(gop, x, y, STATION7, 52, pixel);
    y += fieldHeight;
    Plot_String(gop, x, y, STATION8, 52, pixel);
    y += fieldHeight;
    Plot_String(gop, x, y, STATION9, 52, pixel);
    y += fieldHeight;
    Plot_String(gop, x, y, STATION10, 52, pixel);
    y += fieldHeight;
    Plot_String(gop, x, y, STATION11, 52, pixel);
    y += fieldHeight;
    Plot_String(gop, x, y, STATION12, 52, pixel);
    y += fieldHeight;
    Plot_String(gop, x, y, STATION13, 52, pixel);
    y += fieldHeight;
    Plot_String(gop, x, y, STATION14, 52, pixel);
    y += fieldHeight;
    Plot_String(gop, x, y, STATION15, 52, pixel);
    y += fieldHeight;
    Plot_String(gop, x, y, STATION16, 52, pixel);
    y += fieldHeight;
    Plot_String(gop, x, y, STATION17, 52, pixel);
}

static inline void Plot_D51(EFI_GRAPHICS_OUTPUT_PROTOCOL *gop, int x, int y, uint32_t pixel){
    int startX = x;
    int startY = y;
    char *c1 = D51STR1;
    char *c2 = D51STR2;
    char *c3 = D51STR3;
    char *c4 = D51STR4;
    char *c5 = D51STR5;
    char *c6 = D51STR6;
    char *c7 = D51STR7;
    Plot_String(gop, x, y, c1, 56, pixel);
    y += fieldHeight;
    Plot_String(gop, x, y, c2, 56, pixel);
    y += fieldHeight;
    Plot_String(gop, x, y, c3, 56, pixel);
    y += fieldHeight;
    Plot_String(gop, x, y, c4, 56, pixel);
    y += fieldHeight;
    Plot_String(gop, x, y, c5, 56, pixel);
    y += fieldHeight;
    Plot_String(gop, x, y, c6, 56, pixel);
    y += fieldHeight;
    Plot_String(gop, x, y, c7, 56, pixel);
    y += fieldHeight;
    if(D51State == 0){
        char *w1 = D51WHL41;
        char *w2 = D51WHL42;
        char *w3 = D51WHL43;
        Plot_String(gop, x, y, w1, 56, pixel);
        y += fieldHeight;
        Plot_String(gop, x, y, w2, 56, pixel);
        y += fieldHeight;
        Plot_String(gop, x, y, w3, 56, pixel);
    }else if(D51State == 1){
        char *w1 = D51WHL31;
        char *w2 = D51WHL32;
        char *w3 = D51WHL33;
        Plot_String(gop, x, y, w1, 56, pixel);
        y += fieldHeight;
        Plot_String(gop, x, y, w2, 56, pixel);
        y += fieldHeight;
        Plot_String(gop, x, y, w3, 56, pixel);
    }else if(D51State == 2){
        char *w1 = D51WHL21;
        char *w2 = D51WHL22;
        char *w3 = D51WHL23;
        Plot_String(gop, x, y, w1, 56, pixel);
        y += fieldHeight;
        Plot_String(gop, x, y, w2, 56, pixel);
        y += fieldHeight;
        Plot_String(gop, x, y, w3, 56, pixel);
    }else if(D51State == 3){
        char *w1 = D51WHL11;
        char *w2 = D51WHL12;
        char *w3 = D51WHL13;
        Plot_String(gop, x, y, w1, 56, pixel);
        y += fieldHeight;
        Plot_String(gop, x, y, w2, 56, pixel);
        y += fieldHeight;
        Plot_String(gop, x, y, w3, 56, pixel);
    }else if(D51State == 4){
        char *w1 = D51WHL61;
        char *w2 = D51WHL62;
        char *w3 = D51WHL63;
        Plot_String(gop, x, y, w1, 56, pixel);
        y += fieldHeight;
        Plot_String(gop, x, y, w2, 56, pixel);
        y += fieldHeight;
        Plot_String(gop, x, y, w3, 56, pixel);
    }else if(D51State == 5){
        char *w1 = D51WHL51;
        char *w2 = D51WHL52;
        char *w3 = D51WHL53;
        Plot_String(gop, x, y, w1, 56, pixel);
        y += fieldHeight;
        Plot_String(gop, x, y, w2, 56, pixel);
        y += fieldHeight;
        Plot_String(gop, x, y, w3, 56, pixel);
    }
}

static inline void Plot_Coal(EFI_GRAPHICS_OUTPUT_PROTOCOL *gop, int x, int y, uint32_t pixel){
    int startX = x;
    int startY = y;
    char *c1 = COAL03;
    char *c2 = COAL04;
    char *c3 = COAL05;
    char *c4 = COAL06;
    char *c5 = COAL07;
    char *c6 = COAL08;
    char *c7 = COAL09;
    char *c8 = COAL10;
    Plot_String(gop, x, y, c1, 30, pixel);
    y += fieldHeight;
    Plot_String(gop, x, y, c2, 30, pixel);
    y += fieldHeight;
    Plot_String(gop, x, y, c3, 30, pixel);
    y += fieldHeight;
    Plot_String(gop, x, y, c4, 30, pixel);
    y += fieldHeight;
    Plot_String(gop, x, y, c5, 30, pixel);
    y += fieldHeight;
    Plot_String(gop, x, y, c6, 30, pixel);
    y += fieldHeight;
    Plot_String(gop, x, y, c7, 30, pixel);
    y += fieldHeight;
    Plot_String(gop, x, y, c8, 30, pixel);
    y += fieldHeight;
    
}

static inline void Plot_Car(EFI_GRAPHICS_OUTPUT_PROTOCOL *gop, int x, int y, uint32_t pixel){
    int startX = x;
    int startY = y;
    char *c1 = LCAR1;
    char *c2 = LCAR2;
    char *c3 = LCAR3;
    char *c4 = LCAR4;
    char *c5 = LCAR5;
    char *c6 = LCAR6;
    Plot_String(gop, x, y, c1, 21, pixel);
    y += fieldHeight;
    Plot_String(gop, x, y, c2, 21, pixel);
    y += fieldHeight;
    Plot_String(gop, x, y, c3, 21, pixel);
    y += fieldHeight;
    Plot_String(gop, x, y, c4, 21, pixel);
    y += fieldHeight;
    Plot_String(gop, x, y, c5, 21, pixel);
    y += fieldHeight;
    Plot_String(gop, x, y, c6, 21, pixel);
}

static inline void Plot_LCoal(EFI_GRAPHICS_OUTPUT_PROTOCOL *gop, int x, int y, uint32_t pixel){
    int startX = x;
    int startY = y;
    char *c1 = LCOAL1;
    char *c2 = LCOAL2;
    char *c3 = LCOAL3;
    char *c4 = LCOAL4;
    char *c5 = LCOAL5;
    char *c6 = LCOAL6;
    Plot_String(gop, x, y, c1, 21, pixel);
    y += fieldHeight;
    Plot_String(gop, x, y, c2, 21, pixel);
    y += fieldHeight;
    Plot_String(gop, x, y, c3, 21, pixel);
    y += fieldHeight;
    Plot_String(gop, x, y, c4, 21, pixel);
    y += fieldHeight;
    Plot_String(gop, x, y, c5, 21, pixel);
    y += fieldHeight;
    Plot_String(gop, x, y, c6, 21, pixel);
}

static inline void Plot_Logo(EFI_GRAPHICS_OUTPUT_PROTOCOL *gop, int x, int y, uint32_t pixel){
    int startX = x;
    int startY = y;
    char *c1 = LOGO1;
    char *c2 = LOGO2;
    char *c3 = LOGO3;
    char *c4 = LOGO4;
    Plot_String(gop, x, y, c1, 21, pixel);
    y += fieldHeight;
    Plot_String(gop, x, y, c2, 21, pixel);
    y += fieldHeight;
    Plot_String(gop, x, y, c3, 21, pixel);
    y += fieldHeight;
    Plot_String(gop, x, y, c4, 21, pixel);
    y += fieldHeight;
    if(LogoState == 0){
        char *w1 = LWHL31;
        char *w2 = LWHL32;
        Plot_String(gop, x, y, w1, 21, pixel);
        y += fieldHeight;
        Plot_String(gop, x, y, w2, 21, pixel);
    }else if(LogoState == 1){
        char *w1 = LWHL21;
        char *w2 = LWHL22;
        Plot_String(gop, x, y, w1, 21, pixel);
        y += fieldHeight;
        Plot_String(gop, x, y, w2, 21, pixel);
    }else if(LogoState == 2){
        char *w1 = LWHL11;
        char *w2 = LWHL12;
        Plot_String(gop, x, y, w1, 21, pixel);
        y += fieldHeight;
        Plot_String(gop, x, y, w2, 21, pixel);
    }else if(LogoState == 3){
        char *w1 = LWHL61;
        char *w2 = LWHL62;
        Plot_String(gop, x, y, w1, 21, pixel);
        y += fieldHeight;
        Plot_String(gop, x, y, w2, 21, pixel);
    }else if(LogoState == 4){
        char *w1 = LWHL51;
        char *w2 = LWHL52;
        Plot_String(gop, x, y, w1, 21, pixel);
        y += fieldHeight;
        Plot_String(gop, x, y, w2, 21, pixel);
    }else if(LogoState == 5){
        char *w1 = LWHL41;
        char *w2 = LWHL42;
        Plot_String(gop, x, y, w1, 21, pixel);
        y += fieldHeight;
        Plot_String(gop, x, y, w2, 21, pixel);
    }
}

static inline void Plot_C51(EFI_GRAPHICS_OUTPUT_PROTOCOL *gop, int x, int y, uint32_t pixel){
    int startX = x;
    int startY = y;
    char *c1 = C51STR1;
    char *c2 = C51STR2;
    char *c3 = C51STR3;
    char *c4 = C51STR4;
    char *c5 = C51STR5;
    char *c6 = C51STR6;
    char *c7 = C51STR7;
    char *cWheel1 = C51WH61;
    char *cWheel4 = C51WH64;
    Plot_String(gop, x, y, c1, 55, pixel);
    y += fieldHeight;
    Plot_String(gop, x, y, c2, 55, pixel);
    y += fieldHeight;
    Plot_String(gop, x, y, c3, 55, pixel);
    y += fieldHeight;
    Plot_String(gop, x, y, c4, 55, pixel);
    y += fieldHeight;
    Plot_String(gop, x, y, c5, 55, pixel);
    y += fieldHeight;
    Plot_String(gop, x, y, c6, 55, pixel);
    y += fieldHeight;
    Plot_String(gop, x, y, c7, 55, pixel);
    y += fieldHeight;
    Plot_String(gop, x, y, cWheel1, 55, pixel);
    y += fieldHeight;
    if(C51State == 0){
        char *w1 = C51WH42;
        char *w2 = C51WH43;
        Plot_String(gop, x, y, w1, 55, pixel);
        y += fieldHeight;
        Plot_String(gop, x, y, w2, 55, pixel);
    }else if(C51State == 1){
        char *w1 = C51WH32;
        char *w2 = C51WH33;
        Plot_String(gop, x, y, w1, 55, pixel);
        y += fieldHeight;
        Plot_String(gop, x, y, w2, 55, pixel);
    }else if(C51State == 2){
        char *w1 = C51WH22;
        char *w2 = C51WH23;
        Plot_String(gop, x, y, w1, 55, pixel);
        y += fieldHeight;
        Plot_String(gop, x, y, w2, 55, pixel);
    }else if(C51State == 3){
        char *w1 = C51WH12;
        char *w2 = C51WH13;
        Plot_String(gop, x, y, w1, 55, pixel);
        y += fieldHeight;
        Plot_String(gop, x, y, w2, 55, pixel);
    }else if(C51State == 4){
        char *w1 = C51WH62;
        char *w2 = C51WH63;
        Plot_String(gop, x, y, w1, 55, pixel);
        y += fieldHeight;
        Plot_String(gop, x, y, w2, 55, pixel);
    }else if(C51State == 5){
        char *w1 = C51WH52;
        char *w2 = C51WH53;
        Plot_String(gop, x, y, w1, 55, pixel);
        y += fieldHeight;
        Plot_String(gop, x, y, w2, 55, pixel);
    }
    y += fieldHeight;
    Plot_String(gop, x, y, cWheel4, 55, pixel);
}

void drawD51(EFI_GRAPHICS_OUTPUT_PROTOCOL *gop, struct train* D51, int x, int y, uint32_t pixel){
    D51->startPosX = x;
    D51->baselineY = y;
    D51->endPosX = D51->startPosX + 54 * fieldWidth;
    D51->drawPosX = D51->startPosX + 3;
    D51->drawPosY = D51->baselineY - 10 * fieldHeight + 6;
    if(D51->endPosX > 0 && D51->startPosX < screenWidth){
        Plot_D51(gop, D51->drawPosX, D51->drawPosY, pixel);
    }
}

void drawCoal(EFI_GRAPHICS_OUTPUT_PROTOCOL *gop, struct train* Coal, int x, int y, uint32_t pixel){
    Coal->startPosX = x;
    Coal->baselineY = y;
    Coal->endPosX = Coal->startPosX + 29 * fieldWidth;
    Coal->drawPosX = Coal->startPosX + 3;
    Coal->drawPosY = Coal->baselineY - 8 * fieldHeight + 6;
    if(Coal->endPosX >= 0 || Coal->startPosX < screenWidth){
        Plot_Coal(gop, Coal->drawPosX, Coal->drawPosY, pixel);
    }
}

void drawCar(EFI_GRAPHICS_OUTPUT_PROTOCOL *gop, struct train* Car, int x, int y, uint32_t pixel){
    Car->startPosX = x;
    Car->baselineY = y;
    Car->endPosX = Car->startPosX + 21 * fieldWidth;
    Car->drawPosX = Car->startPosX + 3;
    Car->drawPosY = Car->baselineY - 6 * fieldHeight + 6;
    if(Car->endPosX >= 0 || Car->startPosX < screenWidth){
        Plot_Car(gop, Car->drawPosX, Car->drawPosY, pixel);
    }
}

void drawLCoal(EFI_GRAPHICS_OUTPUT_PROTOCOL *gop, struct train* LCoal, int x, int y, uint32_t pixel){
    LCoal->startPosX = x;
    LCoal->baselineY = y;
    LCoal->endPosX = LCoal->startPosX + 21 * fieldWidth;
    LCoal->drawPosX = LCoal->startPosX + 3;
    LCoal->drawPosY = LCoal->baselineY - 6 * fieldHeight + 6;
    if(LCoal->endPosX >= 0 || LCoal->startPosX < screenWidth){
        Plot_LCoal(gop, LCoal->drawPosX, LCoal->drawPosY, pixel);
    }
}

void drawLogo(EFI_GRAPHICS_OUTPUT_PROTOCOL *gop, struct train* Logo, int x, int y, uint32_t pixel){
    Logo->startPosX = x;
    Logo->baselineY = y;
    Logo->endPosX = Logo->startPosX + 21 * fieldWidth;
    Logo->drawPosX = Logo->startPosX + 3;
    Logo->drawPosY = Logo->baselineY - 6 * fieldHeight + 6;
    if(Logo->endPosX >= 0 || Logo->startPosX < screenWidth){
        Plot_Logo(gop, Logo->drawPosX, Logo->drawPosY, pixel);
    }
}

void drawC51(EFI_GRAPHICS_OUTPUT_PROTOCOL *gop, struct train* C51, int x, int y, uint32_t pixel){
    C51->startPosX = x;
    C51->baselineY = y;
    C51->endPosX = C51->startPosX + 56 * fieldWidth;
    C51->drawPosX = C51->startPosX + 3;
    C51->drawPosY = C51->baselineY - 11 * fieldHeight + 6;
    if(C51->endPosX >= 0 || C51->startPosX < screenWidth){
        Plot_C51(gop, C51->drawPosX, C51->drawPosY, pixel);
    }
}

void drawParticle(EFI_GRAPHICS_OUTPUT_PROTOCOL *gop, struct particle *particle, uint32_t pixel){
    PlotPixel_32bpp(gop, particle->location->x, particle->location->y, pixel);
}

struct particle* getParticle(int x, int y){
    struct particle *particle = AllocatePool(sizeof(struct particle));
    particle->location = AllocatePool(sizeof(struct vector));
    particle->velocity = AllocatePool(sizeof(struct vector));
    particle->acceleration = AllocatePool(sizeof(struct vector));
    particle->location->x = x;
    particle->location->y = y;
    return particle;
}

void FreeParticle(struct particle *particle){
    FreePool(particle->location);
    FreePool(particle->velocity);
    FreePool(particle->acceleration);
    FreePool(particle);
}

struct ParticleSystem* getParticleSystem(int x, int y, int number, int lifespan, float emissionRate, float accelX, float accelY, struct vector *velX, struct vector *velY){
    struct ParticleSystem *ps = AllocatePool(sizeof(struct ParticleSystem));
    ps->numberOfParticels = number;
    ps->startPos = AllocatePool(sizeof(struct vector));
    ps->startPos->x = x;
    ps->startPos->y = y;
    ps->startForce = AllocatePool(sizeof(struct vector));
    ps->startForce->x = accelX;
    ps->startForce->y = accelY;
    ps->velocityX = velX;
    ps->velocityY = velY;
    ps->lifespan = lifespan;
    ps->emissionRate = emissionRate;
    EFI_TIME time;
    uefi_call_wrapper(RT->GetTime, 1, &time, NULL);
    ps->lastSpawn = time.Second;
    ps->spawned = 0;
    ps->particle_array = AllocatePool(ps->numberOfParticels * sizeof(struct particle *));
    for (int i = 0; i < ps->numberOfParticels; i++) {
        ps->particle_array[i] = NULL;
    }
    spawnParticlePS(ps);
    return ps;
}

void spawnParticlePS(struct ParticleSystem *ps){
    EFI_TIME time;
    uefi_call_wrapper(RT->GetTime, 1, &time, NULL);
    if(time.Second == ps->lastSpawn){
        for(int i = 0; i < ps->numberOfParticels; i++){
            if(!ps->particle_array[i] && ps->spawned < ps->emissionRate){
                ps->particle_array[i] = getParticle(ps->startPos->x + mapValues(mersenne_twister(), 0, 1, -20, 20), ps->startPos->y);
                ps->particle_array[i]->lifespan = ps->lifespan;
                applyVelocity(ps->particle_array[i], mapValues(mersenne_twister(), 0, 1, ps->velocityX->x, ps->velocityX->y), mapValues(mersenne_twister(), 0, 1, ps->velocityY->x, ps->velocityY->y));
                applyForce(ps->particle_array[i], ps->startForce->x, ps->startForce->y);
                ps->spawned++;
                i += 10;
            }
        }
    }else{
        ps->lastSpawn = time.Second;
        ps->spawned = 0;
    }
}

void addVelocityPS(struct ParticleSystem *ps, float velXmin, float velXmax, float velYmin, float velYmax){
    for(int i = 0; i < ps->numberOfParticels; i++){
        if(ps->particle_array[i]){
            applyVelocity(ps->particle_array[i], mapValues(mersenne_twister(), 0, 1, velXmin, velXmax), mapValues(mersenne_twister(), 0, 1, velYmin, velYmax));
        }
    }
}

void applyForcePS(struct ParticleSystem *ps, float accelX, float accelY){
    for(int i = 0; i < ps->numberOfParticels; i++){
        if(ps->particle_array[i]){
            applyForce(ps->particle_array[i], accelX, accelY);
        }
    }
}

void runPS(EFI_GRAPHICS_OUTPUT_PROTOCOL *gop, struct ParticleSystem *ps, uint32_t pixel){
    for (int i = 0; i < ps->numberOfParticels; i++) {
        if(ps->particle_array[i]){
            drawParticle(gop, ps->particle_array[i], pixel);
            updateParticle(ps->particle_array[i]);
            if(isDead(ps->particle_array[i])){
                FreeParticle(ps->particle_array[i]);
                ps->particle_array[i] = NULL;
            }else{
                drawParticle(gop, ps->particle_array[i], pixel);
            }
        }
    }
    spawnParticlePS(ps);
}

void FreeParticleSystem(struct ParticleSystem *ps){
    for(int i = 0; i < ps->numberOfParticels; i++){
        if(ps->particle_array[i]){
            FreeParticle(ps->particle_array[i]);
        }
    }
    FreePool(ps->startPos);
    FreePool(ps->startForce);
    FreePool(ps->velocityX);
    FreePool(ps->velocityY);
}

void drawTrain(EFI_GRAPHICS_OUTPUT_PROTOCOL *gop, struct train* train, int x, int y, uint32_t pixel){
    if(train){
        train->draw(gop, train, x, y, pixel);
        x = train->endPosX;
        drawTrain(gop, train->next, x, y, pixel);
    }
}

int calculateTrainLenght(struct train* train){
    if(train){
        return calculateTrainLenght(train->next) + (train->endPosX - train->startPosX);
    }else{
        return 0;
    }
}

void driveTrain(EFI_GRAPHICS_OUTPUT_PROTOCOL *gop, struct train* train, int x, int y){
    int white = (255 << 16) + (255 << 8) + 255;
    int black = 0;
    drawTrain(gop, train, x, y, white);
    int stop = calculateTrainLenght(train);
    stop *= -1;
    while(x > stop){
        drawTrain(gop, train, x, y, white);
        uefi_call_wrapper(ST->BootServices->Stall, 1, 70000);
        drawTrain(gop, train, x, y, black);
        D51State = (D51State + 1) % 6;
        LogoState = (LogoState + 1) % 6;
        C51State = (C51State + 1) % 6;
        x -= 5;
    }
}

void FreeTrain(struct train *train){
    if(train->next){
        FreeTrain(train->next);
    }
    FreePool(train);
}

struct train * parseTrain(char *trainStr, int trainStrLen){
    struct train *Head = AllocatePool(sizeof(struct train));
    Head->next = NULL;
    struct train *actual = Head;
    int index = 0;
    int end = 0;
    if((char)trainStr[index] == '{' && (char)trainStr[trainStrLen -1] == '}'){
        index++;
        while((char)trainStr[end] != '}'){
            while((char)trainStr[end] != ' '){
                end++;
            }
            if(matchstring(trainStr + index, "D51", 3)){
                if(DEBUG){
                    Print(L"D51\r\n");
                }
                actual->draw = drawD51;
                actual->next = NULL;
            }
            if(matchstring(trainStr + index, "C51", 3)){
                if(DEBUG){
                    Print(L"C51\r\n");
                }
                actual->draw = drawC51;
                actual->next = NULL;
            }
            if(matchstring(trainStr + index, "Coal", 4)){
                if(DEBUG){
                    Print(L"Coal\r\n");
                }
                actual->draw = drawCoal;
                actual->next = NULL;
            }
            if(matchstring(trainStr + index, "Logo", 4)){
                if(DEBUG){
                    Print(L"Logo\r\n");
                }
                actual->draw = drawLogo;
                actual->next = NULL;
            }
            if(matchstring(trainStr + index, "Car", 3)){
                if(DEBUG){
                    Print(L"Car\r\n");
                }
                actual->draw = drawCar;
                actual->next = NULL;
            }
            if(matchstring(trainStr + index, "LCoal", 5)){
                if(DEBUG){
                    Print(L"LCoal\r\n");
                }
                actual->draw = drawLCoal;
                actual->next = NULL;
            }
            index = end + 1;
            end += 2;
            if(end < trainStrLen - 1){
                actual->next = AllocatePool(sizeof(struct train));
                actual = actual->next;
            }
        }
    }else{
        Print(L"Invalid Train Config\r\n");
    }
    return Head;
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
    if(DEBUG){
        Print(L"Check for GUID\n\r");
    }
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
    struct choot_conf chootConf = parseChootChootLoaderConf(fs, L"EFI\\chootloader\\choot.conf");
    printChootConf(chootConf);
    struct gummiboot_conf conf = parseGummibootConf(fs, L"loader\\loader.conf");
    printGummiBootConf(conf);
    struct file entries_file = openFile(fs, L"loader\\entries\\", EFI_FILE_MODE_READ, EFI_FILE_READ_ONLY);
    int entryCount = scanEntries(entries, fs, entries_file.file, entries_file.file);
    closeFile(entries_file);
    for(int i = 0; i < entryCount; i++){
        Print(L"%u\r\n", i);
        printLoaderEntry(entries[i]);
    }
    efiStatus = uefi_call_wrapper(ST->BootServices->OpenProtocol, 1, ImageHandle, &gEfiLoadedImageProtocolGuid, (void**)&LoadedImage);
    UINTN sourcebufferSize = 0;
    VOID *sourcebuffer = NULL;
    /*
    if(EFI_SUCCESS != LoadImg(fs, L"initramfs-linux.img", &sourcebuffer, &sourcebufferSize)){
        Print(L"Something went wrong Loading: %u\r\n", efiStatus);
    }
    */
    //Print(L"Options: %s\r\n", LoadedImage->LoadOptions);
    //CHAR16* options = L"initrd=/initramfs-linux.img";
    //Print(L"Length: %u\r\n", stringLen(options));
    efiStatus = uefi_call_wrapper(ST->BootServices->CloseProtocol, 1, ImageHandle, &gEfiLoadedImageProtocolGuid);
    //executeImage(ImageHandle, LoadedImage->DeviceHandle, /*L"EFI\\tetris.efi"*/L"vmlinuz-linux", NULL, 0, options, stringLen(options)*2);
    //executeImage(ImageHandle, LoadedImage->DeviceHandle, L"EFI\\tetris.efi", NULL, 0, NULL, 0);
    //simpleBoot(ImageHandle, LoadedImage->DeviceHandle);
    
    //uefi_call_wrapper(ST->BootServices->Stall, 1, 70000000);
    if(matchstring(chootConf.mode, "simple", 6)){
        simpleBoot(ImageHandle, LoadedImage->DeviceHandle);
        freeEntries();
        return EFI_SUCCESS;
    }
    struct train *train = parseTrain(chootConf.train, chootConf.trainLen);    
    
    Print(L"Try GOP\r\n");
    EFI_GUID gopGuid = EFI_GRAPHICS_OUTPUT_PROTOCOL_GUID;
    EFI_GRAPHICS_OUTPUT_PROTOCOL *gop;

    efiStatus = uefi_call_wrapper(BS->LocateProtocol, 3, &gopGuid, NULL, (void**)&gop);
    if(EFI_ERROR(efiStatus))
        Print(L"Unable to locate GOP\r\n");
    
    Print(L"Opened GOP\r\n");
    EFI_GRAPHICS_OUTPUT_MODE_INFORMATION *info;
    UINTN SizeOfInfo, numModes, nativeMode;
    efiStatus = uefi_call_wrapper(gop->QueryMode, 4, gop, gop->Mode==NULL?0:gop->Mode->Mode, &SizeOfInfo, &info);
    // this is needed to get the current video mode
    if (efiStatus == EFI_NOT_STARTED)
        Print(L"Mode\r\n");
    if(EFI_ERROR(efiStatus)) {
        Print(L"Unable to get native mode\r\n");
    } else {
        Print(L"Set ModeVariable\r\n");
        nativeMode = gop->Mode->Mode;
        numModes = gop->Mode->MaxMode;
    }
    Print(L"ModeList\r\n");
    for (i = 0; i < numModes; i++) {
        efiStatus = uefi_call_wrapper(gop->QueryMode, 4, gop, i, &SizeOfInfo, &info);
        Print(L"mode %03d width %d height %d format %x%s\r\n",
            i,
            info->HorizontalResolution,
            info->VerticalResolution,
            info->PixelFormat,
            i == nativeMode ? "(current)" : ""
        );
    }
    Print(L"Framebuffer address %x size %d, width %d height %d pixelsperline %d\r\n",
    gop->Mode->FrameBufferBase,
    gop->Mode->FrameBufferSize,
    gop->Mode->Info->HorizontalResolution,
    gop->Mode->Info->VerticalResolution,
    gop->Mode->Info->PixelsPerScanLine);
    int selectedMode = 3;
    efiStatus = uefi_call_wrapper(gop->SetMode, 2, gop, selectedMode);
    efiStatus = uefi_call_wrapper(gop->QueryMode, 4, gop, selectedMode, &SizeOfInfo, &info);
    screenWidth = gop->Mode->Info->HorizontalResolution;
    screenHeight = gop->Mode->Info->VerticalResolution;
    
    /*
    struct train *D51 = AllocatePool(sizeof(struct train));
    struct train *D512 = AllocatePool(sizeof(struct train));
    struct train *Coal = AllocatePool(sizeof(struct train));
    struct train *Coal2 = AllocatePool(sizeof(struct train));
    struct train *Car = AllocatePool(sizeof(struct train));
    struct train *LCoal = AllocatePool(sizeof(struct train));
    struct train *Logo = AllocatePool(sizeof(struct train));
    struct train *C51 = AllocatePool(sizeof(struct train));

    D51->next = D512;
    D51->draw = drawD51;
    D512->next = Coal;
    D512->draw = drawD51;
    Coal->next = Coal2;
    Coal->draw = drawCoal;
    Coal2->next = Car;
    Coal2->draw = drawCoal;
    Car->next = LCoal;
    Car->draw = drawCar;
    LCoal->next = Logo;
    LCoal->draw = drawLCoal;
    Logo->next = C51;
    Logo->draw = drawLogo;
    C51->next = NULL;
    C51->draw = drawC51;
    */
    
    /*
    struct vector *velX = AllocatePool(sizeof(struct vector));
    velX->x = 0;
    velX->y = 0;
    struct vector *velY = AllocatePool(sizeof(struct vector));
    velY->x = -0.5;
    velY->y = 0;
    struct ParticleSystem *ps = getParticleSystem(150, 500, 100000, 200, 50, 0, -0.03, velX, velY);
    */
    
    int x = screenWidth;
    int y = 600;
    //driveTrain(gop, C51, x, y);
    
    //drawCar(gop, Car, x - 700, y - 300, 255);
    //drawLCoal(gop, LCoal, x - 700, y - 300, 255);
    //drawLogo(gop, Logo, x - 700, y - 300, 255);
    //drawC51(gop, C51, x - 700, y - 300, 255);
    
    //Plot_Station(gop, 100, y - 17 * fieldHeight, 255);
    driveTrain(gop, train, x, y);
    //Plot_Station(gop, 100, y - 17 * fieldHeight, background);
    

    int white = (255 << 16) + (255 << 8) + 255;
    int black = 0;
    drawTrain(gop, train, x, y, white);
    int stop = calculateTrainLenght(train);
    stop *= -1;
    while(x > stop){
        drawTrain(gop, train, x, y, white);
        uefi_call_wrapper(ST->BootServices->Stall, 1, 70000);
        drawTrain(gop, train, x, y, black);
        D51State = (D51State + 1) % 6;
        LogoState = (LogoState + 1) % 6;
        C51State = (C51State + 1) % 6;
        x -= 5;
	EFI_INPUT_KEY key;
    	EFI_STATUS st;
	st = uefi_call_wrapper(ST->ConIn->ReadKeyStroke, 1, ST->ConIn, &key);
	if(st == EFI_SUCCESS){
		if(key.ScanCode == ENTER){
			Print(L"Select Current Train Position OS\r\n");
		}
	}else if(st == EFI_NOT_READY){
	     Print(L"Read Key not ready\r\n");
	}else if(st == EFI_DEVICE_ERROR){
		 Print(L"Read Key Device Error\r\n");
	}else{
		 Print(L"Read Key Error That should not happen\r\n");
	}
    }
    
    /*
    FreePool(D51);
    FreePool(D512);
    FreePool(Coal);
    FreePool(Coal2);
    FreePool(Car);
    FreePool(LCoal);
    FreePool(Logo);
    FreePool(C51);
    */
    
    //FreeTrain(train);
    
    /*
    fieldWidth *= 1.3;
    fieldHeight *= 1.3;
    */
    Plot_String(gop, 150, 150, "ABCDEFGHIJKLMNOPQRSTUVWXYZ", 26, 255 << 8);
    Plot_String(gop, 150, 151 + fieldHeight, "abcdefghijklmnopqrstuvwxyz", 26, 255 << 8);
    Plot_String(gop, 150, 152 + 2 * fieldHeight, "!\"#$%&\\()*+'-_./0123456789", 26, 255 << 8);
    Plot_String(gop, 150, 153 + 3 * fieldHeight, ":;<=>?@[~]@{}", 13, 255 << 8);
    
    
    
    //freeEntries();
    return EFI_SUCCESS;
}

//http://www.ascii-art.de/ascii/t/train.txt
