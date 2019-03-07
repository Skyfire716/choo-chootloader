#include <efi.h>
#include <efilib.h>
#include <stdlib.h>

short unsigned int LOGO1[21] = {' ', ' ', ' ', ' ', ' ', '+', '+', ' ', ' ', ' ', ' ', ' ', ' ', '+', '-', '-', '-', '-', '-', '-', ' '};
short unsigned int LOGO2[21] = {' ', ' ', ' ', ' ', ' ', '|', '|', ' ', ' ', ' ', ' ', ' ', ' ', '|', '+', '-', '+', ' ', '|', ' ', ' '};
short unsigned int LOGO3[21] = {' ', ' ', ' ', '/', '-', '-', '-', '-', '-', '-', '-', '-', '-', '|', '|', ' ', '|', ' ', '|', ' ', ' '};
short unsigned int LOGO4[21] = {' ', ' ', '+', ' ', '=', '=', '=', '=', '=', '=', '=', '=', ' ', ' ', '+', '-', '+', ' ', '|', ' ', ' '};

short unsigned int LWHL11[21] = {' ', '_', '|', '-', '-', 'O', '=', '=', '=', '=', '=', '=', '=', '=', 'O', '~', '\\', '-', '+', ' ', ' '};
short unsigned int LWHL12[21] = {'/', '/', '/', '/', ' ', '\\', '_', '/', ' ', ' ', ' ', ' ', ' ', ' ', '\\', '_', '/', ' ', ' ', ' ', ' '};

short unsigned int LWHL21[21]  = {' ', '_', '|', '-', '-', '/', 'O', '=', '=', '=', '=', '=', '=', '=', '=', 'O', '\\', '-', '+', ' ', ' '};
short unsigned int LWHL22[21]  = {'/', '/', '/', '/', ' ', '\\', '_', '/', ' ', ' ', ' ', ' ', ' ', ' ', '\\', '_', '/', ' ', ' ', ' ', ' '};

short unsigned int LWHL31[21] = {' ', '_', '|', '-', '-', '/', '~', 'O', '=', '=', '=', '=', '=', '=', '=', '=', 'O', '-', '+', ' ', ' '};
short unsigned int LWHL32[21] = {'/', '/', '/', '/', ' ', '\\', '_', '/', ' ', ' ', ' ', ' ', ' ', ' ', '\\', '_', '/', ' ', ' ', ' ', ' '};

short unsigned int LWHL41[21] = {' ', '_', '|', '-', '-', '/', '~', '\\', '-', '-', '-', '-', '-', '-', '/', '~', '\\', '-', '+', ' ', ' '};
short unsigned int LWHL42[21] = {'/', '/', '/', '/', ' ', '\\', '_', 'O', '=', '=', '=', '=', '=', '=', '=', '=', 'O', ' ', ' ', ' ', ' '};

short unsigned int LWHL51[21] = {' ', '_', '|', '-', '-', '/', '~', '\\', '-', '-', '-', '-', '-', '-', '/', '~', '\\', '-', '+', ' ', ' '};
short unsigned int LWHL52[21] = {'/', '/', '/', '/', ' ', '\\', 'O', '=', '=', '=', '=', '=', '=', '=', '=', 'O', '/', ' ', ' ', ' ', ' '};

short unsigned int LWHL61[21] = {' ', '_', '|', '-', '-', '/', '~', '\\', '-', '-', '-', '-', '-', '-', '/', '~', '\\', '-', '+', ' ', ' '};
short unsigned int LWHL62[21] = {'/', '/', '/', '/', ' ', 'O', '=', '=', '=', '=', '=', '=', '=', '=', 'O', '_', '/', ' ', ' ', ' ', ' '};

