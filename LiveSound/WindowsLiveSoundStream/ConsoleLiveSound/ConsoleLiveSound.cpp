// ConsoleLiveSound.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>

//int main()
//{
//    std::cout << "Hello World!\n";
//}

// Run program: Ctrl + F5 or Debug > Start Without Debugging menu
// Debug program: F5 or Debug > Start Debugging menu

// Tips for Getting Started: 
//   1. Use the Solution Explorer window to add/manage files
//   2. Use the Team Explorer window to connect to source control
//   3. Use the Output window to see build output and other messages
//   4. Use the Error List window to view errors
//   5. Go to Project > Add New Item to create new code files, or Project > Add Existing Item to add existing code files to the project
//   6. In the future, to open this project again, go to File > Open > Project and select the .sln file


#include <basetyps.h>
#include <stdlib.h>
#include <wtypes.h>
#include <strsafe.h>
#include <cfgmgr32.h>
#include <initguid.h>
#include <stdio.h>
#include <string.h>
#include <winioctl.h>
//#include "public.h"
#include <dontuse.h>

#include "WinPCMs32le.h"
//unsigned int WinPCMs32_len = 15360*2*2*2;
unsigned int WinPCMs32_len2 = 524288;

unsigned int WinPCMs32_len = 32768;
#include "voice.h"

#define IOCTL_CSMT_READ_METHOD_BUFFERED CTL_CODE(FILE_DEVICE_UNKNOWN, 0x900, METHOD_BUFFERED, FILE_ANY_ACCESS)

DEFINE_GUID(GUID_DEVINTERFACE_BUSENUM_TOASTER,
    0x05513b4d, 0x6462, 0x45e1, 0x8c, 0xbd, 0x0a, 0x43, 0xac, 0x17, 0x6b, 0x91);
//  {D35F7840-6A0C-11d2-B841-00C04FAD5171}
//  "05513b4d-6462-45e1-8cbd-0a43ac176b91"

//
// Prototypes
//
BOOLEAN GetDevicePath(_In_ LPCGUID InterfaceGuid, _Out_writes_(BufLen) PWCHAR DevicePath, _In_ size_t BufLen);

BOOLEAN OpenBusInterface(_In_z_ LPCWSTR DevicePath);

#define USAGE  \
"Usage: Enum [-p SerialNo] Plugs in a device. SerialNo must be greater than zero.\n\
             [-u SerialNo or 0] Unplugs device(s) - specify 0 to unplug all \
                                the devices enumerated so far.\n\
             [-e SerialNo or 0] Ejects device(s) - specify 0 to eject all \
                                the devices enumerated so far.\n"

#define MAX_DEVPATH_LENGTH                       256

BOOLEAN     bPlugIn, bUnplug, bEject;
ULONG       SerialNo;

INT __cdecl
main(
    _In_ ULONG argc,
    _In_reads_(argc) PCHAR argv[]
)
{
    WCHAR devicePath[MAX_DEVPATH_LENGTH] = { 0 };

    bPlugIn = bUnplug = bEject = FALSE;

    if (argc < 1) {
        goto usage;
    }
    /*
    if (argv[1][0] == '-') {
        if (tolower(argv[1][1]) == 'p') {
            if (argv[2])
                SerialNo = (USHORT)atol(argv[2]);
            bPlugIn = TRUE;
        }
        else if (tolower(argv[1][1]) == 'u') {
            if (argv[2])
                SerialNo = (ULONG)atol(argv[2]);
            bUnplug = TRUE;
        }
        else if (tolower(argv[1][1]) == 'e') {
            if (argv[2])
                SerialNo = (ULONG)atol(argv[2]);
            bEject = TRUE;
        }
        else {
            goto usage;
        }
    }
    else
        goto usage;

    if (bPlugIn && 0 == SerialNo)
        goto usage;
    */

    if (GetDevicePath(&GUID_DEVINTERFACE_BUSENUM_TOASTER,
        devicePath,
        sizeof(devicePath) / sizeof(devicePath[0]))) {
        OpenBusInterface(devicePath);
    }

    return 0;
usage:
    printf(USAGE);
    exit(0);
}

