// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef WINCE
#include "iothubtransportamqp.h"
#else
#include "iothubtransporthttp.h"
#endif
#include "schemalib.h"
#include "iothub_client.h"
#include "serializer.h"
#include "schemaserializer.h"
#include "threadapi.h"

#ifdef MBED_BUILD_TIMESTAMP
#include "certs.h"
#endif // MBED_BUILD_TIMESTAMP

#include <wiringPi.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#define MAX_TIME 85
#define DHT11PIN 7

int dht11_val[5]={0,0,0,0,0};

// Define the Model
BEGIN_NAMESPACE(Contoso);

DECLARE_STRUCT(SystemProperties,
    ascii_char_ptr, DeviceID,
    _Bool, Enabled
);

DECLARE_STRUCT(DeviceProperties,
ascii_char_ptr, DeviceID, 
_Bool, HubEnabledState,
ascii_char_ptr, Manufacturer,
ascii_char_ptr, ModelNumber,
ascii_char_ptr, SerialNumber,
ascii_char_ptr, FirmwareVersion,
ascii_char_ptr, Platform,
ascii_char_ptr, Processor,
ascii_char_ptr, InstalledRAM,
double, Latitude,
double, Longitude
);

DECLARE_MODEL(Thermostat,

    /* Event data (temperature, external temperature and humidity) */
	WITH_DATA(double, Temperature),
	WITH_DATA(double, ExternalTemperature),
	WITH_DATA(double, Humidity),
	WITH_DATA(ascii_char_ptr, DeviceId),

	/* Device Info - This is command metadata + some extra fields */
	WITH_DATA(ascii_char_ptr, ObjectType),
	WITH_DATA(_Bool, IsSimulatedDevice),
	WITH_DATA(ascii_char_ptr, Version),
	WITH_DATA(DeviceProperties, DeviceProperties),
    WITH_DATA(ascii_char_ptr_no_quotes, Commands),

    /* Commands implemented by the device */
    WITH_ACTION(SetTemperature, double, temperature),
	WITH_ACTION(SetHumidity, double, humidity),
	WITH_ACTION(BlinkRedLed, double, seconds),
	WITH_ACTION(BlinkGreenLed, double, seconds)
);

END_NAMESPACE(Contoso);

EXECUTE_COMMAND_RESULT SetTemperature(Thermostat* thermostat, double temperature)
{
    (void)printf("Received temperature %.02fs\r\n", temperature);
	thermostat->Temperature = temperature;
    return EXECUTE_COMMAND_SUCCESS;
}

EXECUTE_COMMAND_RESULT SetHumidity(Thermostat* thermostat, double humidity)
{
	(void)printf("Received humidity %.02fs\r\n", humidity);
	thermostat->Humidity = humidity;
	return EXECUTE_COMMAND_SUCCESS;
}

EXECUTE_COMMAND_RESULT BlinkRedLed(Thermostat* thermostat, double seconds)
{
	(void)printf("Received order to blink red led for %.02fs seconds\r\n", seconds);
	pinMode (0, OUTPUT) ;
	digitalWrite (0, HIGH);
    delay (seconds*1000);
    digitalWrite (0, LOW);
	return EXECUTE_COMMAND_SUCCESS;
}

EXECUTE_COMMAND_RESULT BlinkGreenLed(Thermostat* thermostat, double seconds)
{
	(void)printf("Received order to blink green led for %.02fs seconds\r\n", seconds);
	pinMode (2, OUTPUT) ;
	digitalWrite (2, HIGH);
    delay (seconds*1000);
    digitalWrite (2, LOW);
	return EXECUTE_COMMAND_SUCCESS;
}

static void sendMessage(IOTHUB_CLIENT_HANDLE iotHubClientHandle, const unsigned char* buffer, size_t size)
{
    IOTHUB_MESSAGE_HANDLE messageHandle = IoTHubMessage_CreateFromByteArray(buffer, size);
    if (messageHandle == NULL)
    {
        printf("unable to create a new IoTHubMessage\r\n");
    }
    else
    {
        if (IoTHubClient_SendEventAsync(iotHubClientHandle, messageHandle, NULL, NULL) != IOTHUB_CLIENT_OK)
        {
            printf("failed to hand over the message to IoTHubClient");
        }
        else
        {
            printf("IoTHubClient accepted the message for delivery\r\n");
        }

        IoTHubMessage_Destroy(messageHandle);
    }
    free((void*)buffer);
}

