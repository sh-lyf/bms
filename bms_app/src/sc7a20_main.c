#include "lsapi_config.h"
#include "lsapi_sys.h"
#include "lsapi_os.h"
#include "osi_api.h"
#include "lsapi_i2c.h"
#include "lsapi_spi.h"
#if 1
#include "lsapi_device.h"
#else
#include "lsapi_gpio.h"
#endif
#include "SL_Watch_Algorithm_driver.h"
#include "sc7a20_i2c_spi.h"

#if 1
typedef struct
{
    uint32_t gpioId;
    LSAPI_Device_t *d;
    osiThread_t *task;
} SC7A20_Data_t;
#else
typedef struct
{
    uint32_t gpioId;
    LSAPI_Gpio_t *d;
    osiThread_t *task;
} SC7A20_Data_t;
#endif

#define LS_API_EV_BASE  0x00310000 //LS_OPENCPU_EV_BASE
#define LS_SENSOR_READ_EVENT (LS_API_EV_BASE+100)

static SC7A20_Data_t sc7a20 = {10};
static const unsigned char spi_i2c_mode = 1; //0 is SPI mode; 1 is IIC mode;

/*
    READ ME && FAQ
	The following are Chinese characters, please read them in UTF-8 environment.
	
    关于 isr callback:
	在 GPIO 作为输入模式的情况下，GPIO 需要以中断输入的方式进行配置；
	外部中断发生后，底层 isr 会调用客户编写的 callback 函数，所以该 callback 的编写需要遵循中断函数注意的事项；
	1. 不要在该 callback 中处理大量的工作；当工作量过大时，请使用 中断发消息 + 线程\任务队列方式进行处理；
	2. 不要在该 callback 中调用具有阻塞性质的接口；
	3. 不要在该 callback 中进行睡眠（LSAPI_OSI_ThreadSleep）
*/
static void prvSensorIrqHandle(void *ctx)
{
    SC7A20_Data_t *s = (SC7A20_Data_t *)ctx;
    LSAPI_OSI_Event_t sensor_event;
    uint32_t evt = 0;

    sensor_event.id = LS_SENSOR_READ_EVENT;
    sensor_event.param1 = evt;

    // don't replace osiEventTrySend with osiEventSend here !!!
    if(osiEventTrySend((osiThread_t *)s, (const osiEvent_t *)&sensor_event, 0) == false)
    {
        LSAPI_Log_Debug("prvSensorIrqHandle: send sensor event(%x) failed\n", evt);
    }
    else
    {
        LSAPI_Log_Debug("prvSensorIrqHandle: send uart event(%x) success\n", evt);      
    }
}

#if 1
static bool _init_sensor_irq(void)
{
    LSAPI_GpioConfig_t gpioConfig = {0};
    sc7a20.gpioId = 10; //number of gpio used for sensor irq
    gpioConfig.id = sc7a20.gpioId;  
    gpioConfig.mode = LS_GPIO_INPUT;
    gpioConfig.intr_enabled = true;
    gpioConfig.intr_level = false;
    gpioConfig.rising = true;
    gpioConfig.falling = true;
    gpioConfig.debounce = true;
    gpioConfig.cb = prvSensorIrqHandle;
    gpioConfig.cb_ctx = NULL;

    // create instance
    sc7a20.d = LSAPI_Device_GPIOCreate(&gpioConfig);
    if (sc7a20.d == NULL)
    {
        LSAPI_Log_Debug("_init_sensor_irq: irq pin open faild, irq gpio number:%d", sc7a20.gpioId);
        return false;
    }
    LSAPI_Log_Debug("_init_sensor_irq: register irq success!");
    return LSAPI_Device_Open(sc7a20.d); 
}
#else
static bool _init_sensor_irq(void)
{
    sc7a20.gpioId = 10; //number of gpio used for sensor irq
    LSAPI_GpioConfig_t sensor_irq = {
        .mode = LSAPI_GPIO_INPUT,
        .intr_enabled = true,
        .rising = true,
        .falling = true,
        .debounce = true,
    };
    sc7a20.d = LSAPI_GpioOpen(sc7a20.gpioId, &sensor_irq, prvSensorIrqHandle, (void*)&sc7a20);
    if (sc7a20.d == NULL)
    {
        LSAPI_Log_Debug("_init_sensor_irq: irq pin open faild, irq gpio number:%d", sc7a20.gpioId);
        return false;
    }
    LSAPI_Log_Debug("_init_sensor_irq: register irq success!");
    return true;
}
#endif

static bool _init_sensorc_i2c(void)
{
    //unsigned char sl_spi_iic = 1; //0 for spi , 1 for i2c;
    SL_SC7S20_I2C_Cfg i2c_cfg = {0};
    i2c_cfg.i2cName = LSAPI_NAME_I2C1; // which i2c to be used?
    i2c_cfg.bps = LSAPI_I2C_BPS_400K;
    i2c_cfg.slave_addr = 0x18;
    return SL_SC7A20_I2c_Spi_Init(spi_i2c_mode, &i2c_cfg, NULL);
}

