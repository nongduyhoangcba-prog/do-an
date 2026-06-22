/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
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
#include "tcs34725.h"
#include "i2c-lcd.h"
#include "stdio.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

// chon thuat toan de su dung
#define TEST_ALGO_HSV
//#define TEST_ALGO_RGB

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
I2C_HandleTypeDef hi2c1;

TIM_HandleTypeDef htim1;
TIM_HandleTypeDef htim2;

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_I2C1_Init(void);
static void MX_TIM1_Init(void);
static void MX_TIM2_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

// =======================================================================
// 1. KHOI HAM TOAN HOC CHUYEN DOI KHONG GIAN MAU (RGB - HSV)
// =======================================================================
float max3(float a, float b, float c) { float m = a; if (b > m) m = b; if (c > m) m = c; return m; }
float min3(float a, float b, float c) { float m = a; if (b < m) m = b; if (c < m) m = c; return m; }

void RGBtoHSV(float r, float g, float b, float *h, float *s, float *v) {
    float cmax = max3(r, g, b); float cmin = min3(r, g, b); float diff = cmax - cmin;
    *v = cmax * 100;
    if (cmax == 0) *s = 0; else *s = (diff / cmax) * 100;
    if (diff == 0) *h = 0;
    else if (cmax == r) { *h = 60 * ((g - b) / diff) + 360; while (*h >= 360) *h -= 360; } 
    else if (cmax == g) *h = 60 * ((b - r) / diff) + 120;
    else *h = 60 * ((r - g) / diff) + 240;
}

// =======================================================================
// 2. HAM DIEU KHIEN BANG CHUYEN ( L298N  ) 
// =======================================================================
void Motor_Run(uint16_t speed) {
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_1, GPIO_PIN_SET);   // IN1 = 1 (Chay tien)
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_2, GPIO_PIN_RESET); // IN2 = 0
    __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_1, speed);  // Xuat xung PWM dieu khien toc do
}

void Motor_Stop(void) {
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_1, GPIO_PIN_RESET); 
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_2, GPIO_PIN_RESET);
    __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_1, 0);       // Duty Cycle = 0%
}

// =======================================================================
// 3. HAM DIEU KHIEN SERVO (TAN SO 50Hz)
// Muc xung: 500 (Goc 0 do ) | 1500 (Goc 90 do - vung ra gat phoi)
// =======================================================================
void Servo_Push(uint32_t channel) {
    __HAL_TIM_SET_COMPARE(&htim1, channel, 1100); 
    HAL_Delay(600); // Thoi gian vung tay ra day chuoi 
}

void Servo_Home(uint32_t channel) {
    __HAL_TIM_SET_COMPARE(&htim1, channel, 500);  
    HAL_Delay(400); // Thoi gian rut tay 
}

// =======================================================================
// 4. KHAI BAO BIEN TOAN CUC
// =======================================================================
uint16_t red, green, blue, clear;
float h_val, s_val, v_val;
uint8_t loai_chuoi = 0; // dinh tuyen: 0: Cho phoi, 1: Chuoi Xanh, 2: Chuoi Vang, 3: Chuoi Nau/den

uint16_t dem_xanh = 0;
uint16_t dem_vang = 0;
uint16_t dem_den  = 0;
uint16_t tong_so  = 0;