/*this function "links" IoTHub to the serialization library*/
static IOTHUBMESSAGE_DISPOSITION_RESULT IoTHubMessage(IOTHUB_MESSAGE_HANDLE message, void* userContextCallback)
{
    IOTHUBMESSAGE_DISPOSITION_RESULT result;
    const unsigned char* buffer;
    size_t size;
    if (IoTHubMessage_GetByteArray(message, &buffer, &size) != IOTHUB_MESSAGE_OK)
    {
        printf("unable to IoTHubMessage_GetByteArray\r\n");
        result = EXECUTE_COMMAND_ERROR;
    }
    else
    {
        /*buffer is not zero terminated*/
        char* temp = malloc(size + 1);
        if (temp == NULL)
        {
            printf("failed to malloc\r\n");
            result = EXECUTE_COMMAND_ERROR;
        }
        else
        {
			EXECUTE_COMMAND_RESULT executeCommandResult;

            memcpy(temp, buffer, size);
            temp[size] = '\0';
            executeCommandResult = EXECUTE_COMMAND(userContextCallback, temp);
            result =
                (executeCommandResult == EXECUTE_COMMAND_ERROR) ? IOTHUBMESSAGE_ABANDONED :
                (executeCommandResult == EXECUTE_COMMAND_SUCCESS) ? IOTHUBMESSAGE_ACCEPTED :
                IOTHUBMESSAGE_REJECTED;
            free(temp);
        }
    }
    return result;
}

