#ifndef MessageDecoder_h
#define MessageDecoder_h

#include "HexUtils.h"

#define DS18B20_ID_LEN 8
#define TEMPERATURE_SENSOR_ID_LEN 1


typedef struct temperature_sensor_info{
  char type;
  char id[DS18B20_ID_LEN * 2 + 1];
  float value;
} temperature_sensor_info;

typedef struct temperature_info{
  char num_sensors;
  temperature_sensor_info sensors[];	
} temperature_info;

typedef struct humidity_info{
  float value;	
} humidity_info;

float decodeSensorTemperatureValue(char* data, float div){
	signed short v = (signed short)(((data[1] & 0xFF) << 8) | (data[0] & 0xFF));
	if (v == 0xFFFF) {
		return v;
	}
	return v / div;
}

void decodeDS18B20SensorTemperature(char* data, temperature_sensor_info* info){
	charArrayToHexString(info->id, data, DS18B20_ID_LEN);
	info->value = decodeSensorTemperatureValue(&data[DS18B20_ID_LEN], 10.0f);
}

void decodeSensorTemperature(char* data, temperature_sensor_info* info, float div){
	sprintf(info->id, "%d", data[0]);
	info->value = decodeSensorTemperatureValue(&data[TEMPERATURE_SENSOR_ID_LEN], div);
}

char getTemperatureInfo(char* data, void* buffer){
	char offset = 0;
	temperature_info* ti = (temperature_info*)buffer;
	ti->num_sensors = data[offset++];
	for (int i=0; i<ti->num_sensors; i++){
		ti->sensors[i].type = data[offset++];
		switch(ti->sensors[i].type){
			case 0:
				decodeDS18B20SensorTemperature(&data[offset], &ti->sensors[i]);
				offset += 10;
				break;
			case 1:
				decodeSensorTemperature(&data[offset], &ti->sensors[i], 10.0f);
				offset += 3;
				break;
			case 2:
				decodeSensorTemperature(&data[offset], &ti->sensors[i], 100.0f);
				offset += 3;
				break;
		}
	}
	return 1;
}

float decodeSensorHumidityValue(char* data){
	unsigned short v = (unsigned short)(((data[1] & 0xFF) << 8) | (data[0] & 0xFF));
	if (v == 0xFFFF) {
		return v;
	}
	return v / 10.0f;
}

char getHumidityInfo(char* data, void* buffer){
	humidity_info* hi = (humidity_info*)buffer;
	hi->value = decodeSensorHumidityValue(data);
	return 1;
}

#endif