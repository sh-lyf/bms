/*===========================================================================
  Copyright (c) 2020  by LongSung, Inc.  All Rights Reserved.

  LongSung Proprietary
  All ideas, data and information contained in or disclosed by
  this document are confidential and proprietary information of
  LongSung Technologies, Inc. and all rights therein are expressly reserved.
  By accepting this material the recipient agrees that this material
  and the information contained therein are held in confidence and in
  trust and will not be used, copied, reproduced in whole or in part,
  nor its contents revealed in any manner to others without the express
  written permission of LongSung, Inc.
  ======================================================================*/
#include "osi_compiler.h"
#include "lsapi_sys.h"
#include "lsapi_os.h"
#include "lsapi_device.h"
#include "bms_event.h"
#include "bms_main.h"
#include "bms_info.h"
#include "bms_mqtt.h"
#include "bms_ring.h"
#include "bms_checksum.h"
#include "bms_gps_parser.h"
#include "bms_tracker_protocol.h"
#include "time.h"
int lsat_pdp_status = 0;
LSAPI_OSI_Thread_t *bms_thread_event = NULL;
LSAPI_OSI_Thread_t *bms_mqtt_thread = NULL;
LSAPI_OSI_Thread_t *bms_tracker_thread = NULL;
LSAPI_OSI_Thread_t *bms_gnss_thread = NULL;
LSAPI_OSI_Thread_t *bms_gnss_parser_thread = NULL;

LSAPI_Device_t *gDeviceUart = NULL;
LSAPI_Device_t *gGnssUart = NULL;

LSAPI_Device_t *gnss_en = NULL;
LSAPI_Device_t *vrit_at_device = NULL;
LSAPI_Device_AtDispatch_t *at_dispatch = NULL;

LSAPI_OSI_Pipe_t *at_rx_pipe = NULL;
LSAPI_OSI_Pipe_t *at_tx_pipe = NULL;


LSAPI_OSI_Timer_t *bms_tracker_timer = NULL;
lxz_ring_t * rawdata_ring_buffer = NULL;
LSAPI_OSI_Mutex_t *g_RingBufferMutex = NULL;
LSAPI_OSI_Mutex_t *bms_RealtimeInfoMutex = NULL;

uint8_t *gPreReadStream = NULL;

uint8_t g_board_code [4] = {0x0,0x0,0x0,0x0};

ZG_BMS_BOARD_BASE_INFO_T g_base_info = {0};
ZG_BMS_BOARD_DYNC_VOL_INFO_T g_dynamic_vol_info = {0};
ZG_BMS_BOARD_DYNC_ELEC_INFO_T g_dynamic_elec_info = {0};
ZG_BMS_MQTT_INFO_T g_zg_mqtt_info = {0};
ZG_BMS_SAFE_EVENT_INFO_T g_zg_safe_event_info = {0};

uint8_t g_gps_buffer[BMS_GNSS_PRE_READ_SIZE_MAX] = {0};
uint16_t g_gps_buffer_len = 0;

#define CONFIG_BMS_GNSS_ENABLE_GPIO (4)
#define BMS_BASE_INFO_CNT_MAX 40
#define BMS_DYNAMIC_VOL_INFO_CNT_MAX 56

#define BMS_GNSS_UART_RX_BUFF_SIZE     (1024 * 4)
#define BMS_GNSS_UART_TX_BUFF_SIZE     (1024 * 4)

#define VIR_WAIT_FOREVER (-1U)

#define LS_TIME_ZONE_SECOND (15 * 60)

#define MAX(a, b, c) (a) > (b)? ((a) > (c)? (a) : (c)) : ((b) > (c)? (b):(c)) 

extern BMS_REALTIME_INFO_T tracker_realtime_info;
extern Bat_info_t bms_cell_bat_info;
extern void sc7a20_func(void);


