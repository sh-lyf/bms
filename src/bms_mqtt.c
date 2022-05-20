#include "stdlib.h"
#include <string.h>

#include "lsapi_sock.h"
#include "lsapi_sys.h"
#include "lsapi_os.h"
#include "lsapi_network.h"
#include "../../examples/menu_test/menu_event.h"
#include "lsapi_mqtt.h"
#include "any.pb.h"
#include "time.h"
#include "pb_encode.h"
#include "pb_decode.h"
#include "LibSafeTrackerMessage.pb.h"
#include "bms_event.h"
#include "bms_tracker_protocol.h"
#include "bms_info.h"
#include "bms_mqtt.h"
#include "lsapi_aes.h"


#include "lsapi_fota.h"

#include "lsapi_sock.h"

#include "lsapi_http.h"
#include "hal_config.h"
#include "fupdate.h"

#include "fupdate_config.h"
#include "lsapi_fs.h"
#include "lsapi_md5.h"

LSAPI_OSI_Thread_t *bms_mqtt_threadid = NULL;

BMS_BASIC_INFO_T *tacker_base_message = NULL;
BMS_BASIC_INFO_T *tacker_base_init = NULL;
LSAPI_OSI_Timer_t *bms_realtime_timer_t = NULL;
//LSAPI_OSI_Timer_t *bms_batinfo_timer_t = NULL;
LSAPI_OSI_Timer_t *bms_safeevent_timer_t = NULL;

BMS_REALTIME_INFO_T tracker_realtime_info = {{0},{0}};
Bat_info_t bms_cell_bat_info = {{0},{0}};

char tracker_ota_url_1[64] = {0};
char tracker_ota_url_2[64] = {0};
char *key = "LibSafeAesKeys22";

uint8_t g_mqtt_tracker_init_flag = 0;

#define BMS_TRACKER_MQTT_SUB_TOPIC_BASE_INFO_REPLY "/test/tracker001/tracker/info/base/post_reply"
#define BMS_TRACKER_MQTT_SUB_TOPIC_REALTIME_INFO_REPLY "/test/tracker001/tracker/info/realtime/post_reply"
#define BMS_TRACKER_MQTT_SUB_TOPIC_EVENT_INFO_REPLY "/test/tracker001/tracker/info/event/post_reply"
#define BMS_TRACKER_MQTT_SUB_TOPIC_CONFIG_INFO_REPLY "/test/tracker001/tracker/info/config/post_reply"
#define BMS_TRACKER_MQTT_SUB_TOPIC_OTA_INFO_REPLY "/test/tracker001/tracker/info/ota/post_reply"
#define BMS_TRACKER_MQTT_SUB_TOPIC_BAT_INFO_REPLY "/test/tracker001/tracker/bat/info/post_reply"


#define BMS_TRACKER_MQTT_PUB_TOPIC_BASE_INFO "/test/tracker001/tracker/info/base/post"
#define BMS_TRACKER_MQTT_PUB_TOPIC_REALTIME_INFO "/test/tracker001/tracker/info/realtime/post"
#define BMS_TRACKER_MQTT_PUB_TOPIC_EVENT_INFO "/test/tracker001/tracker/info/event/post"
#define BMS_TRACKER_MQTT_PUB_TOPIC_CONFIG_INFO "/test/tracker001/tracker/info/config/post"
#define BMS_TRACKER_MQTT_PUB_TOPIC_OTA_INFO "/test/tracker001/tracker/info/ota/post"
#define BMS_TRACKER_MQTT_PUB_TOPIC_BAT_INFO "/test/tracker001/tracker/bat/info/post"


#define BMS_TRACKER_MQTT_PUB_TOPIC_QUERY_REPLY "/test/tracker001/platform/tracker/query/action_reply"
#define BMS_TRACKER_MQTT_PUB_TOPIC_TRACKER_OTA_REPLY "/test/tracker001/platform/tracker/ota/action_reply"
#define BMS_TRACKER_MQTT_PUB_TOPIC_LIBSAFE_OTA_REPLY "/test/tracker001/platform/libsafe/ota/action_reply"
#define BMS_TRACKER_MQTT_PUB_TOPIC_CONFIG_REPLY		"/test/tracker001/platform/tracker/config/action_reply"
#define BMS_TRACKER_MQTT_PUB_TOPIC_BMS_OPERATION_REPLY	"/test/tracker001/platform/bms/operation/action_reply"
#define BMS_TRACKER_MQTT_PUB_TOPIC_TRACKER_OPERATION_REPLY	"/test/tracker001/platform/tracker/operation/action_reply"

#define BMS_TRACKER_MQTT_PUB_TOPIC_EXTRA_REPLY	"/test/tracker001/platform/tracker/extra/action_reply"


#define BMS_TRACKER_MQTT_SUB_TOPIC_QUERY 				"/test/tracker001/platform/tracker/query/action"
#define BMS_TRACKER_MQTT_SUB_TOPIC_TRACKER_OTA 			"/test/tracker001/platform/tracker/ota/action"
#define BMS_TRACKER_MQTT_SUB_TOPIC_LIBSAFE_OTA 			"/test/tracker001/platform/libsafe/ota/action"
#define BMS_TRACKER_MQTT_SUB_TOPIC_CONFIG				"/test/tracker001/platform/tracker/config/action"
#define BMS_TRACKER_MQTT_SUB_TOPIC_TRACKER_OPERATION	"/test/tracker001/platform/tracker/operation/action"

#define BMS_TRACKER_MQTT_SUB_TOPIC_BMS_OPERATION		"/test/tracker001/platform/bms/operation/action"
#define BMS_TRACKER_MQTT_SUB_TOPIC_EXTRA				"/test/tracker001/platform/tracker/extra/action"


#define BMS_SUB_TABLE_MAX 14
int bms_baseinfo_replay_from_platform(char *payload,int len);
int bms_baseinfo_post_from_self(char *payload,int len);
int bms_bms_action_from_platform(char *payload,int len);
int bms_tracker_action_from_platform(char *payload,int len);
int bms_tracker_query_from_platform(char *payload,int len);
int bms_tracker_ota_from_platform(char *payload,int len);

int bms_post_baseinfo_to_platform(void);
int bms_post_realtime_info_to_platform(int64_t value);
int bms_post_cellbat_info_to_platform(int64_t value);

extern ZG_BMS_MQTT_INFO_T g_zg_mqtt_info;
extern LSAPI_OSI_Thread_t *bms_mqtt_thread;
extern LSAPI_OSI_Thread_t *bms_thread_event;
extern LSAPI_OSI_Mutex_t *bms_RealtimeInfoMutex;
BMS_MQTT_SUB_TOPIC_T bms_mqtt_sub_table[BMS_SUB_TABLE_MAX] = 
{
	//{BMS_TRACKER_MQTT_PUB_TOPIC_BASE_INFO,bms_baseinfo_post_from_self},
	{BMS_TRACKER_MQTT_SUB_TOPIC_BASE_INFO_REPLY,bms_baseinfo_replay_from_platform,0},
	{BMS_TRACKER_MQTT_SUB_TOPIC_REALTIME_INFO_REPLY,bms_baseinfo_replay_from_platform,0},
	{BMS_TRACKER_MQTT_SUB_TOPIC_BAT_INFO_REPLY,bms_baseinfo_replay_from_platform,0},
	{BMS_TRACKER_MQTT_SUB_TOPIC_EVENT_INFO_REPLY,bms_baseinfo_replay_from_platform,0},
	{BMS_TRACKER_MQTT_SUB_TOPIC_CONFIG_INFO_REPLY,bms_baseinfo_replay_from_platform,0},
	{BMS_TRACKER_MQTT_SUB_TOPIC_OTA_INFO_REPLY,bms_baseinfo_replay_from_platform,0},

	{BMS_TRACKER_MQTT_SUB_TOPIC_QUERY,bms_tracker_query_from_platform,0},
	{BMS_TRACKER_MQTT_SUB_TOPIC_TRACKER_OTA,bms_tracker_ota_from_platform,0},
	{BMS_TRACKER_MQTT_SUB_TOPIC_BMS_OPERATION,bms_bms_action_from_platform,0},
	{BMS_TRACKER_MQTT_SUB_TOPIC_TRACKER_OPERATION,bms_tracker_action_from_platform,0},
	{BMS_TRACKER_MQTT_SUB_TOPIC_LIBSAFE_OTA,NULL,0},
	{BMS_TRACKER_MQTT_SUB_TOPIC_CONFIG,NULL,0},
	{BMS_TRACKER_MQTT_SUB_TOPIC_EXTRA,NULL,0}
};


static unsigned lodepng_crc32_table[256] = {
           0u, 1996959894u, 3993919788u, 2567524794u,  124634137u, 1886057615u, 3915621685u, 2657392035u,
   249268274u, 2044508324u, 3772115230u, 2547177864u,  162941995u, 2125561021u, 3887607047u, 2428444049u,
   498536548u, 1789927666u, 4089016648u, 2227061214u,  450548861u, 1843258603u, 4107580753u, 2211677639u,
   325883990u, 1684777152u, 4251122042u, 2321926636u,  335633487u, 1661365465u, 4195302755u, 2366115317u,
   997073096u, 1281953886u, 3579855332u, 2724688242u, 1006888145u, 1258607687u, 3524101629u, 2768942443u,
   901097722u, 1119000684u, 3686517206u, 2898065728u,  853044451u, 1172266101u, 3705015759u, 2882616665u,
   651767980u, 1373503546u, 3369554304u, 3218104598u,  565507253u, 1454621731u, 3485111705u, 3099436303u,
   671266974u, 1594198024u, 3322730930u, 2970347812u,  795835527u, 1483230225u, 3244367275u, 3060149565u,
  1994146192u,   31158534u, 2563907772u, 4023717930u, 1907459465u,  112637215u, 2680153253u, 3904427059u,
  2013776290u,  251722036u, 2517215374u, 3775830040u, 2137656763u,  141376813u, 2439277719u, 3865271297u,
  1802195444u,  476864866u, 2238001368u, 4066508878u, 1812370925u,  453092731u, 2181625025u, 4111451223u,
  1706088902u,  314042704u, 2344532202u, 4240017532u, 1658658271u,  366619977u, 2362670323u, 4224994405u,
  1303535960u,  984961486u, 2747007092u, 3569037538u, 1256170817u, 1037604311u, 2765210733u, 3554079995u,
  1131014506u,  879679996u, 2909243462u, 3663771856u, 1141124467u,  855842277u, 2852801631u, 3708648649u,
  1342533948u,  654459306u, 3188396048u, 3373015174u, 1466479909u,  544179635u, 3110523913u, 3462522015u,
  1591671054u,  702138776u, 2966460450u, 3352799412u, 1504918807u,  783551873u, 3082640443u, 3233442989u,
  3988292384u, 2596254646u,   62317068u, 1957810842u, 3939845945u, 2647816111u,   81470997u, 1943803523u,
  3814918930u, 2489596804u,  225274430u, 2053790376u, 3826175755u, 2466906013u,  167816743u, 2097651377u,
  4027552580u, 2265490386u,  503444072u, 1762050814u, 4150417245u, 2154129355u,  426522225u, 1852507879u,
  4275313526u, 2312317920u,  282753626u, 1742555852u, 4189708143u, 2394877945u,  397917763u, 1622183637u,
  3604390888u, 2714866558u,  953729732u, 1340076626u, 3518719985u, 2797360999u, 1068828381u, 1219638859u,
  3624741850u, 2936675148u,  906185462u, 1090812512u, 3747672003u, 2825379669u,  829329135u, 1181335161u,
  3412177804u, 3160834842u,  628085408u, 1382605366u, 3423369109u, 3138078467u,  570562233u, 1426400815u,
  3317316542u, 2998733608u,  733239954u, 1555261956u, 3268935591u, 3050360625u,  752459403u, 1541320221u,
  2607071920u, 3965973030u, 1969922972u,   40735498u, 2617837225u, 3943577151u, 1913087877u,   83908371u,
  2512341634u, 3803740692u, 2075208622u,  213261112u, 2463272603u, 3855990285u, 2094854071u,  198958881u,
  2262029012u, 4057260610u, 1759359992u,  534414190u, 2176718541u, 4139329115u, 1873836001u,  414664567u,
  2282248934u, 4279200368u, 1711684554u,  285281116u, 2405801727u, 4167216745u, 1634467795u,  376229701u,
  2685067896u, 3608007406u, 1308918612u,  956543938u, 2808555105u, 3495958263u, 1231636301u, 1047427035u,
  2932959818u, 3654703836u, 1088359270u,  936918000u, 2847714899u, 3736837829u, 1202900863u,  817233897u,
  3183342108u, 3401237130u, 1404277552u,  615818150u, 3134207493u, 3453421203u, 1423857449u,  601450431u,
  3009837614u, 3294710456u, 1567103746u,  711928724u, 3020668471u, 3272380065u, 1510334235u,  755167117u
};

