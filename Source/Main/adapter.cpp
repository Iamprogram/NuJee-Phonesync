/*++

Copyright (c) Microsoft Corporation All Rights Reserved

Module Name:

    adapter.cpp

Abstract:

    Setup and miniport installation.  No resources are used by simple audio sample.
    This sample is to demonstrate how to develop a full featured audio miniport driver.
--*/
#pragma once
#pragma warning (disable : 4127)

//
// All the GUIDS for all the miniports end up in this object.
//
#define PUT_GUIDS_HERE

#include "definitions.h"
#include "endpoints.h"
#include "minipairs.h"

#include "RingBuffer.h"
//#pragma warning (disable : 4067)
#include "WinPCMs32le2.h"
unsigned int WinPCMs32le_rawTest_len = 929880;

// NJ IOCTL
//////////////////////////////////////////////////////////////////////////
// Definitions.
//////////////////////////////////////////////////////////////////////////

#define DRIVER_NAME         L"NuJeePhonosyncDrv"
#define DEVICE_NAME         L"\\Device\\" DRIVER_NAME
#define SYMLINK_NAME        L"\\DosDevices\\" DRIVER_NAME

#define DRIVER_DEVICE_NAME         L"\\NuJeePhonosyncDrv"

UNICODE_STRING DEVICE_SYMBOLIC_NAME;

_Dispatch_type_(IRP_MJ_CREATE)
DRIVER_DISPATCH IoctlCreateClose; //function Body has implemented
_Dispatch_type_(IRP_MJ_CLOSE)
DRIVER_DISPATCH IoctlCreateClose; //function Body has implemented
_Dispatch_type_(IRP_MJ_DEVICE_CONTROL)
DRIVER_DISPATCH NuJeeIOCtlDeviceControl; //function Body has implemented



NTSTATUS IoctlCreateClose(_In_ DEVICE_OBJECT* _DeviceObject, _Inout_ IRP* _Irp)
{
    PAGED_CODE();
    UNREFERENCED_PARAMETER(_DeviceObject);

    ASSERT(_DeviceObject);
    ASSERT(_Irp);

    NTSTATUS        status = STATUS_SUCCESS;
    
    //RtlInitUnicodeString(&g_DeviceNameRefStr, DRIVER_DEVICE_NAME);
    UNICODE_STRING g_DeviceNameRefStr = RTL_CONSTANT_STRING(L"\\NuJeePhonosyncDrv");

    auto IrpStack = IoGetCurrentIrpStackLocation(_Irp);
    DbgPrint("IoctlCreateClose device :=[%wZ]", IrpStack->FileObject->FileName);
    DbgPrint("IoctlCreateClose g_DeviceNameRefStr :=[%wZ]", g_DeviceNameRefStr);
    const auto IsUserSpaceAppRequest = !RtlCompareUnicodeString(&g_DeviceNameRefStr, &IrpStack->FileObject->FileName, TRUE);
    if (!IsUserSpaceAppRequest)
    {
        //RtlFreeUnicodeString(&g_DeviceNameRefStr);
        return PcDispatchIrp(_DeviceObject, _Irp);
    }

    //RtlFreeUnicodeString(&g_DeviceNameRefStr);
    DEVICE_SYMBOLIC_NAME = RTL_CONSTANT_STRING(SYMLINK_NAME);

    PIO_STACK_LOCATION stackLocation = NULL;
    stackLocation = IoGetCurrentIrpStackLocation(_Irp);
    
    unsigned int ringBufferbufferSize = 256;

    switch (stackLocation->MajorFunction)
    {
    case IRP_MJ_CREATE:
        DbgPrint("Handle to symbolink link %wZ opened", DEVICE_SYMBOLIC_NAME);
        
        // NJ allocator for RingBuffer
        //unsigned int ringBufferbufferSize = 256;
        if (!GetRingBufferInstance(ringBufferbufferSize)) {
            status = STATUS_INSUFFICIENT_RESOURCES;
            DPF(D_TERSE, ("[Debug NJ] fail GetRingBufferInstance bufferSize= [%d]", ringBufferbufferSize));
        }
        else {
            status = STATUS_SUCCESS;
            RingBufferDataType* sourceBuffStart = ringBufferReadStart(GetRingBufferReadyInstance());
            if (sourceBuffStart != NULL) {
                RtlCopyBytes(sourceBuffStart, WinPCMs32le_raw2, WinPCMs32le_raw2_len);
                DPF(D_TERSE, ("[Debug NJ] >>>>>>>>copy  WinPCMs32le_raw_len= [%d]", WinPCMs32le_raw2_len));
            }
            DPF(D_TERSE, ("[Debug NJ] temp GetRingBufferInstance bufferSize= [%d]", ringBufferbufferSize));
        }

        _Irp->IoStatus.Information = 0;
        _Irp->IoStatus.Status = STATUS_SUCCESS;
        IoCompleteRequest(_Irp, IO_NO_INCREMENT);
        return STATUS_SUCCESS;
       
        break;
    case IRP_MJ_CLOSE:
        DbgPrint("Handle to symbolink link %wZ closed", DEVICE_SYMBOLIC_NAME);

        // NJ release for RingBuffer
        if (ReleaseRingBufferInstance()) {
            DPF(D_TERSE, ("[Debug NJ] ReleaseRingBufferInstance ok=============="));
        }
        else {
            DPF(D_TERSE, ("[Debug NJ] ReleaseRingBufferInstance fail=============="));
        }

        _Irp->IoStatus.Information = 0;
        _Irp->IoStatus.Status = STATUS_SUCCESS;
        IoCompleteRequest(_Irp, IO_NO_INCREMENT);
        return STATUS_SUCCESS;
        
        break;
    default:
        break;
    }

    //DbgPrint("[Debug 2024] ******** Non MyApp IoctlCreateClose *************** ");
    //NTSTATUS ntStatus = PcDispatchIrp(_DeviceObject, _Irp);
    //DbgPrint("[Debug 2024] ******** Non MyApp IoctlCreateClose *************** ok ");

    return STATUS_SUCCESS;
}

