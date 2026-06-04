/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file    FDCAN/FDCAN_Com_Polling/Src/main.c
 * @author  MCD Application Team
 * @brief   This sample code shows how to achieve Polling Process Communication
 *          between two FDCAN units.
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2024 STMicroelectronics.
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 *
 ******************************************************************************
 */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <string.h>
#include <stdio.h>
/* USER CODE END Includes */
/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
/* USER CODE END PTD */
/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define TX_ID          (0x111)   /* TX CAN message identifier    */
#define RX_ID          (0x111)   /* RX CAN message identifier    */
#define INA219_ADDR 0b1000000 // I2C address
#define INA219_REG_CONFIG 0x00 // config register address
#define INA219_REG_CURRENT 0x04 // current register
#define INA219_REG_CALIBRATION 0x05 // calibration register
#define CMAXINT 85
/* USER CODE END PD */
/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */
#define COUNTOF(BUFFER) (sizeof((BUFFER)) / sizeof(*(BUFFER)))
/* USER CODE END PM */
/* Private variables ---------------------------------------------------------*/
ADC_HandleTypeDef hadc1;
FDCAN_HandleTypeDef hfdcan1;
I2C_HandleTypeDef hi2c2;
TIM_HandleTypeDef htim1;
TIM_HandleTypeDef htim2;
UART_HandleTypeDef huart1;
UART_HandleTypeDef huart2;
/* USER CODE BEGIN PV */
FDCAN_RxHeaderTypeDef rxHeader;
FDCAN_TxHeaderTypeDef txHeader;
uint8_t rxData[16U];
static const uint8_t txData[] = {'H', 'E', 'L', 'L', 'O', 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
                               };
volatile int state = 0;
//Current Control Vars
volatile float desiredCurrent[400];
volatile float measuredCurrent[400];
volatile float Kp_current = .01;
volatile float Ki_current = .1;
volatile int current_tick = 0;
volatile float current_error = 0.0;
volatile float current_error_int = 0.0;
volatile int effort[400];

/* USER CODE END PV */
/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_FDCAN1_Init(void);
static void MX_ADC1_Init(void);
static void MX_TIM1_Init(void);
static void MX_I2C2_Init(void);
static void MX_TIM2_Init(void);
static void MX_USART1_UART_Init(void);
static void MX_USART2_UART_Init(void);
/* USER CODE BEGIN PFP */
static uint32_t BufferCmp8b(const uint8_t *pBuffer1, const uint8_t *pBuffer2, uint16_t BufferLength);
static uint32_t Read_ADC(void);
void init_ina219(void);
float read_ina219(void);
void writeINA219(int reg, int value);
signed short readINA219(unsigned char reg);
void set_speed (float speed);
/* USER CODE END PFP */
/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
/* USER CODE END 0 */
/**
 * @brief  The application entry point.
 * @retval int
 */