/*Return the CRC of the bytes buf[0..len-1].*/
unsigned lodepng_crc32(const unsigned char* data, size_t length)
{
  unsigned r = 0xffffffffu;
  size_t i;
  for(i = 0; i < length; ++i)
  {
    r = lodepng_crc32_table[(r ^ data[i]) & 0xff] ^ (r >> 8);
  }
  return r ^ 0xffffffffu;
}


int get_tracker_base_info(void)
{
	int i_op_status = -1;

	if(tacker_base_message != NULL)
	{
		return i_op_status;
	}

	tacker_base_message = (BMS_BASIC_INFO_T *)LSAPI_OSI_Malloc(sizeof(BMS_BASIC_INFO_T));
	if(tacker_base_message == NULL)
		return i_op_status;

	tacker_base_message->thingId = (char *)LSAPI_OSI_Malloc(64);
	if(tacker_base_message->thingId == NULL)
	{
		free(tacker_base_message);
		tacker_base_message = NULL;
		return i_op_status;
	}

	tacker_base_message->model = (char *)LSAPI_OSI_Malloc(64);
	if(tacker_base_message->model == NULL)
	{
		free(tacker_base_message->thingId);
		tacker_base_message->thingId = NULL;
		free(tacker_base_message);
		tacker_base_message = NULL;
		return i_op_status;
	}
	tacker_base_message->manufacture = (char *)LSAPI_OSI_Malloc(64);
	if(tacker_base_message->manufacture == NULL)
	{
		free(tacker_base_message->thingId);
		tacker_base_message->thingId = NULL;
		free(tacker_base_message->model);
		tacker_base_message->model = NULL;
		free(tacker_base_message);
		tacker_base_message = NULL;
		return i_op_status;
	}
	tacker_base_message->imei = (char *)LSAPI_OSI_Malloc(64);
	if(tacker_base_message->imei == NULL)
	{
		free(tacker_base_message->thingId);
		tacker_base_message->thingId = NULL;
		free(tacker_base_message->model);
		tacker_base_message->model = NULL;
		free(tacker_base_message->manufacture);
		tacker_base_message->manufacture = NULL;
		free(tacker_base_message);
		tacker_base_message = NULL;
		return i_op_status;
	}

	tacker_base_message->imsi = (char *)LSAPI_OSI_Malloc(64);
	if(tacker_base_message->imsi == NULL)
	{
		free(tacker_base_message->thingId);
		tacker_base_message->thingId = NULL;
		free(tacker_base_message->model);
		tacker_base_message->model = NULL;
		free(tacker_base_message->manufacture);
		tacker_base_message->manufacture = NULL;
		free(tacker_base_message->imei);
		tacker_base_message->imei = NULL;
		free(tacker_base_message);
		tacker_base_message = NULL;
		return i_op_status;
	}
	tacker_base_message->mobile = (char *)LSAPI_OSI_Malloc(64);
	if(tacker_base_message->mobile == NULL)
	{
		free(tacker_base_message->thingId);
		tacker_base_message->thingId = NULL;
		free(tacker_base_message->model);
		tacker_base_message->model = NULL;
		free(tacker_base_message->manufacture);
		tacker_base_message->manufacture = NULL;
		free(tacker_base_message->imei);
		tacker_base_message->imei = NULL;
		free(tacker_base_message->imsi);
		tacker_base_message->imsi = NULL;
		free(tacker_base_message);
		tacker_base_message = NULL;
		return i_op_status;
	}
	tacker_base_message->trackerHardwareVersion = (char *)LSAPI_OSI_Malloc(64);
	if(tacker_base_message->trackerHardwareVersion == NULL)
	{
		free(tacker_base_message->thingId);
		tacker_base_message->thingId = NULL;
		free(tacker_base_message->model);
		tacker_base_message->model = NULL;
		free(tacker_base_message->manufacture);
		tacker_base_message->manufacture = NULL;
		free(tacker_base_message->imei);
		tacker_base_message->imei = NULL;
		free(tacker_base_message->imsi);
		tacker_base_message->imsi = NULL;
		free(tacker_base_message->mobile);
		tacker_base_message->mobile = NULL;
		free(tacker_base_message);
		tacker_base_message = NULL;
		return i_op_status;
	}
	
	tacker_base_message->trackerSoftwareVersion = (char *)LSAPI_OSI_Malloc(64);
	if(tacker_base_message->trackerSoftwareVersion == NULL)
	{
		free(tacker_base_message->thingId);
		tacker_base_message->thingId = NULL;
		free(tacker_base_message->model);
		tacker_base_message->model = NULL;
		free(tacker_base_message->manufacture);
		tacker_base_message->manufacture = NULL;
		free(tacker_base_message->imei);
		tacker_base_message->imei = NULL;
		free(tacker_base_message->imsi);
		tacker_base_message->imsi = NULL;
		free(tacker_base_message->mobile);
		tacker_base_message->mobile = NULL;
		free(tacker_base_message->trackerHardwareVersion);
		tacker_base_message->trackerHardwareVersion = NULL;
		free(tacker_base_message);
		tacker_base_message = NULL;
		return i_op_status;
	}
	memset(tacker_base_message->imei,0x00,64);
	memset(tacker_base_message->mobile,0x00,64);
	memset(tacker_base_message->imsi,0x00,64);
	memset(tacker_base_message->manufacture,0x00,64);
	memset(tacker_base_message->model,0x00,64);
	memset(tacker_base_message->thingId,0x00,64);
	memset(tacker_base_message->trackerHardwareVersion,0x00,64);
	memset(tacker_base_message->trackerSoftwareVersion,0x00,64);
	
	LSAPI_SIM_GetIMEI(tacker_base_message->imei);
	LSAPI_Log_Debug("get_tracker_base_info imei is =%s\n", tacker_base_message->imei);
	LSAPI_SIM_GetICCID(tacker_base_message->mobile);
	LSAPI_Log_Debug("get_tracker_base_info mobile is =%s\n", tacker_base_message->mobile);
	LSAPI_SIM_GetIMSI(tacker_base_message->imsi);
	LSAPI_Log_Debug("get_tracker_base_info imsi is =%s\n", tacker_base_message->imsi);
	strcpy(tacker_base_message->manufacture,"ZG_BMS");
	LSAPI_Log_Debug("get_tracker_base_info manufacture is =%s\n", tacker_base_message->manufacture);
	strcpy(tacker_base_message->model,"tracker");
	LSAPI_Log_Debug("get_tracker_base_info model is =%s\n", tacker_base_message->model);
	strcpy(tacker_base_message->thingId,"tracker001");
	LSAPI_Log_Debug("get_tracker_base_info thingId is =%s\n", tacker_base_message->thingId);
	strcpy(tacker_base_message->trackerHardwareVersion,"0011");
	LSAPI_Log_Debug("get_tracker_base_info trackerHardwareVersion is =%s\n", tacker_base_message->trackerHardwareVersion);
	strcpy(tacker_base_message->trackerSoftwareVersion,"0011");
	LSAPI_Log_Debug("get_tracker_base_info trackerSoftwareVersion is =%s\n", tacker_base_message->trackerSoftwareVersion);
	tacker_base_message->timestamp = LSAPI_OSI_EpochTime();
	LSAPI_Log_Debug("get_tracker_base_info timestamp is =%d\n", tacker_base_message->timestamp);

	return 0;
}


int bms_PdpActive()
{
    LSAPI_OSI_Event_t event = {0};
	unsigned res = 0;
	uint8_t nState = 1;
    uint8_t nCid = 1;
	uint8_t nSim = 0;
	char *apn = "ctnet";
	res = LSAPI_NET_SetPdpcont(nCid, (uint8_t *)apn, NULL, NULL);
	LSAPI_Log_Debug("LSAPI_NET_SetPdpcont nCID=%d,nRet=%d\n", nCid, res);
    /* 1. active PDP begin */
	res = LSAPI_NET_GprsAct(nState, nCid, nSim, &event);
	LSAPI_Log_Debug("LSAPI_NET_GprsAct event.id(%d)", event.id);
	LSAPI_Log_Debug("LSAPI_NET_GprsAct res(%d)", res);
	if (1 == res)
	{
	    LSAPI_Log_Debug("LSAPI_NET_GprsAct cid(%d) active succ", nCid);
	}
	else
	{
        LSAPI_Log_Debug("LSAPI_NET_GprsAct cid(%d) active fail", nCid);
	}
    return res;
    /* 1. active PDP end */
	
	/* 2. deactive PDP begin */
	/* 2. deactive PDP end */
}

int LSAPI_NET_CGACT(void) 
{
	int nAttstate = 0; 
	int nActstate = 0;
	uint32_t nRet = 0;
	nRet = LSAPI_NET_GetGprsAttState(&nAttstate);
    do{
        LSAPI_Log_Debug("LSAPI_NET_CGACT:check att status\n");
		LSAPI_OSI_ThreadSleep(2000);
        nRet = LSAPI_NET_GetGprsAttState(&nAttstate);

    }while(nAttstate != 1);
    LSAPI_Log_Debug("LSAPI_NET_CGACT:nAttstate = %d\n",nAttstate);
	//LSAPI_Log_Debug("LS LSAPI_NET_SetPdpcont nCID=%d,nRet=%d\n", nCID, nRet);
	if(nRet != 0)
    {
        LSAPI_Log_Debug("LS LSAPI_NET_SetPdpcont failed\n");
        return -1;
    }   

	nRet = LSAPI_NET_GetGprsActState(&nActstate);
	if (nActstate != 1)
	{
	    bms_PdpActive();
		
	}
	else
	{
		LSAPI_Log_Debug("LS GprsActState has been ACTIVED");
	}
	if (nRet != 0)
	{
	    LSAPI_Log_Debug("LS LSAPI_NET_GetGprsActState failed\n");
		return -1;
	}
	return 0;

}

int32_t bms_payload_encrypte(const char *src,char *dest,uint16_t size)
{
	uint16_t num = 0;
	uint16_t data_len = 0;
	uint16_t stream_len = 0;
	uint16_t padding = 0;
	uint16_t cnt = 0;
	uint16_t encrypt_len = 0;
	uint16_t total_encrpt_length = 0;
	
	char stream_buf[512] = {0};
	char *encrypt_buf = dest;

	memset(stream_buf,0,512);
	memcpy(stream_buf,src,size);
	stream_len = size;
	num = stream_len % 16;
	
	total_encrpt_length = size;
	
	padding = 16 - num;
	for(cnt = 0; cnt < padding; cnt++)
	{
		*(stream_buf + stream_len + cnt) = padding;
	}

	total_encrpt_length = total_encrpt_length + padding;

	num = (total_encrpt_length / 16);

     for(cnt = 0;cnt < num; cnt ++)
     {
           if(!LSAPI_AES_EcbMode(stream_buf+cnt*16,encrypt_buf+cnt*16,key,16,1))
           {
              LSAPI_Log_Debug("bms_payload_encrypte Encrypted error");
              return 0;  
           }
     }
	for(cnt = 0;cnt < total_encrpt_length;cnt ++)
	{
		LSAPI_Log_Debug("payload_encrypt_buf[%d]=%02x",cnt,encrypt_buf[cnt]);
	} 
	return total_encrpt_length;
}


int32_t bms_payload_decrypte(const char *src,char *dest,uint16_t size)
{
	uint16_t num = 0;
	uint16_t cnt = 0;
	uint16_t data_len = 0;
	uint16_t encrypt_len = 0;
	uint16_t padding = 0;

	char *encrypt_buf = src;
	char *decrypt_buf = dest;
	if((src == NULL) || (dest == NULL))
		return 0;

	encrypt_len = size;
	num = encrypt_len % 16;
	if(num)
	{
		LSAPI_Log_Debug("bms_payload_decrypte aes data is wrong!");
		return -1;
	}
	
	num = encrypt_len/16;
	for(cnt = 0;cnt < num; cnt++)
	{
		if(!LSAPI_AES_EcbMode(encrypt_buf+cnt*16,decrypt_buf+cnt*16,key,16,0))
		   {
			  LSAPI_Log_Debug("LSAPI bms_payload_decrypte decrypted error");
			  return 0;  
		   }
	}
#if 0
	for(cnt = 0; cnt < encrypt_len;cnt++)
	{
		LSAPI_Log_Debug("bms_encrypt[%d]=0x%02x",cnt,decrypt_buf[cnt]);
	}
#endif
	padding = decrypt_buf[encrypt_len - 1];
	LSAPI_Log_Debug("LSAPI bms_payload_decrypte padding is %d",padding);
	return padding > 16 ? 0 : encrypt_len - padding;
}