UNICODE_STRING referenceIOCtlString;
UNICODE_STRING linkIOCtlString;
PDEVICE_OBJECT m_pPhysicalIOCtldeviceObject;

// not use this name
#define CSMT_IOCTL_NT_DEVICE_NAME L"\\Device\\Mtctldevice"
// IOCTL GUID
DEFINE_GUIDSTRUCT("05513b4d-6462-45e1-8cbd-0a43ac176b91", MTCS_IOCTL_AUDIO);
#define MTCS_IOCTL_AUDIO DEFINE_GUIDNAMED(MTCS_IOCTL_AUDIO)
#define IOCTL_CSMT_READ_METHOD_BUFFERED CTL_CODE(FILE_DEVICE_UNKNOWN, 0x900, METHOD_BUFFERED, FILE_ANY_ACCESS)

#pragma code_seg("PAGE")
NTSTATUS NuJeeIOCtlDeviceControl(_In_ DEVICE_OBJECT* _DeviceObject, _Inout_ IRP* _Irp)
{
    DbgPrint("[Debug 2024]  NuJeeIOCtlDeviceControl");
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PIO_STACK_LOCATION irpStack;
    ULONG ControlCode;
    //ULONG inBufLength; // Input buffer length
    ULONG inBufLength,outBufLength; // Output buffer length
    PCHAR inBuf, outBuf; // pointer to Input and output buffer
    PCHAR data = "ok sound   set from Device Driver out";
    size_t datalen = strlen(data) + 1;//Length of data including null

    PAGED_CODE();
    UNREFERENCED_PARAMETER(_DeviceObject);

    ASSERT(_DeviceObject);
    ASSERT(_Irp);

    UNICODE_STRING g_DeviceNameRefStr = RTL_CONSTANT_STRING(L"\\NuJeePhonosyncDrv");

    auto IrpStack = IoGetCurrentIrpStackLocation(_Irp);
    DbgPrint("NuJeeIOCtlDeviceControl device :=[%wZ]", IrpStack->FileObject->FileName);
    DbgPrint("NuJeeIOCtlDeviceControl g_DeviceNameRefStr :=[%wZ]", g_DeviceNameRefStr);
    const auto IsUserSpaceAppRequest = !RtlCompareUnicodeString(&g_DeviceNameRefStr, &IrpStack->FileObject->FileName, TRUE);
    if (!IsUserSpaceAppRequest)
    {
        return PcDispatchIrp(_DeviceObject, _Irp);
    }


    irpStack = IoGetCurrentIrpStackLocation(_Irp);
    ControlCode = irpStack->Parameters.DeviceIoControl.IoControlCode;
    DbgPrint("NuJeeIOCtlDeviceControl ControlCode :=[%d]", ControlCode);
    switch (ControlCode) {
    case IOCTL_CSMT_READ_METHOD_BUFFERED:
        inBufLength = irpStack->Parameters.DeviceIoControl.InputBufferLength;
        if (inBufLength != 0) {
            inBuf = (PCHAR)_Irp->AssociatedIrp.SystemBuffer;
            //DbgPrint("NuJeeIOCtlDeviceControl inBuf :=[%s]", (PCHAR)_Irp->AssociatedIrp.SystemBuffer);
            // copy to RingBuff 4096 bytes, client APP must send 4096 bytes of Sound
            if (inBufLength >= 4096) {
                DbgPrint("NuJeeIOCtlDeviceControl inBufLength :=[%d]", inBufLength);
                //ringBufferWrite(GetRingBufferReadyInstance(), inBuf);
                RingBufferDataType* sourceBuffStart = ringBufferReadStart(GetRingBufferReadyInstance());
                if (sourceBuffStart != NULL) {
                    //RtlCopyBytes(sourceBuffStart, inBuf, inBufLength);
                    DPF(D_TERSE, ("[Debug NJ] >>>>>>>>copy  virtual sound = [%d]", inBufLength));
                    if (ringBufferWriteBytes(GetRingBufferReadyInstance(), (RingBufferDataType*)inBuf, inBufLength)) {
                        DPF(D_TERSE, ("[Debug NJ] >>>>>>>>copy  ok virtual sound = [%d]", inBufLength));
                    }
                    else {
                        ntStatus = STATUS_INSUFFICIENT_RESOURCES;
                        DPF(D_TERSE, ("[Debug NJ] >>>>>>>>copy  fail virtual sound = [%d]", inBufLength));
                    }
                }
                else {
                    DbgPrint("NuJeeIOCtlDeviceControl sourceBuffStart is null ......<<<<<<<<");
                }
            }
        }
        outBufLength = irpStack->Parameters.DeviceIoControl.OutputBufferLength;
        DbgPrint("NuJeeIOCtlDeviceControl inBufLength :=[%d], outBufLength :=[%d]", inBufLength, outBufLength);
        if (outBufLength != 0) {
            outBuf = (PCHAR)_Irp->AssociatedIrp.SystemBuffer;
            //
            // Write to the buffer over-writes the input buffer content
            //
            RtlCopyBytes(outBuf, data, outBufLength);
            //
            // Assign the length of the data copied to IoStatus.Information
            // of the Irp and complete the Irp.
            //
            _Irp->IoStatus.Information = (outBufLength < datalen ? outBufLength : datalen);

            //
            // When the Irp is completed the content of the SystemBuffer
            // is copied to the User output buffer and the SystemBuffer is
            // is freed.
            //
        }
        else {
            _Irp->IoStatus.Information = 0;
        }
        ntStatus = (ntStatus != STATUS_INSUFFICIENT_RESOURCES)? STATUS_SUCCESS : STATUS_INSUFFICIENT_RESOURCES;
        break;

    default:
        DbgPrint("[Debug 2024] default NuJeeIOCtlDeviceControl");
        ntStatus = STATUS_INVALID_PARAMETER;
        //_Irp->IoStatus.Information = 0;
        break;
    }

    if (ntStatus != STATUS_INVALID_PARAMETER) {
        DbgPrint("[Debug 2024] STATUS_SUCCESS NuJeeIOCtlDeviceControl");
        //ntStatus = STATUS_SUCCESS;
        ntStatus = (ntStatus != STATUS_INSUFFICIENT_RESOURCES) ? STATUS_SUCCESS : STATUS_INSUFFICIENT_RESOURCES;

        _Irp->IoStatus.Status = ntStatus;
        IoCompleteRequest(_Irp, IO_NO_INCREMENT);
        DbgPrint("[Debug 2024] STATUS_SUCCESS NuJeeIOCtlDeviceControl MyIOCall");
    }
    else {
        DbgPrint("[Debug 2024] STATUS_INVALID_PARAMETER NuJeeIOCtlDeviceControl sysIOCall");
        ntStatus = PcDispatchIrp(_DeviceObject, _Irp);
        DbgPrint("[Debug 2024] STATUS_INVALID_PARAMETER NuJeeIOCtlDeviceControl sysIOCall ok");
    }

    DbgPrint("[Debug 2024] out NuJeeIOCtlDeviceControl [%d]", ntStatus);
    return ntStatus;
}

