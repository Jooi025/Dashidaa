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
#include <string.h>
#include <gif.h>
#include <AnimatedGIF.h>
#include <gif1.h>
#include <gif2.h>

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define OLED_CS_LOW()   HAL_GPIO_WritePin(CS_GPIO_Port, CS_Pin, GPIO_PIN_RESET)
#define OLED_CS_HIGH()  HAL_GPIO_WritePin(CS_GPIO_Port, CS_Pin, GPIO_PIN_SET)
#define OLED_DC_LOW()   HAL_GPIO_WritePin(DC_GPIO_Port, DC_Pin, GPIO_PIN_RESET)
#define OLED_DC_HIGH()  HAL_GPIO_WritePin(DC_GPIO_Port, DC_Pin, GPIO_PIN_SET)
#define OLED_RESET_LOW() HAL_GPIO_WritePin(RST_GPIO_Port, RST_Pin, GPIO_PIN_RESET)
#define OLED_RESET_HIGH() HAL_GPIO_WritePin(RST_GPIO_Port, RST_Pin, GPIO_PIN_SET)
#define DISPLAY_WIDTH 128
#define DISPLAY_HEIGHT 128
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */
static uint16_t framebuffer[128*128]; // RGB565 frame buffer
/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
SPI_HandleTypeDef hspi1;
DMA_HandleTypeDef hdma_spi1_tx;
DMA_HandleTypeDef hdma_spi1_rx;

UART_HandleTypeDef huart2;

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_DMA_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_SPI1_Init(void);
/* USER CODE BEGIN PFP */
uint8_t dma_done_flag;
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

static void OLED_Init(void)
{
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

void OLED_SkipPixel(void)
{
    uint16_t dummy = 0x0000; // black or background color
    OLED_WriteDatas((uint8_t*)&dummy, 2);
}

void OLED_ClearScreen(uint16_t color)
{
    uint16_t line[128*128];
    for (int i = 0; i < 128*128; i++)
        line[i] = color; // fill line with desired color

    OLED_SetAddrWindow(0, 0, 128, 128); // full screen

    OLED_DrawImage((uint8_t*) line);
}


void gifDrawCallback(GIFDRAW *pDraw)
{
    if (!pDraw->pPixels || pDraw->y >= DISPLAY_HEIGHT)
        return;

    const uint8_t *src = pDraw->pPixels;
    const uint16_t *palette = pDraw->pPalette;
    int y = pDraw->iY + pDraw->y;

    if (y >= DISPLAY_HEIGHT)
        return;

    int x0 = pDraw->iX;
    int width = pDraw->iWidth;
    if (x0 >= DISPLAY_WIDTH)
        return;
    if (x0 + width > DISPLAY_WIDTH)
        width = DISPLAY_WIDTH - x0;

    for (int i = 0; i < width; i++) {
        uint8_t idx = src[i];
        if (pDraw->ucHasTransparency && idx == pDraw->ucTransparent) {
            continue; // leave background color (already cleared)
        }
        framebuffer[y * DISPLAY_WIDTH + x0 + i] = palette[idx];
    }
}



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
  MX_USART2_UART_Init();
  MX_SPI1_Init();
  /* USER CODE BEGIN 2 */
  AnimatedGIF gif;

  OLED_Init();
//  HAL_Delay(1000);
//  for (int i = 0; i < 128*128; i++) framebuffer[i] = 0xF800; // Red test
//  OLED_DrawImage((uint8_t*)framebuffer);
//  HAL_Delay(1000);

//  OLED_DrawImage((uint8_t*)gif_frames[0]);
  gif.begin(GIF_PALETTE_RGB565_BE);
  if (gif.open((uint8_t *)gif1, sizeof(gif1), gifDrawCallback)){
	  while (gif.playFrame(true, NULL)) { // live dangerously; run unthrottled :
		  OLED_DrawImage((uint8_t*)framebuffer);
		  memset(framebuffer, 0x00, sizeof(framebuffer));
	  }
	  gif.close();
  }



  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {



//	  HAL_Delay(20);
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

  /** Configure the main internal regulator output voltage
  */
  if (HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure LSE Drive Capability
  */
  HAL_PWR_EnableBkUpAccess();
  __HAL_RCC_LSEDRIVE_CONFIG(RCC_LSEDRIVE_LOW);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_LSE|RCC_OSCILLATORTYPE_MSI;
  RCC_OscInitStruct.LSEState = RCC_LSE_ON;
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

  /** Enable MSI Auto calibration
  */
  HAL_RCCEx_EnableMSIPLLMode();
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
  hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_2;
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
  huart2.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&huart2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART2_Init 2 */

  /* USER CODE END USART2_Init 2 */

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
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, RST_Pin|DC_Pin|CS_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(LD3_GPIO_Port, LD3_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pins : RST_Pin DC_Pin CS_Pin */
  GPIO_InitStruct.Pin = RST_Pin|DC_Pin|CS_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pin : LD3_Pin */
  GPIO_InitStruct.Pin = LD3_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(LD3_GPIO_Port, &GPIO_InitStruct);

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
