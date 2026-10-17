/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  *****************************0*************************************************
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
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
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
SPI_HandleTypeDef hspi1;

TIM_HandleTypeDef htim2;

UART_HandleTypeDef huart1;

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_SPI1_Init(void);
static void MX_TIM2_Init(void);
static void MX_USART1_UART_Init(void);
/* USER CODE BEGIN PFP */

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

  const uint32_t magicno = ########; // Magic number to identify the flash memory page

  typedef struct { // Structure to hold the thermocouple data
    float temperatureC;
    float internalTemperatureC;
    uint8_t fault;
  } ThermocoupleData;

  typedef struct { // Structure to hold the PID constants
    float Kc_Avg;
    float Ti_Avg;
    float Td_Avg;
    float Kc_Gradient;
    float Ti_Gradient;
    float Td_Gradient;
  } PIDConstants;

  typedef struct{ // Structure to hold the PID constants and a magic number for flash memory
    uint32_t magicno;
    float Kc_Avg;
    float Ti_Avg;
    float Td_Avg;
    float Kc_Gradient;
    float Ti_Gradient;
    float Td_Gradient;
    uint32_t reserved[2]; // Reserved space to make the structure size 32 bytes
  } FlashDataStruct;

  int Mode = 0; // Variable to hold the current mode (0: Tuning, 1: Running)

  ThermocoupleData GetThermocoupleData(GPIO_TypeDef * Port, uint16_t Pin) // Function to read data from the thermocouple device
  {
    uint8_t Data[4]; // Buffer to hold the received data
    HAL_GPIO_WritePin(Port, Pin, GPIO_PIN_RESET); // Pull the CS pin low to select the device
    HAL_SPI_Receive(&hspi1, Data, 4, 100); // Receive 4 bytes of data from the SPI device
    HAL_GPIO_WritePin(Port, Pin, GPIO_PIN_SET); // Pull the CS pin high to deselect the device
    uint32_t value = ((uint32_t)Data[0] << 24) | ((uint32_t)Data[1] << 16) | ((uint32_t)Data[2] << 8) | ((uint32_t)Data[3]); // Combine the 4 bytes into a single 32-bit value
    int16_t temp = value >> 18; // Extract the 14-bit temperature value
    int16_t internalTemp = (value >> 4) & 0x0FFF; // Extract the 12-bit internal temperature value
    uint8_t fault = value & 0x0007; // Extract the fault bits

    if (fault & 0x04) // Check for open circuit fault
    {
      // Handle open circuit fault
      HAL_UART_Transmit(&huart1, (uint8_t*)"Open circuit fault detected\r\n", 29, 100);
    }
    else if (fault & 0x02) // Check for short to GND fault
    {
      // Handle short to GND fault
      HAL_UART_Transmit(&huart1, (uint8_t*)"Short to GND fault detected\r\n", 29, 100);
    }
    else if (fault & 0x01) // Check for short to VCC fault
    {
      // Handle short to VCC fault
      HAL_UART_Transmit(&huart1, (uint8_t*)"Short to VCC fault detected\r\n", 29, 100);
    }
    else if (fault == 0) // No fault detected 
    {
      // No fault detected
      HAL_UART_Transmit(&huart1, (uint8_t*)"No fault detected\r\n", 19, 100);
    }
    else // Unknown fault detected
    {
      // Handle unknown fault
      HAL_UART_Transmit(&huart1, (uint8_t*)"Unknown fault detected\r\n", 24, 100);
    }

    ThermocoupleData data; // Create a ThermocoupleData structure to hold the results
    data.temperatureC = temp * 0.25; // Convert the temperature value to Celsius
    data.internalTemperatureC = internalTemp * 0.0625; // Convert the internal temperature value to Celsius
    data.fault = fault;
    return data;
  }  

  ThermocoupleData sensors[6]; // Array to hold the data for 6 thermocouple devices
  
  void GetAllThermocoupleData(ThermocoupleData *sensors)
  {
    GPIO_TypeDef * Ports[6] = {B1_CS_GPIO_Port, B2_CS_GPIO_Port, B3_CS_GPIO_Port, B4_CS_GPIO_Port, B5_CS_GPIO_Port, B6_CS_GPIO_Port}; // Array of GPIO ports for the CS pins
    uint16_t Pins[6] = {B1_CS_Pin, B2_CS_Pin, B3_CS_Pin, B4_CS_Pin, B5_CS_Pin, B6_CS_Pin}; // Array of GPIO pins for the CS pins  


    for (int i = 0; i < 6; i++) // Loop through each thermocouple device
    {
      sensors[i] = GetThermocoupleData(Ports[i], Pins[i]); // Read the data from the device and store it in the sensors array
    }

  }

  void Terminate()
  {
    __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_2, 0); // Set the PWM duty cycle for channel 1 to 0
    __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_3, 0); // Set the PWM duty cycle for channel 2 to 0
    __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_1, 0); // Set the PWM duty cycle for channel 3 to 0
    HAL_UART_Transmit(&huart1, (uint8_t*)"Program terminated\r\n", 20, 100); // Transmit a termination message over UART
    HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_RESET); // Turn off the LED
    __disable_irq(); // Disable interrupts
    while(1); // Enter an infinite loop to stop the program
    {
      __WFI(); // Wait for interrupt to reduce power consumption
    }
  }

  void SafetyCheck(ThermocoupleData *sensors)
  {
    for (int i = 0; i < 6; i++) // Loop through each thermocouple device
    {
      if (sensors[i].fault != 0) // Check if there is a fault in the device
      {
        HAL_UART_Transmit(&huart1, (uint8_t*)"Safety Check: Fault detected in thermocouple device\r\n", 54, 100); // Transmit a safety check message over UART
        Terminate(); // Call the Terminate function to stop the program
      }

      if (sensors[i].temperatureC > 90.0f) // Check if the temperature exceeds 90 degrees Celsius
      {
        HAL_UART_Transmit(&huart1, (uint8_t*)"Safety Check: Temperature exceeds 90 degrees Celsius\r\n", 57, 100); // Transmit a safety check message
        Terminate(); // Call the Terminate function to stop the program
      }
    }
  }

  void PassThermocoupleData(ThermocoupleData *sensors, int *Mode)
  {
    char txbuffer[100]; // Buffer to hold the formatted string
    sprintf(txbuffer, "%d,%lu, %.2f, %.2f,%.2f,%.2f,%.2f,%.2f,\r\n", *Mode, HAL_GetTick(), sensors[0].temperatureC, sensors[1].temperatureC, sensors[2].temperatureC, sensors[3].temperatureC, sensors[4].temperatureC, sensors[5].temperatureC); // Format the data into a string
   
    HAL_UART_Transmit(&huart1, (uint8_t*)txbuffer, strlen(txbuffer), 100); // Transmit the string over UART
  }
  
  void ReceiveTuningData(char *buffer)
  {
    uint8_t receivedByte;
    int index = 0;
    while (1) // Loop until a newline character is received 
    {
      if (HAL_UART_Receive(&huart1, &receivedByte, 1, 100) == HAL_TIMEOUT) // Receive one byte at a time
      {
        strcpy(buffer, "0,0,0,0"); // If no data is received, set the buffer to a zero string
        break; // Exit the loop
      }
      else 
      {
        if (receivedByte == '\n') // Check for newline character
        {
          buffer[index] = '\0'; // Null-terminate the string
          break; // Exit the loop
        }
        else
        {
          buffer[index++] = receivedByte; // Store the received byte in the buffer
        }
      }

    }
  } 
  
  float Clamp(float value, float min, float max) // Function to clamp a value between a minimum and maximum
  {
    if (value < min)
      return min;
    else if (value > max)
      return max;
    else
      return value;
  }

  uint32_t PWM[2]; // Array of PWM channels
  void PassPWMSignals(char *rxbuffer)
  {
    float a, b; // Variables to hold the parsed values
    sscanf(rxbuffer, "%*d,%f,%f", &a, &b); // Parse the received string into two float values
    uint32_t ARR = __HAL_TIM_GET_AUTORELOAD(&htim2); // Get the auto-reload value for the timer

    a = Clamp(a, 0.0f, 100.0f);// Clamp the values between 0 and 100
    b = Clamp(b, 0.0f, 100.0f);// Clamp the values between 0 and 100

    PWM[0] = (uint32_t)((a/100.0f) * ARR); // Convert the first value to a PWM duty cycle
    PWM[1] = (uint32_t)((b/100.0f) * ARR); // Convert the second value to a PWM duty cycle

    __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_2, PWM[0]); // Set the PWM duty cycle for channel 1
    __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_3, PWM[1]); // Set the PWM duty cycle for channel 2
  }

  void ToggleLED()
  { 
    static uint32_t lastTickLED = 0; // Static variable to hold the last tick value
    uint32_t currentTickLED = HAL_GetTick(); // Get the current tick value
    if (currentTickLED - lastTickLED >= 1000) // Check if 1 second has passed
    {
      HAL_GPIO_TogglePin(LED_GPIO_Port, LED_Pin); // Toggle the LED pin
      lastTickLED = currentTickLED; // Update the last tick value
    }
  }

  void CoolingOutput(float cooling)
  {
    if (cooling == 0.0f)
    {
      __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_1, 0); // Handle the case where cooling is set to 0
    }

    else if (cooling == 1.0f)
    {
      __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_1, __HAL_TIM_GET_AUTORELOAD(&htim2)); // Handle the case where cooling is set to 1
    }
    else
    {
      HAL_UART_Transmit(&huart1, (uint8_t*)"Error: Invalid cooling value\r\n", 29, 100); // Return Error
    }  
  }

  void ControlCooling(char*rxbuffer)
  {
    float cooling; // Variable to hold the parsed value
    sscanf(rxbuffer, "%*d,%*f,%*f,%f", &cooling); // Parse the received string into a float value
    CoolingOutput(cooling); // Call the CoolingOutput function to control the cooling system
  }
  
  
  
  void PushPIDValues(char *buffer)
  {
    float Kc_Avg, Ti_Avg, Td_Avg, Kc_Gradient, Ti_Gradient, Td_Gradient; // Variables to hold the parsed values
    sscanf(buffer, "%*d,%f,%f,%f,%f,%f,%f", &Kc_Avg, &Ti_Avg, &Td_Avg, &Kc_Gradient, &Ti_Gradient, &Td_Gradient); // Parse the received string into six float values
    uint32_t address = 0x0801F800; // Set the address to the last page of flash memory
    HAL_FLASH_Unlock(); // Unlock the flash memory for writing
    FLASH_EraseInitTypeDef EraseInitStruct; // Structure to hold the erase parameters
    EraseInitStruct.TypeErase = FLASH_TYPEERASE_PAGES; // Set the erase type to page erase
    EraseInitStruct.Page = 63; // Set the page address to the last page
    EraseInitStruct.NbPages = 1; // Set the number of pages to erase to 1

    uint32_t PageError; // Variable to hold the page error

    HAL_FLASHEx_Erase(&EraseInitStruct, &PageError); // Erase the last page of flash memory

    FlashDataStruct Flashdata = 
    {  
      .magicno = magicno, // Set the magic number to identify the flash memory page
      .Kc_Avg = Kc_Avg, // Set the Kc_Avg value
      .Ti_Avg = Ti_Avg, // Set the Ti_Avg value
      .Td_Avg = Td_Avg, // Set the Td_Avg value
      .Kc_Gradient = Kc_Gradient, // Set the Kc_Gradient value
      .Ti_Gradient = Ti_Gradient, // Set the Ti_Gradient value
      .Td_Gradient = Td_Gradient, // Set the Td_Gradient value
      .reserved = {0} // Set the reserved space to 0
    };

    uint64_t *FlashDataPtr = (uint64_t*)&Flashdata; // Create a pointer to the FlashDataStruct structure
    for(int i = 0; i < 8; i++) // Loop through each 64-bit value in the structure
    {
      HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD, address + (i * 8), FlashDataPtr[i]); // Write the 64-bit value to flash memory
    }

    HAL_FLASH_Lock(); // Lock the flash memory after writing
  }

  PIDConstants ReadPIDConstants()
  {
    PIDConstants pid; // Create a PIDConstants structure to hold the results
    uint32_t address = 0x0801F800; // Set the address to the last page of flash memory
    if (*(uint32_t*)address != magicno) // Check if the magic number is present in flash memory
    {
      // If the magic number is not present, return default PID constants
      pid.Kc_Avg = 0.0f;
      pid.Ti_Avg = 0.0f;
      pid.Td_Avg = 0.0f;
      pid.Kc_Gradient = 0.0f;
      pid.Ti_Gradient = 0.0f;
      pid.Td_Gradient = 0.0f;
      HAL_UART_Transmit(&huart1, (uint8_t*)"Error: PID constants not found in flash memory\r\n", 47, 100); // Return Error
      return pid;
    }

    else
    {
      pid.Kc_Avg = *(float*)(address +4); // Read the Kc_Avg value from flash memory
      pid.Ti_Avg = *(float*)(address + 8); // Read the Ti_Avg value from flash memory
      pid.Td_Avg = *(float*)(address + 12); // Read the Td_Avg value from flash memory
      pid.Kc_Gradient = *(float*)(address + 16); // Read the Kc_Gradient value from flash memory
      pid.Ti_Gradient = *(float*)(address + 20); // Read the Ti_Gradient value from flash memory
      pid.Td_Gradient = *(float*)(address + 24); // Read the Td_Gradient value from flash memory
      return pid;
    }
  }
  
  float previous_error_avg = 0.0f; // Variable to hold the previous error for the average temperature PID controller
  float previous_error_grad = 0.0f; // Variable to hold the previous error for the average gradient PID controller
  float integral_avg = 0.0f; // Variable to hold the integral term for the average temperature PID controller
  float integral_grad = 0.0f; // Variable to hold the integral term for the

  float PIDAlgorithm(float Kc, float Ti, float Td , float setpoint, float measured_value,float *previous_error, float *integral, float dt)
  { 
    float error = setpoint - measured_value; // Calculate the current error
    *integral += error * dt; // Calculate the integral term
    float derivative = (error - *previous_error) / dt; // Calculate the derivative term
    float up = Kc * error; // Proportional term
    float ui = Kc * (*integral / Ti); // Integral term
    float ud = Kc * Td * derivative; // Derivative term
    float output = up + ui + ud; // Calculate the PID output
    *previous_error = error; // Update the previous error for the next iteration
    float clampedOutput = Clamp(output, -100.0f, 100.0f); // Clamp the output between -100 and 100
    if (clampedOutput == 100.0f || clampedOutput == -100.0f) // Check if the output is clamped
    {
      ui = clampedOutput - up;
      *integral = ui * Ti / Kc; // Adjust the integral term to prevent windup
    }
    return clampedOutput;
  }

  float decouplingMatrix[2][2] = {{####, ####}, {####, ####}}; // Initialize the decoupling matrix to the identity matrix
  float PwmMatrix[2] = {0.0f, 0.0f};

  void MatrixCalc(float AvgTempPIDOutput, float AvgGradientPIDOutput, float decouplingMatrix[2][2])
  {
    PwmMatrix[0] = decouplingMatrix[0][0] * AvgTempPIDOutput + decouplingMatrix[0][1] * AvgGradientPIDOutput; // Calculate the first PWM output using the decoupling matrix
    PwmMatrix[1] = decouplingMatrix[1][0] * AvgTempPIDOutput + decouplingMatrix[1][1] * AvgGradientPIDOutput; // Calculate the second PWM output using the decoupling matrix
  }

  void RunningMode(ThermocoupleData *sensors, char *buffer)
  {
    static uint32_t lastTickPID = 0; // Static variable to hold the current tick value for PID calculations
    uint32_t currentTickPID = HAL_GetTick(); // Get the current tick value
    float dt = (currentTickPID - lastTickPID) / 1000.0f; // Calculate the time difference in seconds
    lastTickPID = currentTickPID; // Update the last tick value for PID calculations
    float setpoint; // Variables to hold the parsed values
	  int coolingState;
    sscanf(buffer, "%*d,%f,%d", &setpoint, &coolingState); // Parse the received string into a float value for the setpoint
    float AvgTopTemp = (sensors[0].temperatureC + sensors[1].temperatureC + sensors[2].temperatureC) / 3; // Calculate the average temperature of the top three thermocouples
    float AvgBottomTemp = (sensors[3].temperatureC + sensors[4].temperatureC + sensors[5].temperatureC) / 3; // Calculate the average temperature of the bottom three thermocouples
    float AvgTemp = (AvgTopTemp + AvgBottomTemp) / 2; // Calculate the overall average temperature
    float AvgGradient = AvgTopTemp - AvgBottomTemp; // Calculate the average gradient between the top and bottom thermocouples

    PIDConstants PIDValues = ReadPIDConstants(); // Read the PID constants from flash memory
    float AvgTempPIDOutput = PIDAlgorithm(PIDValues.Kc_Avg, PIDValues.Ti_Avg, PIDValues.Td_Avg, setpoint, AvgTemp, &previous_error_avg, &integral_avg, dt); // Calculate the PID output for the average temperature
    float AvgGradientPIDOutput = PIDAlgorithm(PIDValues.Kc_Gradient, PIDValues.Ti_Gradient, PIDValues.Td_Gradient, 0.0f ,AvgGradient, &previous_error_grad, &integral_grad, dt); // Calculate the PID output for the average gradient
    MatrixCalc(AvgTempPIDOutput, AvgGradientPIDOutput, decouplingMatrix); // Calculate the PWM outputs using the decoupling matrix
    
    uint32_t ARR = __HAL_TIM_GET_AUTORELOAD(&htim2); // Get the auto-reload value for the timer

    PwmMatrix[0] = Clamp(PwmMatrix[0], 0.0f, 100.0f);// Clamp the values between 0 and 100
    PwmMatrix[1] = Clamp(PwmMatrix[1], 0.0f, 100.0f);// Clamp the values between 0 and 100

    PWM[0] = (uint32_t)((PwmMatrix[0]/100.0f) * ARR); // Convert the first value to a PWM duty cycle
    PWM[1] = (uint32_t)((PwmMatrix[1]/100.0f) * ARR); // Convert the second value to a PWM duty cycle

    __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_2, PWM[0]); // Set the PWM duty cycle for channel 1
    __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_3, PWM[1]); // Set the PWM duty cycle for channel 2

    CoolingOutput(coolingState); // Call the CoolingOutput function to control the cooling system

   
  }  

  void TuningMode(char *buffer)
  {
    PassPWMSignals(buffer); // Call the PassPWMSignal function to set the PWM duty cycles
    ControlCooling(buffer); // Call the ControlCooling function to control the cooling system
  }

  void SelectMode(char *buffer, int *mode)
  {
    sscanf(buffer, "%d", mode); // Parse the received string into an integer value
    if (*mode == 0)
    {
      sprintf(buffer,"%d,%f,%f,%f\n", 0, 0.0f, 0.0f, 0.0f); // If mode is 0, set the buffer to a zero string
      TuningMode(buffer); // Call the TuningMode function to handle tuning mode
    }
    else if (*mode == 1)
    {
      TuningMode(buffer); // Call the TuningMode function to handle tuning mode
    }
    else if (*mode == 4)
    {
      PushPIDValues(buffer); // Call the PushPIDValues function to handle PID tuning
      *mode = 2; // Set the mode to 2 (Running mode) after PID tuning
    }
    else if (*mode == 2)
    {
      RunningMode(sensors, buffer); // Call the RunningMode function to handle running mode
    }
    else if (*mode == 3)
    {
      Terminate(); // Stop Program      
    }
    else
    {
      HAL_UART_Transmit(&huart1, (uint8_t*)"Error: Invalid mode value\r\n", 29, 100); // Return Error
    }
  }


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
  MX_SPI1_Init();
  MX_TIM2_Init();
  MX_USART1_UART_Init();
  /* USER CODE BEGIN 2 */

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE BEGIN 3 */

    //LabView Tuning Mode
    GetAllThermocoupleData(sensors); // Call the GetAllThermocoupleData function to read data from all thermocouple devices
    SafetyCheck(sensors); // Call the SafetyCheck function to check for faults and temperature limits
    PassThermocoupleData(sensors, &Mode); // Call the PassThermocoupleData function to read and transmit the thermocouple data

    char rxbuffer[100]; // Buffer to hold the received string
    ReceiveTuningData(rxbuffer); // Call the ReceiveData function to receive data over UART
    SelectMode(rxbuffer, &Mode); // Call the SelectMode function to select the mode based on the received data

    ToggleLED(); // Call the ToggleLED function to toggle the LED every second    
  }
  /* USER CODE END 3 */
}
/*
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSIDiv = RCC_HSI_DIV1;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
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
  hspi1.Init.Direction = SPI_DIRECTION_2LINES_RXONLY;
  hspi1.Init.DataSize = SPI_DATASIZE_4BIT;
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
  htim2.Init.Prescaler = 0;
  htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim2.Init.Period = 4294967295;
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
  if (HAL_TIM_PWM_ConfigChannel(&htim2, &sConfigOC, TIM_CHANNEL_2) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_PWM_ConfigChannel(&htim2, &sConfigOC, TIM_CHANNEL_3) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM2_Init 2 */

  /* USER CODE END TIM2_Init 2 */
  HAL_TIM_MspPostInit(&htim2);

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
  HAL_GPIO_WritePin(GPIOA, B1_CS_Pin|B5_CS_Pin|B4_CS_Pin|B6_CS_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, B2_CS_Pin|B3_CS_Pin|LED_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pins : B1_CS_Pin B5_CS_Pin B4_CS_Pin B6_CS_Pin */
  GPIO_InitStruct.Pin = B1_CS_Pin|B5_CS_Pin|B4_CS_Pin|B6_CS_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pins : B2_CS_Pin B3_CS_Pin LED_Pin */
  GPIO_InitStruct.Pin = B2_CS_Pin|B3_CS_Pin|LED_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
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
