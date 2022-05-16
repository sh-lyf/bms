#include "osi_compiler.h"
#include "lsapi_sys.h"
#include "lsapi_os.h"
#include "lsapi_device.h"

#include "bms_main.h"
#include "bms_mqtt.h"
#include "bms_checksum.h"
#include "bms_tracker_protocol.h"
//美团交互协议
BMS_RESISTER_INFO_T bms_info_table[] = {
	{30000,1},				//电池包工作状态
	{30001,2},				//电池包总电压
	{30003,2},				//电池包实时充放电电流
	{30005,1},				//SOC
	{30006,4},				//保护状态
	{30010,2},				//故障状态
	{30012,1},				//单体最高温度
	{30013,1},				//单体最低温度
	{30014,1},				//当前MOS最高温度
	{30015,1},				//当前PCB最高温度
	{30020,1},				//SOH
	{30021,2},				//循环次数
	{30023,2},				//正常充电次数
	{30025,2},				//过放次数
	{30027,2},				//短路次数
	{30029,1},				//电芯历史最高温度
	{30030,2},				//剩余可用容量
	{30032,1},				//电池包工作模式
	{30033,1},				//电池充电模式	
	{30034,2},				//充电 SOP
	//预留
	{30040,1},				//Boot 版本号
	{30041,16},				//电池 ID
	{30057,1},				//厂商代码
	{30058,2},				//BMS 硬件版本
	{30060,2},				//BMS 软件版本	
	{30062,2},				//电池包额定容量
	{30064,1},				//电池类型
	//预留
	{30070,1},				//Mos 管状态
	{30071,1},				//物料变更记录
	{30080,1},				//电芯节数
	{30081,4},				//FCC
	{30085,4},				//电池内阻
	{30089,4},				//复位次数
	{30093,2},				//协议版本号
	//预留
	{30100,1},				//RSOC
	{30101,2},				//均衡状态
	{30103,1},				//电池充电状态
	{30104,4},				//充电停止真正原因
	{30108,2},				//请求充电器电压值
	//预留
	{30120,2},				//电池包标称电压
	{30122,1},				//温度传感器个数
	{30123,2},				//最高单体电压
	{30125,2},				//最低单体电压
	{30127,1},				//Tracker 关机标志
	{30128,1},				//BMS 低电量提示
	{30129,2},				//Tracker 故障状态
	{30131,1},				//BMS 数据上报类型
	//预留
	{30200,1},				//Tracker 系统模式
	{30203,2},				//运输模式前后期判断时间
	{30205,2},				//运营模式上报周期
	{30207,2},				//运输模式前期上报周期
	{30209,2},				//运输模式后期上报周期
	{30211,2},				//运输转存储模式的静置时长
	{30213,2},				//存储模式进关机周期
	{30215,1},				//存储模式关机功能开关
	{30216,2},				//充电模式 1 上报周期
	{30218,2},				//充电模式 2 上报周期
	{30220,2},				//存储模式上报周期
	{30222,2},				//寻找模式上报周期
	{30224,2},				//电池包跌落次数
	{30226,6},				//实时时间
	{30232,20},				//
	{30252,48},				//MQTT 地址
	{30300,6},				//MQTT 端口
	{30306,20},				//MQTT 设备号
	{30326,20},				//MQTT 用户名
	{30346,36},				//MQTT 密钥
	//预留
	{30700,2},				//历史最高单体电压
	{30702,2},				//历史最低单体电压
	{30704,2},				//历史最大单体压差
	{30706,1},				//历史最高温度
	{30707,1},				//历史最低温度
	{30708,2},				//历史最大放电电流
	{30710,2},				//历史最大充电电流

	//预留
	{30800,1},				//强制 BMS 进入休眠模式
	{30801,1},				//强制 BMS 进入仓储模式
	{30802,1},				//BMS 复位
	{30803,1},				//BMS 关机
	{30804,1},				//强制 Tracker 进入返厂模式
	{30805,1},				//强制 Tracker 进入存储模式
	{30806,1},				//强制 Tracker 进入运输模式

	{30807,1},				//强制 Tracker 进入搜寻模式
	{30808,1},				//Tracker 复位
	{30809,1},				//Tracker 关机
	{30810,1},				//BMS 关 Tracker电电量阈值
	{30811,1},				//BMS 恢 复Tracker 供电电量阈值
	{30812,1},				//放电 MOS 控制
	{30813,1},				//电量 SOC 低值通知
	//预留
	{30900,2},				//设备型号
	{30902,1},				//厂家 ID
	{30903,2},				//Tracker 硬件版本号
	{30905,2},				//Tracker 软件版本号
	{30907,15},				//IMEI 号
	{30922,15},				//IMSI 号
	{30937,20},				//ICCID

	//预留
	{31000,1},				//连接状态
	{31001,1},				//连接断开原因
	{31002,48},				//简要信息
	{31050,12},				//GPS 信息
	{31062,11},				//WIFI-AP 信息
	{31073,4},				//休眠时间
	{31100,37},				//FOTA 升级结果	
};

int16_t tracker_read_bms_func(uint16 register_idx)
{
	uint16_t read_data_len = 0;
	TRACKER_RD_BMS_T tracker_rd_bms_data = {0}; 
	tracker_rd_bms_data.device_id = 0x03;
	tracker_rd_bms_data.func_code = 0x83;
	uint8_t data[8] = {0};
	uint16 idx = 0;
	uint16 crc_checksum = 0;
	for(idx = 0;idx < (sizeof(bms_info_table)/sizeof(BMS_RESISTER_INFO_T));idx++)
	{
		if(bms_info_table[idx].index == register_idx)
		{
			tracker_rd_bms_data.register_msb = ((register_idx & 0xff00) >> 4);
			tracker_rd_bms_data.register_lsb = (register_idx & 0x00ff);
			tracker_rd_bms_data.rd_len_msb = ((bms_info_table[idx].rw_len & 0xff00) >> 4);
			tracker_rd_bms_data.rd_len_msb = (bms_info_table[idx].rw_len & 0x00ff);
			break;
		}
	}

	if(idx > (sizeof(bms_info_table)/sizeof(BMS_RESISTER_INFO_T)))
	{
		return -1;
	}
	memset(data,0x0,8);
	data[0] = tracker_rd_bms_data.device_id;
	data[1] = tracker_rd_bms_data.func_code;
	data[2] = tracker_rd_bms_data.register_msb;
	data[3] = tracker_rd_bms_data.register_lsb;
	data[4] = tracker_rd_bms_data.rd_len_msb;
	data[5] = tracker_rd_bms_data.rd_len_lsb;
	//crc校验
	crc_checksum = GetCRCCode(data,6);
	data[6] = (crc_checksum & 0x00ff);
	data[7] = ((crc_checksum & 0xff00) >> 4);
	//发送给bms
	//covert_data_to_bms(data);
}

