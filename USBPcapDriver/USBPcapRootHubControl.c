/*
 * Copyright (c) 2013-2019 Tomasz Moń <desowin@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0
 */

#include "USBPcapMain.h"
#include <Wdmsec.h>
#include "USBPcapRootHubControl.h"
#include "Ntstrsafe.h"

extern volatile LONG g_controlId;

#define NTNAME_PREFIX L"\\Device\\USBPcap"
#define SYMBOLIC_PREFIX L"\\DosDevices\\USBPcap"
#define MAX_CONTROL_ID L"65535"

#define MAX_NTNAME_LEN (sizeof(NTNAME_PREFIX) + sizeof(MAX_CONTROL_ID))
#define MAX_SYMBOLIC_LEN (sizeof(SYMBOLIC_PREFIX) + sizeof(MAX_CONTROL_ID))

DECLARE_CONST_UNICODE_STRING(
    SDDL_DEVOBJ_SYS_ALL_ADM_ALL_EVERYONE_ANY,
    L"D:P(A;;GA;;;SY)(A;;GA;;;BA)(A;;GX;;;WD)(A;;GX;;;RC)");

__drv_requiresIRQL(PASSIVE_LEVEL)
    NTSTATUS USBPcapCreateRootHubControlDevice(IN PDEVICE_EXTENSION hubExt,
                                               OUT PDEVICE_OBJECT* control,
                                               OUT USHORT* busId)
{
    UNICODE_STRING ntDeviceName;
    UNICODE_STRING symbolicLinkName;
    PDEVICE_OBJECT controlDevice = NULL;
    PDEVICE_EXTENSION controlExt = NULL;
    NTSTATUS status;
    USHORT id;
    LONG newId;
    WCHAR ntNameBuffer[MAX_NTNAME_LEN / sizeof(WCHAR)];
    WCHAR symbolicNameBuffer[MAX_SYMBOLIC_LEN / sizeof(WCHAR)];

    ASSERT(hubExt->deviceMagic == USBPCAP_MAGIC_ROOTHUB);

    /* Acquire the control device ID. The pcap bus field is 16 bits. */
    newId = InterlockedIncrement(&g_controlId);
    if ((newId <= 0) || (newId > 0xFFFF))
    {
        return STATUS_INTEGER_OVERFLOW;
    }
    id = (USHORT)newId;

    ntDeviceName.Length = 0;
    ntDeviceName.MaximumLength = sizeof(ntNameBuffer);
    ntDeviceName.Buffer = ntNameBuffer;

    symbolicLinkName.Length = 0;
    symbolicLinkName.MaximumLength = sizeof(symbolicNameBuffer);
    symbolicLinkName.Buffer = symbolicNameBuffer;

    status = RtlUnicodeStringPrintf(&ntDeviceName,
                                    NTNAME_PREFIX L"%hu", id);
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    status = RtlUnicodeStringPrintf(&symbolicLinkName,
                                    SYMBOLIC_PREFIX L"%hu", id);
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    KdPrint(("Creating device %wZ (%wZ)\n",
             &ntDeviceName, &symbolicLinkName));

    status = IoCreateDeviceSecure(hubExt->pDrvObj,
                                  sizeof(DEVICE_EXTENSION),
                                  &ntDeviceName,
                                  FILE_DEVICE_UNKNOWN,
                                  FILE_DEVICE_SECURE_OPEN,
                                  FALSE, /* Non-Exclusive device */
                                  &SDDL_DEVOBJ_SYS_ALL_ADM_ALL_EVERYONE_ANY,
                                  NULL,
                                  &controlDevice);

    if (NT_SUCCESS(status))
    {
        controlDevice->Flags |= DO_DIRECT_IO;

        status = IoCreateSymbolicLink(&symbolicLinkName, &ntDeviceName);

        if (!NT_SUCCESS(status))
        {
            IoDeleteDevice(controlDevice);
            KdPrint(("IoCreateSymbolicLink failed %x\n", status));
            return status;
        }

        controlExt = (PDEVICE_EXTENSION)controlDevice->DeviceExtension;
        controlExt->deviceMagic = USBPCAP_MAGIC_CONTROL;
        controlExt->pThisDevObj = controlDevice;
        controlExt->pNextDevObj = NULL;
        controlExt->pDrvObj = hubExt->pDrvObj;

        IoInitializeRemoveLock(&controlExt->removeLock, 0, 0, 0);
        controlExt->parentRemoveLock = &hubExt->removeLock;

        /* Initialize USBPcap control context */
        controlExt->context.control.id = id;
        controlExt->context.control.removing = FALSE;
        controlExt->context.control.pRootHubObject = hubExt->pThisDevObj;
        controlExt->context.control.pCaptureObject = NULL;


        KeInitializeSpinLock(&controlExt->context.control.csqSpinLock);
        InitializeListHead(&controlExt->context.control.lePendIrp);
        status = IoCsqInitialize(&controlExt->context.control.ioCsq,
                                 DkCsqInsertIrp, DkCsqRemoveIrp,
                                 DkCsqPeekNextIrp, DkCsqAcquireLock,
                                 DkCsqReleaseLock, DkCsqCompleteCanceledIrp);
        if (!NT_SUCCESS(status))
        {
            DkDbgVal("Error initialize Cancel-safe queue!", status);
            goto End;
        }

        controlDevice->Flags &= ~DO_DEVICE_INITIALIZING;
    }
    else
    {
        KdPrint(("IoCreateDevice failed %x\n", status));
    }

End:
    if ((!NT_SUCCESS(status)) || (controlExt == NULL))
    {
        if (controlDevice != NULL)
        {
            IoDeleteSymbolicLink(&symbolicLinkName);
            IoDeleteDevice(controlDevice);
        }
    }
    else
    {
        IoAcquireRemoveLock(controlExt->parentRemoveLock, NULL);
        *control = controlDevice;
        *busId = id;
    }

    return status;
}


