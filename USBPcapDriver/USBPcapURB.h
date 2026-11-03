/*
 * Copyright (c) 2013-2019 Tomasz Moń <desowin@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef USBPCAP_URB_H
#define USBPCAP_URB_H

#include "USBPcapMain.h"

VOID USBPcapAnalyzeURB(const PIRP pIrp, const PURB pUrb, const BOOLEAN post,
                       PUSBPCAP_DEVICE_DATA pDeviceData);

#endif /* USBPCAP_URB_H */