int16_t tracker_recv_data_from_bms(uint8_t *recv_data,uint16_t recv_len)
{
	uint8_t *ptr = NULL;
	uint8_t cmd = 0;
	uint8_t data_len = 0;
	uint8_t recv_bcc = 0;
	uint8_t bcc_check = 0;
	uint16_t recv_crc = 0;
	uint16_t crc_check = 0;
	uint16_t cnt = 0;
	LSAPI_OSI_Event_t event = {};
	if((recv_data[0] == 0xFE) && (recv_data[1] == 0xFE))
	{
		ptr = recv_data + 2;
		recv_len = recv_len - 2;

		if((*ptr == 0xBB) && (*(ptr + 1) == 0x83))
		{
			data_len = *(ptr + 2);
			cmd = *(ptr + 3);

			//检查BCC及CRC
			recv_bcc = *(ptr + recv_len - 1);
			recv_crc = (*(ptr + recv_len - 3) << 8) + *(ptr + recv_len - 2);
			for(cnt = 0; cnt < (recv_len - 3); cnt++)
			{
				bcc_check = bcc_check ^ (*(ptr + cnt + 2));
			}
			crc_check = GetCRCCode(ptr + 2, recv_len - 5);
			LSAPI_Log_Debug("tracker_recv_data_from_bms,recv_bcc=0x%02x,recv_crc=0x%04x,bcc_check=0x%02x,crc_check=0x%04x\n",
							recv_bcc,recv_crc,bcc_check,crc_check);
			if((bcc_check != recv_bcc) || (crc_check != recv_crc))
			{
				LSAPI_Log_Debug("tracker_recv_data_from_bms,crc or bcc check error\n");
				return -1;
			}
			switch(cmd)
			{
				case ZG_CMD_RD_BOARD_BASE_INFO:
				{
					//获取bms板唯一编码
					if((data_len == 0x0A) && (*(ptr + 4) == 0x00))
					{
						g_base_info.bmsId[0] = *(ptr + 5);
						g_base_info.bmsId[1] = *(ptr + 6);
						g_base_info.bmsId[2] = *(ptr + 7);
						g_base_info.bmsId[3] = *(ptr + 8);
						LSAPI_Log_Debug("tracker_recv_data_from_bms,bmsId is %02x%02x%02x%02x\n",
							g_base_info.bmsId[0],g_base_info.bmsId[1],g_base_info.bmsId[2],g_base_info.bmsId[3]);
						LSAPI_OSI_Event_t send_event = {};
						send_event.id = BMS_GET_BOARD_BASE_INFO_ID;
						LSAPI_OSI_EvnetSend(bms_thread_event, &send_event);
					}
					else if(data_len == (BMS_BASE_INFO_CNT_MAX + 5))
					{
						//获取bms板基本信息表
						memcpy((uint8_t *)&g_base_info,ptr + 4,sizeof(ZG_BMS_BOARD_BASE_INFO_T));
						LSAPI_Log_Debug("tracker_recv_data_from_bms,bmsId is %02x%02x%02x%02x\n",
							g_base_info.bmsId[0],g_base_info.bmsId[1],g_base_info.bmsId[2],g_base_info.bmsId[3]);
						LSAPI_Log_Debug("tracker_recv_data_from_bms,lastSetDate is %d-%d-%d\n",
							g_base_info.lastSetDate[0],g_base_info.lastSetDate[1],g_base_info.lastSetDate[2]);
						LSAPI_Log_Debug("tracker_recv_data_from_bms,dischargeLock is %x\n",
							g_base_info.dischargeLock);
						
						memcpy(tracker_realtime_info.batteryInfo.batteryHardVersion,g_base_info.hwVer,2);
						memcpy(tracker_realtime_info.batteryInfo.batterySoftVersion,g_base_info.hwVer,2);

					}
					else
					{
						LSAPI_Log_Debug("tracker_recv_data_from_bms,data len is error!");
					}
				}
					
					break;
				case ZG_CMD_WR_BOARD_BASE_INFO:
					break;
				case ZG_CMD_RD_BOARD_CONTROL_INFO:
				{
					memcpy((uint8_t *)&g_dynamic_elec_info,ptr + 8,sizeof(ZG_BMS_BOARD_DYNC_ELEC_INFO_T));
					tracker_realtime_info.batteryInfo.batteryErrorCode = g_dynamic_elec_info.runErr;
					tracker_realtime_info.batteryInfo.batterySoc =(int32_t)(g_dynamic_elec_info.soc);	
					tracker_realtime_info.batteryInfo.batteryTemperatureMax = MAX(g_dynamic_elec_info.temp1,g_dynamic_elec_info.temp2,g_dynamic_elec_info.temp3);
					bms_cell_bat_info.batteryTemp[0] = (int32_t)(g_dynamic_elec_info.temp1);
					bms_cell_bat_info.batteryTemp[1] = (int32_t)(g_dynamic_elec_info.temp2);
					bms_cell_bat_info.batteryTemp[2] = (int32_t)(g_dynamic_elec_info.temp3);

					
					if(bms_mqtt_thread)
					{
						event.id = BMS_MQTT_PUB_BATINFO_INFO_ID;
						LSAPI_OSI_EvnetSend(bms_mqtt_thread, &event);

						event.id = BMS_MQTT_PUB_REALTIME_INFO_ID;
						LSAPI_OSI_EvnetSend(bms_mqtt_thread, &event);
					}					
				}
					break;
				case ZG_CMD_WR_BOARD_CONTROL_INFO:
					break;
				case ZG_CMD_RD_BOARD_BALANCE_INFO:
					break;
				case ZG_CMD_WR_BOARD_BALANCE_CTRL:
					break;
				case ZG_CMD_RD_BOARD_VOLTAGE_INFO:
				{
					memcpy((uint8_t *)&g_dynamic_vol_info,ptr + 8,sizeof(ZG_BMS_BOARD_DYNC_VOL_INFO_T));
					LSPAI_OSI_MutexLock(bms_RealtimeInfoMutex);
					tracker_realtime_info.batteryInfo.batteryCurrent = (int32_t)(g_dynamic_vol_info.totalVol);					
					tracker_realtime_info.batteryInfo.batteryProtectCode = 0;//待确认
					tracker_realtime_info.batteryInfo.batteryVoltage = g_dynamic_vol_info.maxParalleVol;
					tracker_realtime_info.batteryInfo.batteryVoltageMin = g_dynamic_vol_info.minParalleVol;
					tracker_realtime_info.batteryInfo.batteryWorkMode = 0;  //待确认；
					tracker_realtime_info.batteryInfo.chargeCycleTime = 0;	//待确认；
					tracker_realtime_info.batteryInfo.mosStatus = 0;		//待确认；
					LSAPI_OSI_MutexUnlock(bms_RealtimeInfoMutex);

					int cnt = 0;
					for(cnt = 0;cnt < sizeof(g_dynamic_vol_info.ParalleVol)/sizeof(uint16_t); cnt++)
					{
						bms_cell_bat_info.cellVol[cnt] = (int32_t)(g_dynamic_vol_info.ParalleVol[cnt]);
					}
				}
					break;
				case ZG_CMD_WR_BOARD_VOLTAGE_CTRL:
					break;
				case ZG_CMD_RD_FACTORY_TEST_INFO:
					break;
				case ZG_CMD_WR_FACTORY_TEST_INFO:
					break;
				case ZG_CMD_RD_MQTT_CONFIG_INFO:
					{
						memset(&g_zg_mqtt_info,0,sizeof(ZG_BMS_MQTT_INFO_T));
						memcpy((uint8_t *)&g_zg_mqtt_info,ptr + 8,sizeof(ZG_BMS_MQTT_INFO_T));
						LSAPI_Log_Debug("mqtt info mqtt_addr is %s",g_zg_mqtt_info.mqtt_addr);
						LSAPI_Log_Debug("mqtt info mqtt_port is %s",g_zg_mqtt_info.mqtt_port);
						LSAPI_Log_Debug("mqtt info devicesID is %s",g_zg_mqtt_info.devicesID);
						LSAPI_Log_Debug("mqtt info mqtt_username is %s",g_zg_mqtt_info.mqtt_username);
						LSAPI_Log_Debug("mqtt info mqtt_passwd is %s",g_zg_mqtt_info.mqtt_passwd);

						event.id = BMS_START_MQTT_ID;
						LSAPI_OSI_EvnetSend(bms_mqtt_thread, &event);
						
					}
					break;
				case ZG_CMD_WD_ACTION_CONFIG_INFO:
					{
						uint8_t action_reply = 0;
						action_reply = *(ptr + 8);
					}
					break;
				case ZG_CMD_RD_SAFE_EVENT_INFO:
					{
						memset(&g_zg_safe_event_info,0,sizeof(ZG_BMS_SAFE_EVENT_INFO_T));
						memcpy((uint8_t *)&g_zg_safe_event_info,ptr + 8,sizeof(ZG_BMS_SAFE_EVENT_INFO_T));
						event.id = BMS_MQTT_PUB_SAFE_EVENT_INFO_ID;
						LSAPI_OSI_EvnetSend(bms_mqtt_thread, &event);
					}
					break;
				default:
					break;
					
			}
		}
		else
		{
			return -1;
		}	
		return 0;
	}
	return -1;
}

