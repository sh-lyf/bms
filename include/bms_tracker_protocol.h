#ifndef __BMS_TRACKER_PROTOCOL_H__
#define __BMS_TRACKER_PROTOCOL_H__


#define ZG_CMD_RD_BOARD_BASE_INFO 				0xC0
#define ZG_CMD_WR_BOARD_BASE_INFO				0xC1
#define ZG_CMD_RD_BOARD_CONTROL_INFO 			0xC2
#define ZG_CMD_WR_BOARD_CONTROL_INFO			0xC3
#define ZG_CMD_RD_BOARD_BALANCE_INFO 			0xC4
#define ZG_CMD_WR_BOARD_BALANCE_CTRL			0xC5

#define ZG_CMD_RD_BOARD_VOLTAGE_INFO 			0xC6
#define ZG_CMD_WR_BOARD_VOLTAGE_CTRL			0xC7

#define ZG_CMD_RD_FACTORY_TEST_INFO 			0xC8
#define ZG_CMD_WR_FACTORY_TEST_INFO 			0xC9

#define ZG_CMD_RD_MQTT_CONFIG_INFO 				0xE0
#define ZG_CMD_WD_ACTION_CONFIG_INFO 			0xE1
#define ZG_CMD_RD_SAFE_EVENT_INFO 				0xE2

#define ZG_CMD_RD_CHARGE_MODE_INFO				0xF1
#define ZG_CMD_WR_SEARCH_MODE_CTRL				0xF2
#define ZG_CMD_WR_FLIGHT_MODE_CTRL				0xF3

typedef struct _bms_register_info_t{

	uint16_t index;
	uint16_t rw_len;
}BMS_RESISTER_INFO_T;
typedef struct tracker_to_bms
{
    uint8_t device_id;
    uint8_t func_code;
    uint8_t register_msb;
    uint8_t register_lsb;
    uint8_t rd_len_msb;
    uint8_t rd_len_lsb;
	uint8_t crc_lsb;
	uint8_t crc_msb;
}TRACKER_RD_BMS_T;

typedef struct _zg_bms_board_base_info_t{
	uint8_t bmsId[4];				//BMS编码（MCU唯一码）
	uint8_t hwVer[2];				//硬件版本
	uint8_t swVer[2];				//软件版本	
	uint8_t normalVal;					//标称电压 (V)
	uint8_t maxPower;				//最大功率（10W）
	uint8_t mumSeties;				//电池串数
	uint8_t numParalle;				//每串并数
	uint8_t nomalCapacity;			//标称容量
	uint8_t industrialYear;			//生产批号(年)
	uint8_t industrialWeek; 		//生产批号)(周)
	uint8_t qc;						//QC(1-200)
	uint8_t maxVal[2];				//电芯最高电压
	uint8_t minVal[2];				//电芯最低电压
	uint8_t maxChargeElectrics;		//充电最大电流
	uint8_t maxDischargeElectrics;	//放电最大电流
	uint8_t  posc;					//短路保护电流
	uint8_t  tempValue1;			//温感参数1
	uint8_t  tempValue2;			//温感参数2
	uint8_t  protectTemp;			//保护温度
	uint8_t releaseTemp;			//释放温度
	uint8_t curSamplingResistor[2];	//采样电阻0.001mΩ
	uint8_t lastSetDate[3];			//最后设置参数年月日
	uint8_t ocdVol;					//BQ过流检测电压mv
	uint8_t ocdDelay;				//BQ过流保护延时10ms
	uint8_t scVol;					//BQ 短路检测电压mv
	uint8_t scDelay;				//BQ短路保护延时10us
	uint8_t chargeLock;				//充电锁（A5）
	uint8_t dischargeLock;			//放电锁（A5）
	uint8_t endFlag;				//0x83
	uint8_t dataBcc;				//data bcc
}ZG_BMS_BOARD_BASE_INFO_T;


typedef struct  _zg_bms_board_dynamic_vol_info_t{
	uint16_t totalVol;					//总电压
	uint16_t ParalleVol[24];			//第1~24串电压
	uint16_t maxParalleVol;				//串电芯最高电压
	uint16_t minParalleVol;				//串电芯最低电压
	uint16_t maxVolDifference;			//串电芯最大压差
}ZG_BMS_BOARD_DYNC_VOL_INFO_T;

typedef struct  _zg_bms_board_dync_elec_info_t{
	uint16_t curElec;		//当前电流
	uint8_t temp1;			//温度1
	uint8_t temp2;			//温度2
	uint8_t temp3;			//温度3
	uint8_t soc;			//SOC
	uint8_t bqErr;			//BQ错误
	uint8_t runErr;			//运行错误
}ZG_BMS_BOARD_DYNC_ELEC_INFO_T;

typedef struct _zg_bms_safe_event_info_t{
	uint16_t totalVol;					//总电压
	uint16_t maxVolDifference;			//串电芯最大压差
	uint16_t ParalleVol[24];			//第1~24串电压
	uint8_t  soc;						//SOC (%)
	uint8_t  charge_status;				//充放电
	uint16_t curElec;					//电流
	uint8_t  runInfo[48];				//运行信息
	uint8_t  reserve[8];				//保留
	
}ZG_BMS_SAFE_EVENT_INFO_T;

typedef struct _zg_bms_mqtt_info_t{
	uint8_t mqtt_addr[48];
	uint8_t mqtt_port[6];
	uint8_t devicesID[8];
	uint8_t mqtt_username[20];
	uint8_t mqtt_passwd[16];
}ZG_BMS_MQTT_INFO_T;

#endif/*__BMS_TRACKER_PROTOCOL_H__*/