int bms_tracker_query_from_platform(char *payload,int len)
{
	MeassageData msg = MeassageData_init_zero;
	
	char encrypt_buf[512] = {0};
	char decrypt_buf[512] = {0};
	uint16_t encrypt_len = 0;
	uint16_t decrypt_len = 0;
	uint16_t num = 0;
	uint16_t cnt = 0;
	uint16_t data_len = 0;
	char thingId[64] = {0};
	int32_t code;
	int32_t any;
	int64_t timestamp; 
	bool base;
	bool realtime;
	bool log;
	bool config;
	bool bat;
	bool status = false;

	LSAPI_OSI_Event_t event = {};
	
	LSAPI_Log_Debug("bms_tracker_query_from_platform payload is %s",payload); 
	LSAPI_Log_Debug("bms_tracker_query_from_platform payload len is %d",len); 

	data_len = (payload[0] << 8 | payload[1]);
	LSAPI_Log_Debug("bms_tracker_query_from_platform,data len is %d",data_len);
	
	encrypt_len = data_len;
	memcpy(encrypt_buf,payload + 2,encrypt_len);

	decrypt_len = bms_payload_decrypte(encrypt_buf,decrypt_buf,encrypt_len);
	LSAPI_Log_Debug("bms_tracker_query_from_platform,decrypt_len is %d",decrypt_len);
		
	pb_istream_t is = pb_istream_from_buffer(decrypt_buf,decrypt_len);
	if(!(pb_decode(&is,MeassageData_fields,&msg)))
	{
		LSAPI_Log_Debug("bms_baseinfo_replay_from_platform pb_decode error"); 
		return -1;
	}
	timestamp = msg.header.timestamp;
	strncpy(thingId,msg.header.thingId,64);
	code = msg.code;

	if (strcmp(msg.data.type_url, "type.googleapis.com/TrackerQuery") == 0)
    {
        TrackerQuery trackerquery = TrackerQuery_init_zero;
        is = pb_istream_from_buffer(msg.data.value.bytes, msg.data.value.size);
        status = pb_decode(&is, TrackerQuery_fields, &trackerquery);
        base = trackerquery.base;
		realtime = trackerquery.realtime;
		log = trackerquery.log;
		config = trackerquery.config;
		bat = trackerquery.bat;
    }
	LSAPI_Log_Debug("bms_tracker_query_from_platform ota base is %d,realtime is %d,log is %d,config is %d,bat is %d", 
		base,realtime,log,config,bat);

	if(base || realtime || bat)
	{
		int64_t timestamp = LSAPI_OSI_EpochTime(); 
		if(bms_safeevent_timer_t != NULL)
			LSAPI_OSI_TimerStop(bms_safeevent_timer_t);
		if(base)
		{
			bms_post_baseinfo_to_platform();
			LSAPI_OSI_ThreadSleep(200);
		}
		if(realtime)
		{
			if(bms_realtime_timer_t != NULL)
			{
				LSAPI_OSI_TimerStop(bms_realtime_timer_t);
				bms_post_realtime_info_to_platform(timestamp);
				LSAPI_OSI_TimerStart(bms_realtime_timer_t,10000);
			}
			else
				bms_post_realtime_info_to_platform(timestamp);
			LSAPI_OSI_ThreadSleep(200);
		}
		if(bat)
		{
			if(bms_realtime_timer_t != NULL)
			{
				LSAPI_OSI_TimerStop(bms_realtime_timer_t);
				bms_post_cellbat_info_to_platform(timestamp);
				LSAPI_OSI_TimerStart(bms_realtime_timer_t,10000);
			}
			else
			{
				bms_post_cellbat_info_to_platform(timestamp);
			}
			LSAPI_OSI_ThreadSleep(200);
		}
		if(bms_safeevent_timer_t != NULL)
			LSAPI_OSI_TimerStart(bms_safeevent_timer_t,1000);
	}

	event.id = BMS_MQTT_PUB_QUERY_REPLY_ID;
	LSAPI_OSI_EvnetSend(bms_mqtt_thread, &event);
	
	return 0;
}

int bms_bms_action_from_platform(char *payload,int len)
{
	MeassageData msg = MeassageData_init_zero;
	
	char encrypt_buf[512] = {0};
	char decrypt_buf[512] = {0};
	uint16_t encrypt_len = 0;
	uint16_t decrypt_len = 0;
	uint16_t num = 0;
	uint16_t cnt = 0;
	uint16_t data_len = 0;
	char thingId[64] = {0};
	int32_t code;
	int32_t any;
	int64_t timestamp; 
	uint8_t action;
	int32_t value;
	bool status = false;

	LSAPI_OSI_Event_t event = {};
	
	LSAPI_Log_Debug("bms_bms_action_from_platform payload is %s",payload); 
	LSAPI_Log_Debug("bms_bms_action_from_platform payload len is %d",len); 

	data_len = (payload[0] << 8 | payload[1]);
	LSAPI_Log_Debug("bms_bms_action_from_platform,data len is %d",data_len);
	
	encrypt_len = data_len;
	memcpy(encrypt_buf,payload + 2,encrypt_len);

	decrypt_len = bms_payload_decrypte(encrypt_buf,decrypt_buf,encrypt_len);
	LSAPI_Log_Debug("bms_bms_action_from_platform,decrypt_len is %d",decrypt_len);
		
	pb_istream_t is = pb_istream_from_buffer(decrypt_buf,decrypt_len);
	if(!(pb_decode(&is,MeassageData_fields,&msg)))
	{
		LSAPI_Log_Debug("bms_bms_action_from_platform pb_decode error"); 
		return -1;
	}

	timestamp = msg.header.timestamp;
	strncpy(thingId,msg.header.thingId,64);
	code = msg.code;

	if (strcmp(msg.data.type_url, "type.googleapis.com/BmsOperation") == 0)
    {
        BmsOperation pbdata = BmsOperation_init_zero;
        is = pb_istream_from_buffer(msg.data.value.bytes, msg.data.value.size);
        status = pb_decode(&is, BmsOperation_fields, &pbdata);
        action = pbdata.action;
		value = pbdata.value;
    }
	
	LSAPI_Log_Debug("bms_bms_action_from_platform action is %d,value is %d", action,value);
	event.id = BMS_MQTT_PUB_BMS_OPERATION_REPLY_ID;
	LSAPI_OSI_EvnetSend(bms_mqtt_thread, &event);
	return 0;
}

int bms_tracker_action_from_platform(char *payload,int len)
{
	MeassageData msg = MeassageData_init_zero;
	
	char encrypt_buf[512] = {0};
	char decrypt_buf[512] = {0};
	uint16_t encrypt_len = 0;
	uint16_t decrypt_len = 0;
	uint16_t num = 0;
	uint16_t cnt = 0;
	uint16_t data_len = 0;
	char thingId[64] = {0};
	int32_t code;
	int32_t any;
	int64_t timestamp; 
	bool turnOffBms;
	bool turnOffTracker;
	bool status =false;

	LSAPI_OSI_Event_t event = {};
		
	LSAPI_Log_Debug("bms_tracker_action_from_platform payload is %s",payload); 
	LSAPI_Log_Debug("bms_tracker_action_from_platform payload len is %d",len); 

	data_len = (payload[0] << 8 | payload[1]);
	LSAPI_Log_Debug("bms_tracker_action_from_platform,data len is %d",data_len);
	
	encrypt_len = data_len;
	memcpy(encrypt_buf,payload + 2,encrypt_len);

	decrypt_len = bms_payload_decrypte(encrypt_buf,decrypt_buf,encrypt_len);
	LSAPI_Log_Debug("bms_tracker_action_from_platform,decrypt_len is %d",decrypt_len);
		
	pb_istream_t is = pb_istream_from_buffer(decrypt_buf,decrypt_len);
	if(!(pb_decode(&is,MeassageData_fields,&msg)))
	{
		LSAPI_Log_Debug("bms_tracker_action_from_platform pb_decode error"); 
		return -1;
	}
	timestamp = msg.header.timestamp;
	strncpy(thingId,msg.header.thingId,64);
	code = msg.code;
	LSAPI_Log_Debug("bms_tracker_action_from_platform ota timestamp is %lld,code is %d", timestamp,code);

	if (strcmp(msg.data.type_url, "type.googleapis.com/TrackerOperation") == 0)
    {
    	LSAPI_Log_Debug("111bms_tracker_action_from_platform ota turnOffBms is %d,turnOffTracker is %d", turnOffBms,turnOffTracker);
        TrackerOperation pbdata = TrackerOperation_init_zero;
        is = pb_istream_from_buffer(msg.data.value.bytes, msg.data.value.size);
        status = pb_decode(&is, TrackerOperation_fields, &pbdata);
		turnOffBms = pbdata.turnOffBms;
		turnOffTracker = pbdata.turnOffTracker;
		LSAPI_Log_Debug("222bms_tracker_action_from_platform ota turnOffBms is %d,turnOffTracker is %d", turnOffBms,turnOffTracker);
    }
	
	LSAPI_Log_Debug("333bms_tracker_action_from_platform ota turnOffBms is %d,turnOffTracker is %d", turnOffBms,turnOffTracker);
	event.id = BMS_MQTT_PUB_TRACKER_OPERATION_REPLY_ID;
	LSAPI_OSI_EvnetSend(bms_mqtt_thread, &event);
	if(turnOffBms)
	{
		//LSAPI_OSI_Event_t event = {};
		///event.id = BMS_SND_ACTION_FROM_PLATFORM_ID;
		//event.param1 = turnOffBms;
		// LSAPI_OSI_EvnetSend(bms_thread_event, &event);
	}
	if(turnOffTracker)
	{
		//LSAPI_SYS_PowerOff(); 
	}	

	return 0;
}

uint8_t *exclamationmark_mov(uint8_t cnt,uint8_t *buff)
{
	uint8_t *mv = buff;
	uint8_t i = 0;
	for(i = 0; i < cnt; i++)
	{
		if(*mv == '\0')
			return NULL;
		while(*mv++ != '!');
	}
	return mv;
}

int16_t OTA_url_get(uint8_t * line)
{
	if(line == NULL)
		return -1;
	
	uint16_t length = 0;
	int16_t result = 0;
	uint8_t *buf = line;
	uint8_t *mv = buf;
	uint8_t *mv_pre = buf;
	uint8_t mv_cnt = 0;
	uint8_t cnt = 0;
	
	while(mv[cnt] != '\0')
	{
		if(mv[cnt++] == '!')
			mv_cnt++;
	}
	if(mv_cnt == 0)
	{
		memset(tracker_ota_url_1,0,sizeof(tracker_ota_url_1));
		memcpy(tracker_ota_url_1,line,strlen(line));
		LSAPI_Log_Debug("OTA_url_get url1 is %s",tracker_ota_url_1);
		return 0;
	}
	else
	{
		memset(tracker_ota_url_1,0,sizeof(tracker_ota_url_1));
		memset(tracker_ota_url_2,0,sizeof(tracker_ota_url_2));
		mv_pre = exclamationmark_mov(1,buf);
		length = mv_pre - mv -1;
		memcpy(tracker_ota_url_1,mv,length);

		mv = mv_pre;
		if(mv_cnt == 1)
		{
			memcpy(tracker_ota_url_2,mv,strlen(mv));
		}
		else
		{
			mv_pre = exclamationmark_mov(2,buf);
			length = mv_pre - mv -1;
			memcpy(tracker_ota_url_2,mv,length);
		}
	}
	LSAPI_Log_Debug("OTA_url_get url1 is %s,url2 is %s",tracker_ota_url_1,tracker_ota_url_2);
	return result;
}


int16_t OTA_info_check(uint8_t * line, uint8_t * check_line)
{
	if(line == NULL)
		return -1;
	
	uint16_t length = 0;
	int16_t result = -1;
	uint8_t *buf = line;
	uint8_t *mv = buf;
	uint8_t *mv_pre = buf;
	uint8_t mv_buf[64] = {0};
	uint8_t mv_cnt = 0;
	uint8_t cnt = 0;

	memset(mv_buf,0,64);
	
	while(mv[cnt] != '\0')
	{
		if(mv[cnt++] == '!')
			mv_cnt++;
	}
	LSAPI_Log_Debug("OTA_info_check mv_cnt is %d",mv_cnt); 
	if(mv_cnt == 0)
	{
		if(!strncmp(check_line,line,strlen(line)))
		{
			result = 0;			
		}
		return result;
	}
	for(cnt = 0; cnt < mv_cnt; cnt++)
	{		
		mv_pre = exclamationmark_mov(cnt + 1, buf);
		LSAPI_Log_Debug("OTA_info_check  mv_pre is %s",mv_pre); 
		if(mv_pre == NULL)
		{	
			length = strlen(mv) - 1;
		}
		else
			length = mv_pre - mv - 1;
		memcpy(mv_buf,mv,length);
		if(!strncmp(check_line,mv_buf,length))
		{
			result = 0;	
			return result;
		}
		mv = mv_pre;
	}
	if(cnt == mv_cnt)
	{
		LSAPI_Log_Debug("OTA_info_check  mv is %s",mv); 
		length = strlen(mv);
		memcpy(mv_buf,mv,length);
		if(!strncmp(check_line,mv_buf,length))
		{
			result = 0;	
			return result;
		}
	}
	return result;
}