int bms_get_frame_test(void)
{
	 uint16_t crc_checksum = 0;
	 uint8_t check = 0;
	 uint8_t cnt = 0;
	 uint8_t frame_data[111] ={0};
	 uint8_t *frame_ptr = NULL;
	 memset(frame_data,0,111);

	 frame_data[0] = 0xFE;
	 frame_data[1] = 0xFE;
	 frame_data[2] = 0xBB;
	 frame_data[3] = 0x83;
	 frame_data[4] = 0x6B;
	 frame_data[5] = 0xE0;
	 frame_data[6] = 0xFA;
	 frame_data[7] = 0x06;
	 frame_data[8] = 0xF9;
	 frame_data[9] = 0x94;
	 frame_ptr = &frame_data[10];
	 strcpy(frame_ptr,"119.61.22.122");
	 strcpy(frame_ptr + 48,"1883");
	 strcpy(frame_ptr + 54,"anycen01");
	 strcpy(frame_ptr + 62,"test");
	 strcpy(frame_ptr + 82,"123456");

	 frame_ptr = frame_data;

	 crc_checksum = GetCRCCode(frame_ptr + 4,104);
	 frame_data[108] = ((crc_checksum & 0xff00) >> 8);
	 frame_data[109] = (crc_checksum & 0x00ff);

	for(cnt = 4;cnt < 110; cnt++)
	{
		check = check^frame_data[cnt];
	}
	frame_data[110] = check;

	for(cnt = 0;cnt < 111;cnt++)
		LSAPI_Log_Debug("bms_get_frame_test,0x%02x\n",frame_data[cnt]);
	tracker_recv_data_from_bms(frame_data,111);
}