int main(void)
{
 /* USER CODE BEGIN 1 */
 /* STM32C0xx HAL library initialization:
      - Configure the Flash prefetch
      - Systick timer is configured by default as source of time base, but user
        can eventually implement his proper time base source (a general purpose
        timer for example or other time source), keeping in mind that Time base
        duration should be kept 1ms since PPP_TIMEOUT_VALUEs are defined and
        handled in milliseconds basis.
      - Low Level Initialization
    */
 /* USER CODE END 1 */
 /* MCU Configuration--------------------------------------------------------*/
 /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
 HAL_Init();
 /* USER CODE BEGIN Init */
 /* USER CODE END Init */
 /* Configure the system clock */
 SystemClock_Config();
 /* USER CODE BEGIN SysInit */
 /* Configure LED1 and LED2 */
 BSP_LED_Init(LED1);
 BSP_LED_Init(LED2);
 /* Configure User push-button in interrupt mode */
 BSP_PB_Init(BUTTON_USER, BUTTON_MODE_EXTI);
 /* USER CODE END SysInit */
 /* Initialize all configured peripherals */
 MX_GPIO_Init();
 MX_FDCAN1_Init();
 MX_ADC1_Init();
 MX_TIM1_Init();
 MX_I2C2_Init();
 MX_TIM2_Init();
 MX_USART1_UART_Init();
 MX_USART2_UART_Init();
 /* USER CODE BEGIN 2 */
 /* Configure reception filter to Rx FIFO 0 */
 FDCAN_FilterTypeDef        sFilterConfig;
 sFilterConfig.IdType       = FDCAN_STANDARD_ID;
 sFilterConfig.FilterIndex  = 0U;
 sFilterConfig.FilterType   = FDCAN_FILTER_MASK;
 sFilterConfig.FilterConfig = FDCAN_FILTER_TO_RXFIFO0;
 sFilterConfig.FilterID1    = TX_ID;
 sFilterConfig.FilterID2    = 0x7FF;
 if (HAL_FDCAN_ConfigFilter(&hfdcan1, &sFilterConfig) != HAL_OK)
 {
   Error_Handler();
 }
 /**
   *  Configure global filter:
   *    - Filter all remote frames with STD and EXT ID
   *    - Reject non matching frames with STD ID and EXT ID
   */
 if (HAL_FDCAN_ConfigGlobalFilter(&hfdcan1,
                                  FDCAN_REJECT, FDCAN_REJECT,
                                  FDCAN_FILTER_REMOTE, FDCAN_FILTER_REMOTE) != HAL_OK)
 {
   Error_Handler();
 }
 /* Prepare Tx message Header */
 txHeader.Identifier          = TX_ID;
 txHeader.IdType              = FDCAN_STANDARD_ID;
 txHeader.TxFrameType         = FDCAN_DATA_FRAME;
 txHeader.DataLength          = FDCAN_DLC_BYTES_16;
 txHeader.ErrorStateIndicator = FDCAN_ESI_ACTIVE;
 txHeader.BitRateSwitch       = FDCAN_BRS_ON;
 txHeader.FDFormat            = FDCAN_FD_CAN;
 txHeader.TxEventFifoControl  = FDCAN_NO_TX_EVENTS;
 txHeader.MessageMarker       = 0U;
 /**
   * Configure and enable Tx Delay Compensation, required for BRS mode.
   * TdcOffset default recommended value: DataTimeSeg1 * DataPrescaler
   * TdcFilter default recommended value: 0
   */
 if (HAL_FDCAN_ConfigTxDelayCompensation(&hfdcan1,
                                         (hfdcan1.Init.DataPrescaler * hfdcan1.Init.DataTimeSeg1), 0U) != HAL_OK)
 {
   Error_Handler();
 }
 if (HAL_FDCAN_EnableTxDelayCompensation(&hfdcan1) != HAL_OK)
 {
   Error_Handler();
 }
 /* Start FDCAN controller */
 if (HAL_FDCAN_Start(&hfdcan1) != HAL_OK)
 {
   Error_Handler();
 }
 /* USER CODE END 2 */
 /* Initialize leds */
 BSP_LED_Init(LED_GREEN);
 BSP_LED_Init(LED_BLUE);
 /* Infinite loop */
 /* USER CODE BEGIN WHILE */
 init_ina219();
 HAL_TIM_Base_Start_IT(&htim2);
 __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, 2400); //init PWM
 __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_2, 2400);
 HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1);
 HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_2);
 uint8_t inputChar;
 uint8_t buf[10000];  // buffer to hold the formatted string
 uint8_t lengthbuf[100];
 while (1)
 {
   /* Polling mode: Wait for one message received */
   while (HAL_FDCAN_GetRxFifoFillLevel(&hfdcan1, FDCAN_RX_FIFO0) < 1U)
   {
     /* Do nothing, wait */
   	BSP_LED_On(LED2);
   	if (HAL_UART_Receive(&huart2, &inputChar, 1, 10) == HAL_OK)
   	{
   	    if (inputChar == 'a')
   	    {
   	    	state=1;
   	    	while (state == 1){
   	    		BSP_LED_Off(LED2);
   	    	}
   	    	BSP_LED_On(LED2);
   	    	//need to print number of data points for python to read
   	    	sprintf(lengthbuf, "%d\n", 400);
   	    	HAL_UART_Transmit(&huart2, (uint8_t*)lengthbuf, strlen(lengthbuf), HAL_MAX_DELAY);
   	    	for (int i =0; i<400;i++){
   	    		sprintf(buf, "%d %d %d\n", i, (int)desiredCurrent[i], (int)measuredCurrent[i]);
   	    		HAL_UART_Transmit(&huart2, (uint8_t*)buf, strlen(buf), HAL_MAX_DELAY);
   	    	}
   	    }
   	}
   }
   /* Retrieve Rx message from RX FIFO0 */
   if (HAL_FDCAN_GetRxMessage(&hfdcan1, FDCAN_RX_FIFO0, &rxHeader, rxData) != HAL_OK)
   {
     Error_Handler();
   }
   /* Compare received RX message to expected data. Ignore if not matching. */
   if ((rxHeader.Identifier == RX_ID) &&
       (rxHeader.IdType     == FDCAN_STANDARD_ID) &&
       (rxHeader.DataLength == FDCAN_DLC_BYTES_16))
   {
     /* Turn LED1 on */
     BSP_LED_On(LED1);
     printf("Received: %s\r\n",txData);
   }
   /* USER CODE END WHILE */
   /* USER CODE BEGIN 3 */
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
 __HAL_FLASH_SET_LATENCY(FLASH_LATENCY_1);
 /** Initializes the RCC Oscillators according to the specified parameters
 * in the RCC_OscInitTypeDef structure.
 */
 RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
 RCC_OscInitStruct.HSIState = RCC_HSI_ON;
 RCC_OscInitStruct.HSIDiv = RCC_HSI_DIV1;
 RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
 if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
 {
   Error_Handler();
 }
 /** Initializes the CPU, AHB and APB buses clocks
 */
 RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                             |RCC_CLOCKTYPE_PCLK1;
 RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
 RCC_ClkInitStruct.SYSCLKDivider = RCC_SYSCLK_DIV1;
 RCC_ClkInitStruct.AHBCLKDivider = RCC_HCLK_DIV1;
 RCC_ClkInitStruct.APB1CLKDivider = RCC_APB1_DIV1;
 if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_1) != HAL_OK)
 {
   Error_Handler();
 }
}
/**
 * @brief ADC1 Initialization Function
 * @param None
 * @retval None
 */
