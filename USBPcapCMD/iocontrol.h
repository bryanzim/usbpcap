/*
 * Copyright (c) 2013 Tomasz Moń <desowin@gmail.com>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#ifndef USBPCAP_CMD_IOCONTROL_H
#define USBPCAP_CMD_IOCONTROL_H

#include <windows.h>

#include "USBPcap.h"

#include <basetsd.h>
#include <wtypes.h>

BOOLEAN USBPcapIsDeviceFiltered(const PUSBPCAP_ADDRESS_FILTER filter, const int address);
BOOLEAN USBPcapSetDeviceFiltered(const PUSBPCAP_ADDRESS_FILTER filter, const int address);
BOOLEAN USBPcapInitAddressFilter(const PUSBPCAP_ADDRESS_FILTER filter, PCHAR list, BOOLEAN filterAll);

#endif /* USBPCAP_CMD_IOCONTROL_H */