//TODO
int16_t tracker_snd_rd_cmd_to_bms(uint8_t cmd)
{
	uint16_t crc_checksum = 0;
	uint8_t check = 0;
	uint8_t data[100] = {0x00};
	uint8_t data_head[2] = {0xFE,0xFE};
	uint8_t *data_ptr = data;
	uint8_t cnt = 0;

	data[0] = 0xAA;
	data[1] = 0x83;
	data[2] = 0x08;
	data[3] = cmd;
	data[4] = g_base_info.bmsId[0];
	data[5] = g_base_info.bmsId[1];
	data[6] = g_base_info.bmsId[2];
	data[7] = g_base_info.bmsId[3];

	crc_checksum = GetCRCCode(data_ptr + 2,6);
	
	data[8] = ((crc_checksum & 0xff00) >> 8);
	data[9] = (crc_checksum & 0x00ff);

	for(cnt = 2;cnt < 10; cnt++)
	{
		check = check^data[cnt];
	}
	data[10] = check;
	LSAPI_Device_Write(gDeviceUart,data_head,2);
	LSAPI_Device_Write(gDeviceUart,data,11);
	return 0;
}

int16_t tracker_snd_action_cmd_to_bms(uint8_t cmd,uint8_t action)
{
	uint16_t crc_checksum = 0;
	uint8_t check = 0;
	uint8_t data[100] = {0x00};
	uint8_t data_head[2] = {0xFE,0xFE};
	uint8_t *data_ptr = data;
	uint8_t cnt = 0;
	data[0] = 0xAA;
	data[1] = 0x83;
	data[2] = 0x29;
	data[3] = cmd;
	data[4] = g_base_info.bmsId[0];
	data[5] = g_base_info.bmsId[1];
	data[6] = g_base_info.bmsId[2];
	data[7] = g_base_info.bmsId[3];
	data[8] = action;

	memcpy(data_ptr + 9,g_zg_mqtt_info.mqtt_username,20);
	memcpy(data_ptr + 29,g_zg_mqtt_info.mqtt_passwd,16);
	crc_checksum = GetCRCCode(data_ptr + 2, 43);
	
	data[45] = ((crc_checksum & 0xff00) >> 8);
	data[46] = (crc_checksum & 0x00ff);

	for(cnt = 2;cnt < 47; cnt++)
	{
		check = check^data[cnt];
	}
	data[47] = check;
	LSAPI_Device_Write(gDeviceUart,data_head,2);
	LSAPI_Device_Write(gDeviceUart,data,48);
	return 0;
}


void bms_tracker_timer_callback(void *param)
{
	LSAPI_OSI_Event_t event = {};
  	LSAPI_Log_Debug("=== bms_tracker_timer_callback enter ====");
  	event.id = BMS_GET_BOARD_DYNAMIC_INFO_ID;
  	LSAPI_OSI_EvnetSend(bms_thread_event, &event);

}

static void prvVirtAtRespCallback(void *param, unsigned event)
{
    LSAPI_OSI_Pipe_t *pipe = (LSAPI_OSI_Pipe_t *)param;
    char buf[256];

    int bytes = LSAPI_OSI_PipeRead(pipe, buf, 255);
    if (bytes > 0)
	{
		buf[bytes] = '\0';
    	LSAPI_Log_Debug("bms_virt_at >>pipe read <--(%d): %s", bytes, buf);
		LSAPI_Device_Write(gDeviceUart, buf, bytes);
	}
}

