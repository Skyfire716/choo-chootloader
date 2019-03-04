#include <efi.h>
#include <efilib.h>
#include <sl.h>


#define LOGO1  L"     ++      +------ \n\r"
#define LOGO2  L"     ||      |+-+ |  \n\r"
#define LOGO3  L"   /---------|| | |  \n\r"
#define LOGO4  L"  + ========  +-+ |  \n\r"

#define LWHL11 L" _|--O========O~\\-+  \n\r"
#define LWHL12 L"//// \\_/      \\_/    \n\r"

#define LWHL21 L" _|--/O========O\\-+  \n\r"
#define LWHL22 L"//// \\_/      \\_/    \n\r"

#define LWHL31 L" _|--/~O========O-+  \n\r"
#define LWHL32 L"//// \\_/      \\_/    \n\r"

#define LWHL41 L" _|--/~\\------/~\\-+  \n\r"
#define LWHL42 L"//// \\_O========O    \n\r"

#define LWHL51 L" _|--/~\\------/~\\-+  \n\r"
#define LWHL52 L"//// \\O========O/    \n\r"

#define LWHL61 L" _|--/~\\------/~\\-+  \n\r"
#define LWHL62 L"//// O========O_/    \n\r"


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
    Status = ST->ConOut->OutputString(ST->ConOut, LOGO1);
    if (EFI_ERROR(Status))
        return Status;
    Status = ST->ConOut->OutputString(ST->ConOut, LOGO2);
    if (EFI_ERROR(Status))
        return Status;
    Status = ST->ConOut->OutputString(ST->ConOut, LOGO3);
    if (EFI_ERROR(Status))
        return Status;
    Status = ST->ConOut->OutputString(ST->ConOut, LOGO4);
    if (EFI_ERROR(Status))
        return Status;
    Status = ST->ConOut->OutputString(ST->ConOut, LWHL11);
    if (EFI_ERROR(Status))
        return Status;
    Status = ST->ConOut->OutputString(ST->ConOut, LWHL12);
    if (EFI_ERROR(Status))
        return Status;
    
    while ((Status = ST->ConIn->ReadKeyStroke(ST->ConIn, &Key)) == EFI_NOT_READY) ;
    return Status;
}