static void MX_ADC1_Init(void)
{
 /* USER CODE BEGIN ADC1_Init 0 */
 /* USER CODE END ADC1_Init 0 */
 ADC_ChannelConfTypeDef sConfig = {0};
 /* USER CODE BEGIN ADC1_Init 1 */
 /* USER CODE END ADC1_Init 1 */
 /** Configure the global features of the ADC (Clock, Resolution, Data Alignment and number of conversion)
 */
 hadc1.Instance = ADC1;
 hadc1.Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV2;
 hadc1.Init.Resolution = ADC_RESOLUTION_12B;
 hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
 hadc1.Init.ScanConvMode = ADC_SCAN_SEQ_FIXED;
 hadc1.Init.EOCSelection = ADC_EOC_SINGLE_CONV;
 hadc1.Init.LowPowerAutoWait = DISABLE;
 hadc1.Init.LowPowerAutoPowerOff = DISABLE;
 hadc1.Init.ContinuousConvMode = DISABLE;
 hadc1.Init.NbrOfConversion = 1;
 hadc1.Init.DiscontinuousConvMode = DISABLE;
 hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;
 hadc1.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
 hadc1.Init.DMAContinuousRequests = DISABLE;
 hadc1.Init.Overrun = ADC_OVR_DATA_PRESERVED;
 hadc1.Init.SamplingTimeCommon1 = ADC_SAMPLETIME_1CYCLE_5;
 hadc1.Init.OversamplingMode = DISABLE;
 hadc1.Init.TriggerFrequencyMode = ADC_TRIGGER_FREQ_HIGH;
 if (HAL_ADC_Init(&hadc1) != HAL_OK)
 {
   Error_Handler();
 }
 /** Configure Regular Channel
 */
 sConfig.Channel = ADC_CHANNEL_0;
 sConfig.Rank = ADC_RANK_CHANNEL_NUMBER;
 if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
 {
   Error_Handler();
 }
 /* USER CODE BEGIN ADC1_Init 2 */
 /* USER CODE END ADC1_Init 2 */
}
/**
 * @brief FDCAN1 Initialization Function
 * @param None
 * @retval None
 */