void dht11_read_val()
{
  uint8_t lststate=HIGH;
  uint8_t counter=0;
  uint8_t j=0,i;
  for(i=0;i<5;i++)
     dht11_val[i]=0;
  pinMode(DHT11PIN,OUTPUT);
  digitalWrite(DHT11PIN,LOW);
  delay(18);
  digitalWrite(DHT11PIN,HIGH);
  delayMicroseconds(40);
  pinMode(DHT11PIN,INPUT);
  for(i=0;i<MAX_TIME;i++)
  {
    counter=0;
    while(digitalRead(DHT11PIN)==lststate){
      counter++;
      delayMicroseconds(1);
      if(counter==255)
        break;
    }
    lststate=digitalRead(DHT11PIN);
    if(counter==255)
       break;
    // top 3 transistions are ignored
    if((i>=4)&&(i%2==0)){
      dht11_val[j/8]<<=1;
      if(counter>16)
        dht11_val[j/8]|=1;
      j++;
    }
  }
  // verify cheksum and print the verified data
   while((j>=40)&&(dht11_val[4]==((dht11_val[0]+dht11_val[1]+dht11_val[2]+dht11_val[3])& 0xFF)))
   {
   return;
	}
}

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
	double Longitude)
{
	
    if (serializer_init(NULL) != SERIALIZER_OK)
    {
        printf("Failed on serializer_init\r\n");
    }
    else
    {
		IOTHUB_CLIENT_CONFIG config;
		IOTHUB_CLIENT_HANDLE iotHubClientHandle;

		config.deviceId = deviceId;
		config.deviceKey = deviceKey;
		config.iotHubName = hubName;
		config.iotHubSuffix = hubSuffix;
#ifndef WINCE
		config.protocol = AMQP_Protocol;
#else
		config.protocol = HTTP_Protocol;
#endif
		iotHubClientHandle = IoTHubClient_Create(&config);
        if (iotHubClientHandle == NULL)
        {
            (void)printf("Failed on IoTHubClient_CreateFromConnectionString\r\n");
        }
        else
        {
#ifdef MBED_BUILD_TIMESTAMP
            // For mbed add the certificate information
            if (IoTHubClient_SetOption(iotHubClientHandle, "TrustedCerts", certificates) != IOTHUB_CLIENT_OK)
            {
                printf("failure to set option \"TrustedCerts\"\r\n");
            }
#endif // MBED_BUILD_TIMESTAMP

            Thermostat* thermostat = CREATE_MODEL_INSTANCE(Contoso, Thermostat);
            if (thermostat == NULL)
            {
                (void)printf("Failed on CREATE_MODEL_INSTANCE\r\n");
            }
            else
            {
                STRING_HANDLE commandsMetadata;

                if (IoTHubClient_SetMessageCallback(iotHubClientHandle, IoTHubMessage, thermostat) != IOTHUB_CLIENT_OK)
                {
                    printf("unable to IoTHubClient_SetMessageCallback\r\n");
                }
                else
                {

                    /* send the device info upon startup so that the cloud app knows
                    what commands are available and the fact that the device is up */
                    thermostat->ObjectType = "DeviceInfo";
					thermostat->IsSimulatedDevice = false;
					thermostat->Version = "1.0";
					thermostat->DeviceProperties.HubEnabledState = true;
                    thermostat->DeviceProperties.DeviceID = (char*)deviceId;

					thermostat->DeviceProperties.Manufacturer = Manufacturer;
					thermostat->DeviceProperties.Platform = Platform;
					thermostat->DeviceProperties.Processor = Processor;
					thermostat->DeviceProperties.ModelNumber = ModelNumber;
					thermostat->DeviceProperties.SerialNumber = SerialNumber;
					thermostat->DeviceProperties.FirmwareVersion = FirmwareVersion;
					thermostat->DeviceProperties.InstalledRAM = InstalledRAM;
					thermostat->DeviceProperties.Latitude = Latitude;
					thermostat->DeviceProperties.Longitude = Longitude;

                    commandsMetadata = STRING_new();
                    if (commandsMetadata == NULL)
                    {
                        (void)printf("Failed on creating string for commands metadata\r\n");
                    }
                    else
                    {
                        /* Serialize the commands metadata as a JSON string before sending */
                        if (SchemaSerializer_SerializeCommandMetadata(GET_MODEL_HANDLE(Contoso, Thermostat), commandsMetadata) != SCHEMA_SERIALIZER_OK)
                        {
                            (void)printf("Failed serializing commands metadata\r\n");
                        }
                        else
                        {
                            unsigned char* buffer;
                            size_t bufferSize;
                            thermostat->Commands = (char*)STRING_c_str(commandsMetadata);

                            /* Here is the actual send of the Device Info */
                            if (SERIALIZE(&buffer, &bufferSize, thermostat->ObjectType, thermostat->Version, thermostat->IsSimulatedDevice, thermostat->DeviceProperties, thermostat->Commands) != IOT_AGENT_OK)
                            {
                                (void)printf("Failed serializing\r\n");
                            }
                            else
                            {
                                sendMessage(iotHubClientHandle, buffer, bufferSize);
                            }

                        }

                        STRING_delete(commandsMetadata);
                    }
					
					srand((unsigned)time(NULL));
					if(wiringPiSetup()==-1)
						exit(1);
                    while (1)
                    {
                        unsigned char*buffer;
                        size_t bufferSize;
						//(double)rand()/(double)RAND_MAX * (max - min) + min;
						//thermostat->Temperature = (double)rand() / (double)RAND_MAX * 20 +20;
						//thermostat->ExternalTemperature = (double)rand() / (double)RAND_MAX * 22 + 20;
						//thermostat->Humidity = (double)rand() / (double)RAND_MAX * 20 +60;
						dht11_read_val();
						thermostat->DeviceId = (char*)deviceId;
						thermostat->Humidity = (double)dht11_val[0]+ (double)dht11_val[1]/100;
						thermostat->Temperature = (double)dht11_val[2] + (double)dht11_val[3]/100;
						thermostat->ExternalTemperature = thermostat->Temperature;
						(void)printf("Sending sensor value Temperature = %02f, Humidity = %02f\r\n", thermostat->Temperature, thermostat->Humidity);

                        if (SERIALIZE(&buffer, &bufferSize, thermostat->DeviceId, thermostat->Temperature, thermostat->Humidity, thermostat->ExternalTemperature) != IOT_AGENT_OK)
                        {
                            (void)printf("Failed sending sensor value\r\n");
                        }
                        else
                        {
                            sendMessage(iotHubClientHandle, buffer, bufferSize);
                        }

                        ThreadAPI_Sleep(3000);
                    }
                }

                DESTROY_MODEL_INSTANCE(thermostat);
            }
            IoTHubClient_Destroy(iotHubClientHandle);
        }
        serializer_deinit();
    }
}
