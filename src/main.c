// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include </home/pi/azure-iot-sdks/c/serializer/samples/remote_monitoring/remote_monitoring.h>
#include <stdlib.h>
#include <stdio.h>

int main(int argc, char *argv[])
{
	char* deviceId = argv[1];
	char* deviceKey = argv[2];
	char* hubName = argv[3];
	char* hubSuffix = argv[4];
	char* Manufacturer = argv[5];
	char* Platform = argv[6];
	char* Processor = argv[7];
	char* ModelNumber = argv[8];
	char* SerialNumber = argv[9];
	char* FirmwareVersion = argv[10];
	char* InstalledRAM = argv[11];
	double Latitude = atof(argv[12]);
	double Longitude = atof(argv[13]);

	//static const char* deviceId = "Davidce-Win10-Lenovo";
	//static const char* deviceKey = "zodJ3vNj/xna2EOcqcBLZg==";
	//static const char* hubName = "rpi2sensors";
	//static const char* hubSuffix = "azure-devices.net";

    remote_monitoring_run(deviceId, deviceKey, hubName, hubSuffix, Manufacturer, Platform, Processor, ModelNumber, SerialNumber, FirmwareVersion, InstalledRAM, Latitude, Longitude);
		
    return 0;
}
