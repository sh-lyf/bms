#include <stdlib.h>

#include "lsapi_sys.h"
#include "lsapi_os.h"

#include "bms_gps_parser.h"

/*$GNRMC,122614.000,A,3113.2887,N,12137.4316,E,0.074,221.89,060721,,,D*45*/

/*$GNGGA,055911.000,3113.2946,N,12137.4498,E,1,4,4.57,75.1,M,8.4,M,,*48*/
/*
$GPGSV,4,1,15,02,70,052,42,20,63,346,,05,49,286,,13,41,184,42*72
$GPGSV,4,2,15,06,40,098,42,29,22,318,15,44,20,250,,15,17,212,*7D
$GPGSV,4,3,15,30,15,108,46,19,11,157,35,07,11,072,46,12,10,238,*70
$GPGSV,4,4,15,09,07,039,28,25,06,270,

*/
#define NMEA_GSV_HEAD "$GPGSV"
extern uint8_t g_gps_buffer[BMS_GNSS_PRE_READ_SIZE_MAX];
extern uint16_t g_gps_buffer_len;

void ls_gnss_MakeGPSRDStr_Single(uint8_t *arrStr, uint32_t *parrLen, uint8_t *NEMA_GSA)
{
    uint16_t GSA_len = 0;
    uint16_t index_end_str = 0;
    uint16_t index_nema = 0;
    uint16_t index_next_nema = 0;
    uint8_t nema_gsa_len = 0;
    uint8_t ignore_str_len = 3; // "$BD"
    uint8_t pre_str_len = 1; //"$" 

    if (!NEMA_GSA)
    {
        return;
    }

    nema_gsa_len = strlen(NEMA_GSA);
    if (nema_gsa_len == 5) // "BDGSV"
    {
        ignore_str_len = pre_str_len; // "$" -> 1
    }
    else if (nema_gsa_len == 3) // "GSV"
    {
        ignore_str_len = pre_str_len + 2; // "$BD" -> "$" + "BD" -> 3
    }

    *parrLen = 0;

    for (index_nema=0; index_nema < g_gps_buffer_len; index_nema++)
    {
        if (('$' == g_gps_buffer[index_nema]) 
			&& (0 == strncmp(&g_gps_buffer[index_nema + ignore_str_len], NEMA_GSA, nema_gsa_len)))
        {
            //AT_TC(g_sw_GPRS, "ls_gnss_MakeGPSRDStr_Single GSA  [%d][%s]", index_nema, &g_gps_buffer[index_nema]);
            for (index_next_nema = (index_nema + ignore_str_len + nema_gsa_len); index_next_nema < g_gps_buffer_len; index_next_nema++)
            {
                if ( ('\r' == g_gps_buffer[index_next_nema - 1]) && ('\n' == g_gps_buffer[index_next_nema]) ) //  find "\r\n"
                {
                    GSA_len = index_next_nema - index_nema + 1;
                    memcpy((arrStr + *parrLen), (g_gps_buffer + index_nema), GSA_len);
                    *parrLen += GSA_len;
                    //AT_TC(g_sw_GPRS, "ls_gnss_MakeGPSRDStr_Single GSA NEXT_NEMA, nema_index:%d, next_nema:%d, GSA_len:%d", index_nema, index_next_nema, GSA_len);
                    index_nema = index_next_nema;
                    break;
                }
            }
        }
    }

    //AT_TC(g_sw_GPRS, "ls_gnss_MakeGPSRDStr_Single END GSA <index:%d, next:%d>[*parrLen:%d][arrStr:%s]", index_nema, index_next_nema, *parrLen, arrStr);
}


UINT8 *comma_mov(UINT8 cnt,UINT8 *buff)
{
	UINT8 *mv = buff;
	UINT8 i = 0;
	for(i = 0; i < cnt; i++)
	{
		if(*mv == '\0')
			return NULL;
		while(*mv++ != ',');
	}
	return mv;
}

UINT8 *dollar_mov(UINT8 cnt,UINT8 *buff)
{
	UINT8 *mv = buff;
	UINT8 i = 0;
	for(i = 0; i < cnt; i++)
	{
		if(*mv == '\0')
			return NULL;
		while(*mv++ != '$');
	}
	return mv;
}