LSAPI_OSI_Thread_t*bms_http_fota = NULL;
uint8_t ota_pac_md5[64] = {0};
void bms_fotaEntry(void *param)
{

    HttpInfoSet httpinfo;
    uint32_t writeLen = 20;	
    uint32_t PostTotal;
    uint32_t i;
    int fd;
    int write_num = 0;
    int length = 0;
    int last_breakpoint = 0;
    uint32_t current_breakpoint_count = 1;
    int request_count = 0;
    uint32_t breakpoint_number;
    memset(&httpinfo,0,sizeof(HttpInfoSet));    
#if 1
    //GETn
    strcpy(httpinfo.url, tracker_ota_url_1);
    httpinfo.cid = 1;
    uint32_t readLen = 100 * 1024;
    uint32_t contentLen=LSAPI_HTTP_GetFileLen(&httpinfo);
    LSAPI_Log_Debug("LSAPI_HTTP_GET contentLen %d,httpinfo.StatusCode = %d\n",contentLen,httpinfo.StatusCode);
    //delete last fota.pack
    fupdateInvalidate(true);
    vfs_mkdir(CONFIG_FS_FOTA_DATA_DIR,0);
    fd = LSAPI_FS_Open(FUPDATE_PACK_FILE_NAME,LSAPI_FS_O_RDWR|LSAPI_FS_O_CREAT|LSAPI_FS_O_TRUNC,0x0);
    if(fd > 0)
    {
        LSAPI_Log_Debug("fd create success FUPDATE_PACK_FILE_NAME = %s\n",FUPDATE_PACK_FILE_NAME);

    }
    else
    {
        LSAPI_Log_Debug("fd < 0 failed\n");    

    }
    breakpoint_number = contentLen / readLen;
    LSAPI_Log_Debug("LSAPI_HTTP_GET breakpoint_number = %d\n",breakpoint_number);
    while(1)
    {
        
       if (contentLen > readLen)
       {
            if(last_breakpoint == current_breakpoint_count)
            {
                request_count++;
                LSAPI_Log_Debug("LSAPI_HTTP_GET requset count = %d\n",request_count);
            }
            if(current_breakpoint_count == breakpoint_number + 1)
            {
                LSAPI_Log_Debug("LSAPI_HTTP_GET breakpoint_count = %d,count = %d\n",current_breakpoint_count,breakpoint_number);
                if(readLen * breakpoint_number != contentLen)
                {
                    httpinfo.BREAK = breakpoint_number * readLen;
                    httpinfo.BREAKEND = contentLen - 1;
                    length = httpinfo.BREAKEND - httpinfo.BREAK + 1;
                }
                else
                {
                    LSAPI_Log_Debug("LSAPI_HTTP_GET download finish\n");
                    LSAPI_FS_Close(fd);
                    fd = -1;
                    break;
                }
            }
            else
            {
                LSAPI_Log_Debug("LSAPI_HTTP_GET else\n");
                httpinfo.BREAK = (current_breakpoint_count-1) * readLen;
                httpinfo.BREAKEND = current_breakpoint_count * readLen - 1;
                length = httpinfo.BREAKEND - httpinfo.BREAK + 1; 
            }
            LSAPI_Log_Debug("LSAPI_HTTP_GET break count = %d\n",current_breakpoint_count);
            last_breakpoint = current_breakpoint_count;
            if(LSAPI_HTTP_GET(&httpinfo))
            {
                LSAPI_Log_Debug("LSAPI_HTTP_GET error,please check reason");
                break;
                
            }
            else
            {
                if(httpinfo.StatusCode == 206)
                {
                    write_num =  LSAPI_FS_Write(fd,httpinfo.RecvData,length); 
                    LSAPI_Log_Debug("LSAPI_HTTP_GET %d,len %d,write_num:%d",current_breakpoint_count,httpinfo.recvlen,write_num);
                    if(write_num != length)
                    {
                        LSAPI_Log_Debug("LSAPI_HTTP_GET write file failed\n");
                        break;
                    }
                    //ÏÂÔØÍê³É
                    if(current_breakpoint_count == breakpoint_number + 1)
                    {
                        
                        LSAPI_Log_Debug("LSAPI_HTTP_GET all data write to file\n");
                        LSAPI_FS_Close(fd);
                        fd = -1;
                        break;
                    }
                    current_breakpoint_count++;
                }
                else
                {
                    LSAPI_Log_Debug("LSAPI_HTTP_GET failed statuscode = %d",httpinfo.StatusCode);
                    LSAPI_OSI_ThreadSleep(500);                        
                }

            }           

       }
       else
       {
            LSAPI_Log_Debug("LSAPI_HTTP_GET not breakpoint\n");
            if(LSAPI_HTTP_GET(&httpinfo))
            {
                LSAPI_Log_Debug("LSAPI_HTTP_GET error,please check reason\n");
                break;
                
            }
            else
            {
                if(httpinfo.StatusCode == 200)
                {
                    write_num =  LSAPI_FS_Write(fd,httpinfo.RecvData,httpinfo.recvlen); //log
                    LSAPI_Log_Debug("LSAPI_HTTP_GET len %d,write_num = %d",httpinfo.recvlen,write_num);
                    if(write_num != httpinfo.recvlen)
                    {
                        LSAPI_Log_Debug("write file failed,please check reason\n");
                    }
                    break;
                }
                else
                {
                    //get request again
                    request_count++;
                    LSAPI_Log_Debug("LSAPI_HTTP_GET failed statuscode = %d",httpinfo.StatusCode);
                }
            }
        }
        if(httpinfo.RecvData)
        {
            free(httpinfo.RecvData);
            httpinfo.RecvData = NULL;
        }
       if(request_count > 20)
       {
            LSAPI_Log_Debug("http get requset response statuscode error,check network status or server reason\n");
            LSAPI_FS_Close(fd);
            fd = -1;
            break;   
       }

    }
    if(httpinfo.RecvData)
    {
        free(httpinfo.RecvData);
        httpinfo.RecvData = NULL;
    }
    LSAPI_HTTP_ParaClear(&httpinfo);
    if(fd > 0)
    {
        LSAPI_FS_Close(fd);
        fd = -1;

    }
	uint8_t md5_ptr[32] = {0};
	memset(md5_ptr,0,32);
	lsapi_file_md5(FUPDATE_PACK_FILE_NAME,md5_ptr);
	LSAPI_Log_Debug("LSAPI_FOTA_Upgrade: md5 check is %s\n",md5_ptr);
	if(strncasecmp(ota_pac_md5,md5_ptr,32))
	{
		LSAPI_Log_Debug("LSAPI_FOTA_Upgrade: md5 check fail");
		LSAPI_OSI_ThreadExit();
	}
#if 1
    LSAPI_Log_Debug("LSAPI_FOTA_Upgrade: start\n");
    if (!fupdateSetReady(NULL))
    {
        LSAPI_Log_Debug("LSAPI_FOTA_Upgrade: invalid pack\n");
		LSAPI_OSI_ThreadExit();
    }
    else
    {
        LSAPI_Log_Debug("fota example module will restart\n");
        LSAPI_SYS_reboot();   
    }
    #endif
#endif

    for(;;)
    {
        LSAPI_OSI_ThreadSleep(3000);

    }

}

void tracker_ota_start(void)
{
    bms_http_fota  = LSAPI_OSI_ThreadCreate("bms_fota_Task", bms_fotaEntry,NULL,LSAPI_OSI_PRIORITY_NORMAL,1024*10,4);
    if(NULL == bms_http_fota)
    {
        LSAPI_Log_Debug("http threadid full\n");
        return;
    }
    else
    {
        LSAPI_Log_Debug("http threadid not null threadid = 0x%x\n",bms_http_fota);

    }
        return;

}


int bms_tracker_ota_from_platform(char *payload,int len)
{
	MeassageData msg = MeassageData_init_zero;

	char encrypt_buf[512] = {0};
	char decrypt_buf[512] = {0};
	uint16_t encrypt_len = 0;
	uint16_t decrypt_len = 0;
	uint16_t num = 0;
	uint16_t cnt = 0;
	uint16_t data_len = 0;
	char thingId[64] = {0};
	int32_t code;
	int32_t any;
	int64_t timestamp; 
	uint8_t ota_pac_model[64] = {0};
	uint8_t ota_pac_manufacture[64] = {0};
	uint8_t ota_pac_hardwareVersion[64] = {0};
	uint8_t ota_pac_softwareVersion[64] = {0};
	uint8_t ota_pac_url[64] = {0};
	
	int32_t ota_pac_size = 0;
	bool status = false;
	LSAPI_OSI_Event_t event = {};
	
	LSAPI_Log_Debug("bms_tracker_ota_from_platform payload is %s",payload); 
	LSAPI_Log_Debug("bms_tracker_ota_from_platform payload len is %d",len); 

	data_len = (payload[0] << 8 | payload[1]);
	LSAPI_Log_Debug("bms_tracker_ota_from_platform,data len is %d",data_len);
	
	encrypt_len = data_len;
	memcpy(encrypt_buf,payload + 2,encrypt_len);

	decrypt_len = bms_payload_decrypte(encrypt_buf,decrypt_buf,encrypt_len);
	LSAPI_Log_Debug("bms_tracker_ota_from_platform,decrypt_len is %d",decrypt_len);
		
	pb_istream_t is = pb_istream_from_buffer(decrypt_buf,decrypt_len);
	if(!(pb_decode(&is,MeassageData_fields,&msg)))
	{
		LSAPI_Log_Debug("bms_tracker_ota_from_platform pb_decode error"); 
		return -1;
	}
	timestamp = msg.header.timestamp;
	strncpy(thingId,msg.header.thingId,64);
	code = msg.code;

	if (strcmp(msg.data.type_url, "type.googleapis.com/OtaAction") == 0)
    {
        OtaAction pbdata = OtaAction_init_zero;
        is = pb_istream_from_buffer(msg.data.value.bytes, msg.data.value.size);
        status = pb_decode(&is, OtaAction_fields, &pbdata);
		strncpy(ota_pac_hardwareVersion,pbdata.trackerOTA.hardwareVersion,64);
		strncpy(ota_pac_softwareVersion,pbdata.trackerOTA.softwareVersion,64);
		strncpy(ota_pac_manufacture,pbdata.trackerOTA.manufacture,64);
		strncpy(ota_pac_model,pbdata.trackerOTA.model,64);
		strncpy(ota_pac_url,pbdata.trackerOTA.url,64);
		strncpy(ota_pac_md5,pbdata.trackerOTA.md5,64);
		ota_pac_size = pbdata.trackerOTA.size;
    }
	
	LSAPI_Log_Debug("bms_tracker_ota_from_platform ota url is %s,size is %d", ota_pac_url,ota_pac_size);
	LSAPI_Log_Debug("bms_tracker_ota_from_platform ota supprot manufacture is %s,model is %s", ota_pac_manufacture,ota_pac_model);
	LSAPI_Log_Debug("bms_tracker_ota_from_platform ota supprot hardwareVersion is %s,softwareVersion is %s", ota_pac_hardwareVersion,ota_pac_softwareVersion);


	OTA_url_get(ota_pac_url);
	
	event.id = BMS_MQTT_PUB_TRACKER_OTA_REPLY_ID;
	LSAPI_OSI_EvnetSend(bms_mqtt_thread, &event);
	if(!OTA_info_check(ota_pac_hardwareVersion,tacker_base_message->trackerHardwareVersion) && 
		!OTA_info_check(ota_pac_softwareVersion,tacker_base_message->trackerSoftwareVersion) && 
		!OTA_info_check(ota_pac_model,tacker_base_message->model) && 
		!OTA_info_check(ota_pac_manufacture,tacker_base_message->manufacture))
	{
		LSAPI_Log_Debug("bms_tracker_ota_from_platform ota info check success");		
		tracker_ota_start();
	}
	else
		LSAPI_Log_Debug("bms_tracker_ota_from_platform ota info check error");
	return 0;
}