// NJ IOCTL

typedef void (*fnPcDriverUnload) (PDRIVER_OBJECT);
fnPcDriverUnload gPCDriverUnloadRoutine = NULL;
extern "C" DRIVER_UNLOAD DriverUnload;

//-----------------------------------------------------------------------------
// Referenced forward.
//-----------------------------------------------------------------------------

DRIVER_ADD_DEVICE AddDevice;

NTSTATUS
StartDevice
( 
    _In_  PDEVICE_OBJECT,      
    _In_  PIRP,                
    _In_  PRESOURCELIST        
); 

_Dispatch_type_(IRP_MJ_PNP)
DRIVER_DISPATCH PnpHandler;

//
// Rendering streams are saved to a file by default. Use the registry value 
// DoNotCreateDataFiles (DWORD) > 0 to override this default.
//
DWORD g_DoNotCreateDataFiles = 0;  // default is off.
DWORD g_DisableToneGenerator = 0;  // default is to generate tones.
UNICODE_STRING g_RegistryPath;      // This is used to store the registry settings path for the driver

//-----------------------------------------------------------------------------
// Functions
//-----------------------------------------------------------------------------

#pragma code_seg("PAGE")
void ReleaseRegistryStringBuffer()
{
    if (g_RegistryPath.Buffer != NULL)
    {
        ExFreePool(g_RegistryPath.Buffer);
        g_RegistryPath.Buffer = NULL;
        g_RegistryPath.Length = 0;
        g_RegistryPath.MaximumLength = 0;
    }
}

//=============================================================================
#pragma code_seg("PAGE")
extern "C"
void DriverUnload 
(
    _In_ PDRIVER_OBJECT DriverObject
)
/*++

Routine Description:

  Our driver unload routine. This just frees the WDF driver object.

Arguments:

  DriverObject - pointer to the driver object

Environment:

    PASSIVE_LEVEL

--*/
{
    PAGED_CODE(); 

    DPF(D_TERSE, ("[DriverUnload]"));

    ReleaseRegistryStringBuffer();

    //release for NJ IOCTL .......
    UNICODE_STRING LinkName = RTL_CONSTANT_STRING(SYMLINK_NAME);
    IoDeleteSymbolicLink(&LinkName);

    
    if (DriverObject == NULL)
    {
        goto Done;
    }
    
    //release for NJ IOCTL .......
    if (referenceIOCtlString.Buffer !=NULL) {
        RtlFreeUnicodeString(&referenceIOCtlString);
    }

    //if (g_DeviceNameRefStr.Buffer != NULL) {
    //    RtlFreeUnicodeString(&g_DeviceNameRefStr);
    //}


    //
    // Invoke first the port unload.
    //
    if (gPCDriverUnloadRoutine != NULL)
    {
        gPCDriverUnloadRoutine(DriverObject);
    }

    //
    // Unload WDF driver object. 
    //
    if (WdfGetDriver() != NULL)
    {
        WdfDriverMiniportUnload(WdfGetDriver());
    }
Done:
    return;
}