VOID USBPcapDeleteRootHubControlDevice(IN PDEVICE_OBJECT controlDevice)
{
    UNICODE_STRING symbolicLinkName;
    WCHAR symbolicNameBuffer[MAX_SYMBOLIC_LEN / sizeof(WCHAR)];
    USHORT id;
    PDEVICE_EXTENSION pDevExt;
    PDEVICE_EXTENSION rootExt;
    PUSBPCAP_ROOTHUB_DATA pRootData;
    LIST_ENTRY drained;
    PIRP pending;
    KIRQL irql;
    NTSTATUS status;

    pDevExt = ((PDEVICE_EXTENSION)controlDevice->DeviceExtension);

    ASSERT(pDevExt->deviceMagic == USBPCAP_MAGIC_CONTROL);

    id = pDevExt->context.control.id;

    symbolicLinkName.Length = 0;
    symbolicLinkName.MaximumLength = sizeof(symbolicNameBuffer);
    symbolicLinkName.Buffer = symbolicNameBuffer;

    status = RtlUnicodeStringPrintf(&symbolicLinkName,
                                    SYMBOLIC_PREFIX L"%hu", id);

    /* Stop new reads and finish the ones already queued before waiting
     * for the remove lock. Queued reads hold that lock until completed.
     */
    InitializeListHead(&drained);
    pRootData = NULL;
    if (pDevExt->context.control.pRootHubObject != NULL)
    {
        rootExt = (PDEVICE_EXTENSION)pDevExt->context.control.pRootHubObject->DeviceExtension;
        if (rootExt->context.usb.pDeviceData != NULL)
        {
            pRootData = rootExt->context.usb.pDeviceData->pRootData;
        }
    }

    if (pRootData != NULL)
    {
        KeAcquireSpinLock(&pRootData->bufferLock, &irql);
        pDevExt->context.control.removing = TRUE;
        for (;;)
        {
            pending = IoCsqRemoveNextIrp(&pDevExt->context.control.ioCsq, NULL);
            if (pending == NULL)
            {
                break;
            }
            InsertTailList(&drained, &pending->Tail.Overlay.ListEntry);
        }
        KeReleaseSpinLock(&pRootData->bufferLock, irql);
    }
    else
    {
        pDevExt->context.control.removing = TRUE;
    }

    while (!IsListEmpty(&drained))
    {
        pending = CONTAINING_RECORD(RemoveHeadList(&drained),
                                    IRP,
                                    Tail.Overlay.ListEntry);
        pending->IoStatus.Status = STATUS_DEVICE_REMOVED;
        pending->IoStatus.Information = 0;
        IoCompleteRequest(pending, IO_NO_INCREMENT);
        IoReleaseRemoveLock(&pDevExt->removeLock, pending);
    }

    IoAcquireRemoveLock(&pDevExt->removeLock, NULL);
    IoReleaseRemoveLockAndWait(&pDevExt->removeLock, NULL);

    IoReleaseRemoveLock(pDevExt->parentRemoveLock, NULL);

    ASSERT(NT_SUCCESS(status));
    if (NT_SUCCESS(status))
    {
        IoDeleteSymbolicLink(&symbolicLinkName);
        IoDeleteDevice(controlDevice);
    }
    else
    {
        /* Very bad */
        KdPrint(("Failed to init symbolic link name\n"));

        pDevExt->context.control.pRootHubObject = NULL;
    }
}