int bms_baseinfo_replay_from_platform(char *payload,int len)
{	
	MeassageData msg = MeassageData_init_zero;
	
	char encrypt_buf[512] = {0};
	char decrypt_buf[512] = {0};
	uint16_t encrypt_len = 0;
	uint16_t decrypt_len = 0;
	uint16_t num = 0;
	uint16_t cnt = 0;
	uint16_t data_len = 0;
	char thingId[64] = {0};
	int32_t code;
	int32_t any;
	int64_t timestamp; 
	LSAPI_Log_Debug("bms_baseinfo_replay_from_platform payload is %s",payload); 
	LSAPI_Log_Debug("bms_baseinfo_replay_from_platform payload len is %d",len); 

	data_len = (payload[0] << 8 | payload[1]);
	LSAPI_Log_Debug("bms_baseinfo_post_from_self,data len is %d",data_len);
	
	encrypt_len = data_len;
	memcpy(encrypt_buf,payload + 2,encrypt_len);

	decrypt_len = bms_payload_decrypte(encrypt_buf,decrypt_buf,encrypt_len);
	LSAPI_Log_Debug("bms_baseinfo_replay_from_platform,decrypt_len is %d",decrypt_len);
		
	pb_istream_t is = pb_istream_from_buffer(decrypt_buf,decrypt_len);
	if(!(pb_decode(&is,MeassageData_fields,&msg)))
	{
		LSAPI_Log_Debug("bms_baseinfo_replay_from_platform pb_decode error"); 
		return -1;
	}
	timestamp = msg.header.timestamp;
	strncpy(thingId,msg.header.thingId,64);
	code = msg.code;

	LSAPI_Log_Debug("bms_baseinfo_replay_from_platform thingId is %s,timestamp is %lld,code is %d", thingId,timestamp,code);

	return 0;
}
int test_aes_encrypt(void)
{
	char test_info[64] = {0x0A,0x13,0x0A,0x0A,0x74,0x72,0x61,0x63,0x6B,0x65,0x72,0x30,0x30,0x31,0x10,0xDD,0xBF,0x97,0xD5,0x85,0x30,0x18,0xC8,0x01,0x00};
	char test_decrypt_info[33] = {0x61,0xC4,0xA9,0x53,0xA6,0xF4,0x5D,0x62,0x1D,0x72,0xB5,0x4A,0x5C,0x43,0x7D,0x8D,0x5A,0x51,0xE2,0x00,0xF7,0xB9,0xAC,0x25,0xB6,0x26,0x35,0xAB,0x51,0x45,0x67,0x96,0x00};
	char test_encrypt_ptr[128] = {0};
	char test_decrypt_ptr[128] = {0};
	int buf_len = 0;
	uint16_t num = 0; 
	uint8_t cnt = 0; 
	uint16_t i = 0;
	buf_len = strlen(test_info);
	LSAPI_Log_Debug("test_aes_encrypt test info len is %d",buf_len);
	num = buf_len % 16;
	if(num)
	{
		cnt = 16 - num;
		for(i = 0; i < cnt; i++)
		{
			test_info[buf_len + i] = cnt;
		}
	}
	for(i = 0; i < buf_len + cnt; i++)
 	{
 		LSAPI_Log_Debug("%02x",test_info[i]);
 	}
	buf_len = buf_len + cnt;
	LSAPI_Log_Debug("test_aes_encrypt total_buf_length is %d",buf_len);
	num = (buf_len / 16);
	LSAPI_Log_Debug("test_aes_encrypt num is %d",num);
     for(i=0;i<num; i++)
     {
           if(!LSAPI_AES_EcbMode(test_info+i*16,test_encrypt_ptr+i*16,key,16,1))
           {
              LSAPI_Log_Debug("LSAPI LSAPI_AES_EcbMode Encrypted error");
              return 0;  
           }
     }
	 for(i = 0; i < buf_len; i++)
 	{
 		LSAPI_Log_Debug("encrypted[%d] = %02x",i,test_encrypt_ptr[i]);
 	}
	for(i = 0; i < buf_len; i++)
 	{
 		if(test_encrypt_ptr[i] != test_decrypt_info[i])
		{
			LSAPI_Log_Debug("LSAPI test i = %d,a = %02x,b=%02x",i,test_encrypt_ptr[i],test_decrypt_info[i]);
		}
 	}
	 num = 2;
	for(i = 0;i < num; i++)
	{
		if(!LSAPI_AES_EcbMode(test_encrypt_ptr+i*16,test_decrypt_ptr+i*16,key,16,0))
           {
              LSAPI_Log_Debug("LSAPI LSAPI_AES_EcbMode decrypted error");
              return 0;  
           }
	}
	 for(i = 0; i < buf_len; i++)
 	{
 		LSAPI_Log_Debug("111dncrypted[%d] = %02x",i,test_decrypt_ptr[i]);
 	}
	 LSAPI_Log_Debug("test_aes_encrypt test decrypt");
	
	 memset(test_decrypt_ptr,0,128);
	 num = 2;
	for(i = 0;i < num; i++)
	{
		if(!LSAPI_AES_EcbMode(test_decrypt_info+i*16,test_decrypt_ptr+i*16,key,16,0))
           {
              LSAPI_Log_Debug("LSAPI LSAPI_AES_EcbMode decrypted error");
              return 0;  
           }
	}
	for(i = 0; i < buf_len; i++)
 	{
 		LSAPI_Log_Debug("222decrypted[%d] = %02x",i,test_decrypt_ptr[i]);
 	}
	num = 2;
	for(i = 0;i < num; i++)
	{
		if(!LSAPI_AES_EcbMode(test_decrypt_info+i*16,test_decrypt_ptr+i*16,key,16,0))
           {
              LSAPI_Log_Debug("LSAPI LSAPI_AES_EcbMode decrypted error");
              return 0;  
           }
	}
	for(i = 0; i < buf_len; i++)
 	{
 		LSAPI_Log_Debug("333decrypted[%d] = %02x",i,test_decrypt_ptr[i]);
 	}
	return 0;
}

static bool write_repeated_CellVol(pb_ostream_t *stream, const pb_field_t *field, void * const *arg)
{
	uint16_t i = 0;
	for(i = 0; i < 16;i++)
	{
		pb_encode_tag_for_field(stream, field);
        pb_encode_varint(stream, (intptr_t)*arg + i);
	}
   return 1;
}

static bool write_repeated_Temp(pb_ostream_t *stream, const pb_field_t *field, void * const *arg)
{
	uint16_t i = 0;
	for(i = 0; i < 4;i++)
	{
		pb_encode_tag_for_field(stream, field);
        pb_encode_varint(stream, (intptr_t)*arg + i);
	}
   return 1;
}

int bms_post_ota_to_tracker_for_test(void)
{
	char stream_buf[500] = {0};
	size_t stream_size = 1024;
	char payload_buf[512] = {0};
	char payload_encrypt_buf[512] = {0};
	char *payload_ptr = NULL;
	uint16_t total_length = 0;
	uint16_t total_encrpt_length = 0;
	uint16_t cnt = 0;
	
	memset(stream_buf,0x00,500);
	memset(payload_buf,0x00,512);

	MeassageData msg = MeassageData_init_zero;
	FirmwareInfo tarckerOta = FirmwareInfo_init_zero;
	
	msg.header.timestamp = LSAPI_OSI_EpochTime();
	strcpy(msg.header.thingId,tacker_base_message->thingId);
	msg.code = 200;
	
	strcpy(tarckerOta.model,"DTU!tracker!sensor");
	strcpy(tarckerOta.manufacture,"ZG_BMS!MT_BMS!ELM_BMS");
	strcpy(tarckerOta.hardwareVersion,"0013!0012!0011");
	strcpy(tarckerOta.softwareVersion,"0013!0012!0011");
	strcpy(tarckerOta.url,"http://117.131.85.142:60031/output.pac!192.168.1.1");
	strcpy(tarckerOta.md5,"4baba750130029b21cb7d158096a36e4");
	tarckerOta.size = 200;
	strncpy(msg.data.type_url,"type.googleapis.com/FirmwareInfo", sizeof(msg.data.type_url));

	pb_ostream_t stream = pb_ostream_from_buffer(msg.data.value.bytes, sizeof(msg.data.value.bytes));
	pb_encode(&stream, FirmwareInfo_fields, &tarckerOta);
	
	msg.data.value.size = stream.bytes_written;
	for(cnt = 0;cnt < msg.data.value.size; cnt++)
	{
		LSAPI_Log_Debug("ota_event[%d]=%02x\n", cnt,msg.data.value.bytes[cnt]);
	}
	stream = pb_ostream_from_buffer(stream_buf, sizeof(stream_buf));
	pb_encode(&stream, MeassageData_fields, &msg);
	
	total_length = stream.bytes_written;
	
	LSAPI_Log_Debug("bms_MqttEntry protobuf is: %s total len is %d\n", stream_buf,total_length);
	
	total_encrpt_length = bms_payload_encrypte(stream_buf,payload_encrypt_buf,total_length);
	
	payload_ptr = payload_buf;
	*payload_ptr++ = ((total_encrpt_length & 0xff00) >> 8);
	*payload_ptr++ = (total_encrpt_length & 0x00ff);

	
	memcpy(payload_ptr,payload_encrypt_buf,total_encrpt_length);
	
	int32_t crc32_val = lodepng_crc32(payload_buf,total_encrpt_length + 2);
	LSAPI_Log_Debug("bms_MqttEntry crc32 is %08x\n", crc32_val); 

	payload_ptr = payload_ptr + total_encrpt_length;
	*payload_ptr++ = ((crc32_val & 0xff000000) >> 24);
	*payload_ptr++ = ((crc32_val & 0x00ff0000) >> 16);
	*payload_ptr++ = ((crc32_val & 0x0000ff00) >> 8);
	*payload_ptr++ = (crc32_val & 0x000000ff);
	LSAPI_Log_Debug("bms_post_baseinfo_to_platform payload_ptr is %s ",payload_buf);

	LSAPI_MQTT_CFG("topic",BMS_TRACKER_MQTT_SUB_TOPIC_TRACKER_OTA);
	LSAPI_MQTT_CFG2("message",payload_buf,total_encrpt_length + 2 + 4);
	LSAPI_MQTT_PUB(1);
	LSAPI_OSI_ThreadSleep(200);
	return 0;
}

int bms_post_ota_reply_to_platform(void)
{

	char stream_buf[500] = {0};

	size_t stream_size = 1024;
	char payload_buf[512] = {0};
	char payload_encrypt_buf[512] = {0};
	char *payload_ptr = NULL;
	uint16_t total_length = 0;
	uint16_t total_encrpt_length = 0;

	memset(stream_buf,0x00,500);
	memset(payload_buf,0x00,512);

	MeassageData msg = MeassageData_init_zero;
	OtaResponseData otaResponse = BatInfo_init_zero;

	otaResponse.reason = 0;
	otaResponse.result = 1;
	strcpy(otaResponse.slaveThingIds,"tracker001");
	
	//msg.data.has_value = true;
	//msg.data.has_type_url = true;
	pb_ostream_t stream = pb_ostream_from_buffer(msg.data.value.bytes, sizeof(msg.data.value.bytes));
	bool status = pb_encode(&stream, OtaResponseData_fields, &otaResponse);

	msg.data.value.size = stream.bytes_written;
	
	stream = pb_ostream_from_buffer(stream_buf, sizeof(stream_buf));
	pb_encode(&stream, MeassageData_fields, &msg);

	
    total_length = stream.bytes_written;
	
	LSAPI_Log_Debug("bms_MqttEntry protobuf is: %s total len is %d\n", stream_buf,total_length);

	total_encrpt_length = bms_payload_encrypte(stream_buf,payload_encrypt_buf,total_length);
	
	payload_ptr = payload_buf;
	*payload_ptr++ = ((total_encrpt_length & 0xff00) >> 8);
	*payload_ptr++ = (total_encrpt_length & 0x00ff);

	
	memcpy(payload_ptr,payload_encrypt_buf,total_encrpt_length);
	
	int32_t crc32_val = lodepng_crc32(payload_buf,total_encrpt_length + 2);
	LSAPI_Log_Debug("bms_MqttEntry crc32 is %08x\n", crc32_val); 

	payload_ptr = payload_ptr + total_encrpt_length;
	*payload_ptr++ = ((crc32_val & 0xff000000) >> 24);
	*payload_ptr++ = ((crc32_val & 0x00ff0000) >> 16);
	*payload_ptr++ = ((crc32_val & 0x0000ff00) >> 8);
	*payload_ptr++ = (crc32_val & 0x000000ff);
	LSAPI_Log_Debug("bms_post_ota_reply_to_platform payload_ptr is %s ",payload_buf);

	LSAPI_MQTT_CFG("topic",BMS_TRACKER_MQTT_PUB_TOPIC_OTA_INFO);
	LSAPI_MQTT_CFG2("message",payload_buf,total_encrpt_length + 2 + 4);
	LSAPI_MQTT_PUB(1);
	LSAPI_OSI_ThreadSleep(200);

	return 0;
}