static void bmsRtuTask_test(void *param)
{
	uint8_t r_buffer[100] = {0x00};
	uint8_t up_buffer[100] = {0x00};
	uint8_t *up_ptr = NULL;
	int ret = 0;
	at_rx_pipe = LSAPI_OSI_PipeCreate(1024);
    at_tx_pipe = LSAPI_OSI_PipeCreate(1024);
	
	LSAPI_OSI_Event_t *pEvent = (LSAPI_OSI_Event_t *)param;


	gDeviceUart = LSAPI_Device_USBCreate(LSAPI_DEV_USB_COM5);
	if(gDeviceUart == NULL)
	{
		LSAPI_Log_Debug("bmsRtuTask_test: usb uart faild!\n");
		return ;
	}

	ret = LSAPI_Device_Open(gDeviceUart);
	
	if(!ret)
	{
		LSAPI_Log_Debug("uart: Open uart Failed\n");
		return ;
	}
#if 0
	bms_tracker_timer = LSAPI_OSI_TimerCreate(NULL, bms_tracker_timer_callback, NULL);
	if(LSAPI_OSI_TimerStart(bms_tracker_timer, 10000) == FALSE)
    {
        LSAPI_Log_Debug("timer start failed\n");
        LSAPI_OSI_ThreadExit();

    }
	else
	{
		LSAPI_Log_Debug("bmsRtuTask_test: LSAPI_OSI_TimerStart OK\n");
	}
#endif
	tracker_snd_rd_cmd_to_bms(ZG_CMD_RD_BOARD_BASE_INFO);
	//just for test
	bms_get_frame_test();
	memset(r_buffer,0,sizeof(r_buffer));
	memset(up_buffer,0,sizeof(up_buffer));
	while (1) 
	{
		int read_avail_size = LSAPI_Device_ReadAvail(gDeviceUart);
		if (read_avail_size > 0) 
		{
			int read_size = LSAPI_Device_Read(gDeviceUart, r_buffer, sizeof(r_buffer));
			if (read_size > 0)
			{
				LSAPI_Log_Debug("uart: read: {%s}\n", r_buffer);
				if((r_buffer[0] == 'a' || r_buffer[0] == 'A') && 
					(r_buffer[1] == 't' || r_buffer[1] == 'T'))
				{
					strncpy(up_buffer,r_buffer,read_size);
					up_ptr = strupr(up_buffer);
					if(!strncmp(up_ptr,"AT+TEST=1",9) && (vrit_at_device == NULL))
					{
						LSAPI_Device_Write(gDeviceUart,"\r\nOK\r\n",6);
						at_rx_pipe = LSAPI_OSI_PipeCreate(1024);
    					at_tx_pipe = LSAPI_OSI_PipeCreate(1024);
						LSAPI_OSI_PipeSetReaderCallback(at_tx_pipe, LSAPI_PIPE_EVENT_RX_ARRIVED,
                             prvVirtAtRespCallback, at_tx_pipe);

					    LSAPI_Device_AtVirtConfig_t cfg = {
					        .name = LSAPI_MAKE_TAG('V', 'A', 'T', '1'),
					        .rx_pipe = at_rx_pipe,
					        .tx_pipe = at_tx_pipe,
					    };
					    vrit_at_device = LSAPI_Device_AtVirtCreate(&cfg);
					    at_dispatch = LSAPI_Device_AtDispatchCreate(vrit_at_device);
					    LSAPI_Device_AtSetDispatch(vrit_at_device, at_dispatch);
					    LSAPI_Device_Open(vrit_at_device);
					}
					else if(!strncmp(up_ptr,"AT+TEST=0",9) && 
						(vrit_at_device != NULL) && (at_dispatch != NULL) &&
						(at_rx_pipe != NULL) && (at_tx_pipe != NULL))
					{
						LSAPI_Device_Write(gDeviceUart,"\r\nOK\r\n",6);
						LSAPI_OSI_PipeDelete(at_rx_pipe);
						at_rx_pipe = NULL;
						LSAPI_OSI_PipeDelete(at_tx_pipe);
						at_tx_pipe = NULL;
						LSAPI_Device_AtDispatchDelete(at_dispatch);
						at_dispatch = NULL;
						LSAPI_Device_Close(vrit_at_device);
						LSAPI_Device_Delete(vrit_at_device);
						vrit_at_device = NULL;
					}
					else
					{
						if(vrit_at_device != NULL)
							LSAPI_OSI_PipeWriteAll(at_rx_pipe, r_buffer, strlen(r_buffer), VIR_WAIT_FOREVER);
					}
				}
				else
					tracker_recv_data_from_bms(r_buffer,read_size);
				memset(r_buffer,0,sizeof(r_buffer));
			}
		}
		LSAPI_OSI_ThreadSleep(100);
	}
	LSAPI_OSI_ThreadExit();
}


