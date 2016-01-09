// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef REMOTE_MONITORING_H
#define REMOTE_MONITORING_H

#ifdef __cplusplus
extern "C" {
#endif

void remote_monitoring_run(unsigned char* deviceId, 
	unsigned char* deviceKey, 
	unsigned char* hubName, 
	unsigned char* hubSuffix,
	unsigned char* Manufacturer,
	unsigned char* Platform,
	unsigned char* Processor,
	unsigned char* ModelNumber,
	unsigned char* SerialNumber,
	unsigned char* FirmwareVersion,
	unsigned char* InstalledRAM,
	double Latitude,
	double Longitude);

void dht11_read_val();

#ifdef __cplusplus
}
#endif

#endif /* REMOTE_MONITORING_H */
