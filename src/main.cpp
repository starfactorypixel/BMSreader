//#include <math.h>
#include "main.h"
#include <ConstantLibrary.h>
#include <LoggerLibrary.h>
#include <SPI.h>
#include <About.h>
#include <Leds.h>
#include <CANLogic.h>
#include <Temperature.h>
#include <BMSLogic.h>
#include <Analog.h>
#include <OneWire.h>


ADC_HandleTypeDef hadc1;
CAN_HandleTypeDef hcan;
SPI_HandleTypeDef hspi2;
UART_HandleTypeDef hDebugUart;
UART_HandleTypeDef hBms1Uart;
UART_HandleTypeDef hBms2Uart;

TIM_HandleTypeDef htim1;


void SystemClock_Config(void);
static void MX_ADC1_Init(void);
static void MX_CAN_Init(void);
static void MX_SPI2_Init(void);
static void MX_USART1_UART_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_USART3_UART_Init(void);
static void MX_GPIO_Init(void);
static void MX_TIM1_Init(void);


void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *hcan)
{
	CAN_RxHeaderTypeDef RxHeader = {0};
	uint8_t RxData[8] = {0};
	
	if( HAL_CAN_GetRxMessage(hcan, CAN_RX_FIFO0, &RxHeader, RxData) == HAL_OK )
	{
		CANLib::can_manager.IncomingCANFrame(RxHeader.StdId, RxData, RxHeader.DLC);
	}
	
	return;
}

void HAL_CAN_ErrorCallback(CAN_HandleTypeDef *hcan)
{
	Leds::obj.SetOn(Leds::LED_RED, 100);
	
	DEBUG_LOG_TOPIC("CAN", "RX error event, code: 0x%08lX\n", HAL_CAN_GetError(hcan));
	
	return;
}

void HAL_CAN_Send(can_object_id_t id, uint8_t *data, uint8_t length)
{
	CAN_TxHeaderTypeDef TxHeader = {0};
	uint8_t TxData[8] = {0};
	uint32_t TxMailbox = 0;
	
	TxHeader.StdId = id;
	TxHeader.ExtId = 0;
	TxHeader.RTR  = CAN_RTR_DATA;
	TxHeader.IDE = CAN_ID_STD;
	TxHeader.DLC = length;
	TxHeader.TransmitGlobalTime = DISABLE;
	memcpy(TxData, data, length);
	
	while( HAL_CAN_GetTxMailboxesFreeLevel(&hcan) == 0 )
	{
		Leds::obj.SetOn(Leds::LED_RED);
	}
	Leds::obj.SetOff(Leds::LED_RED);
	
	if( HAL_CAN_AddTxMessage(&hcan, &TxHeader, TxData, &TxMailbox) != HAL_OK )
	{
		Leds::obj.SetOn(Leds::LED_RED, 100);

		DEBUG_LOG_TOPIC("CAN", "TX error event, code: 0x%08lX\n", HAL_CAN_GetError(&hcan));
	}
	
	return;
}




void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size)
{
	BMSLogic::data_t *obj;
	
	// USART1 == Debug
	if(huart->Instance == USART1)
	{

	}
	
	// USART2 == BMS1
	else if(huart->Instance == USART2)
	{
		obj = &BMSLogic::uart_data[BMSLogic::BMS_1];
		
		BMSLogic::UART_RX(BMSLogic::BMS_1, Size);
		
		HAL_UARTEx_ReceiveToIdle_IT(obj->hal, obj->hot, sizeof(obj->hot));
	}
	
	// USART3 == BMS2
	else if(huart->Instance == USART3)
	{
		obj = &BMSLogic::uart_data[BMSLogic::BMS_2];
		
		BMSLogic::UART_RX(BMSLogic::BMS_2, Size);
		
		HAL_UARTEx_ReceiveToIdle_IT(obj->hal, obj->hot, sizeof(obj->hot));
	}
}

void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
{
	BMSLogic::data_t *obj;

	// USART1 == Debug
	if(huart->Instance == USART1)
	{
		DEBUG_LOG_TOPIC("USART1", "ERR: %d\n", huart->ErrorCode);
		
		HAL_UART_AbortReceive_IT(&hDebugUart);
	}
	
	// USART2 == BMS1
	else if(huart->Instance == USART2)
	{
		obj = &BMSLogic::uart_data[BMSLogic::BMS_1];
		
		DEBUG_LOG_TOPIC("USART2", "ERR: %d\n", huart->ErrorCode);
		
		HAL_UART_AbortReceive_IT(obj->hal);
		HAL_UARTEx_ReceiveToIdle_IT(obj->hal, obj->hot, sizeof(obj->hot));
	}
	
	// USART3 == BMS2
	else if(huart->Instance == USART3)
	{
		obj = &BMSLogic::uart_data[BMSLogic::BMS_2];
		
		DEBUG_LOG_TOPIC("USART3", "ERR: %d\n", huart->ErrorCode);
		
		HAL_UART_AbortReceive_IT(obj->hal);
		HAL_UARTEx_ReceiveToIdle_IT(obj->hal, obj->hot, sizeof(obj->hot));
	}
}