static void bmsRtuTask(void *param)
{
	//uint32_t uart_name = 1;
	uint8_t r_buffer[100] = {0x00};
	//size_t uart_baud = 115200;
	int ret = 0;
	LSAPI_OSI_Event_t *pEvent = (LSAPI_OSI_Event_t *)param;
	LSAPI_Device_UartConfig_t uart_cfg = {
		.name = LSAPI_DEV_UART1, //LSAPI_DEV_UART2 , //LSAPI_DEV_UART1
		.baud = 115200,
		.format = LSAPI_DEVICE_FORMAT_8N1,
		.parity = LSAPI_DEVICE_PARITY_ODD,
	};

	uart_cfg.event_cb = NULL ;//prvMenuUartEvtCb;
	uart_cfg.event_cb_ctx = NULL;//gUartWRTd;

	gDeviceUart = LSAPI_Device_UartCreate(&uart_cfg);
	if(gDeviceUart == NULL)
	{
		LSAPI_Log_Debug("uart: creat uart faild!\n");
		return ;
	}

	ret = LSAPI_Device_Open(gDeviceUart);
	if(!ret)
	{
		LSAPI_Log_Debug("uart: Open uart Failed\n");
		return ;
	}
	/*TODO*/
	/*
	1、get bms board code;then send event to mqtt task
	2、get mqtt info
	*/
	while (1) 
	{
		int read_avail_size = LSAPI_Device_ReadAvail(gDeviceUart);
		if (read_avail_size > 0) 
		{
			int read_size = LSAPI_Device_Read(gDeviceUart, r_buffer, sizeof(r_buffer));
			if (read_size > 0)
			{
				LSAPI_Log_Debug("uart: read: {%s}\n", r_buffer);
			}
		}
		LSAPI_OSI_ThreadSleep(100);
	}
	LSAPI_OSI_ThreadExit();
}


void bms_event_handle_entry(void *param)
{
	LSAPI_NET_EventReg(LSAPI_OSI_ThreadCurrent(), BMS_GET_BOARD_BASE_INFO_ID | BMS_GET_BOARD_DYNAMIC_INFO_ID);
	LSAPI_OSI_Event_t send_event = {};
	for(;;)
	{
        LSAPI_OSI_Event_t event = {};
        LSAPI_OSI_EventWait(LSAPI_OSI_ThreadCurrent(),&event);
        LSAPI_Log_Debug("waitevenid = 0x%x\n",event.id);
        switch(event.id)
        {
        	case BMS_SND_ACTION_FROM_PLATFORM_ID:
    		{
    			uint8_t cmd = ZG_CMD_WD_ACTION_CONFIG_INFO;
				uint8_t action = (uint8_t)event.param1;
    			tracker_snd_action_cmd_to_bms(cmd,action);
    		}
			break;
			case BMS_GET_BOARD_BASE_INFO_ID:
			{
				tracker_snd_rd_cmd_to_bms(ZG_CMD_RD_SAFE_EVENT_INFO);
				break;
			}
			case BMS_GET_SAFE_EVENT_INFO_ID:
			{
				uint8_t cmd = ZG_CMD_RD_SAFE_EVENT_INFO;
				tracker_snd_rd_cmd_to_bms(cmd);
			}
			break;
			case BMS_GET_BOARD_DYNAMIC_INFO_ID:
			{
				tracker_snd_rd_cmd_to_bms(ZG_CMD_RD_BOARD_VOLTAGE_INFO);
				LSAPI_OSI_ThreadSleep(100);
				tracker_snd_rd_cmd_to_bms(ZG_CMD_RD_BOARD_CONTROL_INFO);
				//LSAPI_OSI_TimerStop(bms_tracker_timer);
				//LSAPI_OSI_TimerStart(bms_tracker_timer, 10000);
				break;
			}
            default:
            break;             
        }
		LSAPI_OSI_ThreadSleep(100);
    }
	LSAPI_OSI_ThreadExit();
}

int bms_event_handle_init(void)
{
    bms_thread_event  = LSAPI_OSI_ThreadCreate("bms_event_handle", bms_event_handle_entry,NULL,LSAPI_OSI_PRIORITY_NORMAL,1024 * 8,40);
    if(bms_thread_event == NULL)
   {
        LSAPI_Log_Debug("bms_thread_event full\n");
        return 0;
    }
    else
   {
         LSAPI_Log_Debug("bms_thread_event not null threadid = 0x%x\n",bms_thread_event);

    }
    return 0;
  
}