//=============================================================================
#pragma code_seg("INIT")
__drv_requiresIRQL(PASSIVE_LEVEL)
NTSTATUS
CopyRegistrySettingsPath(
    _In_ PUNICODE_STRING RegistryPath
)
/*++

Routine Description:

Copies the following registry path to a global variable.

\REGISTRY\MACHINE\SYSTEM\ControlSetxxx\Services\<driver>\Parameters

Arguments:

RegistryPath - Registry path passed to DriverEntry

Returns:

NTSTATUS - SUCCESS if able to configure the framework

--*/

{
    // Initializing the unicode string, so that if it is not allocated it will not be deallocated too.
    RtlInitUnicodeString(&g_RegistryPath, NULL);

    g_RegistryPath.MaximumLength = RegistryPath->Length + sizeof(WCHAR);

    g_RegistryPath.Buffer = (PWCH)ExAllocatePool2(POOL_FLAG_PAGED, g_RegistryPath.MaximumLength, MINADAPTER_POOLTAG);

    if (g_RegistryPath.Buffer == NULL)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlAppendUnicodeToString(&g_RegistryPath, RegistryPath->Buffer);

    return STATUS_SUCCESS;
}

//=============================================================================
#pragma code_seg("INIT")
__drv_requiresIRQL(PASSIVE_LEVEL)
NTSTATUS
GetRegistrySettings(
    _In_ PUNICODE_STRING RegistryPath
   )
/*++

Routine Description:

    Initialize Driver Framework settings from the driver
    specific registry settings under

    \REGISTRY\MACHINE\SYSTEM\ControlSetxxx\Services\<driver>\Parameters

Arguments:

    RegistryPath - Registry path passed to DriverEntry

Returns:

    NTSTATUS - SUCCESS if able to configure the framework

--*/

{
    NTSTATUS                    ntStatus;
    UNICODE_STRING              parametersPath;
    RTL_QUERY_REGISTRY_TABLE    paramTable[] = {
    // QueryRoutine     Flags                                               Name                     EntryContext             DefaultType                                                    DefaultData              DefaultLength
        { NULL,   RTL_QUERY_REGISTRY_DIRECT | RTL_QUERY_REGISTRY_TYPECHECK, L"DoNotCreateDataFiles", &g_DoNotCreateDataFiles, (REG_DWORD << RTL_QUERY_REGISTRY_TYPECHECK_SHIFT) | REG_DWORD, &g_DoNotCreateDataFiles, sizeof(ULONG)},
        { NULL,   RTL_QUERY_REGISTRY_DIRECT | RTL_QUERY_REGISTRY_TYPECHECK, L"DisableToneGenerator", &g_DisableToneGenerator, (REG_DWORD << RTL_QUERY_REGISTRY_TYPECHECK_SHIFT) | REG_DWORD, &g_DisableToneGenerator, sizeof(ULONG)},
        { NULL,   0,                                                        NULL,                    NULL,                    0,                                                             NULL,                    0}
    };

    DPF(D_TERSE, ("[GetRegistrySettings]"));

    PAGED_CODE(); 

    RtlInitUnicodeString(&parametersPath, NULL);

    parametersPath.MaximumLength =
        RegistryPath->Length + sizeof(L"\\Parameters") + sizeof(WCHAR);

    parametersPath.Buffer = (PWCH) ExAllocatePool2(POOL_FLAG_PAGED, parametersPath.MaximumLength, MINADAPTER_POOLTAG);
    if (parametersPath.Buffer == NULL) 
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlAppendUnicodeToString(&parametersPath, RegistryPath->Buffer);
    RtlAppendUnicodeToString(&parametersPath, L"\\Parameters");

    ntStatus = RtlQueryRegistryValues(
                 RTL_REGISTRY_ABSOLUTE | RTL_REGISTRY_OPTIONAL,
                 parametersPath.Buffer,
                 &paramTable[0],
                 NULL,
                 NULL
                );

    if (!NT_SUCCESS(ntStatus)) 
    {
        DPF(D_VERBOSE, ("RtlQueryRegistryValues failed, using default values, 0x%x", ntStatus));
        //
        // Don't return error because we will operate with default values.
        //
    }

    //
    // Dump settings.
    //
    DPF(D_VERBOSE, ("DoNotCreateDataFiles: %u", g_DoNotCreateDataFiles));
    DPF(D_VERBOSE, ("DisableToneGenerator: %u", g_DisableToneGenerator));

    //
    // Cleanup.
    //
    ExFreePool(parametersPath.Buffer);

    return STATUS_SUCCESS;
}

