#ifndef __BMS_INFO_H__
#define __BMS_INFO_H__

#include "lsapi_types.h"
#include "lsapi_os.h"

typedef struct _bms_platform_reply_t{
	char *thingId;
	int64_t timestamp; 
	int32_t code;
}BMS_PALTFPRM_REPLY_T;

typedef struct _bms_basic_info_t{
	char *thingId;
	int64_t timestamp; 
	char *model;
	char *manufacture;
	char *imei;
	char *imsi;
	char *mobile;
	char *trackerHardwareVersion;
	char *trackerSoftwareVersion;
	int32_t code;
}BMS_BASIC_INFO_T;

typedef struct _bms_battery_info_t{
	int32_t		batteryVoltage;
	int32_t		batteryCurrent;
	int32_t	 	batterySoc;
	uint8_t		batteryHardVersion[64];
	uint8_t		batterySoftVersion[64];
	int32_t		batteryWorkMode;
	int32_t		batteryProtectCode;
	int32_t		batteryErrorCode;
	int32_t		batteryTemperatureMax;
	int32_t		batteryVoltageMax;
	int32_t		batteryVoltageMin;
	int32_t		mosStatus;
	int32_t		chargeCycleTime;
	
}BMS_BATTERY_INFO_T;

typedef struct _bms_location_info_t{
	int32_t		reportReason;
	int32_t		detStatus;
	int32_t	 	csq;
	int32_t		networkType;
	int32_t		locationMode;
	double		longitude;
	int32_t		longitudeDirection;
	double		latitude;
	int32_t		latitudeDirection;
	int32_t		gpsSpeed;
	int32_t		gpsSignal;
	int32_t		satelliteNum;
	int32_t		accuracy;
	
}BMS_LOCATION_INFO_T;

typedef struct _bms_realtime_info_t{
	BMS_BATTERY_INFO_T batteryInfo;
	BMS_LOCATION_INFO_T gpsLocationInfo;
}BMS_REALTIME_INFO_T;

typedef struct _Battery_LowPower_Event_t{
	int32_t batterySoc;
	BMS_BATTERY_INFO_T gpsLocationInfo;
}Battery_LowPower_Event_t;

typedef struct _Battery_Fault_Event_t
{
	int32_t batteryErrorCode;
}Battery_Fault_Event_t;

typedef struct _Battery_Protect_Event_t{
	int32_t batteryProtectCode;
}Battery_Protect_Event_t;

typedef struct _Tracker_Fault_Event_t{
	int32_t logDetail;
}Tracker_Fault_Event_t;

typedef struct _Tracker_Reset_Event_t{
	int32_t serviceId;
}Tracker_Reset_Event_t;

typedef struct _Battery_Safe_Event_t{
	int32_t	batteryVoltage;
	int32_t batteryCurrent;
	uint8_t batterySoc;
	int32_t	*cellV;
	uint8_t	chargeState;
	uint8_t	faultState3;
	uint8_t	faultCellNumber3;
	uint8_t	*faultCellID3;
	int32_t meanOCV;
	int32_t	oCVIntervalCNT;
	uint8_t	faultState2;
	uint8_t	faultCellNumber2;
	uint8_t *faultCellID2;
	uint8_t	faultState1;
	uint8_t faultCellNumber1;
	uint8_t *faultCellID1;
}Battery_Safe_Event_t;

typedef struct _Event_Request_Data_t{
	Battery_LowPower_Event_t batteryLowPowerEvent;
	Battery_Fault_Event_t batteryFaultEvent;
	Battery_Protect_Event_t batteryProtectEvent;
	Tracker_Fault_Event_t trackerFaultEvent;
	Tracker_Reset_Event_t trackerResetEvent;
	Battery_Safe_Event_t batterySafeEvent;
	int64_t eventTime;
}Event_Request_Date_t;

typedef struct _Tracker_Query_t{	
	bool base;
	bool realtime;
	bool log;
	bool config;
	bool bat;
}Tracker_Query_t;

typedef struct _Firmware_Info_t{
	uint8_t *model;
	uint8_t *manufacture;
	uint8_t *hardwareVersion;
	uint8_t *softwareVersion;
	uint8_t *url;
	uint8_t *md5;
	int32_t	size;
}Firmware_Info_t;

typedef struct _Bms_Ota_Info_t{
	uint8_t *model;
	uint8_t *manufacture;
	uint8_t *hardwareVersion;
	uint8_t *softwareVersion;
	uint8_t *url;
	uint8_t *md5;
	int32_t	size;
}Bms_Ota_Info_t;


typedef struct _Bms_Ota_Action_t{
	Bms_Ota_Info_t otaInfo[3];
}Bms_Ota_Action_t;


typedef struct _Tracker_Operation_t{
	bool turnOffBms;
	bool turnOffTracker;
}Tracker_Operation_t;

typedef struct _Bms_Operation_t{
	bool action;
	bool value;
}Bms_Operation_t;

typedef struct _Bat_info_t{
	int32_t cellVol[64];
	int32_t batteryTemp[64];
}Bat_info_t;

#endif/*__BMS_INFO_H__*/
