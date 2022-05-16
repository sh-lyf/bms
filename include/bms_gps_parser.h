#ifndef __JTT_GPS_PARSER_H__
#define __JTT_GPS_PARSER_H__

#include "lsapi_sys.h"
#include "lsapi_os.h"

typedef uint8_t UINT8;
typedef uint16_t UINT16;
typedef uint32_t UINT32;

#define BMS_GNSS_PRE_READ_SIZE_MAX (1024 * 2)

typedef struct
{
	UINT8 year;
	UINT8 mounth;
	UINT8 day;
	UINT8 hour;
	UINT8 minute;
	UINT8 second;
}UTC_DATA_TIME;

typedef struct
{
	UINT8 status;
	UINT8 latitude_hemisphere;
	UINT8 longitude_hemisphere;

	UINT16 speed;
	UINT16 direct;
	UINT16 altitude;
	float latitude;
	float longitude;
	UINT32 satellite_num;
	UTC_DATA_TIME utc;
	UINT8 cn[8];
}GPS_INFO;

void ls_gnss_MakeGPSRDStr_Single(uint8_t *arrStr, uint32_t *parrLen, uint8_t *NEMA_GSA);

UINT16 GPS_RMC_Parser(UINT8 * line,GPS_INFO *info);
/*获取海拔高度及卫星数*/
UINT16 GPS_GGA_Parser(UINT8 * line,GPS_INFO *info);
UINT16 GPS_GSV_Parser(UINT8 * line,GPS_INFO *info);


#endif/*__JTT_GPS_PARSER_H__*/