#pragma code_seg("INIT")
extern "C" DRIVER_INITIALIZE DriverEntry;
extern "C" NTSTATUS
DriverEntry
( 
    _In_  PDRIVER_OBJECT          DriverObject,
    _In_  PUNICODE_STRING         RegistryPathName
)
{
/*++

Routine Description:

  Installable driver initialization entry point.
  This entry point is called directly by the I/O system.

  All audio adapter drivers can use this code without change.

Arguments:

  DriverObject - pointer to the driver object

  RegistryPath - pointer to a unicode string representing the path,
                   to driver-specific key in the registry.

Return Value:

  STATUS_SUCCESS if successful,
  STATUS_UNSUCCESSFUL otherwise.

--*/
    NTSTATUS                    ntStatus;
    WDF_DRIVER_CONFIG           config;

    DPF(D_TERSE, ("[DriverEntry]"));

    // Copy registry Path name in a global variable to be used by modules inside driver.
    // !! NOTE !! Inside this function we are initializing the registrypath, so we MUST NOT add any failing calls
    // before the following call.
    ntStatus = CopyRegistrySettingsPath(RegistryPathName);
    IF_FAILED_ACTION_JUMP(
        ntStatus,
        DPF(D_ERROR, ("Registry path copy error 0x%x", ntStatus)),
        Done);

    //
    // Get registry configuration.
    //
    ntStatus = GetRegistrySettings(RegistryPathName);
    IF_FAILED_ACTION_JUMP(
        ntStatus,
        DPF(D_ERROR, ("Registry Configuration error 0x%x", ntStatus)),
        Done);
    
    WDF_DRIVER_CONFIG_INIT(&config, WDF_NO_EVENT_CALLBACK);
    //
    // Set WdfDriverInitNoDispatchOverride flag to tell the framework
    // not to provide dispatch routines for the driver. In other words,
    // the framework must not intercept IRPs that the I/O manager has
    // directed to the driver. In this case, they will be handled by Audio
    // port driver.
    //
    config.DriverInitFlags |= WdfDriverInitNoDispatchOverride;
    config.DriverPoolTag    = MINADAPTER_POOLTAG;

    ntStatus = WdfDriverCreate(DriverObject,
                               RegistryPathName,
                               WDF_NO_OBJECT_ATTRIBUTES,
                               &config,
                               WDF_NO_HANDLE);
    IF_FAILED_ACTION_JUMP(
        ntStatus,
        DPF(D_ERROR, ("WdfDriverCreate failed, 0x%x", ntStatus)),
        Done);

    //
    // Tell the class driver to initialize the driver.
    //
    ntStatus =  PcInitializeAdapterDriver(DriverObject,
                                          RegistryPathName,
                                          (PDRIVER_ADD_DEVICE)AddDevice);
    IF_FAILED_ACTION_JUMP(
        ntStatus,
        DPF(D_ERROR, ("PcInitializeAdapterDriver failed, 0x%x", ntStatus)),
        Done);

    //
    // To intercept stop/remove/surprise-remove.
    //
    DriverObject->MajorFunction[IRP_MJ_PNP] = PnpHandler;

    // NJ IOCTL
#pragma warning(disable:4533)
    UNICODE_STRING DeviceName = RTL_CONSTANT_STRING(DEVICE_NAME);
    UNICODE_STRING LinkName = RTL_CONSTANT_STRING(SYMLINK_NAME);
    ntStatus = IoCreateSymbolicLink(&LinkName, &DeviceName);
    IF_FAILED_ACTION_JUMP(
        ntStatus,
        DPF(D_ERROR, ("IoCreateSymbolicLink failed, 0x%x", ntStatus)),
        Done);
    DriverObject->MajorFunction[IRP_MJ_CREATE] = IoctlCreateClose;
    DriverObject->MajorFunction[IRP_MJ_CLOSE] = IoctlCreateClose;
    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = NuJeeIOCtlDeviceControl;

    //
    // Hook the port class unload function
    //
    gPCDriverUnloadRoutine = DriverObject->DriverUnload;
    DriverObject->DriverUnload = DriverUnload;

    //
    // All done.
    //
    ntStatus = STATUS_SUCCESS;
    
Done:

    if (!NT_SUCCESS(ntStatus))
    {
        if (WdfGetDriver() != NULL)
        {
            WdfDriverMiniportUnload(WdfGetDriver());
        }

        ReleaseRegistryStringBuffer();
    }
    
    return ntStatus;
} // DriverEntry