int bms_post_cellbat_info_to_platform(int64_t value)
{
	char stream_buf[500] = {0};
	size_t stream_size = 1024;
	char payload_buf[512] = {0};
	char payload_encrypt_buf[512] = {0};
	char *payload_ptr = NULL;
	uint16_t total_length = 0;
	uint16_t total_encrpt_length = 0;
	uint16_t cnt = 0;
	memset(stream_buf,0x00,500);
	memset(payload_buf,0x00,512);

	MeassageData msg = MeassageData_init_zero;
	BatInfo batinfo = BatInfo_init_zero;

	//LSAPI_Log_Debug("bms_post_cellbat_info_to_platform timestamp is %lld,code is %d\n", tacker_base_message->timestamp,tacker_base_message->code);

	strcpy(msg.header.thingId,"tracker001");
	msg.header.timestamp = value;
	msg.code = 0;

	LSAPI_Log_Debug("bms_post_cellbat_info_to_platform timestamp is %lld\n", msg.header.timestamp);
	strncpy(msg.data.type_url,"type.googleapis.com/BatInfo", sizeof(msg.data.type_url));
	batinfo.cellVol_count = 16;
	for(cnt = 0;cnt < 16; cnt++)
	{
		batinfo.cellVol[cnt] = 3917;
	}
	batinfo.batteryTemp_count = 3;
	for(cnt = 0;cnt < 3; cnt++)
	{
		batinfo.batteryTemp[cnt] = 40+cnt;
	}
	//msg.data.has_value = true;
	//msg.data.has_type_url = true;
	pb_ostream_t stream = pb_ostream_from_buffer(msg.data.value.bytes, sizeof(msg.data.value.bytes));
	bool status = pb_encode(&stream, BatInfo_fields, &batinfo);

	msg.data.value.size = stream.bytes_written;

	for(cnt = 0;cnt < msg.data.value.size; cnt++)
	{
		LSAPI_Log_Debug("bat_info[%d]=%02x\n", cnt,msg.data.value.bytes[cnt]);
	}
	
	stream = pb_ostream_from_buffer(stream_buf, sizeof(stream_buf));
	pb_encode(&stream, MeassageData_fields, &msg);
	
	total_length = stream.bytes_written;
	
	LSAPI_Log_Debug("bms_post_cellbat_info_to_platform	protobuf is: %s total len is %d\n", stream_buf,total_length);
	
	for(cnt = 0; cnt < total_length; cnt++)
	{
		LSAPI_Log_Debug("bat_info_protobuf[%d]=%02x",cnt,stream_buf[cnt]);
	}
	total_encrpt_length = bms_payload_encrypte(stream_buf,payload_encrypt_buf,total_length);
	
	payload_ptr = payload_buf;
	*payload_ptr++ = ((total_encrpt_length & 0xff00) >> 8);
	*payload_ptr++ = (total_encrpt_length & 0x00ff);

	
	memcpy(payload_ptr,payload_encrypt_buf,total_encrpt_length);
	
	int32_t crc32_val = lodepng_crc32(payload_buf,total_encrpt_length + 2);
	LSAPI_Log_Debug("bms_MqttEntry crc32 is %08x\n", crc32_val);

	payload_ptr = payload_ptr + total_encrpt_length;
	*payload_ptr++ = ((crc32_val & 0xff000000) >> 24);
	*payload_ptr++ = ((crc32_val & 0x00ff0000) >> 16);
	*payload_ptr++ = ((crc32_val & 0x0000ff00) >> 8);
	*payload_ptr++ = (crc32_val & 0x000000ff);
	LSAPI_Log_Debug("bms_post_cellbat_info_to_platform payload_ptr is %s ",payload_buf);

	LSAPI_MQTT_CFG("topic",BMS_TRACKER_MQTT_PUB_TOPIC_BAT_INFO);
	LSAPI_MQTT_CFG2("message",payload_buf,total_encrpt_length + 2 + 4);
	LSAPI_MQTT_PUB(1);
	LSAPI_OSI_ThreadSleep(200);

	return 0;
}

int bms_post_realtime_info_to_platform(int64_t value)
{
	char stream_buf[500] = {0};
	size_t stream_size = 1024;
	char payload_buf[512] = {0};
	char payload_encrypt_buf[512] = {0};
	char *payload_ptr = NULL;
	uint16_t total_length = 0;
	uint16_t total_encrpt_length = 0;

	memset(stream_buf,0x00,500);
	memset(payload_buf,0x00,512);
	
	MeassageData msg = MeassageData_init_zero;
	TrackerRealTimeInfo trackrealtimeinfo = TrackerRealTimeInfo_init_zero;

	msg.header.timestamp = value;
	strcpy(msg.header.thingId,tacker_base_message->thingId);
	msg.code = 0;

	LSAPI_Log_Debug("bms_post_realtime_info_to_platform timestamp is %lld\n", msg.header.timestamp);
#if 0
	LSPAI_OSI_MutexLock(bms_RealtimeInfoMutex);	
	pbmsgdata.data.batteryInfo.batteryVoltage = tracker_realtime_info.batteryInfo.batteryVoltage;
	pbmsgdata.data.batteryInfo.batteryCurrent = tracker_realtime_info.batteryInfo.batteryCurrent;
	pbmsgdata.data.batteryInfo.batterySoc = tracker_realtime_info.batteryInfo.batterySoc;
	strcpy(pbmsgdata.data.batteryInfo.batteryHardVersion,tracker_realtime_info.batteryInfo.batteryHardVersion);
	strcpy(pbmsgdata.data.batteryInfo.batterySoftVersion,tracker_realtime_info.batteryInfo.batterySoftVersion);
	pbmsgdata.data.batteryInfo.batteryWorkMode = tracker_realtime_info.batteryInfo.batteryWorkMode;
	pbmsgdata.data.batteryInfo.batteryProtectCode = tracker_realtime_info.batteryInfo.batteryProtectCode;
	pbmsgdata.data.batteryInfo.batteryErrorCode = tracker_realtime_info.batteryInfo.batteryErrorCode;
	pbmsgdata.data.batteryInfo.batteryTemperatureMax = tracker_realtime_info.batteryInfo.batteryTemperatureMax;
	pbmsgdata.data.batteryInfo.batteryVoltageMax = tracker_realtime_info.batteryInfo.batteryVoltageMax;
	pbmsgdata.data.batteryInfo.batteryVoltageMin = tracker_realtime_info.batteryInfo.batteryVoltageMin;
	pbmsgdata.data.batteryInfo.mosStatus = tracker_realtime_info.batteryInfo.mosStatus;
	pbmsgdata.data.batteryInfo.chargeCycleTime = tracker_realtime_info.batteryInfo.chargeCycleTime;

	pbmsgdata.data.gpsLocationInfo.accuracy = tracker_realtime_info.gpsLocationInfo.accuracy;
	pbmsgdata.data.gpsLocationInfo.satelliteNum = tracker_realtime_info.gpsLocationInfo.satelliteNum;
	pbmsgdata.data.gpsLocationInfo.gpsSignal = tracker_realtime_info.gpsLocationInfo.gpsSignal;
	pbmsgdata.data.gpsLocationInfo.gpsSpeed = tracker_realtime_info.gpsLocationInfo.gpsSpeed;
	pbmsgdata.data.gpsLocationInfo.latitudeDirection = tracker_realtime_info.gpsLocationInfo.latitudeDirection;
	pbmsgdata.data.gpsLocationInfo.latitude = tracker_realtime_info.gpsLocationInfo.latitude;
	pbmsgdata.data.gpsLocationInfo.longitudeDirection = tracker_realtime_info.gpsLocationInfo.longitudeDirection;
	pbmsgdata.data.gpsLocationInfo.longitude = tracker_realtime_info.gpsLocationInfo.longitude;
	pbmsgdata.data.gpsLocationInfo.locationMode = tracker_realtime_info.gpsLocationInfo.locationMode;
	pbmsgdata.data.gpsLocationInfo.networkType = tracker_realtime_info.gpsLocationInfo.networkType;
	pbmsgdata.data.gpsLocationInfo.csq = tracker_realtime_info.gpsLocationInfo.csq;
	pbmsgdata.data.gpsLocationInfo.detStatus = tracker_realtime_info.gpsLocationInfo.detStatus;
	pbmsgdata.data.gpsLocationInfo.reportReason = tracker_realtime_info.gpsLocationInfo.reportReason;
	LSAPI_OSI_MutexUnlock(bms_RealtimeInfoMutex);	
#else
	trackrealtimeinfo.batteryInfo.batteryVoltage = 50;
	trackrealtimeinfo.batteryInfo.batteryCurrent = 12;
	trackrealtimeinfo.batteryInfo.batterySoc = 80;
	strcpy(trackrealtimeinfo.batteryInfo.batteryHardVersion,"0045");
	strcpy(trackrealtimeinfo.batteryInfo.batterySoftVersion,"0031");
	trackrealtimeinfo.batteryInfo.batteryWorkMode = 0;
	trackrealtimeinfo.batteryInfo.batteryProtectCode = 0;
	trackrealtimeinfo.batteryInfo.batteryErrorCode = 0;
	trackrealtimeinfo.batteryInfo.batteryTemperatureMax = 12;
	trackrealtimeinfo.batteryInfo.batteryVoltageMax = 4200;
	trackrealtimeinfo.batteryInfo.batteryVoltageMin = 3900;
	trackrealtimeinfo.batteryInfo.mosStatus = 0;
	trackrealtimeinfo.batteryInfo.chargeCycleTime = 0;

	trackrealtimeinfo.gpsLocationInfo.accuracy = 3;
	trackrealtimeinfo.gpsLocationInfo.satelliteNum = 21;
	trackrealtimeinfo.gpsLocationInfo.gpsSignal = 41;
	trackrealtimeinfo.gpsLocationInfo.gpsSpeed = 5;
	trackrealtimeinfo.gpsLocationInfo.latitudeDirection = 0x78;
	trackrealtimeinfo.gpsLocationInfo.latitude = 2503.71465;
	trackrealtimeinfo.gpsLocationInfo.longitudeDirection = 0x69;
	trackrealtimeinfo.gpsLocationInfo.longitude = 12138.73922;
	trackrealtimeinfo.gpsLocationInfo.locationMode = 0;
	trackrealtimeinfo.gpsLocationInfo.networkType = 4;
	trackrealtimeinfo.gpsLocationInfo.csq = 31;
	trackrealtimeinfo.gpsLocationInfo.detStatus = 0;
	trackrealtimeinfo.gpsLocationInfo.reportReason = 0;
#endif
	//msg.data.has_value = true;
	//msg.data.has_type_url = true;
	strncpy(msg.data.type_url,"type.googleapis.com/TrackerRealTimeInfo", sizeof(msg.data.type_url));
	
	pb_ostream_t stream = pb_ostream_from_buffer(msg.data.value.bytes, sizeof(msg.data.value.bytes));
	bool status = pb_encode(&stream, TrackerRealTimeInfo_fields, &trackrealtimeinfo);

	msg.data.value.size = stream.bytes_written;
	
	stream = pb_ostream_from_buffer(stream_buf, sizeof(stream_buf));
	pb_encode(&stream, MeassageData_fields, &msg);
    total_length = stream.bytes_written;
	
	LSAPI_Log_Debug("bms_post_realtime_info_to_platform protobuf is: %s total len is %d\n", stream_buf,total_length);
	total_encrpt_length = bms_payload_encrypte(stream_buf,payload_encrypt_buf,total_length);
	
	payload_ptr = payload_buf;
	*payload_ptr++ = ((total_encrpt_length & 0xff00) >> 8);
	*payload_ptr++ = (total_encrpt_length & 0x00ff);

	
	memcpy(payload_ptr,payload_encrypt_buf,total_encrpt_length);
	
	int32_t crc32_val = lodepng_crc32(payload_buf,total_encrpt_length + 2);
	LSAPI_Log_Debug("bms_post_realtime_info_to_platform crc32 is %08x\n", crc32_val); 

	payload_ptr = payload_ptr + total_encrpt_length;
	*payload_ptr++ = ((crc32_val & 0xff000000) >> 24);
	*payload_ptr++ = ((crc32_val & 0x00ff0000) >> 16);
	*payload_ptr++ = ((crc32_val & 0x0000ff00) >> 8);
	*payload_ptr++ = (crc32_val & 0x000000ff);
	LSAPI_Log_Debug("bms_post_realtime_info_to_platform payload_ptr is %s ",payload_buf);

	LSAPI_MQTT_CFG("topic",BMS_TRACKER_MQTT_PUB_TOPIC_REALTIME_INFO);
	LSAPI_MQTT_CFG2("message",payload_buf,total_encrpt_length + 2 + 4);
	LSAPI_MQTT_PUB(1);
	LSAPI_OSI_ThreadSleep(200);

	return 0;
}