BOOLEAN
OpenBusInterface2(
    _In_z_ LPCWSTR DevicePath
)
{
    HANDLE                              file;
    ULONG                               bytes;
    //BUSENUM_UNPLUG_HARDWARE             unplug;
    //BUSENUM_EJECT_HARDWARE              eject;
    //PBUSENUM_PLUGIN_HARDWARE            hardware;
    BOOLEAN                             bSuccess = FALSE;
    char data[] = "This String is from Device Driver iii";
    size_t datalen = strlen("This String is from Device Driver !!!") + 1;//Length of data including null
    PCHAR hardware = (PCHAR)malloc(bytes = datalen);
    //strcpy_s((hardware, bytes-1, "This String is from Device Driver !!!");
    int countloop = 0, totalloop = 0;

    printf("Opening %ws\n", DevicePath);
    /*
       file = CreateFile(DevicePath,
        GENERIC_READ | GENERIC_WRITE, // Only read access
        FILE_SHARE_READ | FILE_SHARE_WRITE, // FILE_SHARE_READ | FILE_SHARE_WRITE
        NULL, // no SECURITY_ATTRIBUTES structure
        OPEN_EXISTING, // No special create flags
        0, // No special attributes
        NULL); // No template file
     */
    file = CreateFile(
        TEXT("\\\\?\\ROOT#MEDIA#0000#{05513b4d-6462-45e1-8cbd-0a43ac176b91}\\NuJeePhonosyncDrv"), // pointer to name of the file
        GENERIC_READ | GENERIC_WRITE,                      // access (read-write) mode
        FILE_SHARE_READ | FILE_SHARE_WRITE,                      // share mode
        NULL,                   // pointer to security attributes
        OPEN_EXISTING,          // how to create
        0,  // file attributes
        NULL                    // handle to file with attributes to copy
    );

    if (INVALID_HANDLE_VALUE == file) {
        free(hardware);
        printf("CreateFile failed: 0x%x", GetLastError());
        goto End;
    }

    //printf("connected ready ...... pls keyin anything ......\n");
    //char Ss[101];
    //scanf_s("%[^\n]%*c", Ss);
    char S[1];
    //scanf_s("%[^\n]%*c", S);

    countloop = 0;
    //while ((S[0] = getchar()) != 'e')
    while (1)
    {
        //scanf_s("%c", &S[0], 1);
        /*
        if (S[0] == 'C') {
            printf("C contitnue input. Try again\n");
        }
        else if (S[0] == 'e') {
            break;
        }
        else {
            printf("unknown contitnue input. Try again\n");
        }

        printf("LiveSoundStream interface opened!!!  %d\n", bytes);
        */
        //printf(data);
        if (!DeviceIoControl(file, IOCTL_CSMT_READ_METHOD_BUFFERED, Ring08_pcm + WinPCMs32_len * countloop, WinPCMs32_len, hardware, bytes, &bytes, NULL)) {
            //free(hardware);
            printf("PlugIn failed:0x%x\n", GetLastError());
            //goto End;
            Sleep(5);
        }
        else {
            Sleep(550);
            countloop++;
        }
        if (countloop >= 8) {
            countloop = 0;
            totalloop++;
            if (totalloop >= 40) {
                break;
            }
            printf("LiveSoundStream loop!!!\n");
        }
        //printf("ok next LiveSoundStream interface opened!!!  %d\n", bytes);
    }

    printf("=====string from drv==============\n");
    printf(hardware);
    printf("==============================================\n");

    free(hardware);
    //
    // Enumerate Devices
    //

    if (bPlugIn) {

        printf("SerialNo. of the device to be enumerated: %d\n", SerialNo);

        //hardware = malloc(bytes = (sizeof(BUSENUM_PLUGIN_HARDWARE) +  BUS_HARDWARE_IDS_LENGTH));

        //if (hardware) {
        //    hardware->Size = sizeof(BUSENUM_PLUGIN_HARDWARE);
        //    hardware->SerialNo = SerialNo;
        //}
        //else {
        //    printf("Couldn't allocate %d bytes for busenum plugin hardware structure.\n", bytes);
        //    goto End;
        //}

        //
        // Allocate storage for the Device ID
        //
        //memcpy(hardware->HardwareIDs, BUS_HARDWARE_IDS, BUS_HARDWARE_IDS_LENGTH);

        //if (!DeviceIoControl(file,IOCTL_BUSENUM_PLUGIN_HARDWARE,hardware, bytes,NULL, 0,&bytes, NULL)) {
        //    free(hardware);
        //    printf("PlugIn failed:0x%x\n", GetLastError());
        //    goto End;
        //}

        //free(hardware);
    }

    //
    // Removes a device if given the specific Id of the device. Otherwise this
    // ioctls removes all the devices that are enumerated so far.
    //

    if (bUnplug) {
        printf("Unplugging device(s)....\n");
        /*
        unplug.Size = bytes = sizeof(unplug);
        unplug.SerialNo = SerialNo;
        if (!DeviceIoControl(file,
            IOCTL_BUSENUM_UNPLUG_HARDWARE,
            &unplug, bytes,
            NULL, 0,
            &bytes, NULL)) {
            printf("Unplug failed: 0x%x\n", GetLastError());
            goto End;
        }
        */
    }

    //
    // Ejects a device if given the specific Id of the device. Otherwise this
    // ioctls ejects all the devices that are enumerated so far.
    //
    if (bEject) {
        printf("Ejecting Device(s)\n");
        /*
        eject.Size = bytes = sizeof(eject);
        eject.SerialNo = SerialNo;
        if (!DeviceIoControl(file,
            IOCTL_BUSENUM_EJECT_HARDWARE,
            &eject, bytes,
            NULL, 0,
            &bytes, NULL)) {
            printf("Eject failed: 0x%x\n", GetLastError());
            goto End;
        }
        */
    }

    printf("Success!!!\n");
    bSuccess = TRUE;

    printf("close App ...... pls keyin anything ......\n");


End:
    if (INVALID_HANDLE_VALUE != file) {
        CloseHandle(file);
    }
    return bSuccess;
}