#pragma code_seg()
// disable prefast warning 28152 because 
// DO_DEVICE_INITIALIZING is cleared in PcAddAdapterDevice
#pragma warning(disable:28152)
#pragma code_seg("PAGE")
//=============================================================================
NTSTATUS AddDevice
( 
    _In_  PDRIVER_OBJECT    DriverObject,
    _In_  PDEVICE_OBJECT    PhysicalDeviceObject 
)
/*++

Routine Description:

  The Plug & Play subsystem is handing us a brand new PDO, for which we
  (by means of INF registration) have been asked to provide a driver.

  We need to determine if we need to be in the driver stack for the device.
  Create a function device object to attach to the stack
  Initialize that device object
  Return status success.

  All audio adapter drivers can use this code without change.

Arguments:

  DriverObject - pointer to a driver object

  PhysicalDeviceObject -  pointer to a device object created by the
                            underlying bus driver.

Return Value:

  NT status code.

--*/
{
    PAGED_CODE();

    NTSTATUS        ntStatus;
    ULONG           maxObjects;

    DbgPrint("[Debug 2024]  AddDevice");
    DPF(D_TERSE, ("[AddDevice]"));

    maxObjects = g_MaxMiniports;

    // Tell the class driver to add the device.
    //
    ntStatus = PcAddAdapterDevice( 
            DriverObject,
            PhysicalDeviceObject,
            PCPFNSTARTDEVICE(StartDevice),
            maxObjects,
            0
        );

    DPF(D_TERSE, ("[Debug NJ] AddDevice no"));
    // NJ IOCTL
    UNICODE_STRING DeviceRefStr = RTL_CONSTANT_STRING(L"NuJeePhonosyncDrv");
    //RtlInitUnicodeString(&referenceIOCtlString, CSMT_IOCTL_NT_DEVICE_NAME);
    RtlInitUnicodeString(&referenceIOCtlString, DEVICE_NAME);
    NTSTATUS ioctlStatus = IoRegisterDeviceInterface(
        PhysicalDeviceObject,
        &MTCS_IOCTL_AUDIO,
        &DeviceRefStr,
        &referenceIOCtlString
    );

    if (NT_SUCCESS(ioctlStatus)) {
        DbgPrint(" Registered Device symbolic Name : %wZ \n", referenceIOCtlString);
        DbgPrint(" Registered Device Status %d Name Length %d \n", ioctlStatus, referenceIOCtlString.Length);
        ioctlStatus = IoSetDeviceInterfaceState(&referenceIOCtlString, TRUE);
        if (NT_SUCCESS(ioctlStatus)) {
            DbgPrint(" Ready for Communication Status %d \n", ioctlStatus);
        }
    }
    // NJ IOCTL

    DbgPrint("[Debug 2024] out AddDevice");
    return ntStatus;
} // AddDevice

#pragma code_seg()
NTSTATUS
_IRQL_requires_max_(DISPATCH_LEVEL)
PowerControlCallback
(
    _In_        LPCGUID PowerControlCode,
    _In_opt_    PVOID   InBuffer,
    _In_        SIZE_T  InBufferSize,
    _Out_writes_bytes_to_(OutBufferSize, *BytesReturned) PVOID OutBuffer,
    _In_        SIZE_T  OutBufferSize,
    _Out_opt_   PSIZE_T BytesReturned,
    _In_opt_    PVOID   Context
)
{
    UNREFERENCED_PARAMETER(PowerControlCode);
    UNREFERENCED_PARAMETER(InBuffer);
    UNREFERENCED_PARAMETER(InBufferSize);
    UNREFERENCED_PARAMETER(OutBuffer);
    UNREFERENCED_PARAMETER(OutBufferSize);
    UNREFERENCED_PARAMETER(BytesReturned);
    UNREFERENCED_PARAMETER(Context);
    
    return STATUS_NOT_IMPLEMENTED;
}