UINT16 GPS_GSV_Parser(UINT8 * line,GPS_INFO *info)
{
	if(line == NULL|| info == NULL)
		return 0;

	UINT32 length = 0;
	UINT32 gsv_info_cnt = 0;
	UINT32 cnt = 0;
	UINT32 satellite_info_cnt = 0;
	UINT8 satellite_num = 0;
	UINT8 comma_num = 0;
	UINT8 cn_num = 0;
	UINT32 i = 0;
	UINT8 *cn_ptr = NULL;
	UINT8 *buf = line;
	UINT8 *mv = buf;
	//UINT32 satellite_num = 0;
	//AT_TC(g_sw_GPRS,"GSV str is: %s", line);

	while(line[i] != '\0')
	{
		if(line[i] == '$')
			gsv_info_cnt++;
		i++;
	}
#if 0
	AT_TC(g_sw_GPRS,"GPS_GSV_Parser,gsv_info_cnt is: %d", gsv_info_cnt);
	for(cnt = 0; cnt < gsv_info_cnt; cnt++)
	{
		mv = dollar_mov(cnt + 1,buf);
		mv = comma_mov(7,mv);
		if(mv[0] != ',')
			info->cn[cnt] = (mv[0] -'0')*10 + mv[1] - '0';
		else
		{
			while(mv == )
		}
		AT_TC(g_sw_GPRS,"GPS_GSV_Parser,CN[%d]  is: %d", cnt, info->cn[cnt]);
		cnt++;
	}
#endif
	mv = comma_mov(3,buf);
	length = comma_mov(4,buf) - comma_mov(3,buf) - 1;
	if(length == 2)
	{
		satellite_num = (mv[0] -'0')*10 + mv[1] - '0';
	}
	else if(length == 1)
	{
		satellite_num = mv[0] -'0';
	}
	else
		return 0;
	//AT_TC(g_sw_GPRS,"GPS_GSV_Parser,satellite_num is: %d gsv_info_cnt is %d", satellite_num, gsv_info_cnt);

	cn_ptr = (UINT8 *)LSAPI_OSI_Malloc(satellite_num *sizeof(UINT8));
	if(cn_ptr == NULL)
		return 0;
	mv = buf;
	for(cnt = 0; cnt < gsv_info_cnt; cnt++)
	{
		mv = dollar_mov(cnt + 1,buf);
		while(mv[i] != '*')
		{
			if(mv[i] == ',')
				comma_num++;
			i++;
		}
		satellite_info_cnt = (comma_num - 3)/4;
		comma_num = 0;
		mv = comma_mov(3,mv);
		for(i = 0; i < satellite_info_cnt; i++)
		{
			mv = comma_mov(4,mv);
			if(mv[0] == ',')
				continue;
			else if(mv[0] == '*')
				break;
			else
			{
				cn_ptr[cn_num] = (mv[0] -'0')*10 + mv[1] - '0';
				cn_num++;
			}
		}
		i = 0;
	}
	
	UINT8 temp_cn = 0;
	UINT32 j = cn_num - 1;
	while(j)
	{
		for(i = 0; i < j; i++)
		{
			if(cn_ptr[i] < cn_ptr[i + 1])
			{
				temp_cn = cn_ptr[i + 1];
				cn_ptr[i + 1] = cn_ptr[i];
				cn_ptr[i] = temp_cn;
			}
		}
		j--;
	}
	cn_num = (cn_num >= 8?8:cn_num);
	for(i = 0; i < cn_num; i++)
	{
		info->cn[i] = cn_ptr[i];
		//AT_TC(g_sw_GPRS,"GPS_GSV_Parser,CN[%d]  is: %d", i, info->cn[i]);
	}
	if(cn_ptr != NULL)
	{
		LSAPI_OSI_Free(cn_ptr);
		cn_ptr = NULL;
	}
	return 1;
}