static void MX_FDCAN1_Init(void)
{
 /* USER CODE BEGIN FDCAN1_Init 0 */
 /* USER CODE END FDCAN1_Init 0 */
 /* USER CODE BEGIN FDCAN1_Init 1 */
 /* USER CODE END FDCAN1_Init 1 */
 hfdcan1.Instance = FDCAN1;
 hfdcan1.Init.ClockDivider = FDCAN_CLOCK_DIV1;
 hfdcan1.Init.FrameFormat = FDCAN_FRAME_FD_BRS;
 hfdcan1.Init.Mode = FDCAN_MODE_NORMAL;
 hfdcan1.Init.AutoRetransmission = ENABLE;
 hfdcan1.Init.TransmitPause = ENABLE;
 hfdcan1.Init.ProtocolException = DISABLE;
 hfdcan1.Init.NominalPrescaler = 1;
 hfdcan1.Init.NominalSyncJumpWidth = 12;
 hfdcan1.Init.NominalTimeSeg1 = 35;
 hfdcan1.Init.NominalTimeSeg2 = 12;
 hfdcan1.Init.DataPrescaler = 1;
 hfdcan1.Init.DataSyncJumpWidth = 6;
 hfdcan1.Init.DataTimeSeg1 = 17;
 hfdcan1.Init.DataTimeSeg2 = 6;
 hfdcan1.Init.StdFiltersNbr = 1;
 hfdcan1.Init.ExtFiltersNbr = 0;
 hfdcan1.Init.TxFifoQueueMode = FDCAN_TX_FIFO_OPERATION;
 if (HAL_FDCAN_Init(&hfdcan1) != HAL_OK)
 {
   Error_Handler();
 }
 /* USER CODE BEGIN FDCAN1_Init 2 */
 /* USER CODE END FDCAN1_Init 2 */
}
/**
 * @brief I2C2 Initialization Function
 * @param None
 * @retval None
 */
static void MX_I2C2_Init(void)
{
 /* USER CODE BEGIN I2C2_Init 0 */
 /* USER CODE END I2C2_Init 0 */
 /* USER CODE BEGIN I2C2_Init 1 */
 /* USER CODE END I2C2_Init 1 */
 hi2c2.Instance = I2C2;
 hi2c2.Init.Timing = 0x10805D88;
 hi2c2.Init.OwnAddress1 = 0;
 hi2c2.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
 hi2c2.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
 hi2c2.Init.OwnAddress2 = 0;
 hi2c2.Init.OwnAddress2Masks = I2C_OA2_NOMASK;
 hi2c2.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
 hi2c2.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
 if (HAL_I2C_Init(&hi2c2) != HAL_OK)
 {
   Error_Handler();
 }
 /** Configure Analogue filter
 */
 if (HAL_I2CEx_ConfigAnalogFilter(&hi2c2, I2C_ANALOGFILTER_ENABLE) != HAL_OK)
 {
   Error_Handler();
 }
 /** Configure Digital filter
 */
 if (HAL_I2CEx_ConfigDigitalFilter(&hi2c2, 0) != HAL_OK)
 {
   Error_Handler();
 }
 /* USER CODE BEGIN I2C2_Init 2 */
 /* USER CODE END I2C2_Init 2 */
}
/**
 * @brief TIM1 Initialization Function
 * @param None
 * @retval None
 */