BOOLEAN
OpenBusInterface(
    _In_z_ LPCWSTR DevicePath
)
{
    HANDLE                              file;
    ULONG                               bytes;
    //BUSENUM_UNPLUG_HARDWARE             unplug;
    //BUSENUM_EJECT_HARDWARE              eject;
    //PBUSENUM_PLUGIN_HARDWARE            hardware;
    BOOLEAN                             bSuccess = FALSE;
    char data[] = "This String is from Device Driver iii";
    size_t datalen = strlen("This String is from Device Driver !!!") + 1;//Length of data including null
    PCHAR hardware = (PCHAR)malloc(bytes = datalen);
    //strcpy_s((hardware, bytes-1, "This String is from Device Driver !!!");
    int countloop = 0, totalloop = 0;

    printf("Opening %ws\n", DevicePath);
    /*
       file = CreateFile(DevicePath,
        GENERIC_READ | GENERIC_WRITE, // Only read access
        FILE_SHARE_READ | FILE_SHARE_WRITE, // FILE_SHARE_READ | FILE_SHARE_WRITE
        NULL, // no SECURITY_ATTRIBUTES structure
        OPEN_EXISTING, // No special create flags
        0, // No special attributes
        NULL); // No template file
     */
    file = CreateFile(
        TEXT("\\\\?\\ROOT#MEDIA#0000#{05513b4d-6462-45e1-8cbd-0a43ac176b91}\\NuJeePhonosyncDrv"), // pointer to name of the file
        GENERIC_READ | GENERIC_WRITE,                      // access (read-write) mode
        FILE_SHARE_READ | FILE_SHARE_WRITE,                      // share mode
        NULL,                   // pointer to security attributes
        OPEN_EXISTING,          // how to create
        0,  // file attributes
        NULL                    // handle to file with attributes to copy
    );

    if (INVALID_HANDLE_VALUE == file) {
        free(hardware);
        printf("CreateFile failed: 0x%x", GetLastError());
        goto End;
    }

    //printf("connected ready ...... pls keyin anything ......\n");
    //char Ss[101];
    //scanf_s("%[^\n]%*c", Ss);
    char S[1];
    //scanf_s("%[^\n]%*c", S);

    countloop = 0;
    //while ((S[0] = getchar()) != 'e')
    while (1) 
    {
        //scanf_s("%c", &S[0], 1);
        /*
        if (S[0] == 'C') {
            printf("C contitnue input. Try again\n");
        }
        else if (S[0] == 'e') {
            break;
        }
        else {
            printf("unknown contitnue input. Try again\n");
        }

        printf("LiveSoundStream interface opened!!!  %d\n", bytes);
        */
        //printf(data);
        if (!DeviceIoControl(file, IOCTL_CSMT_READ_METHOD_BUFFERED, WinPCMs32le_raw + WinPCMs32_len * countloop, WinPCMs32_len, hardware, bytes, &bytes, NULL)) {
            //free(hardware);
            printf("PlugIn failed:0x%x\n", GetLastError());
            //goto End;
            Sleep(5);
        }
        else {
            Sleep(60);
            countloop++;
        }
        if (countloop>=30) {
            countloop = 0;
            totalloop++;
            if (totalloop>=20) {
                break;
            }
            printf("LiveSoundStream loop!!!\n");
        }
        //printf("ok next LiveSoundStream interface opened!!!  %d\n", bytes);
    }
 
    printf("=====string from drv==============\n");
    printf(hardware);
    printf("==============================================\n");

    free(hardware);
    //
    // Enumerate Devices
    //

    if (bPlugIn) {

        printf("SerialNo. of the device to be enumerated: %d\n", SerialNo);

        //hardware = malloc(bytes = (sizeof(BUSENUM_PLUGIN_HARDWARE) +  BUS_HARDWARE_IDS_LENGTH));

        //if (hardware) {
        //    hardware->Size = sizeof(BUSENUM_PLUGIN_HARDWARE);
        //    hardware->SerialNo = SerialNo;
        //}
        //else {
        //    printf("Couldn't allocate %d bytes for busenum plugin hardware structure.\n", bytes);
        //    goto End;
        //}

        //
        // Allocate storage for the Device ID
        //
        //memcpy(hardware->HardwareIDs, BUS_HARDWARE_IDS, BUS_HARDWARE_IDS_LENGTH);

        //if (!DeviceIoControl(file,IOCTL_BUSENUM_PLUGIN_HARDWARE,hardware, bytes,NULL, 0,&bytes, NULL)) {
        //    free(hardware);
        //    printf("PlugIn failed:0x%x\n", GetLastError());
        //    goto End;
        //}

        //free(hardware);
    }

    //
    // Removes a device if given the specific Id of the device. Otherwise this
    // ioctls removes all the devices that are enumerated so far.
    //

    if (bUnplug) {
        printf("Unplugging device(s)....\n");
        /*
        unplug.Size = bytes = sizeof(unplug);
        unplug.SerialNo = SerialNo;
        if (!DeviceIoControl(file,
            IOCTL_BUSENUM_UNPLUG_HARDWARE,
            &unplug, bytes,
            NULL, 0,
            &bytes, NULL)) {
            printf("Unplug failed: 0x%x\n", GetLastError());
            goto End;
        }
        */
    }

    //
    // Ejects a device if given the specific Id of the device. Otherwise this
    // ioctls ejects all the devices that are enumerated so far.
    //
    if (bEject) {
        printf("Ejecting Device(s)\n");
        /*
        eject.Size = bytes = sizeof(eject);
        eject.SerialNo = SerialNo;
        if (!DeviceIoControl(file,
            IOCTL_BUSENUM_EJECT_HARDWARE,
            &eject, bytes,
            NULL, 0,
            &bytes, NULL)) {
            printf("Eject failed: 0x%x\n", GetLastError());
            goto End;
        }
        */
    }

    printf("Success!!!\n");
    bSuccess = TRUE;

    printf("close App ...... pls keyin anything ......\n");


End:
    if (INVALID_HANDLE_VALUE != file) {
        CloseHandle(file);
    }
    return bSuccess;
}

