/* 
Copyright (c) 2017 Silan MEMS. All Rights Reserved.
*/

#ifndef SL_Watch_ALGO_DRV__H__
#define SL_Watch_ALGO_DRV__H__

unsigned char SL_SC7A20_Read_FIFO_Buf(signed short *x_buf,signed short *y_buf,signed short *z_buf);

/********�ͻ���Ҫ���е�IIC�ӿڷ������****************/
extern unsigned char SL_SC7A20_I2c_Spi_Write(unsigned char sl_spi_iic,unsigned char reg, unsigned char data);
extern unsigned char SL_SC7A20_I2c_Spi_Read(unsigned char sl_spi_iic,unsigned char reg, unsigned char len, unsigned char *buf);
/**SL_SC7A20_I2c_Spi_Write �����У� sl_spi_iic:0=spi  1=i2c  Reg���Ĵ�����ַ   data���Ĵ���������ֵ******************/
/**SL_SC7A20_I2c_Spi_Write ���� ��һ������д�ĺ���******************************************/
/***SL_SC7A20_I2c_Spi_Read �����У� sl_spi_iic:0=spi  1=i2c Reg ͬ�ϣ�len:��ȡ���ݳ��ȣ�buf:�洢�����׵�ַ��ָ�룩**/
/***SL_SC7A20_I2c_Spi_Read ���� �ǿ��Խ��е��ζ�����������ȡ�ĺ���************************/

/*************������ʼ������**************/
signed char SL_SC7A20_Driver_Init(unsigned char Sl_spi_iic_init,unsigned char Sl_pull_up_mode);
/***�������:1,Sl_spi_iic_init:0-1***2,PULL_UP_MODE:0x00 0x08 0x04 0x0c********/
/****Sl_spi_iic_init=0��SPI MODE, Sl_pull_up_mode config failed****************/
/****Sl_spi_iic_init=1��IIC MODE***********************************************/
/****Sl_pull_up_mode=0x00: SDO  I2C  pull up***********************************/
/****Sl_pull_up_mode=0x08: I2C  pull up and  SDO  open drain*******************/
/****Sl_pull_up_mode=0x04: SDO  pull up and  I2C  open drain*******************/
/****Sl_pull_up_mode=0x0C: SDO  I2C  open drain********************************/

/*************���������������**************/
/**return : 1    ��ʾCHIP ID ����***********/
/**return : 0    ��ʾ��ȡ�쳣***************/
/**return :-1;   SPI ͨ������***************/
/**return :-2;   IIC ͨ������***************/

#endif/****SL_Watch_ALGO_DRV__H__****/