int main(void)
{
    /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
    HAL_Init();

    /* Configure the system clock */
	SystemClock_Config();

	MX_ADC1_Init();
	MX_CAN_Init();
	MX_SPI2_Init();
	MX_USART1_UART_Init();
	MX_USART2_UART_Init();
	MX_USART3_UART_Init();
	MX_GPIO_Init();
	MX_TIM1_Init();
	

    // Сразу после инициализации периферии, иначе программа упадёт, если попробовать включить диод.
    // Green LED:  lights up, when programm falls in the Error_Handler()
    // Blue LED:   is unused yet
    // Red LED:    is unused yet
    // Yellow LED: is on while free CAN mailboxes are not available.
    //             When at least one mailbox is free, LED will go off.
    About::Setup();
    Leds::Setup();
	SPI::Setup();
	BMSLogic::Setup();
	OneWire::Setup();
	Temp::Setup();
    CANLib::Setup();

	uint32_t current_time = HAL_GetTick();
	while (1)
	{
		About::Loop(current_time);
		Leds::Loop(current_time);
		SPI::Loop(current_time);
		BMSLogic::Loop(current_time);
		OneWire::Loop(current_time);
		Temp::Loop(current_time);
		CANLib::Loop(current_time);
	}
}

void SystemClock_Config(void)
{
	RCC_OscInitTypeDef RCC_OscInitStruct = {0};
	RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
	RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

	RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
	RCC_OscInitStruct.HSEState = RCC_HSE_ON;
	RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
	RCC_OscInitStruct.HSIState = RCC_HSI_ON;
	RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
	RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
	RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL8;
	if(HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
	{
		Error_Handler();
	}

	RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK|RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
	RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
	RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
	RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
	RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
	if(HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
	{
		Error_Handler();
	}
	
	PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_ADC;
	PeriphClkInit.AdcClockSelection = RCC_ADCPCLK2_DIV8;
	if(HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
	{
		Error_Handler();
	}
}

static void MX_ADC1_Init(void)
{
	ADC_ChannelConfTypeDef sConfig = {0};
	
	hadc1.Instance = ADC1;
	hadc1.Init.ScanConvMode = ADC_SCAN_DISABLE;
	hadc1.Init.ContinuousConvMode = DISABLE;
	hadc1.Init.DiscontinuousConvMode = DISABLE;
	hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;
	hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
	hadc1.Init.NbrOfConversion = 1;
	if(HAL_ADC_Init(&hadc1) != HAL_OK)
	{
		Error_Handler();
	}

	sConfig.Channel = ADC_CHANNEL_9;
	sConfig.Rank = ADC_REGULAR_RANK_1;
	sConfig.SamplingTime = ADC_SAMPLETIME_1CYCLE_5;
	if(HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
	{
		Error_Handler();
	}
}

static void MX_CAN_Init(void)
{
	CAN_FilterTypeDef sFilterConfig;
	
	hcan.Instance = CAN1;
	hcan.Init.Prescaler = 4;
	hcan.Init.Mode = CAN_MODE_NORMAL;
	hcan.Init.SyncJumpWidth = CAN_SJW_1TQ;
	hcan.Init.TimeSeg1 = CAN_BS1_13TQ;
	hcan.Init.TimeSeg2 = CAN_BS2_2TQ;
	hcan.Init.TimeTriggeredMode = DISABLE;
	hcan.Init.AutoBusOff = ENABLE;
	hcan.Init.AutoWakeUp = ENABLE;
	hcan.Init.AutoRetransmission = DISABLE;
	hcan.Init.ReceiveFifoLocked = ENABLE;
	hcan.Init.TransmitFifoPriority = ENABLE;
	if(HAL_CAN_Init(&hcan) != HAL_OK)
	{
		Error_Handler();
	}

	sFilterConfig.FilterBank = 0;
	sFilterConfig.FilterMode = CAN_FILTERMODE_IDMASK;
	sFilterConfig.FilterScale = CAN_FILTERSCALE_32BIT;
	sFilterConfig.FilterIdHigh = 0x0000;
	sFilterConfig.FilterIdLow = 0x0000;
	sFilterConfig.FilterMaskIdHigh = 0x0000;
	sFilterConfig.FilterMaskIdLow = 0x0000;
	sFilterConfig.FilterFIFOAssignment = CAN_RX_FIFO0;
	sFilterConfig.FilterActivation = ENABLE;
	// sFilterConfig.SlaveStartFilterBank = 14;
	if(HAL_CAN_ConfigFilter(&hcan, &sFilterConfig) != HAL_OK)
	{
		Error_Handler();
	}
}

static void MX_SPI2_Init(void)
{
	hspi2.Instance = SPI2;
	hspi2.Init.Mode = SPI_MODE_MASTER;
	hspi2.Init.Direction = SPI_DIRECTION_2LINES;
	hspi2.Init.DataSize = SPI_DATASIZE_8BIT;
	hspi2.Init.CLKPolarity = SPI_POLARITY_LOW;
	hspi2.Init.CLKPhase = SPI_PHASE_1EDGE;
	hspi2.Init.NSS = SPI_NSS_SOFT;
	hspi2.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_256;
	hspi2.Init.FirstBit = SPI_FIRSTBIT_MSB;
	hspi2.Init.TIMode = SPI_TIMODE_DISABLE;
	hspi2.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
	hspi2.Init.CRCPolynomial = 10;
	if(HAL_SPI_Init(&hspi2) != HAL_OK)
	{
		Error_Handler();
	}
}

static void MX_USART1_UART_Init(void)
{
	hDebugUart.Instance = USART1;
	hDebugUart.Init.BaudRate = 500000;
	hDebugUart.Init.WordLength = UART_WORDLENGTH_8B;
	hDebugUart.Init.StopBits = UART_STOPBITS_1;
	hDebugUart.Init.Parity = UART_PARITY_NONE;
	hDebugUart.Init.Mode = UART_MODE_TX_RX;
	hDebugUart.Init.HwFlowCtl = UART_HWCONTROL_NONE;
	hDebugUart.Init.OverSampling = UART_OVERSAMPLING_16;
	if(HAL_UART_Init(&hDebugUart) != HAL_OK)
	{
		Error_Handler();
	}
}

static void MX_USART2_UART_Init(void)
{
	hBms1Uart.Instance = USART2;
	hBms1Uart.Init.BaudRate = 19200;
	hBms1Uart.Init.WordLength = UART_WORDLENGTH_8B;
	hBms1Uart.Init.StopBits = UART_STOPBITS_1;
	hBms1Uart.Init.Parity = UART_PARITY_NONE;
	hBms1Uart.Init.Mode = UART_MODE_TX_RX;
	hBms1Uart.Init.HwFlowCtl = UART_HWCONTROL_NONE;
	hBms1Uart.Init.OverSampling = UART_OVERSAMPLING_16;
	if(HAL_UART_Init(&hBms1Uart) != HAL_OK)
	{
		Error_Handler();
	}
}

static void MX_USART3_UART_Init(void)
{
	hBms2Uart.Instance = USART3;
	hBms2Uart.Init.BaudRate = 19200;
	hBms2Uart.Init.WordLength = UART_WORDLENGTH_8B;
	hBms2Uart.Init.StopBits = UART_STOPBITS_1;
	hBms2Uart.Init.Parity = UART_PARITY_NONE;
	hBms2Uart.Init.Mode = UART_MODE_TX_RX;
	hBms2Uart.Init.HwFlowCtl = UART_HWCONTROL_NONE;
	hBms2Uart.Init.OverSampling = UART_OVERSAMPLING_16;
	if(HAL_UART_Init(&hBms2Uart) != HAL_OK)
	{
		Error_Handler();
	}
}

static void MX_GPIO_Init(void)
{
	__HAL_RCC_GPIOC_CLK_ENABLE();
	__HAL_RCC_GPIOD_CLK_ENABLE();
	__HAL_RCC_GPIOA_CLK_ENABLE();
	__HAL_RCC_GPIOB_CLK_ENABLE();
}

void Error_Handler(void)
{
	Leds::obj.SetOff();
	Leds::obj.SetOn(Leds::LED_RED);
	
	__disable_irq();
	while (1)
	{

	}
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
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */

static void MX_TIM1_Init(void)
{
	__HAL_RCC_TIM1_CLK_ENABLE();

  /* USER CODE BEGIN TIM1_Init 0 */

  /* USER CODE END TIM1_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* USER CODE BEGIN TIM1_Init 1 */

  /* USER CODE END TIM1_Init 1 */
  htim1.Instance = TIM1;
  htim1.Init.Prescaler = 63;
  htim1.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim1.Init.Period = 65535;
  htim1.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim1.Init.RepetitionCounter = 0;
  htim1.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim1) != HAL_OK)
  {
	Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim1, &sClockSourceConfig) != HAL_OK)
  {
	Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim1, &sMasterConfig) != HAL_OK)
  {
	Error_Handler();
  }
  /* USER CODE BEGIN TIM1_Init 2 */

  /* USER CODE END TIM1_Init 2 */

  HAL_TIM_Base_Start(&htim1);

}