#pragma code_seg("PAGE")
NTSTATUS 
InstallEndpointRenderFilters(
    _In_ PDEVICE_OBJECT     _pDeviceObject, 
    _In_ PIRP               _pIrp, 
    _In_ PADAPTERCOMMON     _pAdapterCommon,
    _In_ PENDPOINT_MINIPAIR _pAeMiniports
    )
{
    NTSTATUS                    ntStatus                = STATUS_SUCCESS;
    PUNKNOWN                    unknownTopology         = NULL;
    PUNKNOWN                    unknownWave             = NULL;
    PPORTCLSETWHELPER           pPortClsEtwHelper       = NULL;
#ifdef _USE_IPortClsRuntimePower
    PPORTCLSRUNTIMEPOWER        pPortClsRuntimePower    = NULL;
#endif // _USE_IPortClsRuntimePower
    PPORTCLSStreamResourceManager pPortClsResMgr        = NULL;
    PPORTCLSStreamResourceManager2 pPortClsResMgr2      = NULL;

    PAGED_CODE();
    
    UNREFERENCED_PARAMETER(_pDeviceObject);

    ntStatus = _pAdapterCommon->InstallEndpointFilters(
        _pIrp,
        _pAeMiniports,
        NULL,
        &unknownTopology,
        &unknownWave,
        NULL, NULL);

    if (unknownWave) // IID_IPortClsEtwHelper and IID_IPortClsRuntimePower interfaces are only exposed on the WaveRT port.
    {
        ntStatus = unknownWave->QueryInterface (IID_IPortClsEtwHelper, (PVOID *)&pPortClsEtwHelper);
        if (NT_SUCCESS(ntStatus))
        {
            _pAdapterCommon->SetEtwHelper(pPortClsEtwHelper);
            ASSERT(pPortClsEtwHelper != NULL);
            pPortClsEtwHelper->Release();
        }

#ifdef _USE_IPortClsRuntimePower
        // Let's get the runtime power interface on PortCls.  
        ntStatus = unknownWave->QueryInterface(IID_IPortClsRuntimePower, (PVOID *)&pPortClsRuntimePower);
        if (NT_SUCCESS(ntStatus))
        {
            // This interface would typically be stashed away for later use.  Instead,
            // let's just send an empty control with GUID_NULL.
            NTSTATUS ntStatusTest =
                pPortClsRuntimePower->SendPowerControl
                (
                    _pDeviceObject,
                    &GUID_NULL,
                    NULL,
                    0,
                    NULL,
                    0,
                    NULL
                );

            if (NT_SUCCESS(ntStatusTest) || STATUS_NOT_IMPLEMENTED == ntStatusTest || STATUS_NOT_SUPPORTED == ntStatusTest)
            {
                ntStatus = pPortClsRuntimePower->RegisterPowerControlCallback(_pDeviceObject, &PowerControlCallback, NULL);
                if (NT_SUCCESS(ntStatus))
                {
                    ntStatus = pPortClsRuntimePower->UnregisterPowerControlCallback(_pDeviceObject);
                }
            }
            else
            {
                ntStatus = ntStatusTest;
            }

            pPortClsRuntimePower->Release();
        }
#endif // _USE_IPortClsRuntimePower

        //
        // Test: add and remove current thread as streaming audio resource.  
        // In a real driver you should only add interrupts and driver-owned threads 
        // (i.e., do NOT add the current thread as streaming resource).
        //
        // testing IPortClsStreamResourceManager:
        ntStatus = unknownWave->QueryInterface(IID_IPortClsStreamResourceManager, (PVOID *)&pPortClsResMgr);
        if (NT_SUCCESS(ntStatus))
        {
            PCSTREAMRESOURCE_DESCRIPTOR res;
            PCSTREAMRESOURCE hRes = NULL;
            PDEVICE_OBJECT pdo = NULL;

            PcGetPhysicalDeviceObject(_pDeviceObject, &pdo);
            PCSTREAMRESOURCE_DESCRIPTOR_INIT(&res);
            res.Pdo = pdo;
            res.Type = ePcStreamResourceThread;
            res.Resource.Thread = PsGetCurrentThread();
            
            NTSTATUS ntStatusTest = pPortClsResMgr->AddStreamResource(NULL, &res, &hRes);
            if (NT_SUCCESS(ntStatusTest))
            {
                pPortClsResMgr->RemoveStreamResource(hRes);
                hRes = NULL;
            }

            pPortClsResMgr->Release();
            pPortClsResMgr = NULL;
        }
        
        // testing IPortClsStreamResourceManager2:
        ntStatus = unknownWave->QueryInterface(IID_IPortClsStreamResourceManager2, (PVOID *)&pPortClsResMgr2);
        if (NT_SUCCESS(ntStatus))
        {
            PCSTREAMRESOURCE_DESCRIPTOR res;
            PCSTREAMRESOURCE hRes = NULL;
            PDEVICE_OBJECT pdo = NULL;

            PcGetPhysicalDeviceObject(_pDeviceObject, &pdo);
            PCSTREAMRESOURCE_DESCRIPTOR_INIT(&res);
            res.Pdo = pdo;
            res.Type = ePcStreamResourceThread;
            res.Resource.Thread = PsGetCurrentThread();
            
            NTSTATUS ntStatusTest = pPortClsResMgr2->AddStreamResource2(pdo, NULL, &res, &hRes);
            if (NT_SUCCESS(ntStatusTest))
            {
                pPortClsResMgr2->RemoveStreamResource(hRes);
                hRes = NULL;
            }

            pPortClsResMgr2->Release();
            pPortClsResMgr2 = NULL;
        }
    }

    SAFE_RELEASE(unknownTopology);
    SAFE_RELEASE(unknownWave);

    return ntStatus;
}

#pragma code_seg("PAGE")
NTSTATUS 
InstallAllRenderFilters(
    _In_ PDEVICE_OBJECT _pDeviceObject, 
    _In_ PIRP           _pIrp, 
    _In_ PADAPTERCOMMON _pAdapterCommon
    )
{
    NTSTATUS            ntStatus;
    PENDPOINT_MINIPAIR* ppAeMiniports   = g_RenderEndpoints;
    
    PAGED_CODE();

    for(ULONG i = 0; i < g_cRenderEndpoints; ++i, ++ppAeMiniports)
    {
        ntStatus = InstallEndpointRenderFilters(_pDeviceObject, _pIrp, _pAdapterCommon, *ppAeMiniports);
        IF_FAILED_JUMP(ntStatus, Exit);
    }
    
    ntStatus = STATUS_SUCCESS;

Exit:
    return ntStatus;
}

#pragma code_seg("PAGE")
NTSTATUS
InstallEndpointCaptureFilters(
    _In_ PDEVICE_OBJECT     _pDeviceObject,
    _In_ PIRP               _pIrp,
    _In_ PADAPTERCOMMON     _pAdapterCommon,
    _In_ PENDPOINT_MINIPAIR _pAeMiniports
)
{
    NTSTATUS    ntStatus = STATUS_SUCCESS;

    PAGED_CODE();

    UNREFERENCED_PARAMETER(_pDeviceObject);

    ntStatus = _pAdapterCommon->InstallEndpointFilters(
        _pIrp,
        _pAeMiniports,
        NULL,
        NULL,
        NULL,
        NULL, NULL);

    return ntStatus;
}

#pragma code_seg("PAGE")
NTSTATUS
InstallAllCaptureFilters(
    _In_ PDEVICE_OBJECT _pDeviceObject,
    _In_ PIRP           _pIrp,
    _In_ PADAPTERCOMMON _pAdapterCommon
)
{
    NTSTATUS            ntStatus;
    PENDPOINT_MINIPAIR* ppAeMiniports = g_CaptureEndpoints;

    PAGED_CODE();

    for (ULONG i = 0; i < g_cCaptureEndpoints; ++i, ++ppAeMiniports)
    {
        ntStatus = InstallEndpointCaptureFilters(_pDeviceObject, _pIrp, _pAdapterCommon, *ppAeMiniports);
        IF_FAILED_JUMP(ntStatus, Exit);
    }

    ntStatus = STATUS_SUCCESS;

Exit:
    return ntStatus;
}

