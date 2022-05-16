

//#include "SL_Watch_Algorithm.h"
#include "SL_Watch_Algorithm_driver.h"

/**********sc7a20**********/

#define SL_SC7A20_CHIP_ID_ADDR    (unsigned char)0X0F
#define SL_SC7A20_CHIP_ID_VALUE   (unsigned char)0X11
#define SL_SC7A20_POWER_ODR_REG   (unsigned char)0X20
#define SL_SC7A20_FIFO_CTRL_REG   (unsigned char)0X2E
#define SL_SC7A20_FIFO_SRC_REG    (unsigned char)0X2F
#define SL_SC7A20_SPI_OUT_X_L     (unsigned char)0X27
#define SL_SC7A20_IIC_OUT_X_L     (unsigned char)0XA8

static unsigned char  SL_SPI_IIC_INTERFACE         = 0;
static unsigned char  sc7a20_data_flag             = 0;
static signed short   sc7a20_data_off              = 0;

static unsigned char SL_SC7A20_Watch_Algo_ODR_Current  = 0x47;

#define SL_SC7A20_INIT_REG1_NUM 3
static unsigned char SL_SC7A20_INIT_REG1[9]=
{
    0x1E,0x05,0x00,
    0x57,0x00,0x00,
    0x1E,0x00,0x00,
};
#define SL_SC7A20_INIT_REG2_NUM 5
static unsigned char SL_SC7A20_INIT_REG2[15]=
{
    0x2E,0x00,0x00,
    0x20,0x47,0x00,
    0x23,0x98,0x00,
    0x24,0x40,0x00,
    0x2E,0x4F,0x00,
};

//find device and init 
signed char SL_SC7A20_Driver_Init(unsigned char Sl_spi_iic_init,unsigned char Sl_pull_up_mode)
{
	unsigned char i=0;

	if(Sl_spi_iic_init==0)
		SL_SPI_IIC_INTERFACE  = 0;//spi
	else
		SL_SPI_IIC_INTERFACE  = 1;//iic
 
    if(SL_SPI_IIC_INTERFACE==1)
    {
        SL_SC7A20_INIT_REG1[4]=(SL_SC7A20_INIT_REG1[4]&0xF3)|Sl_pull_up_mode;
        for(i=0;i<SL_SC7A20_INIT_REG1_NUM;i++)
        {
            SL_SC7A20_I2c_Spi_Write(SL_SPI_IIC_INTERFACE, SL_SC7A20_INIT_REG1[3*i], SL_SC7A20_INIT_REG1[3*i+1]);
        }
    }
    for(i=0;i<SL_SC7A20_INIT_REG2_NUM;i++)
    {
        SL_SC7A20_I2c_Spi_Write(SL_SPI_IIC_INTERFACE, SL_SC7A20_INIT_REG2[3*i], SL_SC7A20_INIT_REG2[3*i+1]);
    }  
    for(i=0;i<SL_SC7A20_INIT_REG2_NUM;i++)
    {
        SL_SC7A20_I2c_Spi_Read(SL_SPI_IIC_INTERFACE, SL_SC7A20_INIT_REG2[3*i],1, &SL_SC7A20_INIT_REG2[3*i+2]);
    }
    for(i=1;i<SL_SC7A20_INIT_REG2_NUM;i++)
    {
        if(SL_SC7A20_INIT_REG2[3*i+1]!=SL_SC7A20_INIT_REG2[3*i+2]) break;
    }  
    if(i != SL_SC7A20_INIT_REG2_NUM)
    {
		if(SL_SPI_IIC_INTERFACE==0)
			return -1;//reg write and read error by SPI
		else
			return -2;//reg write and read error by IIC
    }
	SL_SC7A20_I2c_Spi_Read(SL_SPI_IIC_INTERFACE, SL_SC7A20_CHIP_ID_ADDR,1, &i);
    return i;
//	if(i==SL_SC7A20_CHIP_ID_VALUE)    return 1;
//	else                              return 0;
}


