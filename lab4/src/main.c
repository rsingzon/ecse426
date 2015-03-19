/*!
 * @file main.c
 * @brief This is the main file for lab 4
 * @author Ryan Singzon 260397455
 * @author Jeffrey Tichelman 260446802
 * @date March 18, 2015
 */

#define osObjectsPublic                     // define objects in main module
#include "osObjects.h"                      // RTOS object definitions
#include "stm32f4xx.h"                  // Device header
#include "stm32f4xx_conf.h"
#include <stdio.h>

// Include files
#include "7seg.h"
#include "accelerometer.h"
#include "init.h"
#include "keypad.h"
#include "kstate.h"
#include "led.h"

// Temperature sensor definitions
#define AVG_SLOPE 2.5
#define AVG_SLOPE_INVERSE 1/AVG_SLOPE
#define SCALE 0.715

// Define flags
#define ACC_FLAG 0x01
#define TEMP_FLAG 0x01

// Thread IDs
osThreadId accThread;
osThreadId tempThread;
osThreadId dispThread;

// Accelerometer/keypad function prototypes
void setAccelerometerFlag(void);
void resetAccelerometerFlag(void);
void setTemperatureFlag(void);
void resetTemperatureFlag(void);
void TIM3_IRQHandler(void);

// Temperature function prototypes
float to_celsius(int v_sense);
static volatile uint_fast16_t ticks;
void delay_ms(int ms);
int readADC();
void fadeLEDs();

// Flags
int accelerometerReady = 0; 	
int temperatureReady = 0;
int mode = 0;										// 0: Accelerometer, 1: Temperature
int interruptCounter = 0;
int keypad_flag = 0;
int display_flag = 0;
int flash_display = 1;				//should we flash the displayed value

// Define threads 
void accelerometerThread(void const *argument);
void temperatureThread(void const *argument);
void displayThread(void const *argument);

// Define the thread priorities
osThreadDef(accelerometerThread, osPriorityAboveNormal, 1, 1024);
osThreadDef(temperatureThread, osPriorityAboveNormal, 1, 1024);
osThreadDef(displayThread, osPriorityAboveNormal, 1, 1024);

// Mutex name definitions
osMutexDef(Mutex_Angle);				
osMutexId angle_mutex;
osMutexDef(Mutex_Temperature);
osMutexId temp_mutex;

// Values for display
int digit=3;
float value = 0;
float roll;
float temp;

// Initialize variables used by the threads
float targetAngle = 45;

/**
*@brief Thread to read accelerometer values
*/
void accelerometerThread(void const *argument){
	float angles[2];
	// Define kalman states for each accelerometer output
	kalman_state x_state = {0.05, 0.981, 0.0, 0.0, 0.0};
	kalman_state y_state = {0.05, 0.981, 0.0, 0.0, 0.0};
	kalman_state z_state = {0.05, 0.981, 0.0, 0.0, 0.0};
	
	while(1){
		
		// Wait for the accelerometer to set an interrupt
		osSignalWait(ACC_FLAG, osWaitForever ); 			
			
		// Read accelerometers and set the display to the roll
		readAcc(angles, &x_state, &y_state, &z_state);
			
		// Wait for angle mutex before setting the angle
		osMutexWait(angle_mutex, osWaitForever);
		roll = angles[0];	
		osMutexRelease(angle_mutex);
		
		osSignalClear(accThread, ACC_FLAG);
	}
}

/**
*@brief Thread to read temperature values
*/
void temperatureThread(void const *argument){
	
	float rawTemp;
	float filteredTemp;
	kalman_state kstate = {15.0, 1.0, 0.0, 0.0, 0.0};
	
	while(1){
		
		// Wait for HW timer to set a signal
		osSignalWait(TEMP_FLAG, osWaitForever);
		
		// Read temperature
		rawTemp = to_celsius(readADC());
		filteredTemp = kalman_update(&kstate, rawTemp);
			
		// Wait for temp mutex before setting the temperature variable
		osMutexWait(temp_mutex, osWaitForever);
		temp = filteredTemp;
		osMutexRelease(temp_mutex);
		
		// Clear timer signal
		osSignalClear(tempThread, TEMP_FLAG);
	}
}