//=============================================================================
#pragma code_seg("PAGE")
NTSTATUS
StartDevice
( 
    _In_  PDEVICE_OBJECT          DeviceObject,     
    _In_  PIRP                    Irp,              
    _In_  PRESOURCELIST           ResourceList      
)  
{
/*++

Routine Description:

  This function is called by the operating system when the device is 
  started.
  It is responsible for starting the miniports.  This code is specific to    
  the adapter because it calls out miniports for functions that are specific 
  to the adapter.                                                            

Arguments:

  DeviceObject - pointer to the driver object

  Irp - pointer to the irp 

  ResourceList - pointer to the resource list assigned by PnP manager

Return Value:

  NT status code.

--*/
    UNREFERENCED_PARAMETER(ResourceList);

    PAGED_CODE();

    ASSERT(DeviceObject);
    ASSERT(Irp);
    ASSERT(ResourceList);

    NTSTATUS                    ntStatus        = STATUS_SUCCESS;

    PADAPTERCOMMON              pAdapterCommon  = NULL;
    PUNKNOWN                    pUnknownCommon  = NULL;
    PortClassDeviceContext*     pExtension      = static_cast<PortClassDeviceContext*>(DeviceObject->DeviceExtension);

    DPF_ENTER(("[StartDevice]"));

    //
    // create a new adapter common object
    //
    ntStatus = NewAdapterCommon( 
                                &pUnknownCommon,
                                IID_IAdapterCommon,
                                NULL,
                                POOL_FLAG_NON_PAGED 
                                );
    IF_FAILED_JUMP(ntStatus, Exit);

    ntStatus = pUnknownCommon->QueryInterface( IID_IAdapterCommon,(PVOID *) &pAdapterCommon);
    IF_FAILED_JUMP(ntStatus, Exit);

    ntStatus = pAdapterCommon->Init(DeviceObject);
    IF_FAILED_JUMP(ntStatus, Exit);

    //
    // register with PortCls for power-management services
    ntStatus = PcRegisterAdapterPowerManagement( PUNKNOWN(pAdapterCommon), DeviceObject);
    IF_FAILED_JUMP(ntStatus, Exit);

    //
    // Install wave+topology filters for render devices
    //
    ntStatus = InstallAllRenderFilters(DeviceObject, Irp, pAdapterCommon);
    IF_FAILED_JUMP(ntStatus, Exit);

    //
    // Install wave+topology filters for capture devices
    //
    ntStatus = InstallAllCaptureFilters(DeviceObject, Irp, pAdapterCommon);
    IF_FAILED_JUMP(ntStatus, Exit);

Exit:

    //
    // Stash the adapter common object in the device extension so
    // we can access it for cleanup on stop/removal.
    //
    if (pAdapterCommon)
    {
        ASSERT(pExtension != NULL);
        pExtension->m_pCommon = pAdapterCommon;
    }

    //
    // Release the adapter IUnknown interface.
    //
    SAFE_RELEASE(pUnknownCommon);
    
    return ntStatus;
} // StartDevice

//=============================================================================
#pragma code_seg("PAGE")
NTSTATUS 
PnpHandler
(
    _In_ DEVICE_OBJECT *_DeviceObject, 
    _Inout_ IRP *_Irp
)
/*++

Routine Description:

  Handles PnP IRPs                                                           

Arguments:

  _DeviceObject - Functional Device object pointer.

  _Irp - The Irp being passed

Return Value:

  NT status code.

--*/
{
    DbgPrint("[Debug 2024]  PnpHandler");
    NTSTATUS                ntStatus = STATUS_UNSUCCESSFUL;
    IO_STACK_LOCATION      *stack;
    PortClassDeviceContext *ext;

    // Documented https://msdn.microsoft.com/en-us/library/windows/hardware/ff544039(v=vs.85).aspx
    // This method will be called in IRQL PASSIVE_LEVEL
#pragma warning(suppress: 28118)
    PAGED_CODE(); 

    ASSERT(_DeviceObject);
    ASSERT(_Irp);

    //
    // Check for the REMOVE_DEVICE irp.  If we're being unloaded, 
    // uninstantiate our devices and release the adapter common
    // object.
    //
    stack = IoGetCurrentIrpStackLocation(_Irp);

    switch (stack->MinorFunction)
    {
    case IRP_MN_REMOVE_DEVICE:
    case IRP_MN_SURPRISE_REMOVAL:
    case IRP_MN_STOP_DEVICE:
        ext = static_cast<PortClassDeviceContext*>(_DeviceObject->DeviceExtension);

        if (ext->m_pCommon != NULL)
        {
            ext->m_pCommon->Cleanup();
            
            ext->m_pCommon->Release();
            ext->m_pCommon = NULL;
        }
        break;

    default:
        break;
    }
    
    ntStatus = PcDispatchIrp(_DeviceObject, _Irp);

    DbgPrint("[Debug 2024] out PnpHandler");
    return ntStatus;
}

#pragma code_seg()