UINT16 GPS_GGA_Parser(UINT8 * line,GPS_INFO *info)
{
	if(line == NULL|| info == NULL)
		return 0;

	UINT32 length = 0;
	UINT8 altitude[8]  = {0};
	UINT8 *buf = line;
	UINT8 *mv = buf;
	//UINT32 satellite_num = 0;
	//AT_TC(g_sw_GPRS,"GGA str is: %s", line);

	mv = comma_mov(7,buf);
	info->satellite_num = mv[0] -'0';
	
	mv = comma_mov(9,buf);
	length = comma_mov(10,buf) - comma_mov(9,buf) - 1;
	memcpy(altitude,mv,length);
	//AT_TC(g_sw_GPRS,"altitude str is: %s", altitude);
	info->altitude = (UINT16)(atof((const char*)altitude));
	//AT_TC(g_sw_GPRS,"decode altitude is: %d", info->altitude);

	return 1;
}


UINT16 GPS_RMC_Parser(UINT8 * line,GPS_INFO *info)
{
	if(line == NULL|| info == NULL)
		return 0;

	UINT8 latitude[11] = {0};
	UINT8 longitude[12] = {0};
	UINT8 speed[8] = {0};
	UINT8 derect[8]  = {0};
	UINT32 length = 0;
	UINT8 *buf = line;
	UINT8 *mv = buf;

	//AT_TC(g_sw_GPRS,"rmc str is: %s", line);
	//获取状态
	mv = comma_mov(2,buf);
	info->status = mv[0];

	if(info->status != 'A')
	{
		return -1;
	}
	//获取UTC时间
	mv = comma_mov(1,buf);
	info->utc.hour = (mv[0] - '0') * 10 + mv[1] -'0';
	info->utc.minute = (mv[2] - '0') * 10 + mv[3] -'0';
	info->utc.second = (mv[4] - '0') * 10 + mv[5] -'0';

	//获取纬度，转换为度，乘以10^6
	mv = comma_mov(3,buf);
	memset(latitude,0,11);
	memcpy(latitude,mv,10);
	info->latitude = atof(latitude);
#if 0
	memcpy(latitude_second,mv + 2,7);

	//info->latitude = ((mv[0] - '0') * 10 + (mv[1] -'0'))*1000000 + (UINT32)(atof(latitude_second)/60.0 * 10000);
	info->latitude = (UINT32)(((mv[0] - '0') * 10 + (mv[1] -'0') + atof((const char*)latitude_second)/60.0)*1000000);
#endif	
	//获取维度半球
	mv = comma_mov(4,buf);
	info->latitude_hemisphere = mv[0];

	//获取经度，转换为度，乘以10^6
	mv = comma_mov(5,buf);
	memset(longitude,0,12);
	memcpy(longitude,mv,11);
	info->longitude = atof(longitude);
#if 0
	memcpy(longitude_second,mv + 3,7);
	//info->longitude = ((mv[0] - '0') * 100 + (mv[1] - '0') * 10 + (mv[2] -'0'))*1000000 + (UINT32)(atof(longitude_second)/60.0 * 10000);	
	info->longitude = (UINT32)(((mv[0] - '0') * 100 + (mv[1] - '0') * 10 + (mv[2] -'0') + atof((const char*)longitude_second)/60.0)*1000000);
#endif	
	//获取经度半球
	mv = comma_mov(6,buf);
	info->longitude_hemisphere = mv[0];

	//获取速度
	mv = comma_mov(7,buf);
	length = comma_mov(8,buf) - comma_mov(7,buf) - 1;
	memcpy(speed,mv,length);
	//AT_TC(g_sw_GPRS,"speed str is: %s", speed);
	info->speed = (UINT16)(atof((const char*)speed)*1.85*10);
	//AT_TC(g_sw_GPRS,"decode speed is: %d", info->speed);

	//获取方向
	mv = comma_mov(8,buf);
	length = comma_mov(9,buf) - comma_mov(8,buf) - 1;
	memcpy(derect,mv,length);
	//AT_TC(g_sw_GPRS,"derect str is: %s", derect);
	info->direct = (UINT16)(atof((const char*)derect));
	//AT_TC(g_sw_GPRS,"decode direct is: %d", info->direct);

	//获取UTC日期
	mv = comma_mov(9,buf);
	info->utc.day = (mv[0] - '0') * 10 + mv[1] -'0';
	info->utc.mounth = (mv[2] - '0') * 10 + mv[3] -'0';
	info->utc.year = (mv[4] - '0') * 10 + mv[5] -'0';

	return 1;
}