/**
*@brief Thread to display values on the 7seg
*/
void displayThread(void const *argument){
	
	int flashCounter=0;
	while(1){
		//gets button pressed if keypad interrupt flag is set
		if(keypad_flag){
			int i = get_button_pressed();		//change state based on this
		}
		
		
		if(display_flag){
			if(!flash_display){
				displayValue(value, digit, 1);
				if(digit == 1)
					digit = 4;
				digit--;
				interruptCounter++;
				display_flag = 0;
			}
			else{
				flashCounter++;
				if(flashCounter<100){
					displayValue(value, digit, 1);
				}
				else{
					displayValue(value, digit, 0);
				}
				if(flashCounter>200){
					flashCounter=0;
				}	
				if(digit == 1)
					digit = 4;
				digit--;
				interruptCounter++;
				display_flag = 0;
			}				
		}
	
	}
}

/*
 * main: initialize and start the system
 */
int main (void) {
  osKernelInitialize ();                    // initialize CMSIS-RTOS
	
	// Create mutexes
	angle_mutex = osMutexCreate(osMutex (Mutex_Angle));
	temp_mutex = osMutexCreate(osMutex (Mutex_Temperature));
		
  // initialize peripherals here
	initIO();
	initADC();
	initTimer();
	initAccelerometer();
	initAccelerometerInterrupt();
	
  // create 'thread' functions that start executing,
  // example: tid_name = osThreadCreate (osThread(name), NULL);
	accThread = osThreadCreate(osThread(accelerometerThread), NULL);
	tempThread = osThreadCreate(osThread(temperatureThread), NULL);
	dispThread = osThreadCreate(osThread(displayThread), NULL);
	
	// Start thread execution
	osKernelStart ();                         
	
	// The 7seg can display either the temperature or the tilt
	// Mode 0: Accelerometer mode
	//int count = 0;
	//float referenceAngle = 0.0;
}

/**
* @brief Reads the temperature sensor 
* @retval integer 
*/
int readADC() 
{
	ADC_SoftwareStartConv(ADC1); //Starting Conversion, waiting for it to finish, clearing the flag, reading the result
	while(ADC_GetFlagStatus(ADC1, ADC_FLAG_EOC) == RESET); //Could be through interrupts (Later)
	ADC_ClearFlag(ADC1, ADC_FLAG_EOC); //EOC means End Of Conversion
	ADC_GetConversionValue(ADC1); // Result available in ADC1->DR
	return ADC1->DR;
}


/**
* @brief Converts ADC reading to temperature in celsius
* @retval integer 
*/
float to_celsius(int v_sense)
{
	float v_sense_f = (float) v_sense;
	v_sense_f = v_sense_f * SCALE;
	return ((v_sense_f - 760.0) * AVG_SLOPE_INVERSE) + 25;
}

/**
*@brief Sets the accelertometer flag when a sample can be read
*@retval None
*/
void setAccelerometerFlag(void)
{
	accelerometerReady = 1;
}

/**
*@brief Resets the accelerometer flag
*@retval None
*/
void resetAccelerometerFlag(void)
{
	accelerometerReady = 0;
}

/**
*@brief Sets the temperature flag indicating when a sample can be read
*@retval None
*/
void setTemperatureFlag(void)
{
	temperatureReady = 1;
}

/**
*@brief Resets the temperature flag
*@retval None
*/
void resetTemperatureFlag(void)
{
	accelerometerReady = 0;
}

/**
*@brief Interupt handler for EXTI0.  Informs uP that a sample is ready
*@retval None
*/
void EXTI0_IRQHandler(void)
{
	//setAccelerometerFlag(); 						//Set the flag
	osSignalSet(accThread, ACC_FLAG);			// Set a signal for the accelerometer thread
	EXTI_ClearITPendingBit(EXTI_Line0); //Clear the EXTI0 interupt flag
}

/**
*@brief Interupt handler for TIM3, generates interrupts with configured frequency
*@retval None
*/
void TIM3_IRQHandler(void)
{
	TIM_ClearITPendingBit(TIM3, TIM_IT_Update);			// Clears incoming interrupt bit
	display_flag = 1;
	//setTemperatureFlag();
	osSignalSet(tempThread, TEMP_FLAG);
}