/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
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
#include "usb_device.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define ICM42760P_WHO_AM_I     0x75
#define ICM42760P_WHO_AM_I_VAL 0x67

#define GY_CS_LOW()		HAL_GPIO_WritePin(GY_CS_GPIO_Port,GY_CS_Pin , GPIO_PIN_RESET);
#define GY_CS_HIGH() 	HAL_GPIO_WritePin(GY_CS_GPIO_Port,GY_CS_Pin , GPIO_PIN_SET);

#define OLED_CS_LOW()   HAL_GPIO_WritePin(DIS_CS_GPIO_Port, DIS_CS_Pin, GPIO_PIN_RESET)
#define OLED_CS_HIGH()  HAL_GPIO_WritePin(DIS_CS_GPIO_Port, DIS_CS_Pin, GPIO_PIN_SET)
#define OLED_DC_LOW()   HAL_GPIO_WritePin(DIS_DC_GPIO_Port, DIS_DC_Pin, GPIO_PIN_RESET)
#define OLED_DC_HIGH()  HAL_GPIO_WritePin(DIS_DC_GPIO_Port, DIS_DC_Pin, GPIO_PIN_SET)
#define OLED_RESET_LOW() HAL_GPIO_WritePin(DIS_RST_GPIO_Port, DIS_RST_Pin, GPIO_PIN_RESET)
#define OLED_RESET_HIGH() HAL_GPIO_WritePin(DIS_RST_GPIO_Port, DIS_RST_Pin, GPIO_PIN_SET)

#define DISPLAY_WIDTH 128
#define DISPLAY_HEIGHT 128
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
SPI_HandleTypeDef hspi1;
DMA_HandleTypeDef hdma_spi1_rx;
DMA_HandleTypeDef hdma_spi1_tx;

TIM_HandleTypeDef htim1;

/* USER CODE BEGIN PV */
static uint16_t framebuffer[128*128];
uint8_t dma_done_flag;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_DMA_Init(void);
static void MX_SPI1_Init(void);
static void MX_TIM1_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

// IMU

static inline uint8_t icm_read(uint8_t reg){
    uint8_t txdata[2] = { reg | 0x80, 0x00};
    uint8_t rxdata[2];
    OLED_CS_LOW();
    GY_CS_LOW()
    dma_done_flag = 0;
    HAL_SPI_TransmitReceive_DMA(&hspi1, txdata, rxdata, 2);
    GY_CS_HIGH();
    return rxdata[1];
}

void check_icm(void){
    HAL_Delay(20); // wait for power-up
    char buffer[12];
    uint8_t id = icm_read(ICM42760P_WHO_AM_I);
    int len = snprintf(buffer, sizeof(buffer), "%d\r\n", id);
    USB_Print((uint8_t*)buffer);
}

// USB
void USB_Print(char *str) {
    CDC_Transmit_FS((uint8_t *)str, strlen(str));
}

// DISPLAY
void HAL_SPI_TxCpltCallback(SPI_HandleTypeDef *hspi) {
    if (hspi->Instance == SPI1) {
        // DMA done for SPI1 TX
        dma_done_flag = 1;
    }
}

void OLED_WriteReg(uint8_t cmd) {
    OLED_DC_LOW();
    OLED_CS_LOW();
    HAL_SPI_Transmit(&hspi1, &cmd, 1, HAL_MAX_DELAY);
    OLED_CS_HIGH();
}

void OLED_WriteData(uint8_t data) {
    OLED_DC_HIGH();
    OLED_CS_LOW();
    HAL_SPI_Transmit(&hspi1, &data, 1, HAL_MAX_DELAY);
    OLED_CS_HIGH();
}

void OLED_WriteDatas(uint8_t *data, uint16_t size) {
    OLED_DC_HIGH();
    OLED_CS_LOW();
    dma_done_flag = 0;
    HAL_SPI_Transmit_DMA(&hspi1, data, size);
    while (!dma_done_flag);
    OLED_CS_HIGH();
}

void OLED_Reset() {
    OLED_RESET_LOW();
    HAL_Delay(20);
    OLED_RESET_HIGH();
    HAL_Delay(20);
}

static void OLED_Init(void){
	OLED_Reset();
    OLED_WriteReg(0xfd);  // command lock
    OLED_WriteData(0x12);
    OLED_WriteReg(0xfd);  // command lock
    OLED_WriteData(0xB1);

    OLED_WriteReg(0xae);  // display off
    OLED_WriteReg(0xa4);  // Normal Display mode

    OLED_WriteReg(0x15);  //set column address
    OLED_WriteData(0x00);     //column address start 00
    OLED_WriteData(0x7f);     //column address end 127
    OLED_WriteReg(0x75);  //set row address
    OLED_WriteData(0x00);     //row address start 00
    OLED_WriteData(0x7f);     //row address end 127

    OLED_WriteReg(0xB3);
    OLED_WriteData(0xF1);

    OLED_WriteReg(0xCA);
    OLED_WriteData(0x7F);

    OLED_WriteReg(0xa0);  //set re-map & data format
    OLED_WriteData(0x74);     //Horizontal address increment

    OLED_WriteReg(0xa1);  //set display start line
    OLED_WriteData(0x00);     //start 00 line

    OLED_WriteReg(0xa2);  //set display offset
    OLED_WriteData(0x00);

    OLED_WriteReg(0xAB);
    OLED_WriteReg(0x01);

    OLED_WriteReg(0xB4);
    OLED_WriteData(0xA0);
    OLED_WriteData(0xB5);
    OLED_WriteData(0x55);

    OLED_WriteReg(0xC1);
    OLED_WriteData(0xC8);
    OLED_WriteData(0x80);
    OLED_WriteData(0xC0);

    OLED_WriteReg(0xC7);
    OLED_WriteData(0x0F);

    OLED_WriteReg(0xB1);
    OLED_WriteData(0x32);

    OLED_WriteReg(0xB2);
    OLED_WriteData(0xA4);
    OLED_WriteData(0x00);
    OLED_WriteData(0x00);

    OLED_WriteReg(0xBB);
    OLED_WriteData(0x17);

    OLED_WriteReg(0xB6);
    OLED_WriteData(0x01);

    OLED_WriteReg(0xBE);
    OLED_WriteData(0x05);

    OLED_WriteReg(0xA6);
    OLED_WriteReg(0xAF);
}

