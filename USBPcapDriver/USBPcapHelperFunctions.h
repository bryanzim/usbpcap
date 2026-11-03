/*
 * Copyright (c) 2013-2019 Tomasz Moń <desowin@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef USBPCAP_HELPER_FUNCTIONS_H
#define USBPCAP_HELPER_FUNCTIONS_H

#include "USBPcapMain.h"

NTSTATUS USBPcapGetTargetDevicePdo(IN const PDEVICE_OBJECT DeviceObject,
                                   OUT PDEVICE_OBJECT* pdo);

NTSTATUS USBPcapGetNumberOfPorts(PDEVICE_OBJECT parent,
                                 PULONG numberOfPorts);

#if DBG
NTSTATUS USBPcapPrintUSBPChildrenInformation(const PDEVICE_OBJECT hub);
#else
#define USBPcapPrintUSBPChildrenInformation(hub) \
    {                                            \
    }
#endif

NTSTATUS USBPcapGetDeviceUSBInfo(const PDEVICE_EXTENSION pDevExt);

BOOLEAN USBPcapIsDeviceRootHub(const PDEVICE_OBJECT device);

PWSTR USBPcapGetHubInterfaces(const PDEVICE_OBJECT hub);


BOOLEAN USBPcapIsDeviceFiltered(const PUSBPCAP_ADDRESS_FILTER filter, const int address);
BOOLEAN USBPcapSetDeviceFiltered(const PUSBPCAP_ADDRESS_FILTER filter, const int address);

LARGE_INTEGER USBPcapGetCurrentTimestamp(VOID);

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, USBPcapGetTargetDevicePdo)
#pragma alloc_text(PAGE, USBPcapGetNumberOfPorts)
#if DBG
#pragma alloc_text(PAGE, USBPcapPrintUSBPChildrenInformation)
#endif
#pragma alloc_text(PAGE, USBPcapGetDeviceUSBInfo)
#pragma alloc_text(PAGE, USBPcapIsDeviceRootHub)
#pragma alloc_text(PAGE, USBPcapGetHubInterfaces)
#endif

#endif /* USBPCAP_HELPER_FUNCTIONS_H */
