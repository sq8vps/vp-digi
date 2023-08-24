/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2020 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under Ultimate Liberty license
  * SLA0044, the "License"; You may not use this file except in compliance with
  * the License. You may obtain a copy of the License at:
  *                             www.st.com/SLA0044
  *
  ******************************************************************************
  */
/*
This file is part of VP-Digi.

VP-Digi is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 3 of the License, or
(at your option) any later version.

VP-Digi is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with VP-Digi.  If not, see <http://www.gnu.org/licenses/>.
*/
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "drivers/modem.h"
#include <stdint.h>
#include "ax25.h"
#include "drivers/uart.h"
#include "drivers/systick.h"
#include "stm32f1xx.h"
#include "digipeater.h"
#include "common.h"
#include "drivers/watchdog.h"
#include "beacon.h"
#include "terminal.h"
#include "config.h"
#include <string.h>

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/**
 * \brief Handle received frame from RF
 */
void handleFrame(void)
{
	uint8_t modemReceived = ax25.frameReceived; //store states
	ax25.frameReceived = 0; //clear flag

	uint8_t bufto[FRAMELEN + 30], buf[FRAMELEN]; //buffer for raw frames to TNC2 frames conversion
	uint16_t bufidx = 0;
	uint16_t i = ax25.frameBufRd;

	while(i != ax25.frameBufWr)
	{
		if(ax25.frameBuf[i] != 0xFF)
		{
			buf[bufidx++] = ax25.frameBuf[i++]; //store frame in temporary buffer
		}
		else
		{
			break;
		}
		i %= (FRAMEBUFLEN);
	}

	ax25.frameBufRd = ax25.frameBufWr;

	for(i = 0; i < (bufidx); i++)
	{
		if(buf[i] & 1)
			break; //look for path end bit
	}


	SendKiss(buf, bufidx); //send KISS frames if ports available


	if(((uart1.mode == MODE_MONITOR) || (uart2.mode == MODE_MONITOR)))
	{
		common_toTNC2(buf, bufidx, bufto); //convert to TNC2 format

		//in general, the RMS of the frame is calculated (excluding preamble!)
		//it it calculated from samples ranging from -4095 to 4095 (amplitude of 4095)
		//that should give a RMS of around 2900 for pure sine wave
		//for pure square wave it should be equal to the amplitude (around 4095)
		//real data contains lots of imperfections (especially mark/space amplitude imbalance) and this value is far smaller than 2900 for standard frames
		//division by 9 was selected by trial and error to provide a value of 100(%) when the input signal had peak-peak voltage of 3.3V
		//this probably should be done in a different way, like some peak amplitude tracing
		ax25.sLvl /= 9;

		if(ax25.sLvl > 100)
		{
			term_sendMonitor((uint8_t*)"\r\nInput level too high! Please reduce so most stations are around 50-70%.\r\n", 0);
		}

		if(ax25.sLvl < 10)
		{
			term_sendMonitor((uint8_t*)"\r\nInput level too low! Please increase so most stations are around 50-70%.\r\n", 0);
		}

		term_sendMonitor((uint8_t*)"(AX.25) Frame received [", 0); //show which modem received the frame: [FP] (flat and preemphasized), [FD] (flat and deemphasized - in flat audio input mode)
																   //[F_] (only flat), [_P] (only preemphasized) or [_D] (only deemphasized - in flat audio input mode)
		uint8_t t[2] = {0};
		if(modemReceived & 1)
		{
			t[0] = 'F';
		}
		else
			t[0] = '_';
		if(modemReceived & 2)
		{
			if(afskCfg.flatAudioIn)
				t[1] = 'D';
			else
				t[1] = 'P';
		}
		else
			t[1] = '_';

		term_sendMonitor(t, 2);
		term_sendMonitor((uint8_t*)"], signal level ", 0);
		term_sendMonitorNumber(ax25.sLvl);
		term_sendMonitor((uint8_t*)"%: ", 0);

		term_sendMonitor(bufto, 0);
		term_sendMonitor((uint8_t*)"\r\n", 0);

	}


	if(digi.enable)
	{
		Digi_digipeat(buf, bufidx);
	}



}


/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  SysTick_init();


  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  /* USER CODE BEGIN 2 */



	Wdog_init(); //initialize watchdog


	//set some initial values in case there is no configuration saved in memory
	uart1.baudrate = 9600;
	uart2.baudrate = 9600;
	afskCfg.usePWM = 0;
	afskCfg.flatAudioIn = 0;
	ax25Cfg.quietTime = 300;
	ax25Cfg.txDelayLength = 300;
	ax25Cfg.txTailLength = 30;
	digi.dupeTime = 30;

	Config_read();

	Ax25_init();

	uart_init(&uart1, USART1, uart1.baudrate);
	uart_init(&uart2, USART2, uart2.baudrate);

	uart_config(&uart1, 1);
	uart_config(&uart2, 1);

	Afsk_init();
	Beacon_init();


	autoResetTimer = autoReset * 360000;

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */

  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
	  Wdog_reset();

	  if(ax25.frameReceived)
		  handleFrame();

	  Digi_viscousRefresh(); //refresh viscous-delay buffers


	  Ax25_transmitBuffer(); //transmit buffer (will return if nothing to be transmitted)

	  Ax25_transmitCheck(); //check for pending transmission request

	  if(uart1.rxflag != DATA_NOTHING)
	  {
		  term_parse(uart1.bufrx, uart1.bufrxidx, TERM_UART1, uart1.rxflag, uart1.mode);
		  uart1.rxflag = DATA_NOTHING;
		  uart1.bufrxidx = 0;
		  memset(uart1.bufrx, 0, UARTBUFLEN);
	  }
	  if(uart2.rxflag != DATA_NOTHING)
	  {
		  term_parse(uart2.bufrx, uart2.bufrxidx, TERM_UART2, uart2.rxflag, uart2.mode);
		  uart2.rxflag = DATA_NOTHING;
		  uart2.bufrxidx = 0;
		  memset(uart2.bufrx, 0, UARTBUFLEN);
	  }

	  Beacon_check(); //check beacons


	  if(((autoResetTimer != 0) && (ticks > autoResetTimer)) || (ticks > 4294960000))
		  NVIC_SystemReset();



  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Initializes the CPU, AHB and APB busses clocks
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI_DIV2;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL16;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }
  /** Initializes the CPU, AHB and APB busses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */

  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     tex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