static void _read_acc_data(void)
{
	char temp = 0x0;
	char x_l8,x_h8,y_l8,y_h8,z_l8,z_h8;
    signed short int SL_ACCEL_X, SL_ACCEL_Y, SL_ACCEL_Z;//三轴数据
	//temp = Read_One_Byte(IICADDR, 0x27);
    SL_SC7A20_I2c_Spi_Read(spi_i2c_mode, 0x27, 1, &temp);
	temp = temp & 0x0f;
	if (0x0f == temp)
	{
		//x_l8 = Read_One_Byte(IICADDR, 0x28);
        SL_SC7A20_I2c_Spi_Read(spi_i2c_mode, 0x28, 1, &x_l8);
		//x_h8 = Read_One_Byte(IICADDR, 0x29);
        SL_SC7A20_I2c_Spi_Read(spi_i2c_mode, 0x29, 1, &x_h8);	
		//y_l8 = Read_One_Byte(IICADDR, 0x2A);
        SL_SC7A20_I2c_Spi_Read(spi_i2c_mode, 0x2A, 1, &y_l8);
		//y_h8 = Read_One_Byte(IICADDR, 0x2B);
        SL_SC7A20_I2c_Spi_Read(spi_i2c_mode, 0x2B, 1, &y_h8);				
		//z_l8 = Read_One_Byte(IICADDR, 0x2C);
        SL_SC7A20_I2c_Spi_Read(spi_i2c_mode, 0x2C, 1, &z_l8);
		//z_h8 = Read_One_Byte(IICADDR, 0x2D);
        SL_SC7A20_I2c_Spi_Read(spi_i2c_mode, 0x2D, 1, &z_h8);

        ///拼接数据 SL_ACCEL_X = （signed short int） ((X_H<< 8) | X_L);
        SL_ACCEL_X = (signed short int)(x_h8 << 8 | x_l8);
        //取10 位带符号整型数据，Y,Z 同理 SL_ACCEL_X = SL_ ACCEL_X>>6;
        SL_ACCEL_X = SL_ACCEL_X >> 6;
        SL_ACCEL_Y = (signed short int)(y_h8 << 8 | y_l8);
        SL_ACCEL_Y = SL_ACCEL_Y >> 6;
        SL_ACCEL_Z = (signed short int)(z_h8 << 8 | z_l8);
        SL_ACCEL_Z = SL_ACCEL_Z >> 6;

        LSAPI_Log_Debug("_read_acc_data: X=%d,Y=%d,Z=%d", SL_ACCEL_X, SL_ACCEL_Y, SL_ACCEL_Z);
	}
}

static void sc7a20Thread(void *param)
{
    /**return : 1    表示CHIP ID 正常***********/
    /**return : 0    表示读取异常***************/
    /**return :-1;   SPI 通信问题***************/
    /**return :-2;   IIC 通信问题***************/
    unsigned char ret = -2;
    /****spi_i2c_mode=0：SPI MODE, Sl_pull_up_mode config failed****************/
    /****spi_i2c_mode=1：IIC MODE***********************************************/
    /****Sl_pull_up_mode=0x00: SDO  I2C  pull up***********************************/
    /****Sl_pull_up_mode=0x08: I2C  pull up and  SDO  open drain*******************/
    /****Sl_pull_up_mode=0x04: SDO  pull up and  I2C  open drain*******************/
    /****Sl_pull_up_mode=0x0C: SDO  I2C  open drain********************************/
    unsigned char Sl_pull_up_mode = 0;
    //1. open i2c
    if (_init_sensorc_i2c() == false)
    {
        LSAPI_Log_Debug("sc7a20Thread: init i2c faild!");
        return;
    }
    //2. init sensor    
    ret = SL_SC7A20_Driver_Init(spi_i2c_mode,Sl_pull_up_mode);
    if (ret == 1)
    {
        LSAPI_Log_Debug("sc7a20Thread: Get Chip ID success!");
    }
    else if (ret == 0)
    {
        LSAPI_Log_Debug("sc7a20Thread: Get Chip ID faild!");
        return;
    }
    else if (ret == -1)
    {
        LSAPI_Log_Debug("sc7a20Thread: SPI communication error");
        return;
    }
    else if (ret == -2)
    {
        LSAPI_Log_Debug("sc7a20Thread: I2C communication error");
        return;
    }

    //if get Chip ID success, so to do follow:
    //3. init irq;
    if (_init_sensor_irq() == false)
    {
        LSAPI_Log_Debug("sc7a20Thread: irq faild!");
        //release i2c or spi
        SL_SC7A20_I2c_Spi_DeInit(spi_i2c_mode);
        return ;
    }
    
    LSAPI_OSI_Thread_t *thread_this = LSAPI_OSI_ThreadCurrent();
    LSAPI_OSI_Event_t waitevent;
    
    for (;;)
    {
        LSAPI_OSI_EventWait(LSAPI_OSI_ThreadCurrent(), &waitevent);
        if (LS_SENSOR_READ_EVENT == waitevent.id)
        {
            //Get sensor data x.y.z here
            _read_acc_data();
        }
		LSAPI_OSI_ThreadSleep(10);
    }
}

void sc7a20_func(void)
{
    LSAPI_OSI_ThreadCreate("[sc7a20_gsensor_task]", sc7a20Thread, NULL, LSAPI_OSI_PRIORITY_NORMAL, 1024*2, 4);
}
