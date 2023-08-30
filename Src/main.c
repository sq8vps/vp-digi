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
#include "usb_device.h"

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
#ifdef ENABLE_FX25
#include "fx25.h"
#endif
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
 * @brief Handle received frame
 */
static void handleFrame(void)
{
	uint8_t modemBitmap = Ax25GetReceivedFrameBitmap(); //store states
	Ax25ClearReceivedFrameBitmap();

	uint8_t *buf;
	uint16_t size = 0;
	int8_t peak = 0;
	int8_t valley = 0;
	uint8_t signalLevel = 0;
	uint8_t fixed = 0;

	while(Ax25ReadNextRxFrame(&buf, &size, &peak, &valley, &signalLevel, &fixed))
	{
		TermSendToAll(MODE_KISS, buf, size);

		if(((UartUsb.mode == MODE_MONITOR) || (Uart1.mode == MODE_MONITOR) || (Uart2.mode == MODE_MONITOR)))
		{
			if(signalLevel > 70)
			{
				TermSendToAll(MODE_MONITOR, (uint8_t*)"\r\nInput level too high! Please reduce so most stations are around 30-50%.\r\n", 0);
			}
			else if(signalLevel < 5)
			{
				TermSendToAll(MODE_MONITOR, (uint8_t*)"\r\nInput level too low! Please increase so most stations are around 30-50%.\r\n", 0);
			}

			TermSendToAll(MODE_MONITOR, (uint8_t*)"(AX.25) Frame received [", 0);
			for(uint8_t i = 0; i < ModemGetDemodulatorCount(); i++)
			{
				if(modemBitmap & (1 << i))
				{
					enum ModemPrefilter m = ModemGetFilterType(i);
					switch(m)
					{
						case PREFILTER_PREEMPHASIS:
							TermSendToAll(MODE_MONITOR, (uint8_t*)"P", 1);
							break;
						case PREFILTER_DEEMPHASIS:
							TermSendToAll(MODE_MONITOR, (uint8_t*)"D", 1);
							break;
						case PREFILTER_FLAT:
							TermSendToAll(MODE_MONITOR, (uint8_t*)"F", 1);
							break;
						case PREFILTER_NONE:
							TermSendToAll(MODE_MONITOR, (uint8_t*)"*", 1);
							break;
					}
				}
				else
					TermSendToAll(MODE_MONITOR, (uint8_t*)"_", 1);
			}

			TermSendToAll(MODE_MONITOR, (uint8_t*)"], ", 0);
			if(fixed != AX25_NOT_FX25)
			{
				TermSendNumberToAll(MODE_MONITOR, fixed);
				TermSendToAll(MODE_MONITOR, (uint8_t*)" bytes fixed, ", 0);
			}
			TermSendToAll(MODE_MONITOR, (uint8_t*)"signal level ", 0);
			TermSendNumberToAll(MODE_MONITOR, signalLevel);
			TermSendToAll(MODE_MONITOR, (uint8_t*)"% (", 0);
			TermSendNumberToAll(MODE_MONITOR, peak);
			TermSendToAll(MODE_MONITOR, (uint8_t*)"%/", 0);
			TermSendNumberToAll(MODE_MONITOR, valley);
			TermSendToAll(MODE_MONITOR, (uint8_t*)"%): ", 0);

			SendTNC2(buf, size);
			TermSendToAll(MODE_MONITOR, (uint8_t*)"\r\n", 0);
		}


		DigiDigipeat(buf, size);
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

  SysTickInit();

  //force usb re-enumeration after reset
  RCC->APB2ENR |= RCC_APB2ENR_IOPAEN; //pull D+ to ground for a moment
  GPIOA->CRH |= GPIO_CRH_MODE12_1;
  GPIOA->CRH &= ~GPIO_CRH_CNF12;
  GPIOA->BSRR = GPIO_BSRR_BR12;
  Delay(100);
  GPIOA->CRH &= ~GPIO_CRH_MODE12;
  GPIOA->CRH |= GPIO_CRH_CNF12_0;


  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_USB_DEVICE_Init();
  /* USER CODE BEGIN 2 */



	WdogInit(); //initialize watchdog

	memset(&beacon, 0, sizeof(beacon));
	memset(&Ax25Config, 0, sizeof(Ax25Config));
	memset(&ModemConfig, 0, sizeof(ModemConfig));
	memset(&DigiConfig, 0, sizeof(DigiConfig));

	//set some initial values in case there is no configuration saved in memory
	Uart1.baudrate = 9600;
	Uart2.baudrate = 9600;
	ModemConfig.usePWM = 0;
	ModemConfig.flatAudioIn = 0;
	Ax25Config.quietTime = 300;
	Ax25Config.txDelayLength = 300;
	Ax25Config.txTailLength = 30;
	DigiConfig.dupeTime = 30;

	ConfigRead();

	Ax25Init();
#ifdef ENABLE_FX25
	Fx25Init();
#endif

	UartInit(&Uart1, USART1, Uart1.baudrate);
	UartInit(&Uart2, USART2, Uart2.baudrate);
	UartInit(&UartUsb, NULL, 1);

	UartConfig(&Uart1, 1);
	UartConfig(&Uart2, 1);
	UartConfig(&UartUsb, 1);

	ModemInit();
	BeaconInit();

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */

  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
	  WdogReset();

	  if(Ax25GetReceivedFrameBitmap())
		  handleFrame();

	  DigiViscousRefresh(); //refresh viscous-delay buffers

	  Ax25TransmitBuffer(); //transmit buffer (will return if nothing to be transmitted)

	  Ax25TransmitCheck(); //check for pending transmission request

	  if(UartUsb.rxType != DATA_NOTHING)
	  {
		  TermHandleSpecial(&UartUsb);
		  if(UartUsb.rxType != DATA_USB)
		  {
			  TermParse(&UartUsb);
		  	  UartClearRx(&UartUsb);
		  }
		  UartUsb.rxType = DATA_NOTHING;
	  }
	  if(Uart1.rxType != DATA_NOTHING)
	  {
		  TermParse(&Uart1);
		  UartClearRx(&Uart1);
	  }
	  if(Uart2.rxType != DATA_NOTHING)
	  {
		  TermParse(&Uart2);
		  UartClearRx(&Uart2);
	  }
	  UartHandleKissTimeout(&UartUsb);

	  BeaconCheck(); //check beacons

	  if(SysTickGet() > 0xFFFFF000) //going to wrap around soon - hard reset the device
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
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

  /** Initializes the CPU, AHB and APB busses clocks
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
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
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_USB;
  PeriphClkInit.UsbClockSelection = RCC_USBCLKSOURCE_PLL_DIV1_5;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
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
  __HAL_RCC_GPIOD_CLK_ENABLE();
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