void OLED_SetAddrWindow(uint8_t x, uint8_t y, uint8_t w, uint8_t h)
{
    // SSD1351 uses inclusive end coordinates
    uint8_t x_start = x;
    uint8_t x_end   = x + w - 1;
    uint8_t y_start = y;
    uint8_t y_end   = y + h - 1;

    // Column address set
    OLED_WriteReg(0x15);
    OLED_WriteData(x_start);
    OLED_WriteData(x_end);

    // Row address set
    OLED_WriteReg(0x75);
    OLED_WriteData(y_start);
    OLED_WriteData(y_end);

    // Prepare to write RAM
    OLED_WriteReg(0x5C);
}

void OLED_DrawImage(uint8_t* image) {
	OLED_WriteReg(0x5C);
	OLED_WriteDatas(image, sizeof(uint16_t)*128*128);
}

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */
	int count = 0;
  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_USB_DEVICE_Init();
  MX_SPI1_Init();
  MX_TIM1_Init();
  /* USER CODE BEGIN 2 */
  OLED_Init();

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
	check_icm();

	HAL_GPIO_TogglePin(BLINKY_GPIO_Port, BLINKY_Pin);

	HAL_Delay(2000);

	for (int i = 0; i < 128*128; i++) framebuffer[i] = 0xF800; // Red test
	HAL_Delay(1000);
	OLED_DrawImage((uint8_t*)framebuffer);
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

  /** Configure the main internal regulator output voltage
  */
  if (HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI48|RCC_OSCILLATORTYPE_MSI;
  RCC_OscInitStruct.HSI48State = RCC_HSI48_ON;
  RCC_OscInitStruct.MSIState = RCC_MSI_ON;
  RCC_OscInitStruct.MSICalibrationValue = 0;
  RCC_OscInitStruct.MSIClockRange = RCC_MSIRANGE_6;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_MSI;
  RCC_OscInitStruct.PLL.PLLM = 1;
  RCC_OscInitStruct.PLL.PLLN = 16;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV7;
  RCC_OscInitStruct.PLL.PLLQ = RCC_PLLQ_DIV2;
  RCC_OscInitStruct.PLL.PLLR = RCC_PLLR_DIV2;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_1) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief SPI1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_SPI1_Init(void)
{

  /* USER CODE BEGIN SPI1_Init 0 */

  /* USER CODE END SPI1_Init 0 */

  /* USER CODE BEGIN SPI1_Init 1 */

  /* USER CODE END SPI1_Init 1 */
  /* SPI1 parameter configuration*/
  hspi1.Instance = SPI1;
  hspi1.Init.Mode = SPI_MODE_MASTER;
  hspi1.Init.Direction = SPI_DIRECTION_2LINES;
  hspi1.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi1.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi1.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi1.Init.NSS = SPI_NSS_SOFT;
  hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_16;
  hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi1.Init.CRCPolynomial = 7;
  hspi1.Init.CRCLength = SPI_CRC_LENGTH_DATASIZE;
  hspi1.Init.NSSPMode = SPI_NSS_PULSE_ENABLE;
  if (HAL_SPI_Init(&hspi1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN SPI1_Init 2 */

  /* USER CODE END SPI1_Init 2 */

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

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* USER CODE BEGIN TIM1_Init 1 */

  /* USER CODE END TIM1_Init 1 */
  htim1.Instance = TIM1;
  htim1.Init.Prescaler = 0;
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
  sMasterConfig.MasterOutputTrigger2 = TIM_TRGO2_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim1, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM1_Init 2 */

  /* USER CODE END TIM1_Init 2 */

}

/**
  * Enable DMA controller clock
  */
static void MX_DMA_Init(void)
{

  /* DMA controller clock enable */
  __HAL_RCC_DMA1_CLK_ENABLE();

  /* DMA interrupt init */
  /* DMA1_Channel2_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Channel2_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA1_Channel2_IRQn);
  /* DMA1_Channel3_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Channel3_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA1_Channel3_IRQn);

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
/* USER CODE BEGIN MX_GPIO_Init_1 */
/* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, DIS_CS_Pin|DIS_DC_Pin|DIS_RST_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, GY_CS_Pin|BLINKY_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pins : DIS_CS_Pin DIS_DC_Pin DIS_RST_Pin */
  GPIO_InitStruct.Pin = DIS_CS_Pin|DIS_DC_Pin|DIS_RST_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pin : GY_INT1_Pin */
  GPIO_InitStruct.Pin = GY_INT1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GY_INT1_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : GY_CS_Pin BLINKY_Pin */
  GPIO_InitStruct.Pin = GY_CS_Pin|BLINKY_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pin : GY_INT2_Pin */
  GPIO_InitStruct.Pin = GY_INT2_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GY_INT2_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : nCHG_Pin */
  GPIO_InitStruct.Pin = nCHG_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(nCHG_GPIO_Port, &GPIO_InitStruct);

/* USER CODE BEGIN MX_GPIO_Init_2 */
/* USER CODE END MX_GPIO_Init_2 */
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
  __disable_irq();
  while (1)
  {
  }
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
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
