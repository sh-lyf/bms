#ifndef __BMS_MQTT_H__
#define __BMS_MQTT_H__

typedef int(* bms_platform_parse_cb)(char *buffer,int len);

typedef enum _bms_sub_topic_index{
	BMS_MQTT_PLATFORM_INDEX_0 = 0,
	BMS_MQTT_PLATFORM_INDEX_1,
	BMS_MQTT_PLATFORM_INDEX_2,
	BMS_MQTT_PLATFORM_INDEX_3,
	BMS_MQTT_PLATFORM_INDEX_4,
	BMS_MQTT_PLATFORM_INDEX_MAX
}BMS_SUB_TOPIC_INDEX_E;

typedef struct _bms_mqtt_sub_topic_t
{
	char topic[64];
	bms_platform_parse_cb parse_func;
	int sub_flag;
}BMS_MQTT_SUB_TOPIC_T;

int LSAPI_NET_CGACT(void);
void bms_MqttEntry(void *param);
int get_tracker_base_info(void);

#endif/*__BMS_MQTT_H__*/
