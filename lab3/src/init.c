/**
  * @file    init.c
  * @author  Ryan Singzon, Jeffrey Tichelman
  * @date    February 28, 2015
  * @brief   Initializes peripheral devices
	*/

#include "init.h"

void initAccelerometer() 
{
	
	// Configure accelerometer parameters
	LIS3DSH_InitTypeDef accelerometer_init;
	
	accelerometer_init.Power_Mode_Output_DataRate = LIS3DSH_DATARATE_100;				/* Power down or /active mode with output data rate 3.125 / 6.25 / 12.5 / 25 / 50 / 100 / 400 / 800 / 1600 HZ */
	accelerometer_init.Axes_Enable = LIS3DSH_X_ENABLE | LIS3DSH_Y_ENABLE | LIS3DSH_Z_ENABLE;		/* Enable all axes */
  accelerometer_init.Continous_Update = LIS3DSH_ContinousUpdate_Enabled;			/* Block or update Low/High registers of data until all data is read */
	accelerometer_init.AA_Filter_BW = LIS3DSH_AA_BW_50;													/* Choose anti-aliasing filter BW 800 / 400 / 200 / 50 Hz*/
  accelerometer_init.Full_Scale = LIS3DSH_FULLSCALE_2;       									/* Full scale 2 / 4 / 6 / 8 / 16 g */
  accelerometer_init.Self_Test = LIS3DSH_SELFTEST_NORMAL;    									/* Self test */
	
	// Initialize accelerometer
	LIS3DSH_Init(&accelerometer_init);
	
	// Enable interrupts for accelerometer
	LIS3DSH_DRYInterruptConfigTypeDef interrupt_init;
	
	interrupt_init.Dataready_Interrupt = LIS3DSH_DATA_READY_INTERRUPT_ENABLED;   /* Enable/Disable data ready interrupt */
  interrupt_init.Interrupt_signal = LIS3DSH_ACTIVE_HIGH_INTERRUPT_SIGNAL;      /* Interrupt Signal Active Low / Active High */
  interrupt_init.Interrupt_type = LIS3DSH_INTERRUPT_REQUEST_PULSED;            /* Interrupt type as latched or pulsed */ 
	
	// Initialize interrupts
	LIS3DSH_DataReadyInterruptConfig(&interrupt_init);
}