int bms_post_safe_event_to_paltfrom(void)
{
	char stream_buf[500] = {0};
	size_t stream_size = 1024;
	char payload_buf[512] = {0};
	char payload_encrypt_buf[512] = {0};
	char *payload_ptr = NULL;
	uint16_t total_length = 0;
	uint16_t total_encrpt_length = 0;
	uint16_t cnt = 0;
	
	memset(stream_buf,0x00,500);
	memset(payload_buf,0x00,512);

	MeassageData msg = MeassageData_init_zero;
	EventRequestData eventRequestData = EventRequestData_init_zero;
	//EventRequestData *pbs = &eventRequestData;

	BatterySafeEvent *pbs = &(eventRequestData.batterySafeEvent);
	
	msg.header.timestamp = LSAPI_OSI_EpochTime();
	strcpy(msg.header.thingId,tacker_base_message->thingId);
	msg.code = 200;
	
#if 1
	eventRequestData.batterySafeEvent.batteryVoltage = 6000;

	eventRequestData.batterySafeEvent.batteryCurrent = 1000;
	eventRequestData.batterySafeEvent.batterySoc = 80;
	eventRequestData.batterySafeEvent.batteryAh = 250;

	eventRequestData.batterySafeEvent.cellV_count = 16;
	for(cnt = 0;cnt < 16; cnt++)
	{
		eventRequestData.batterySafeEvent.cellV[cnt] = 3917;
	}
	eventRequestData.eventTime = LSAPI_OSI_EpochTime();


	//int32_t cellV[11] = {10,3917,3917,3917,3917,3917,3917,3917,3917,3917,3917};
	//eventRequestData.batterySafeEvent.faultCellID3.funcs.encode = &write_repeated_varint;
	//eventRequestData.batterySafeEvent.faultCellID3.arg = (void*)&cellV;

	strncpy(msg.data.type_url,"type.googleapis.com/EventRequestData", sizeof(msg.data.type_url));
#else
	int32_t batteryVoltage = 3917;
	
	int32_t batteryCurrent = 1000;
	int32_t batterySoc = 80;
	int32_t batteryAh = 250;
	int64_t eventTime = msg.header.timestamp;
	cellV_t cellV[4] = {{3917},{3917},{3917},{3917}};

	PB_ENC_ARRAY(cellV, cellV,testEncode);
	PB_ENC_ASSIGN(batteryVoltage, batteryVoltage);
	PB_ENC_ASSIGN(batteryCurrent, batteryCurrent);
	PB_ENC_ASSIGN(batterySoc, batterySoc);
	PB_ENC_ASSIGN(batteryAh, batteryAh);
#endif

	pb_ostream_t stream = pb_ostream_from_buffer(msg.data.value.bytes, sizeof(msg.data.value.bytes));
	pb_encode(&stream, EventRequestData_fields, &eventRequestData);
	
	msg.data.value.size = stream.bytes_written;
	for(cnt = 0;cnt < msg.data.value.size; cnt++)
	{
		LSAPI_Log_Debug("safe_event[%d]=%02x\n", cnt,msg.data.value.bytes[cnt]);
	}
	stream = pb_ostream_from_buffer(stream_buf, sizeof(stream_buf));
	pb_encode(&stream, MeassageData_fields, &msg);
	
	total_length = stream.bytes_written;
	
	LSAPI_Log_Debug("bms_MqttEntry protobuf is: %s total len is %d\n", stream_buf,total_length);
	
	total_encrpt_length = bms_payload_encrypte(stream_buf,payload_encrypt_buf,total_length);
	
	payload_ptr = payload_buf;
	*payload_ptr++ = ((total_encrpt_length & 0xff00) >> 8);
	*payload_ptr++ = (total_encrpt_length & 0x00ff);

	
	memcpy(payload_ptr,payload_encrypt_buf,total_encrpt_length);
	
	int32_t crc32_val = lodepng_crc32(payload_buf,total_encrpt_length + 2);
	LSAPI_Log_Debug("bms_MqttEntry crc32 is %08x\n", crc32_val); 

	payload_ptr = payload_ptr + total_encrpt_length;
	*payload_ptr++ = ((crc32_val & 0xff000000) >> 24);
	*payload_ptr++ = ((crc32_val & 0x00ff0000) >> 16);
	*payload_ptr++ = ((crc32_val & 0x0000ff00) >> 8);
	*payload_ptr++ = (crc32_val & 0x000000ff);
	LSAPI_Log_Debug("bms_post_baseinfo_to_platform payload_ptr is %s ",payload_buf);

	LSAPI_MQTT_CFG("topic",BMS_TRACKER_MQTT_PUB_TOPIC_EVENT_INFO);
	LSAPI_MQTT_CFG2("message",payload_buf,total_encrpt_length + 2 + 4);
	LSAPI_MQTT_PUB(1);
	LSAPI_OSI_ThreadSleep(200);
	return 0;
}

int bms_post_reply_to_platform(int32_t reply_topic)
{
	char stream_buf[500] = {0};
	size_t stream_size = 1024;
	char payload_buf[512] = {0};
	char payload_encrypt_buf[512] = {0};
	char *payload_ptr = NULL;
	uint16_t total_length = 0;
	uint16_t total_encrpt_length = 0;

	memset(stream_buf,0x00,500);
	memset(payload_buf,0x00,512);

	LSAPI_Log_Debug("bms_post_reply_to_platform reply_topic is: %d\n",reply_topic);
	MeassageData msg = MeassageData_init_zero;

	msg.header.timestamp = LSAPI_OSI_EpochTime();
	strcpy(msg.header.thingId,tacker_base_message->thingId);
	msg.code = 200;
		
	pb_ostream_t stream = pb_ostream_from_buffer(stream_buf, sizeof(stream_buf));
	pb_encode(&stream, MeassageData_fields, &msg);
	
	total_length = stream.bytes_written;
	
	LSAPI_Log_Debug("bms_MqttEntry protobuf is: %s total len is %d\n", stream_buf,total_length);
	
	total_encrpt_length = bms_payload_encrypte(stream_buf,payload_encrypt_buf,total_length);
	
	payload_ptr = payload_buf;
	*payload_ptr++ = ((total_encrpt_length & 0xff00) >> 8);
	*payload_ptr++ = (total_encrpt_length & 0x00ff);

	
	memcpy(payload_ptr,payload_encrypt_buf,total_encrpt_length);
	
	int32_t crc32_val = lodepng_crc32(payload_buf,total_encrpt_length + 2);
	LSAPI_Log_Debug("bms_MqttEntry crc32 is %08x\n", crc32_val); 

	payload_ptr = payload_ptr + total_encrpt_length;
	*payload_ptr++ = ((crc32_val & 0xff000000) >> 24);
	*payload_ptr++ = ((crc32_val & 0x00ff0000) >> 16);
	*payload_ptr++ = ((crc32_val & 0x0000ff00) >> 8);
	*payload_ptr++ = (crc32_val & 0x000000ff);
	LSAPI_Log_Debug("bms_post_reply_to_platform payload_ptr is %s ",payload_buf);

	switch(reply_topic)
	{
		LSAPI_Log_Debug("bms_post_reply_to_platform default topic is %d ",reply_topic);
		case BMS_MQTT_PUB_LIBSAFE_OTA_REPLY_ID:
		{
			LSAPI_MQTT_CFG("topic",BMS_TRACKER_MQTT_PUB_TOPIC_LIBSAFE_OTA_REPLY);
			LSAPI_MQTT_CFG2("message",payload_buf,total_encrpt_length + 2 + 4);
			LSAPI_MQTT_PUB(1);
			LSAPI_OSI_ThreadSleep(200);
		}
		break;
		case BMS_MQTT_PUB_TRACKER_OTA_REPLY_ID:
		{
			LSAPI_MQTT_CFG("topic",BMS_TRACKER_MQTT_PUB_TOPIC_TRACKER_OTA_REPLY);
			LSAPI_MQTT_CFG2("message",payload_buf,total_encrpt_length + 2 + 4);
			LSAPI_MQTT_PUB(1);
			LSAPI_OSI_ThreadSleep(200);
		}
		break;
		case BMS_MQTT_PUB_BMS_OPERATION_REPLY_ID:
		{
			LSAPI_MQTT_CFG("topic",BMS_TRACKER_MQTT_PUB_TOPIC_BMS_OPERATION_REPLY);
			LSAPI_MQTT_CFG2("message",payload_buf,total_encrpt_length + 2 + 4);
			LSAPI_MQTT_PUB(1);
			LSAPI_OSI_ThreadSleep(200);
		}
		break;
		case BMS_MQTT_PUB_TRACKER_OPERATION_REPLY_ID:
		{
			LSAPI_MQTT_CFG("topic",BMS_TRACKER_MQTT_PUB_TOPIC_TRACKER_OPERATION_REPLY);
			LSAPI_MQTT_CFG2("message",payload_buf,total_encrpt_length + 2 + 4);
			LSAPI_MQTT_PUB(1);
			LSAPI_OSI_ThreadSleep(200);
		}
		break;
		case BMS_MQTT_PUB_QUERY_REPLY_ID:
		{
			LSAPI_MQTT_CFG("topic",BMS_TRACKER_MQTT_PUB_TOPIC_QUERY_REPLY);
			LSAPI_MQTT_CFG2("message",payload_buf,total_encrpt_length + 2 + 4);
			LSAPI_MQTT_PUB(1);
			LSAPI_OSI_ThreadSleep(200);
		}
		break;
		default:
			break;
	}	
	return 0;
}

int bms_post_baseinfo_to_platform(void)
{
	char stream_buf[500] = {0};
	size_t stream_size = 1024;
	char payload_buf[512] = {0};
	char payload_encrypt_buf[512] = {0};
	char *payload_ptr = NULL;
	uint16_t total_length = 0;
	uint16_t total_encrpt_length = 0;

	memset(stream_buf,0x00,500);
	memset(payload_buf,0x00,512);

	MeassageData msg = MeassageData_init_zero;
	TrackerBaseInfo trackbaseinfo = TrackerBaseInfo_init_zero;

	msg.header.timestamp = LSAPI_OSI_EpochTime();
	strcpy(msg.header.thingId,tacker_base_message->thingId);
	msg.code = 200;
		
	strcpy(trackbaseinfo.model,tacker_base_message->model);
	strcpy(trackbaseinfo.manufacture,tacker_base_message->manufacture);
	strcpy(trackbaseinfo.imei,tacker_base_message->imei);
	strcpy(trackbaseinfo.imsi,tacker_base_message->imsi);
	strcpy(trackbaseinfo.mobile,tacker_base_message->mobile);
	strcpy(trackbaseinfo.trackerHardwareVersion,tacker_base_message->trackerHardwareVersion);
	strcpy(trackbaseinfo.trackerSoftwareVersion,tacker_base_message->trackerSoftwareVersion);

	//msg.data.has_value = true;
	//msg.data.has_type_url = true;
	strncpy(msg.data.type_url,"type.googleapis.com/TrackerBaseInfo", sizeof(msg.data.type_url));
	
	pb_ostream_t stream = pb_ostream_from_buffer(msg.data.value.bytes, sizeof(msg.data.value.bytes));
	bool status = pb_encode(&stream, TrackerBaseInfo_fields, &trackbaseinfo);

	msg.data.value.size = stream.bytes_written;
	
	stream = pb_ostream_from_buffer(stream_buf, sizeof(stream_buf));
	pb_encode(&stream, MeassageData_fields, &msg);
	
	total_length = stream.bytes_written;
	
	LSAPI_Log_Debug("bms_MqttEntry protobuf is: %s total len is %d\n", stream_buf,total_length);
	
	total_encrpt_length = bms_payload_encrypte(stream_buf,payload_encrypt_buf,total_length);
	
	payload_ptr = payload_buf;
	*payload_ptr++ = ((total_encrpt_length & 0xff00) >> 8);
	*payload_ptr++ = (total_encrpt_length & 0x00ff);

	
	memcpy(payload_ptr,payload_encrypt_buf,total_encrpt_length);
	
	int32_t crc32_val = lodepng_crc32(payload_buf,total_encrpt_length + 2);
	LSAPI_Log_Debug("bms_MqttEntry crc32 is %08x\n", crc32_val); 

	payload_ptr = payload_ptr + total_encrpt_length;
	*payload_ptr++ = ((crc32_val & 0xff000000) >> 24);
	*payload_ptr++ = ((crc32_val & 0x00ff0000) >> 16);
	*payload_ptr++ = ((crc32_val & 0x0000ff00) >> 8);
	*payload_ptr++ = (crc32_val & 0x000000ff);
	LSAPI_Log_Debug("bms_post_baseinfo_to_platform payload_ptr is %s ",payload_buf);

	LSAPI_MQTT_CFG("topic",BMS_TRACKER_MQTT_PUB_TOPIC_BASE_INFO);
	LSAPI_MQTT_CFG2("message",payload_buf,total_encrpt_length + 2 + 4);
	LSAPI_MQTT_PUB(1);
	LSAPI_OSI_ThreadSleep(200);
	
	return 0;
}