static void SC7A20_DATA_EXE(signed short *sc7a20_xyz_data)
{
    static unsigned char sc7a20_data_num  =0;
    static signed int    sc7a20_data_z    =0;

    if(sc7a20_xyz_data[0]<700&&sc7a20_xyz_data[0]>-700&&sc7a20_xyz_data[1]<700&&sc7a20_xyz_data[1]>-700)
    {
        sc7a20_data_z=sc7a20_data_z+sc7a20_xyz_data[2];
        sc7a20_data_num++;
        if(sc7a20_data_num>=20)
        {
             sc7a20_data_z=sc7a20_data_z/20;
             if(sc7a20_data_z>4000&&sc7a20_data_z<12000)
             {
                sc7a20_data_off=8096-sc7a20_data_z;
                sc7a20_data_flag=1;
             }
             if(sc7a20_data_z<-4000&&sc7a20_data_z>-12000)
             {
                 sc7a20_data_off=-8096-sc7a20_data_z;
                 sc7a20_data_flag=1;
             }
             sc7a20_data_z    =0;
             sc7a20_data_num  =0;	
        }
    }
    else
    {
        sc7a20_data_z    =0;
        sc7a20_data_num  =0;	
    }
}

unsigned char SL_SC7A20_Read_FIFO_Buf(signed short *x_buf,signed short *y_buf,signed short *z_buf)
{
    unsigned char  i=0;
    unsigned char  sc7a20_data[7];
    signed short   sl_sc7a20_data[3];
    unsigned char  SL_FIFO_ACCEL_NUM;

    SL_SC7A20_I2c_Spi_Read(SL_SPI_IIC_INTERFACE, SL_SC7A20_FIFO_SRC_REG,1,&SL_FIFO_ACCEL_NUM); 
    SL_FIFO_ACCEL_NUM = SL_FIFO_ACCEL_NUM&0x1f;

    for(i=0;i<SL_FIFO_ACCEL_NUM;i++)
    {
        if(SL_SPI_IIC_INTERFACE==0)
            SL_SC7A20_I2c_Spi_Read(SL_SPI_IIC_INTERFACE, SL_SC7A20_SPI_OUT_X_L,7, &sc7a20_data[0]);
        else
            SL_SC7A20_I2c_Spi_Read(SL_SPI_IIC_INTERFACE, SL_SC7A20_IIC_OUT_X_L,6, &sc7a20_data[1]);
        x_buf[i] =(signed short int)(((unsigned char)sc7a20_data[2] * 256 ) + (unsigned char)sc7a20_data[1]);
        y_buf[i] =(signed short int)(((unsigned char)sc7a20_data[4] * 256 ) + (unsigned char)sc7a20_data[3]);
        z_buf[i] =(signed short int)(((unsigned char)sc7a20_data[6] * 256 ) + (unsigned char)sc7a20_data[5]);
        if(sc7a20_data_flag ==1) z_buf[i]=z_buf[i]+sc7a20_data_off;
        x_buf[i] =x_buf[i] >>6;//10bits
        y_buf[i] =y_buf[i] >>6;//10bits
        z_buf[i] =z_buf[i] >>6;//10bits
	}	
	SL_SC7A20_I2c_Spi_Write(SL_SPI_IIC_INTERFACE, SL_SC7A20_FIFO_CTRL_REG, 0X00);		

    SL_SC7A20_I2c_Spi_Write(SL_SPI_IIC_INTERFACE, SL_SC7A20_FIFO_CTRL_REG, 0X4F);

	if(sc7a20_data_flag ==0)
	{
        for(i=0;i<SL_FIFO_ACCEL_NUM;i++)
        {
            sl_sc7a20_data[0]=x_buf[i];
            sl_sc7a20_data[1]=y_buf[i];
            sl_sc7a20_data[2]=z_buf[i];
            SC7A20_DATA_EXE(&sl_sc7a20_data[0]);
        }
	}
    return SL_FIFO_ACCEL_NUM;
}