static void gnssParseTask(void *param)
{
   //should config platform gnss uart here
	memset(g_gps_buffer,0,sizeof(g_gps_buffer));
 	GPS_INFO bms_gps_info = {0};
	uint8_t arrStr[BMS_GNSS_PRE_READ_SIZE_MAX] = {0x00};
	uint32_t arrLen = 0;
	while (1) {
		LSPAI_OSI_MutexLock(g_RingBufferMutex);
		g_gps_buffer_len = lxz_ring_f_read(rawdata_ring_buffer,g_gps_buffer,1024);
		LSAPI_OSI_MutexUnlock(g_RingBufferMutex);	
		if( g_gps_buffer_len > 0 )
		{
			
			memset(arrStr,0x00,BMS_GNSS_PRE_READ_SIZE_MAX);
			ls_gnss_MakeGPSRDStr_Single(arrStr, &arrLen, "RMC");
			GPS_RMC_Parser(arrStr,&bms_gps_info);
			
			ls_gnss_MakeGPSRDStr_Single(arrStr, &arrLen, "GGA");
			GPS_GGA_Parser(arrStr,&bms_gps_info);

			ls_gnss_MakeGPSRDStr_Single(arrStr, &arrLen, "GPGSV");
			GPS_GSV_Parser(arrStr,&bms_gps_info);
			LSPAI_OSI_MutexLock(bms_RealtimeInfoMutex);
			tracker_realtime_info.gpsLocationInfo.accuracy = 2;
			LSAPI_NET_GetCsq(&(tracker_realtime_info.gpsLocationInfo.csq)); ;
			tracker_realtime_info.gpsLocationInfo.detStatus = bms_gps_info.status;//
			tracker_realtime_info.gpsLocationInfo.gpsSignal = bms_gps_info.cn[0];
			tracker_realtime_info.gpsLocationInfo.gpsSpeed =  bms_gps_info.speed;
			tracker_realtime_info.gpsLocationInfo.latitude = bms_gps_info.latitude;
			tracker_realtime_info.gpsLocationInfo.latitudeDirection = bms_gps_info.latitude_hemisphere;
			tracker_realtime_info.gpsLocationInfo.longitude = bms_gps_info.longitude;
			tracker_realtime_info.gpsLocationInfo.longitudeDirection = bms_gps_info.longitude_hemisphere;
			tracker_realtime_info.gpsLocationInfo.locationMode = 0; //待确认
			tracker_realtime_info.gpsLocationInfo.networkType = 0;	//待确认
			tracker_realtime_info.gpsLocationInfo.reportReason = 0;	//待确认
			tracker_realtime_info.gpsLocationInfo.satelliteNum = bms_gps_info.satellite_num;
			LSAPI_OSI_MutexUnlock(bms_RealtimeInfoMutex);
		}

		osiThreadSleep(100);
	}
}

int16_t bms_gnss_enable(void)
{
    LSAPI_GpioConfig_t gpioConfig = {0};
	bool write_value = 0;
    gpioConfig.id = CONFIG_BMS_GNSS_ENABLE_GPIO; 
    gpioConfig.mode = LS_GPIO_OUTPUT;
    gpioConfig.intr_enabled = false;
    gpioConfig.intr_level = false;
    gpioConfig.rising = false;
    gpioConfig.falling = false;
    gpioConfig.cb = NULL;
    gpioConfig.cb_ctx = NULL;

    // create instance
    gnss_en = LSAPI_Device_GPIOCreate(&gpioConfig);
    // open GPIO 2
    LSAPI_Device_Open(gnss_en);

	 write_value = 1;
    LSAPI_Device_Write(gnss_en, (void *)&write_value, 1);

	return 0;
}

int16_t bms_gnss_uart_create(void)
{

	int ret = 0;
	LSAPI_Device_UartConfig_t uart_cfg = {
		.name = LSAPI_DEV_UART3, //LSAPI_DEV_UART2 , //LSAPI_DEV_UART1
		.baud = 9600,
		.format = LSAPI_DEVICE_FORMAT_8N1,
		.parity = LSAPI_DEVICE_PARITY_ODD,
	};

	uart_cfg.event_cb = NULL ;//prvMenuUartEvtCb;
	uart_cfg.event_cb_ctx = NULL;//gUartWRTd;

	gGnssUart = LSAPI_Device_UartCreate(&uart_cfg);
	if(gGnssUart == NULL)
	{
		LSAPI_Log_Debug("uart: creat uart faild!\n");
		return -1;
	}

	ret = LSAPI_Device_Open(gDeviceUart);
	if(!ret)
	{
		LSAPI_Log_Debug("uart: Open uart Failed\n");
		return -1;
	}
	return 0;
}