static void MX_TIM1_Init(void)
{
 /* USER CODE BEGIN TIM1_Init 0 */
 /* USER CODE END TIM1_Init 0 */
 TIM_MasterConfigTypeDef sMasterConfig = {0};
 TIM_OC_InitTypeDef sConfigOC = {0};
 TIM_BreakDeadTimeConfigTypeDef sBreakDeadTimeConfig = {0};
 /* USER CODE BEGIN TIM1_Init 1 */
 /* USER CODE END TIM1_Init 1 */
 htim1.Instance = TIM1;
 htim1.Init.Prescaler = 0;
 htim1.Init.CounterMode = TIM_COUNTERMODE_UP;
 htim1.Init.Period = 2400;
 htim1.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
 htim1.Init.RepetitionCounter = 0;
 htim1.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
 if (HAL_TIM_PWM_Init(&htim1) != HAL_OK)
 {
   Error_Handler();
 }
 sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
 sMasterConfig.MasterOutputTrigger2 = TIM_TRGO2_RESET;
 sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
 if (HAL_TIMEx_MasterConfigSynchronization(&htim1, &sMasterConfig) != HAL_OK)
 {
   Error_Handler();
 }
 sConfigOC.OCMode = TIM_OCMODE_PWM1;
 sConfigOC.Pulse = 0;
 sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
 sConfigOC.OCNPolarity = TIM_OCNPOLARITY_HIGH;
 sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
 sConfigOC.OCIdleState = TIM_OCIDLESTATE_RESET;
 sConfigOC.OCNIdleState = TIM_OCNIDLESTATE_RESET;
 if (HAL_TIM_PWM_ConfigChannel(&htim1, &sConfigOC, TIM_CHANNEL_1) != HAL_OK)
 {
   Error_Handler();
 }
 if (HAL_TIM_PWM_ConfigChannel(&htim1, &sConfigOC, TIM_CHANNEL_2) != HAL_OK)
 {
   Error_Handler();
 }
 sBreakDeadTimeConfig.OffStateRunMode = TIM_OSSR_DISABLE;
 sBreakDeadTimeConfig.OffStateIDLEMode = TIM_OSSI_DISABLE;
 sBreakDeadTimeConfig.LockLevel = TIM_LOCKLEVEL_OFF;
 sBreakDeadTimeConfig.DeadTime = 0;
 sBreakDeadTimeConfig.BreakState = TIM_BREAK_DISABLE;
 sBreakDeadTimeConfig.BreakPolarity = TIM_BREAKPOLARITY_HIGH;
 sBreakDeadTimeConfig.BreakFilter = 0;
 sBreakDeadTimeConfig.BreakAFMode = TIM_BREAK_AFMODE_INPUT;
 sBreakDeadTimeConfig.Break2State = TIM_BREAK2_DISABLE;
 sBreakDeadTimeConfig.Break2Polarity = TIM_BREAK2POLARITY_HIGH;
 sBreakDeadTimeConfig.Break2Filter = 0;
 sBreakDeadTimeConfig.Break2AFMode = TIM_BREAK_AFMODE_INPUT;
 sBreakDeadTimeConfig.AutomaticOutput = TIM_AUTOMATICOUTPUT_DISABLE;
 if (HAL_TIMEx_ConfigBreakDeadTime(&htim1, &sBreakDeadTimeConfig) != HAL_OK)
 {
   Error_Handler();
 }
 /* USER CODE BEGIN TIM1_Init 2 */
 /* USER CODE END TIM1_Init 2 */
 HAL_TIM_MspPostInit(&htim1);
}
/**
 * @brief TIM2 Initialization Function
 * @param None
 * @retval None
 */
static void MX_TIM2_Init(void)
{
 /* USER CODE BEGIN TIM2_Init 0 */
 /* USER CODE END TIM2_Init 0 */
 TIM_ClockConfigTypeDef sClockSourceConfig = {0};
 TIM_MasterConfigTypeDef sMasterConfig = {0};
 /* USER CODE BEGIN TIM2_Init 1 */
 /* USER CODE END TIM2_Init 1 */
 htim2.Instance = TIM2;
 htim2.Init.Prescaler = 0;
 htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
 htim2.Init.Period = 48000;
 htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
 htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
 if (HAL_TIM_Base_Init(&htim2) != HAL_OK)
 {
   Error_Handler();
 }
 sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
 if (HAL_TIM_ConfigClockSource(&htim2, &sClockSourceConfig) != HAL_OK)
 {
   Error_Handler();
 }
 sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
 sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
 if (HAL_TIMEx_MasterConfigSynchronization(&htim2, &sMasterConfig) != HAL_OK)
 {
   Error_Handler();
 }
 /* USER CODE BEGIN TIM2_Init 2 */
 /* USER CODE END TIM2_Init 2 */
}
/**
 * @brief USART1 Initialization Function
 * @param None
 * @retval None
 */
