#include "lsapi_config.h"
#include "lsapi_sys.h"
#include "lsapi_os.h"
#include "lsapi_i2c.h"
#include "lsapi_spi.h"
#include "sc7a20_i2c_spi.h"

typedef struct
{
    LSAPI_I2C_MASTER_T *master;
    uint8_t slave_addr;
} I2C_Data_t;

typedef struct
{
    LSAPI_SPI_MASTER_t *master;
    //TODO..
} SPI_Data_t;

static I2C_Data_t i2cData = {0};
static SPI_Data_t spiData = {0};

static bool _spi_init(SL_SC7S20_SPI_Cfg *spi_cfg)
{
    //TODO..
    return true;
}

static void _spi_release(void)
{
    //TODO...
}

static bool _spi_write(unsigned char reg, unsigned char data)
{
    //TODO...
    return true;
}

static bool _spi_read(unsigned char reg, unsigned char len, unsigned char *buf)
{
    //TODO...
    return true;
}

static bool _i2c_init(SL_SC7S20_I2C_Cfg *cfg)
{
    //i2cMaster
    if (cfg == NULL)
    {
        LSAPI_Log_Debug("_i2c_init: cfg NULL!!!");
        return false;
    }

    i2cData.master = LSAPI_I2C_MasterAcquire(cfg->i2cName, cfg->bps);
    if (i2cData.master == NULL)
    {
       LSAPI_Log_Debug("_i2c_init: i2c master acquire faild!!"); 
       return false;
    }
    i2cData.slave_addr = cfg->slave_addr;
    //LSAPI_Log_Debug("_i2c_init: i2c slave addr =0X%02X",i2cData.slave_addr);
    LSAPI_Log_Debug("_i2c_init: master acquire/open successfully, slave addr =0X%02X", i2cData.slave_addr);

    return true;
}

static void _i2c_release(void)
{
    if (i2cData.master != NULL)
    {
        LSAPI_I2C_MasterRelease(i2cData.master);
        i2cData.master = NULL;
        i2cData.slave_addr = 0x0;
        LSAPI_Log_Debug("_i2c_release: release!!!");
    }
}

static bool _i2c_write(unsigned char reg, unsigned char data)
{
	if (i2cData.master == NULL)
	{
		LSAPI_Log_Debug("_i2c_write: i2c master is not open! ");
		return false;
	}

	LSAPI_I2C_Slave_T slave = 
	{
		.addr_device = i2cData.slave_addr,
		.addr_data = reg,
		.addr_data_low = 0,
		.reg_16bit = false,
	};

	return LSAPI_I2C_Write(i2cData.master, &slave, &data, 1);
}

static bool _i2c_read(unsigned char reg, unsigned char len, unsigned char *buf)
{
	if (i2cData.master == NULL)
	{
		LSAPI_Log_Debug("_i2c_read: i2c master is not open! ");
		return false;
	}

	LSAPI_I2C_Slave_T slave = 
	{
		.addr_device = i2cData.slave_addr,
		.addr_data = reg,
		.addr_data_low = 0,
		.reg_16bit = false,
	};

	return LSAPI_I2C_Read(i2cData.master, &slave, buf, len);
}

unsigned char SL_SC7A20_I2c_Spi_Write(unsigned char sl_spi_iic,unsigned char reg, unsigned char data)
{
    //sl_spi_iic:0 = spi  1 = i2c
    if (sl_spi_iic == 0)
    {
        return _spi_write(reg, data);
    }
    else
    {
        return _i2c_write(reg, data);
    }
}

unsigned char SL_SC7A20_I2c_Spi_Read(unsigned char sl_spi_iic,unsigned char reg, unsigned char len, unsigned char *buf)
{
    //sl_spi_iic:0 = spi  1 = i2c
    if (sl_spi_iic == 0)
    {
        return _spi_read(reg, len, buf);
    }
    else
    {
        return _i2c_read(reg, len, buf);
    }
}

unsigned char SL_SC7A20_I2c_Spi_Init(unsigned char sl_spi_iic, SL_SC7S20_I2C_Cfg *i2c_cfg, SL_SC7S20_SPI_Cfg *spi_cfg)
{
    //sl_spi_iic:0 = spi  1 = i2c
    LSAPI_Log_Debug("SL_SC7A20_I2c_Spi_Init: sl_spi_iic=%d", sl_spi_iic);

    if (sl_spi_iic == 0)
    {
        return _spi_init(spi_cfg);
    }
    else
    {
        return _i2c_init(i2c_cfg);
    }
}

void SL_SC7A20_I2c_Spi_DeInit(unsigned char sl_spi_iic)
{
    //sl_spi_iic:0 = spi  1 = i2c
    LSAPI_Log_Debug("SL_SC7A20_I2c_Spi_DeInit: sl_spi_iic=%d", sl_spi_iic);

    if (sl_spi_iic == 0)
    {
        _spi_release();
    }
    else
    {
        _i2c_release();
    }
}