static void gnssUartTask(void *param)
{
	bms_gnss_enable();
	bms_gnss_uart_create();
	gPreReadStream = (uint8_t *)LSAPI_OSI_Malloc(BMS_GNSS_PRE_READ_SIZE_MAX);
	if(gPreReadStream == NULL)
		return;
	memset(gPreReadStream,0x00,BMS_GNSS_PRE_READ_SIZE_MAX);
	rawdata_ring_buffer = lxz_ring_f_create(BMS_GNSS_UART_RX_BUFF_SIZE);
	if(rawdata_ring_buffer == NULL)
	{
		if(gPreReadStream != NULL)
		{
			free(gPreReadStream);
			gPreReadStream = NULL;
		}
		return;
	}
	if(g_RingBufferMutex == NULL)
	{
		g_RingBufferMutex = LSAPI_OSI_MutexCreate();
	}
	for(;;)
	{
		int16_t ring_write_size = 0;
		int16_t read_size = LSAPI_Device_ReadAvail(gGnssUart);
		if(read_size > 0)
		{
			int16_t length = LSAPI_Device_Read(gGnssUart,gPreReadStream,read_size);
			if(length > 0)
			{
				LSPAI_OSI_MutexLock(g_RingBufferMutex);
				ring_write_size = lxz_ring_f_write(rawdata_ring_buffer,gPreReadStream,length);
				if(ring_write_size == 0)
				{
					lxz_ring_f_clear(rawdata_ring_buffer);
				}					
			}
			memset((void *)gPreReadStream,0x00,BMS_GNSS_PRE_READ_SIZE_MAX);
		}
		LSAPI_OSI_ThreadSleep(10);
	}
	
}


int get_time_week(void)
{
	SET_TIME_T cur_time = {0};

	int timeoffset = 32 * LS_TIME_ZONE_SECOND;
	time_t lt = LSAPI_OSI_EpochSecond() + timeoffset;
	struct tm ltm;
	gmtime_r(&lt, &ltm);
	char ts[64];
	static const char *fmt1 = "%02d/%02d/%02d,%02d:%02d:%02d%+03d";
	sprintf(ts, fmt1, (ltm.tm_year + 1900) % 100, ltm.tm_mon + 1, ltm.tm_mday,
	           ltm.tm_hour, ltm.tm_min, ltm.tm_sec, 32);
	LSAPI_Log_Debug("bms ===time ts = %s\n",ts);

	cur_time.years = (ltm.tm_year + 1900) % 100;
	cur_time.months = ltm.tm_mon + 1;
	cur_time.days = ltm.tm_mday;
	cur_time.hours = ltm.tm_hour;
	
	uint8_t month = (cur_time.months < 3 ? (cur_time.months + 12): cur_time.months);
	uint8_t week = (cur_time.years + cur_time.years / 4 + 20/4 - 2*20 + 13 * (month + 1) /5 + cur_time.days - 1) % 7;
	cur_time.week = (week == 0 ? 7 : week);
	LSAPI_Log_Debug("bms ===week ts = %d\n",week);

}


int appimg_enter(void *param)
{
    LSAPI_Log_Debug("bms_app>>appimg_enter()");

#if 1	
    LSAPI_OSI_ThreadSleep(10000);

	LSAPI_NET_CGACT();  
	LSAPI_Log_Debug("bms_app create netif\n");
	LSAPI_NET_NetIf_Create();
	if(LSAPI_NET_GET_GprsNetIf() == FALSE)
	{
	    LSAPI_Log_Debug("qxwz_demo netif failed\n");   
	}
#endif
	get_time_week();

	g_RingBufferMutex = LSAPI_OSI_MutexCreate();
	bms_RealtimeInfoMutex = LSAPI_OSI_MutexCreate();
	bms_event_handle_init();
	    
    bms_mqtt_thread = LSAPI_OSI_ThreadCreate("[BMS_MQTT_task]", bms_MqttEntry, NULL, LSAPI_OSI_PRIORITY_ABOVE_NORMAL, 1024*64, 4);
	bms_tracker_thread = LSAPI_OSI_ThreadCreate("[BMS_Rtu_task]", bmsRtuTask_test, NULL, LSAPI_OSI_PRIORITY_NORMAL, 1024*4, 4);
	//bms_gnss_thread = LSAPI_OSI_ThreadCreate("[BMS_Gnss_task]", gnssUartTask, NULL, LSAPI_OSI_PRIORITY_NORMAL, 1024*64, 4);
	//bms_gnss_parser_thread = LSAPI_OSI_ThreadCreate("[Gnss_Parse_task]", gnssParseTask, NULL, LSAPI_OSI_PRIORITY_NORMAL, 1024*64, 4);
	sc7a20_func();
    return 0;
}

void appimg_exit(void)
{
    LSAPI_Log_Debug("uartDemo>>appimg_exit()");

}