static void MX_USART1_UART_Init(void)
{
 /* USER CODE BEGIN USART1_Init 0 */
 /* USER CODE END USART1_Init 0 */
 /* USER CODE BEGIN USART1_Init 1 */
 /* USER CODE END USART1_Init 1 */
 huart1.Instance = USART1;
 huart1.Init.BaudRate = 115200;
 huart1.Init.WordLength = UART_WORDLENGTH_8B;
 huart1.Init.StopBits = UART_STOPBITS_1;
 huart1.Init.Parity = UART_PARITY_NONE;
 huart1.Init.Mode = UART_MODE_TX_RX;
 huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
 huart1.Init.OverSampling = UART_OVERSAMPLING_16;
 huart1.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
 huart1.Init.ClockPrescaler = UART_PRESCALER_DIV1;
 huart1.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
 if (HAL_UART_Init(&huart1) != HAL_OK)
 {
   Error_Handler();
 }
 if (HAL_UARTEx_SetTxFifoThreshold(&huart1, UART_TXFIFO_THRESHOLD_1_8) != HAL_OK)
 {
   Error_Handler();
 }
 if (HAL_UARTEx_SetRxFifoThreshold(&huart1, UART_RXFIFO_THRESHOLD_1_8) != HAL_OK)
 {
   Error_Handler();
 }
 if (HAL_UARTEx_DisableFifoMode(&huart1) != HAL_OK)
 {
   Error_Handler();
 }
 /* USER CODE BEGIN USART1_Init 2 */
 /* USER CODE END USART1_Init 2 */
}
/**
 * @brief USART2 Initialization Function
 * @param None
 * @retval None
 */
static void MX_USART2_UART_Init(void)
{
 /* USER CODE BEGIN USART2_Init 0 */
 /* USER CODE END USART2_Init 0 */
 /* USER CODE BEGIN USART2_Init 1 */
 /* USER CODE END USART2_Init 1 */
 huart2.Instance = USART2;
 huart2.Init.BaudRate = 115200;
 huart2.Init.WordLength = UART_WORDLENGTH_8B;
 huart2.Init.StopBits = UART_STOPBITS_1;
 huart2.Init.Parity = UART_PARITY_NONE;
 huart2.Init.Mode = UART_MODE_TX_RX;
 huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
 huart2.Init.OverSampling = UART_OVERSAMPLING_16;
 huart2.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
 huart2.Init.ClockPrescaler = UART_PRESCALER_DIV1;
 huart2.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
 if (HAL_UART_Init(&huart2) != HAL_OK)
 {
   Error_Handler();
 }
 /* USER CODE BEGIN USART2_Init 2 */
 /* USER CODE END USART2_Init 2 */
}
/**
 * @brief GPIO Initialization Function
 * @param None
 * @retval None
 */
static void MX_GPIO_Init(void)
{
 /* USER CODE BEGIN MX_GPIO_Init_1 */
 /* USER CODE END MX_GPIO_Init_1 */
 /* GPIO Ports Clock Enable */
 __HAL_RCC_GPIOC_CLK_ENABLE();
 __HAL_RCC_GPIOA_CLK_ENABLE();
 __HAL_RCC_GPIOD_CLK_ENABLE();
 /* USER CODE BEGIN MX_GPIO_Init_2 */
 /* USER CODE END MX_GPIO_Init_2 */
}
/* USER CODE BEGIN 4 */
/**
 * @brief  BSP User push-button callback
 * @param  Button Specifies the pin connected EXTI line
 * @retval None.
 */
void BSP_PB_Callback(Button_TypeDef Button)
{
 if (Button == BUTTON_USER)
 {
   /* Turn LED1 off */
   BSP_LED_Off(LED1);
   /* Add message to TX FIFO */
   if (HAL_FDCAN_AddMessageToTxFifoQ(&hfdcan1, &txHeader, txData) != HAL_OK)
   {
     Error_Handler();
   }
   /* Delay for simple button debounce */
   HAL_Delay(100U);
 }
}
/**
 * @brief  Compares two buffers.
 * @param  pBuffer1 buffer to be compared.
 * @param  pBuffer2 buffer to be compared.
 * @param  BufferLength: buffer's length.
 * @retval 0: pBuffer1 is identical to pBuffer2
 * @retval 1: pBuffer1 differs from pBuffer2
 */
static uint32_t BufferCmp8b(const uint8_t *pBuffer1, const uint8_t *pBuffer2, uint16_t BufferLength)
{
 while (BufferLength--)
 {
   if (*pBuffer1 != *pBuffer2)
   {
     return 1U;
   }
   pBuffer1++;
   pBuffer2++;
 }
 return 0U;
}
/**
 * @brief  Reads ADC.
 * @param  None
 * @retval None
 */