void bms_mqtt_realtime_timer_callback(void *param)
{
	LSAPI_OSI_Event_t event = {};
  	LSAPI_Log_Debug("=== bms_mqtt_realtime_timer_callback enter ====");

#ifndef REAL_BMS_DEBUG
	event.id = BMS_MQTT_PUB_REALTIME_INFO_ID;
	LSAPI_OSI_EvnetSend(bms_mqtt_thread, &event);
#else
	event.id = BMS_GET_BOARD_DYNAMIC_INFO_ID;
	LSAPI_OSI_EvnetSend(bms_thread_event, &event);
#endif

}

void bms_mqtt_batinfo_timer_callback(void *param)
{
	LSAPI_OSI_Event_t event = {};
	LSAPI_Log_Debug("=== bms_mqtt_batinfo_timer_callback enter ====");

#ifndef REAL_BMS_DEBUG

	event.id = BMS_MQTT_PUB_BATINFO_INFO_ID;
	LSAPI_OSI_EvnetSend(bms_mqtt_thread, &event);
#else
	event.id = BMS_GET_BOARD_DYNAMIC_INFO_ID;
	LSAPI_OSI_EvnetSend(bms_thread_event, &event);
#endif


}

void bms_mqtt_safe_event_timer_callback(void *param)
{
	LSAPI_OSI_Event_t event = {};
  	LSAPI_Log_Debug("=== bms_mqtt_safe_event_timer_callback enter ====");

#ifndef REAL_BMS_DEBUG
	event.id = BMS_MQTT_PUB_SAFE_EVENT_INFO_ID;
	LSAPI_OSI_EvnetSend(bms_mqtt_thread, &event);
#else
	event.id = BMS_GET_SAFE_EVENT_INFO_ID;
	LSAPI_OSI_EvnetSend(bms_thread_event, &event);
#endif

}



void bms_MqttEntry(void *param)
{
	int res = 0;
    LSAPI_Log_Debug("MqttEntry threadid = 0x%x\n",LSAPI_OSI_ThreadCurrent());
	
    LSAPI_MqttRegOpenThread(LSAPI_OSI_ThreadCurrent());
    LSAPI_NET_EventReg(LSAPI_OSI_ThreadCurrent(), BMS_START_MQTT_ID | MENU_MQTT_CONN_RSP | MENU_MQTT_CLOSE_RSP | MENU_MQTT_SUB_RSP |MENU_MQTT_UNSUB_RSP |MENU_MQTT_PUB_RSP |MENU_MQTT_DOWNLINK_MSG | BMS_MQTT_PUB_REALTIME_INFO_ID);
    for(;;)
    {
        LSAPI_OSI_Event_t event = {};
        // Wait for an event. No event will block. If the connection is successful, 
        //the module will send an event and the callback function will go in.
        LSAPI_OSI_EventWait(LSAPI_OSI_ThreadCurrent(), &event);
		LSAPI_Log_Debug("bms_MqttEntry,waitevenid = 0x%08x\n",event.id);
		switch(event.id)
		{
			case BMS_START_MQTT_ID:
			{
				uint8_t clientID[9] = {0};
				uint8_t host_name[49] = {0};
				uint8_t mqtt_username[21] = {0};
				uint8_t mqtt_port[7] = {0};
				uint8_t mqtt_passwd[17] = {0};
				uint16_t port = 0;
				get_tracker_base_info();

				memset(clientID,0,9);
				memset(host_name,0,49);
				memset(mqtt_username,0,21);
				memset(mqtt_port,0,7);
				memset(mqtt_passwd,0,17);

				memcpy(host_name,g_zg_mqtt_info.mqtt_addr,48);
				memcpy(clientID,g_zg_mqtt_info.devicesID,8);
				memcpy(mqtt_port,g_zg_mqtt_info.mqtt_port,6);
				memcpy(mqtt_username,g_zg_mqtt_info.mqtt_username,20);
				memcpy(mqtt_passwd,g_zg_mqtt_info.mqtt_passwd,16);

				port = atoi(mqtt_port);
			    /* cfg */
				LSAPI_MQTT_CFG("clientid",clientID);
				LSAPI_MQTT_CFG("username",mqtt_username);
				LSAPI_MQTT_CFG("password",mqtt_passwd);
				res = LSAPI_MQTT_OPEN(host_name,port,120);

			    if (res != 0)
			    {
			        LSAPI_Log_Debug("\n\r MQTT_OPEN... fail \n");
			        return;
			    }
				if(bms_realtime_timer_t == NULL)
				{
					bms_realtime_timer_t = LSAPI_OSI_TimerCreate(LSAPI_OSI_ThreadCurrent(), bms_mqtt_realtime_timer_callback, NULL);
					if(LSAPI_OSI_TimerStart(bms_realtime_timer_t, 10000) == FALSE)
				    {
				        LSAPI_Log_Debug("timer start failed\n");
				        LSAPI_OSI_ThreadExit();

				    }
				}
#if 0
				if(bms_batinfo_timer_t == NULL)
				{
					bms_batinfo_timer_t = LSAPI_OSI_TimerCreate(LSAPI_OSI_ThreadCurrent(), bms_mqtt_batinfo_timer_callback, NULL);
					if(LSAPI_OSI_TimerStart(bms_batinfo_timer_t, 10000) == FALSE)
				    {
				        LSAPI_Log_Debug("timer start failed\n");
				        LSAPI_OSI_ThreadExit();

				    }
				}
#endif
				if(bms_safeevent_timer_t == NULL)
				{
					bms_safeevent_timer_t = LSAPI_OSI_TimerCreate(LSAPI_OSI_ThreadCurrent(), bms_mqtt_safe_event_timer_callback, NULL);
					if(LSAPI_OSI_TimerStart(bms_safeevent_timer_t, 10000) == FALSE)
				    {
				        LSAPI_Log_Debug("timer start failed\n");
				        LSAPI_OSI_ThreadExit();

				    }
				}
			}
			break;
		    case MENU_MQTT_CONN_RSP:
	            LSAPI_Log_Debug("MQTT_CONN: %d \n", event.param1);

				if(event.param1 == 1)
				{
					LSAPI_OSI_Event_t event = {};
  					event.id = BMS_MQTT_SUB_INFO_ID;
  					LSAPI_OSI_EvnetSend(LSAPI_OSI_ThreadCurrent(), &event);
				}
	            break;
			case BMS_MQTT_SUB_INFO_ID:
			{
			 	static int16_t cnt = 0;
				static int16_t base_info_cnt = 0;
				if((cnt < 10) && (bms_mqtt_sub_table[cnt].sub_flag == 0))
				{	
					LSAPI_MQTT_CFG("topic",bms_mqtt_sub_table[cnt].topic);
					LSAPI_MQTT_SUB(1);
					cnt++;
				}
				else
				{
					if((base_info_cnt == 0) && (cnt >= 10))
					{
						bms_post_baseinfo_to_platform();
						g_mqtt_tracker_init_flag = 1;
						LSAPI_OSI_ThreadSleep(200);
						//bms_post_ota_to_tracker_for_test();
						base_info_cnt = 1;
					}
				}
								
			}
			break;
	        case MENU_MQTT_CLOSE_RSP:
	            LSAPI_Log_Debug("\n\r MQTT_CLOSE: %d \n", event.param1); 
	            break;
	        
	        case MENU_MQTT_SUB_RSP:
	            LSAPI_Log_Debug("\n\r MQTT_SUB: %d \n", event.param1);
				if(event.param1 == 1)
				{
					LSAPI_OSI_Event_t event = {};
  					event.id = BMS_MQTT_SUB_INFO_ID;
  					LSAPI_OSI_EvnetSend(LSAPI_OSI_ThreadCurrent(), &event);
				}				
	            break;
	            
	        case MENU_MQTT_UNSUB_RSP:
	            LSAPI_Log_Debug("\n\r MQTT_UNSUB: %d \n", event.param1);
	            break;
	            
	        case MENU_MQTT_PUB_RSP:
	        {
	            LSAPI_Log_Debug("\n\r MQTT_PUB: %d \n", event.param1);
        	}
	            break;
			case BMS_MQTT_PUB_BATINFO_INFO_ID:
			{
#if 0
				LSAPI_OSI_TimerStop(bms_batinfo_timer_t);
				bms_post_cellbat_info_to_platform();
				LSAPI_OSI_TimerStart(bms_batinfo_timer_t,10000);
#endif
			}
			break;
			case BMS_MQTT_PUB_SAFE_EVENT_INFO_ID:
			{
				LSAPI_OSI_TimerStop(bms_safeevent_timer_t);
				bms_post_safe_event_to_paltfrom();
				LSAPI_OSI_TimerStart(bms_safeevent_timer_t,1000);
			}
			break;
			case BMS_MQTT_PUB_REALTIME_INFO_ID:
				{
					int64_t timestamp = LSAPI_OSI_EpochTime();
					LSAPI_OSI_TimerStop(bms_realtime_timer_t);
					bms_post_realtime_info_to_platform(timestamp);
					LSAPI_OSI_ThreadSleep(200);
					bms_post_cellbat_info_to_platform(timestamp);
					LSAPI_OSI_TimerStart(bms_realtime_timer_t,10000);
				}
				break;
			case BMS_MQTT_PUB_LIBSAFE_OTA_REPLY_ID:
			case BMS_MQTT_PUB_TRACKER_OTA_REPLY_ID:
			case BMS_MQTT_PUB_BMS_OPERATION_REPLY_ID:
			case BMS_MQTT_PUB_TRACKER_OPERATION_REPLY_ID:
			case BMS_MQTT_PUB_QUERY_REPLY_ID:
			{	
				if(bms_safeevent_timer_t != NULL)
					LSAPI_OSI_TimerStop(bms_safeevent_timer_t);
				bms_post_reply_to_platform(event.id);
				if(bms_safeevent_timer_t != NULL)
					LSAPI_OSI_TimerStart(bms_safeevent_timer_t,1000);
			}
			break;
			case BMS_MQTT_PUB_OTA_RESULT_ID:
			{
				bms_post_ota_reply_to_platform();
			}
			break;
			case MENU_MQTT_DOWNLINK_MSG:
			{
		        LSAPI_Log_Debug("MQTT_DOWNLINK_MSG: %s\n", event.param1);
				LSAPI_Log_Debug("MQTT_DOWNLINK_MSG topic len is: %d\n", event.param2);
				LSAPI_Log_Debug("MQTT_DOWNLINK_MSG payload len is: %d\n", event.param3);
				if(g_mqtt_tracker_init_flag == 1)
				{
					char *payload_buff = NULL;
					uint32_t topic_len = event.param2;
					uint32_t payload_len = event.param3;
					uint32_t msg_len = topic_len + payload_len;
					payload_buff = (char *)LSAPI_OSI_Malloc(msg_len + 1);
					if(payload_buff == NULL)
					{
						LSAPI_Log_Debug("\n\r MQTT_DOWNLINK_MSG: buf malloc errpr \n");
					}
					else
					{
						uint16_t cnt = 0;
						memcpy(payload_buff,event.param1,msg_len);

						for(cnt = 0; cnt < BMS_SUB_TABLE_MAX; cnt++)
						{
							if(!strncmp(payload_buff,bms_mqtt_sub_table[cnt].topic,topic_len) && (bms_mqtt_sub_table[cnt].parse_func != NULL ))
							{
								bms_mqtt_sub_table[cnt].parse_func(payload_buff + topic_len,payload_len);
								break;
							}
						}
					}
					if(payload_buff != NULL)
					{
						free(payload_buff);
						payload_buff = NULL;
					}
				}
		        if (NULL != (void *)event.param1)
		            free((void *)event.param1);
				
				break;
			}
	        default:
				//LSAPI_MENU_Printf("\n\r unknown msg id: %d \n", ev.param1);
	            break;    
		}	
    } 
    LSAPI_OSI_ThreadExit();
}

