/*
 * Copyright (c) 2013-2019 Tomasz Moń <desowin@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef USBPCAP_TABLES_H
#define USBPCAP_TABLES_H

#include "USBPcapMain.h"

typedef struct _USBPCAP_ENDPOINT_INFO
{
    /* handle is used as a key */
    USBD_PIPE_HANDLE handle;
    USBD_PIPE_TYPE type;
    UCHAR endpointAddress;
    USHORT deviceAddress;
} USBPCAP_ENDPOINT_INFO, *PUSBPCAP_ENDPOINT_INFO;

VOID USBPcapRemoveEndpointInfo(IN const PRTL_GENERIC_TABLE table,
                               IN USBD_PIPE_HANDLE handle);
VOID USBPcapAddEndpointInfo(IN const PRTL_GENERIC_TABLE table,
                            IN const PUSBD_PIPE_INFORMATION pipeInfo,
                            IN USHORT deviceAddress);

PUSBPCAP_ENDPOINT_INFO USBPcapGetEndpointInfo(IN const PRTL_GENERIC_TABLE table,
                                              IN USBD_PIPE_HANDLE handle);

VOID USBPcapFreeEndpointTable(IN const PRTL_GENERIC_TABLE table);
PRTL_GENERIC_TABLE USBPcapInitializeEndpointTable(IN const PVOID context);


BOOLEAN USBPcapRetrieveEndpointInfo(IN PUSBPCAP_DEVICE_DATA pDeviceData,
                                    IN const USBD_PIPE_HANDLE handle,
                                    const PUSBPCAP_ENDPOINT_INFO pInfo);

typedef struct _USBPCAP_URB_IRP_INFO
{
    /* IRP pointer is used as a key */
    PIRP irp;
    /* Data collected when the URB was travelling from FDO to PDO */
    LARGE_INTEGER timestamp;
    USBD_STATUS status;
    USHORT function;
    UCHAR info;    /* I/O Request info */
    USHORT bus;    /* bus (RootHub) number */
    USHORT device; /* device address */
} USBPCAP_URB_IRP_INFO, *PUSBPCAP_URB_IRP_INFO;

VOID USBPcapRemoveURBIRPInfo(IN const PRTL_GENERIC_TABLE table,
                             IN PIRP irp);
VOID USBPcapAddURBIRPInfo(IN const PRTL_GENERIC_TABLE table,
                          IN const PUSBPCAP_URB_IRP_INFO irpinfo);

VOID USBPcapFreeURBIRPInfoTable(IN const PRTL_GENERIC_TABLE table);
PRTL_GENERIC_TABLE USBPcapInitializeURBIRPInfoTable(IN const PVOID context);

BOOLEAN USBPcapObtainURBIRPInfo(IN PUSBPCAP_DEVICE_DATA pDeviceData,
                                IN const PIRP irp,
                                const PUSBPCAP_URB_IRP_INFO pInfo);

#endif /* USBPCAP_TABLES_H */
