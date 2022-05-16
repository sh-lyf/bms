#ifndef __SC7A20_INTERFACE_H_
#define __SC7A20_INTERFACE_H_

/*
*  i2c cfg. to see lsapi_i2c.h
*/
typedef struct
{
    uint32_t i2cName;       //i2c name: LSAPI_NAME_I2C1 ~ 3
    LSAPI_I2C_Bps_T bps;    //100k; 400k; 3.5M
    uint8_t slave_addr;     // slave addr
} SL_SC7S20_I2C_Cfg;

/*
*  spi cfg.to see lsapi_spi.h
*/
typedef struct
{
    LSAPI_SPI_CONFIG_t cfg;
} SL_SC7S20_SPI_Cfg;

/*
*  This function only for single writing.
*
* @ param sl_spi_iic    : 0 = spi  1 = i2c
* @ param reg           : register addr
* @ param data          : value loaded by reg
* 
* return 
* - 0 for false
* - 1 for true
*/
unsigned char SL_SC7A20_I2c_Spi_Write(unsigned char sl_spi_iic,unsigned char reg, unsigned char data);

/*
*  This function can be used for single reading or multiple consecutive readings.
*
* @ param sl_spi_iic    : 0 = spi  1 = i2c
* @ param reg           : register addr
* @ param len           : buffer length
* @ param buf           : buffer to receive data
* 
* return 
* - 0 for false
* - 1 for true
*/
unsigned char SL_SC7A20_I2c_Spi_Read(unsigned char sl_spi_iic,unsigned char reg, unsigned char len, unsigned char *buf);

/*
*  This function is used to initialize I2C or SPI 
*
* @ param sl_spi_iic        : 0 = spi  1 = i2c
* @ param i2c_cfg           : i2c_cfg
* @ param spi_cfg           : spi_cfg
* 
* return 
* - 0 for false
* - 1 for true
*/
unsigned char SL_SC7A20_I2c_Spi_Init(unsigned char sl_spi_iic, SL_SC7S20_I2C_Cfg *i2c_cfg, SL_SC7S20_SPI_Cfg *spi_cfg);

/*
*  This function is used to deinitialize I2C or SPI , release I2C or SPI resource.
*
* @ param sl_spi_iic        : 0 = spi  1 = i2c
* @ param i2c_cfg           : i2c_cfg
* @ param spi_cfg           : spi_cfg
* 
*/
void SL_SC7A20_I2c_Spi_DeInit(unsigned char sl_spi_iic);

#endif /* __SC7A20_INTERFACE_H_ */