BOOLEAN
GetDevicePath(
    _In_ LPCGUID InterfaceGuid,
    _Out_writes_(BufLen) PWCHAR DevicePath,
    _In_ size_t BufLen
)
{
    CONFIGRET cr = CR_SUCCESS;
    PWSTR deviceInterfaceList = NULL;
    ULONG deviceInterfaceListLength = 0;
    PWSTR nextInterface;
    HRESULT hr = E_FAIL;
    BOOLEAN bRet = TRUE;

    cr = CM_Get_Device_Interface_List_Size(
        &deviceInterfaceListLength,
        (LPGUID)InterfaceGuid,
        NULL,
        CM_GET_DEVICE_INTERFACE_LIST_PRESENT);
    if (cr != CR_SUCCESS) {
        printf("Error 0x%x retrieving device interface list size.\n", cr);
        goto clean0;
    }

    if (deviceInterfaceListLength <= 1) {
        bRet = FALSE;
        printf("Error: No active device interfaces found.\n"
            " Is the sample driver loaded?");
        goto clean0;
    }

    deviceInterfaceList = (PWSTR)malloc(deviceInterfaceListLength * sizeof(WCHAR));
    if (deviceInterfaceList == NULL) {
        printf("Error allocating memory for device interface list.\n");
        goto clean0;
    }
    ZeroMemory(deviceInterfaceList, deviceInterfaceListLength * sizeof(WCHAR));

    cr = CM_Get_Device_Interface_List(
        (LPGUID)InterfaceGuid,
        NULL,
        deviceInterfaceList,
        deviceInterfaceListLength,
        CM_GET_DEVICE_INTERFACE_LIST_PRESENT);
    if (cr != CR_SUCCESS) {
        printf("Error 0x%x retrieving device interface list.\n", cr);
        goto clean0;
    }

    nextInterface = deviceInterfaceList + wcslen(deviceInterfaceList) + 1;
    if (*nextInterface != UNICODE_NULL) {
        printf("Warning: More than one device interface instance found. \n"
            "Selecting first matching device.\n\n");
    }

    hr = StringCchCopy(DevicePath, BufLen, deviceInterfaceList);
    if (FAILED(hr)) {
        bRet = FALSE;
        printf("Error: StringCchCopy failed with HRESULT 0x%x", hr);
        goto clean0;
    }

clean0:
    if (deviceInterfaceList != NULL) {
        free(deviceInterfaceList);
    }
    if (CR_SUCCESS != cr) {
        bRet = FALSE;
    }

    return bRet;
}