short unsigned int LCOAL1[21] = {'_', '_', '_', '_', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' '};
short unsigned int LCOAL2[21] = {'|', ' ', ' ', ' ', '\\', '@', '@', '@', '@', '@', '@', '@', '@', '@', '@', '@', ' ', ' ', ' ', ' ', ' '};
short unsigned int LCOAL3[21] = {'|', ' ', ' ', ' ', ' ', '\\', '@', '@', '@', '@', '@', '@', '@', '@', '@', '@', '@', '@', '@', '_', ' '};
short unsigned int LCOAL4[21] = {'|', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', '|', ' '};
short unsigned int LCOAL5[21] = {'|', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '|', ' '};
short unsigned int LCOAL6[21] = {' ', ' ', ' ', '(', 'O', ')', ' ', ' ', ' ', ' ', ' ', ' ', ' ', '(', 'O', ')', ' ', ' ', ' ', ' ', ' '};

short unsigned int LCAR1[21] = {'_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', ' '};
short unsigned int LCAR2[21] = {'|', ' ', ' ', '_', '_', '_', ' ', '_', '_', '_', ' ', '_', '_', '_', ' ', '_', '_', '_', ' ', '|', ' '};
short unsigned int LCAR3[21] = {'|', ' ', ' ', '|', '_', '|', ' ', '|', '_', '|', ' ', '|', '_', '|', ' ', '|', '_', '|', ' ', '|', ' '};
short unsigned int LCAR4[21] = {'|', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '|', ' '};
short unsigned int LCAR5[21] = {'|', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '|', ' '};
short unsigned int LCAR6[21] = {' ', ' ', ' ', '(', 'O', ')', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', '(', 'O', ')', ' ', ' ', ' ', ' '};

struct field{
    int width;
    int height;
    short unsigned int [200][200];
} field;

typedef struct field field_t;

field_t* get_field(int width, int height);
void finit_field(field_t *field);
short unsigned int* get_String(field_t field);
void setCharacter(int x, int y, short unsigned int character, field_t *field);

EFI_STATUS efi_main(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable)
{
    EFI_STATUS Status;
    EFI_INPUT_KEY Key;
 
    /* Store the system table for future use in other functions */
    ST = SystemTable;
 
    /* Say hi */
    Status = ST->ConOut->OutputString(ST->ConOut, L"Hello Choot\n\r");
    if (EFI_ERROR(Status))
        return Status;
 
    /* Now wait for a keystroke before continuing, otherwise your
       message will flash off the screen before you see it.
 
       First, we need to empty the console input buffer to flush
       out any keystrokes entered before this point */
    Status = ST->ConIn->Reset(ST->ConIn, FALSE);
    if (EFI_ERROR(Status))
        return Status;
 
    /* Now wait until a key becomes available.  This is a simple
       polling implementation.  You could try and use the WaitForKey
       event instead if you like */
    while ((Status = ST->ConIn->ReadKeyStroke(ST->ConIn, &Key)) == EFI_NOT_READY) ;
 
    /*
     * Show the sl
     */
    
    field_t *field = get_field(10, 10);
    if(field == NULL){
        Status = ST->ConOut->OutputString(ST->ConOut, L"Field nicht erstellt\n\r");
        if (EFI_ERROR(Status)){
            return Status;
        }
    }else{
        setCharacter(0, 0, 'a', field);
        
        finit_field(field);
    }
    /*
    Status = ST->ConOut->OutputString(ST->ConOut, background_generator(6, 21));
    if (EFI_ERROR(Status)){
        return Status;
    }
    */
    
    while ((Status = ST->ConIn->ReadKeyStroke(ST->ConIn, &Key)) == EFI_NOT_READY) ;
    return Status;
}

field_t* get_field(int width, int height){
    field_t *field;
    if((malloc(sizeof(field_t))) == NULL){
        return NULL;
    }
    field->width = width;
    field->height = height;
    field->field = malloc(width * height * sizeof(short unsigned int));
    return field;
}
 
void setCharacter(int x, int y, short unsigned int character, field_t *field) {
    if(field->width <= x || field->height <= y || y < 0 || x < 0){
        return;
    }
    field->field[field->width * field->height * y + x] = character;
}

void finit_field(field_t* field){
    free(field->field);
    free(field);
}