char lcd_buffer[20]; // Bo dem xu ly chuoi hien thi len LCD

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
  MX_I2C1_Init();
  MX_TIM1_Init();
  MX_TIM2_Init();
  /* USER CODE BEGIN 2 */
	
	// Khoi dong servo
  HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1); Servo_Home(TIM_CHANNEL_1); // Tram 1 
  HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_2); Servo_Home(TIM_CHANNEL_2); // Tram 2 
  HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_3); Servo_Home(TIM_CHANNEL_3); // Tram 3 
	
	// Khoi dong xung dieu khien cho bang tai
  HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_1);
	
	// Khoi tao lcd
  lcd_init();
  lcd_clear();
  lcd_goto_xy(0, 0); lcd_send_string("HE THONG PHAN LOAI");
  lcd_goto_xy(1, 0); lcd_send_string("data   : READY     ");
  lcd_goto_xy(2, 0); lcd_send_string("Ket qua: WAITING... ");
	lcd_goto_xy(3, 0); lcd_send_string("X:00 V:00 N:00 T:00 ");
	
	// Kich hoat cam bien mau sac TCS34725
  TCS34725_Init();
	
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
		
		// =======================================================================
      // TRANG THAI KHI CHUA CO PHOI VAO BUONG DOC MAU
      // =======================================================================
      if (loai_chuoi == 0) 
      {
          // cho bang chuyen van hanh 
          Motor_Run(10000); 
          
          // Kiem tra cam bien buong doc mau 
          if (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_4) == GPIO_PIN_RESET) 
          {
              // dong co dung
              Motor_Stop();    
              HAL_Delay(500); // tre

              lcd_goto_xy(2, 9); lcd_send_string("CHECKING... ");
              
              // doc du lieu tu cam bien
              TCS34725_GetRawData(&red, &green, &blue, &clear);
              
              // Nguong bao ve
              if (clear > 100) 
              { 
                  // Chuan haa du lieu mau de loai bo sut giam do sang
                  float r_norm = (float)red / clear;
                  float g_norm = (float)green / clear;
                  float b_norm = (float)blue / clear;
                  
                  // Chuyen doi d? lieu sang khong gian goc mau don truc HSV
                  RGBtoHSV(r_norm, g_norm, b_norm, &h_val, &s_val, &v_val);
                  
                  // ---------------------------------------------------
                  // NHaNH Xu Li 1: THUaT TOaN dON TRuC HSV 
                  // ---------------------------------------------------
                  #ifdef TEST_ALGO_HSV
                      // Hien thi truc quan thong so goc Hue thuc te len dong 2 LCD
                      sprintf(lcd_buffer, "H:%.2f Deg   ", h_val); 
                      lcd_goto_xy(1, 9); lcd_send_string(lcd_buffer);

                      // So sanh phan vung 
                      lcd_goto_xy(2, 9);
                      if (h_val < 20.0f) {
                          loai_chuoi = 3; // chuoi hong vet dom den tham
                          lcd_send_string("CHUOI NAU  ");
                      } 
                      else if (h_val >= 20.0f && h_val < 25.0f) {
                          loai_chuoi = 2; // chuoi vang chin
                          lcd_send_string("CHUOI VANG ");
                      } 
                      else if (h_val >= 25.0f) {
                          loai_chuoi = 1; // chuoi xanh
                          lcd_send_string("CHUOI XANH ");
                      }
                  
                  // ---------------------------------------------------
                  // NHaNH Xu Ly 2: THUaT TOaN RGB TH� 
                  // ---------------------------------------------------
                  #elif defined(TEST_ALGO_RGB)
                      // Hien thi hai kenh mau chu dao 
                      sprintf(lcd_buffer, "R:%d G:%d    ", red, green); 
                      lcd_goto_xy(1, 9); lcd_send_string(lcd_buffer);

                      lcd_goto_xy(2, 9);
                      if (red > 600 && green > 500 && red >= green) {
                          loai_chuoi = 2; 
                          lcd_send_string("CHUOI VANG ");
                      } 
                      else if (green > 500 && green > red) {
                          loai_chuoi = 1; 
                          lcd_send_string("CHUOI XANH ");
                      } 
                      else {
                          loai_chuoi = 3; 
                          lcd_send_string("CHUOI NAU  ");
                      }
                  #endif
              }
          }
      }
      
      // =======================================================================
      // TRANG THAI 1: PHOI DA DUOC DINH DANH TIM TRAM THUC THI
      // =======================================================================
      else 
      {
          // kich hoat bang tai
          Motor_Run(10000); 

				// tram phan loai 1: chuoi xanh
          if (loai_chuoi == 1 && HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_0) == GPIO_PIN_RESET) 
          {
              //Motor_Stop();               // Phanh bang tai
              Servo_Push(TIM_CHANNEL_1);  // Vung tay gat tram 1
							HAL_Delay(800);
              Servo_Home(TIM_CHANNEL_1);  // Thu tay gat tram 1 
							dem_xanh++; tong_so++;
              sprintf(lcd_buffer, "X:%02d V:%02d N:%02d T:%02d ", dem_xanh, dem_vang, dem_den, tong_so);
              lcd_goto_xy(3, 0); lcd_send_string(lcd_buffer);
              loai_chuoi = 0;             // Reset trang thai
							
          }
          
          // --- tram phan loai2: chuoi den
          else if (loai_chuoi == 3 && HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_1) == GPIO_PIN_RESET) 
          {
              //Motor_Stop();
              Servo_Push(TIM_CHANNEL_2);  // Vung tay gat tram 2
							HAL_Delay(800);
              Servo_Home(TIM_CHANNEL_2);  // Thu tay gat tram 2
							dem_den++; tong_so++;
              sprintf(lcd_buffer, "X:%02d V:%02d N:%02d T:%02d ", dem_xanh, dem_vang, dem_den, tong_so);
              lcd_goto_xy(3, 0); lcd_send_string(lcd_buffer);
              loai_chuoi = 0;             // Reset trang thai
          }
          
          // --- TRam phan loai 3: chuoi vang
          else if (loai_chuoi == 2 && HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_10) == GPIO_PIN_RESET) 
          {
              HAL_Delay(1500);
							dem_vang++; tong_so++;
              sprintf(lcd_buffer, "X:%02d V:%02d N:%02d T:%02d ", dem_xanh, dem_vang, dem_den, tong_so);
              lcd_goto_xy(3, 0); lcd_send_string(lcd_buffer);
              loai_chuoi = 0;             // Reset trang thai
          }
      }
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

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
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

  /** Initializes the CPU, AHB and APB buses clocks
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
  * @brief I2C1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_I2C1_Init(void)
{

  /* USER CODE BEGIN I2C1_Init 0 */

  /* USER CODE END I2C1_Init 0 */

  /* USER CODE BEGIN I2C1_Init 1 */

  /* USER CODE END I2C1_Init 1 */
  hi2c1.Instance = I2C1;
  hi2c1.Init.ClockSpeed = 100000;
  hi2c1.Init.DutyCycle = I2C_DUTYCYCLE_2;
  hi2c1.Init.OwnAddress1 = 0;
  hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c1.Init.OwnAddress2 = 0;
  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C1_Init 2 */

  /* USER CODE END I2C1_Init 2 */

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
  TIM_OC_InitTypeDef sConfigOC = {0};
  TIM_BreakDeadTimeConfigTypeDef sBreakDeadTimeConfig = {0};

  /* USER CODE BEGIN TIM1_Init 1 */

  /* USER CODE END TIM1_Init 1 */
  htim1.Instance = TIM1;
  htim1.Init.Prescaler = 71;
  htim1.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim1.Init.Period = 19999;
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
  if (HAL_TIM_PWM_Init(&htim1) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
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
  if (HAL_TIM_PWM_ConfigChannel(&htim1, &sConfigOC, TIM_CHANNEL_3) != HAL_OK)
  {
    Error_Handler();
  }
  sBreakDeadTimeConfig.OffStateRunMode = TIM_OSSR_DISABLE;
  sBreakDeadTimeConfig.OffStateIDLEMode = TIM_OSSI_DISABLE;
  sBreakDeadTimeConfig.LockLevel = TIM_LOCKLEVEL_OFF;
  sBreakDeadTimeConfig.DeadTime = 0;
  sBreakDeadTimeConfig.BreakState = TIM_BREAK_DISABLE;
  sBreakDeadTimeConfig.BreakPolarity = TIM_BREAKPOLARITY_HIGH;
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
  TIM_OC_InitTypeDef sConfigOC = {0};

  /* USER CODE BEGIN TIM2_Init 1 */

  /* USER CODE END TIM2_Init 1 */
  htim2.Instance = TIM2;
  htim2.Init.Prescaler = 71;
  htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim2.Init.Period = 19999;
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
  if (HAL_TIM_PWM_Init(&htim2) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim2, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = 0;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  if (HAL_TIM_PWM_ConfigChannel(&htim2, &sConfigOC, TIM_CHANNEL_1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM2_Init 2 */

  /* USER CODE END TIM2_Init 2 */
  HAL_TIM_MspPostInit(&htim2);

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
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_1|GPIO_PIN_2, GPIO_PIN_RESET);

  /*Configure GPIO pins : PA1 PA2 */
  GPIO_InitStruct.Pin = GPIO_PIN_1|GPIO_PIN_2;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pin : PA4 */
  GPIO_InitStruct.Pin = GPIO_PIN_4;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pins : PB0 PB1 PB10 */
  GPIO_InitStruct.Pin = GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_10;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

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