static uint32_t Read_ADC(void)
{
   uint32_t raw;
   HAL_ADC_Start(&hadc1);
   if (HAL_ADC_PollForConversion(&hadc1, 10) == HAL_OK)
   {
       raw = HAL_ADC_GetValue(&hadc1);
   }
   else
   {
       raw = 0;
   }
   HAL_ADC_Stop(&hadc1);
   return raw;
}
void init_ina219(){
   // set the INA219 sensitivity - 10 bit, plus/minus160mV, 148us per sample
   unsigned short ina219_calValue = 1024;
   unsigned short ina219_config = 0b0011000010001111;
   writeINA219(INA219_REG_CALIBRATION, ina219_calValue);
   writeINA219(INA219_REG_CONFIG, ina219_config);
}
float read_ina219(){
   float ma = 0;
   signed short value = readINA219(INA219_REG_CURRENT);
   ma = value / 3.0;
   return ma;
}
// write 2 bytes
void writeINA219(int reg, int value){
   uint8_t buf[3];
   buf[0] = reg;
   buf[1] = value>>8;
   buf[2] = value&0xff;
   HAL_I2C_Master_Transmit(&hi2c2, INA219_ADDR<<1, buf, 3, 10);
}
// read 2 bytes
signed short readINA219(unsigned char reg){
   HAL_I2C_Master_Transmit(&hi2c2, INA219_ADDR<<1, &reg, 1, 10);
   uint8_t buffer[2];
   HAL_I2C_Master_Receive(&hi2c2, INA219_ADDR<<1, buffer, 2, 10);
   signed short value = (buffer[0]<<8)|buffer[1];
   return value;
}
// timer based interrupt
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
   if (htim == &htim2) {
   	if (state ==1){
   		BSP_LED_On(LED1); //do nothing
           float desired = 0;
           float measured = read_ina219();
           // Select desired current
           if (current_tick < 100) {
               desired = -100.0;
           }
           else if (current_tick < 200) {
               desired = 100.0;
           }
           else if (current_tick < 300) {
               desired = -100.0;
           }
           else if (current_tick < 400) {
               desired = 100.0;
           }
           // Store data
           desiredCurrent[current_tick] = desired;
           measuredCurrent[current_tick] = measured;
           // PI control
           current_error = desired - measured;
           current_error_int += current_error;
           //eint max
           if (current_error_int > CMAXINT) {
               current_error_int = CMAXINT;
           }
           else if (current_error_int < -CMAXINT) {
               current_error_int = -CMAXINT;
           }
           float u = Kp_current * current_error + Ki_current * current_error_int;
           if (u > 100.0) {
               u = 100.0;
           }
           else if (u < -100.0) {
               u = -100.0;
           }

           effort[current_tick]=u;
           // Apply control
           set_speed(u);
           current_tick++;
           if (current_tick >= 400) {
               current_tick = 0;
               current_error = 0.0;
               current_error_int = 0.0;
               state = 0;
               set_speed(0);
               BSP_LED_Off(LED1); //do nothing
           }
       }
   }
}
//Motor Direction and Speed function
void set_speed (float speed)
//need to do error checking to make sure the PWM is not being set to something negative or greater than 2400
{
	if (speed>0){//forward
		__HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, 2400); //leave PWM1 at 100 percent
		__HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_2, (int)((100.0 - speed) / 100.0 * 2400.0)); //set PWM2 to (100-duty) % duty cycle
	}
	else if (speed<0){//reverse
		__HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_2, 2400); //leave PWM2 at 100 percent
		__HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, (int)((100.0 + speed) / 100.0 * 2400.0)); //set PWM2 to (100-duty) % duty cycle
	}
	else{//turn motor off
			__HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, 2400);
			__HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_2, 2400);
		}
}
/* USER CODE END 4 */
/**
 * @brief  This function is executed in case of error occurrence.
 * @retval None
 */
void Error_Handler(void)
{
 /* USER CODE BEGIN Error_Handler_Debug */
 /* User can add his own implementation to report the HAL error return state */
 while (1)
 {
   /* Toggle LED2 on */
   BSP_LED_Toggle(LED2);
   /* 1s delay */
   HAL_Delay(1000U);
 }
 /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
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
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
 /* Infinite loop */
 while (1)
 {
 }
 /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */

