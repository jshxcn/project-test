/**
  ******************************************************************************
  * File Name          : STM32TouchController.cpp
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

/* USER CODE BEGIN STM32TouchController */

#include <STM32TouchController.hpp>

extern "C" {

    /* Private function prototypes -----------------------------------------------*/
    static uint8_t BSP_TS_Init(uint16_t xSize, uint16_t ySize);
    static uint8_t BSP_TS_GetState(TS_StateTypeDef* TS_State);
    static uint8_t BSP_TS3510_IsDetected(void);
    static void I2Cx_Error(uint8_t Addr);

    extern I2C_HandleTypeDef hi2c1;

    /** @defgroup STM324x9I_EVAL_TS_Private_Variables
      * @{
      */
    static TS_DrvTypeDef* ts_driver;
    static uint16_t ts_x_boundary, ts_y_boundary;
    static uint8_t  ts_orientation;
    static uint8_t  I2C_Address;

}

using namespace touchgfx;

void STM32TouchController::init()
{
    /**
     * Initialize touch controller and driver
     *
     */
    //if (BSP_TS_Init(BSP_LCD_GetXSize(), BSP_LCD_GetYSize()) == TS_OK)
    if (BSP_TS_Init(480, 272) == TS_OK)
    {
        isInitialized = true;
    }
}

bool STM32TouchController::sampleTouch(int32_t& x, int32_t& y)
{
    /**
     * By default sampleTouch returns false,
     * return true if a touch has been detected, otherwise false.
     *
     * Coordinates are passed to the caller by reference by x and y.
     *
     * This function is called by the TouchGFX framework.
     * By default sampleTouch is called every tick, this can be adjusted by HAL::setTouchSampleRate(int8_t);
     *
     */
    if (isInitialized)
    {
        TS_StateTypeDef state;
        BSP_TS_GetState(&state);
        if (state.TouchDetected)
        {
            x = state.x;
            y = state.y;

            return true;
        }
    }
    return false;
}

extern "C" {

    /**
      * @brief  Initializes and configures the touch screen functionalities and
      *         configures all necessary hardware resources (GPIOs, clocks..).
      * @param  xSize: Maximum X size of the TS area on LCD
      *         ySize: Maximum Y size of the TS area on LCD
      * @retval TS_OK if all initializations are OK. Other value if error.
      */
    uint8_t BSP_TS_Init(uint16_t xSize, uint16_t ySize)
    {
        uint8_t status = TS_OK;
        ts_x_boundary = xSize;
        ts_y_boundary = ySize;

        /* Read ID and verify if the IO expander is ready */
        if (stmpe811_ts_drv.ReadID(TS_I2C_ADDRESS) == STMPE811_ID)
        {
            /* Initialize the TS driver structure */
            ts_driver = &stmpe811_ts_drv;
            I2C_Address = TS_I2C_ADDRESS;
            ts_orientation = TS_SWAP_XY; //TS_SWAP_NONE; //TS_SWAP_Y;
        }
        else
        {
            IOE_Init();

            /* Check TS3510 touch screen driver presence to determine if TS3510 or
             * EXC7200 driver will be used */
            if (BSP_TS3510_IsDetected() == 0)
            {
                /* Initialize the TS driver structure */
                ts_driver = &ts3510_ts_drv;
                I2C_Address = TS3510_I2C_ADDRESS;
            }
            else
            {
                /* Initialize the TS driver structure */
                ts_driver = &exc7200_ts_drv;
                I2C_Address = EXC7200_I2C_ADDRESS;
            }
            ts_orientation = TS_SWAP_NONE;
        }

        /* Initialize the TS driver */
        ts_driver->Init(I2C_Address);
        ts_driver->Start(I2C_Address);

        return status;
    }

    /**
      * @brief  Returns status and positions of the touch screen.
      * @param  TS_State: Pointer to touch screen current state structure
      * @retval TS_OK if all initializations are OK. Other value if error.
      */
    uint8_t BSP_TS_GetState(TS_StateTypeDef* TS_State)
    {
        static uint32_t _x = 0, _y = 0;
        uint16_t xDiff, yDiff, x, y;
        uint16_t swap;

        TS_State->TouchDetected = ts_driver->DetectTouch(I2C_Address);

        if (TS_State->TouchDetected)
        {
            ts_driver->GetXY(I2C_Address, &x, &y);

            if (ts_orientation & TS_SWAP_X)
            {
                x = 4096 - x;
            }

            if (ts_orientation & TS_SWAP_Y)
            {
                y = 4096 - y;
            }

            if (ts_orientation & TS_SWAP_XY)
            {
                swap = y;
                y = x;
                x = swap;
            }

            xDiff = x > _x ? (x - _x) : (_x - x);
            yDiff = y > _y ? (y - _y) : (_y - y);

            if (xDiff + yDiff > 5)
            {
                _x = x;
                _y = y;
            }

            if (I2C_Address == EXC7200_I2C_ADDRESS)
            {
                TS_State->x = x;
                TS_State->y = y;
            }
            else
            {
                TS_State->x = (ts_x_boundary * _x) >> 12;
                TS_State->y = (ts_y_boundary * _y) >> 12;
            }
        }
        return TS_OK;
    }

    /**
    * @brief  Check TS3510 touch screen presence
    * @param  None
    * @retval Return 0 if TS3510 is detected, return 1 if not detected
    */
    uint8_t BSP_TS3510_IsDetected(void)
    {
        HAL_StatusTypeDef status = HAL_OK;
        uint32_t error = 0;
        uint8_t a_buffer;

        uint8_t tmp_buffer[2] = {0x81, 0x08};

        /* Prepare for LCD read data */
        IOE_WriteMultiple(TS3510_I2C_ADDRESS, 0x8A, tmp_buffer, 2);

        status = HAL_I2C_Mem_Read(&hi2c1, TS3510_I2C_ADDRESS, 0x8A, I2C_MEMADD_SIZE_8BIT, &a_buffer, 1, 1000);

        /* Check the communication status */
        if (status != HAL_OK)
        {
            error = (uint32_t)HAL_I2C_GetError(&hi2c1);

            /* I2C error occured */
            I2Cx_Error(TS3510_I2C_ADDRESS);

            if (error == HAL_I2C_ERROR_AF)
            {
                return 1;
            }
        }
        return 0;
    }

    /**
    * @brief  Manages error callback by re-initializing I2C.
    * @param  Addr: I2C Address
    * @retval None
    */
    static void I2Cx_Error(uint8_t Addr)
    {
        /* De-initialize the I2C communication bus */
        HAL_I2C_DeInit(&hi2c1);

        /* Modification, Draupner Graphics: Add delay to avoid BUSY flag error */
        HAL_Delay(2);

        /* Re-Initialize the I2C communication bus */
        //    I2Cx_Init();
    }

    /*******************************************************************************
                              BUS OPERATIONS
    *******************************************************************************/

    /******************************* I2C Routines *********************************/
    /**
      * @brief  Configures I2C Interrupt.
      * @param  None
      * @retval None
      */
    static void I2Cx_ITConfig(void)
    {
        static uint8_t I2C_IT_Enabled = 0;
        GPIO_InitTypeDef  GPIO_InitStruct;

        if (I2C_IT_Enabled == 0)
        {
            I2C_IT_Enabled = 1;
            /* Enable the GPIO EXTI clock */
            __GPIOI_CLK_ENABLE();
            __SYSCFG_CLK_ENABLE();

            GPIO_InitStruct.Pin   = GPIO_PIN_8;
            GPIO_InitStruct.Pull  = GPIO_NOPULL;
            GPIO_InitStruct.Speed = GPIO_SPEED_LOW;
            GPIO_InitStruct.Mode  = GPIO_MODE_IT_FALLING;
            HAL_GPIO_Init(GPIOI, &GPIO_InitStruct);

            /* Enable and set GPIO EXTI Interrupt to the lowest priority */
            HAL_NVIC_SetPriority((IRQn_Type)(EXTI9_5_IRQn), 0x0F, 0x0F);
            HAL_NVIC_EnableIRQ((IRQn_Type)(EXTI9_5_IRQn));
        }
    }


    /**
      * @brief  Writes a single data.
      * @param  Addr: I2C address
      * @param  Reg: Register address
      * @param  Value: Data to be written
      * @retval None
      */
    static void I2Cx_Write(uint8_t Addr, uint8_t Reg, uint8_t Value)
    {
        HAL_StatusTypeDef status = HAL_OK;

        status = HAL_I2C_Mem_Write(&hi2c1, Addr, (uint16_t)Reg, I2C_MEMADD_SIZE_8BIT, &Value, 1, 100);

        /* Check the communication status */
        if (status != HAL_OK)
        {
            /* Execute user timeout callback */
            I2Cx_Error(Addr);
        }
    }

    /**
      * @brief  Reads a single data.
      * @param  Addr: I2C address
      * @param  Reg: Register address
      * @retval Read data
      */
    static uint8_t I2Cx_Read(uint8_t Addr, uint8_t Reg)
    {
        HAL_StatusTypeDef status = HAL_OK;
        uint8_t Value = 0;

        status = HAL_I2C_Mem_Read(&hi2c1, Addr, Reg, I2C_MEMADD_SIZE_8BIT, &Value, 1, 1000);

        /* Check the communication status */
        if (status != HAL_OK)
        {
            /* Execute user timeout callback */
            I2Cx_Error(Addr);
        }
        return Value;
    }

    /**
      * @brief  Reads multiple data.
      * @param  Addr: I2C address
      * @param  Reg: Reg address
      * @param  Buffer: Pointer to data buffer
      * @param  Length: Length of the data
      * @retval Number of read data
      */
    static HAL_StatusTypeDef I2Cx_ReadMultiple(uint8_t Addr, uint16_t Reg, uint16_t MemAddress, uint8_t* Buffer, uint16_t Length)
    {
        HAL_StatusTypeDef status = HAL_OK;

        if (Addr == EXC7200_I2C_ADDRESS)
        {
            status = HAL_I2C_Master_Receive(&hi2c1, Addr, Buffer, Length, 1000);
        }
        else
        {
            status = HAL_I2C_Mem_Read(&hi2c1, Addr, (uint16_t)Reg, MemAddress, Buffer, Length, 1000);
        }

        /* Check the communication status */
        if (status != HAL_OK)
        {
            /* I2C error occured */
            I2Cx_Error(Addr);
        }
        return status;
    }

    /**
      * @brief  Writes a value in a register of the device through BUS in using DMA mode.
      * @param  Addr: Device address on BUS Bus.
      * @param  Reg: The target register address to write
      * @param  pBuffer: The target register value to be written
      * @param  Length: buffer size to be written
      * @retval HAL status
      */
    static HAL_StatusTypeDef I2Cx_WriteMultiple(uint8_t Addr, uint16_t Reg, uint16_t MemAddress, uint8_t* Buffer, uint16_t Length)
    {
        HAL_StatusTypeDef status = HAL_OK;

        status = HAL_I2C_Mem_Write(&hi2c1, Addr, (uint16_t)Reg, MemAddress, Buffer, Length, 1000);

        /* Check the communication status */
        if (status != HAL_OK)
        {
            /* Re-Initialize the I2C Bus */
            I2Cx_Error(Addr);
        }
        return status;
    }

    /**
      * @brief  Initializes IOE low level.
      * @param  None
      * @retval None
      */
    void IOE_Init(void)
    {
        //Dummy
    }

    /**
      * @brief  Configures IOE low level interrupt.
      * @param  None
      * @retval None
      */
    void IOE_ITConfig(void)
    {
        I2Cx_ITConfig();
    }

    /**
      * @brief  IOE writes single data.
      * @param  Addr: I2C address
      * @param  Reg: Register address
      * @param  Value: Data to be written
      * @retval None
      */
    void IOE_Write(uint8_t Addr, uint8_t Reg, uint8_t Value)
    {
        I2Cx_Write(Addr, Reg, Value);
    }

    /**
      * @brief  IOE reads single data.
      * @param  Addr: I2C address
      * @param  Reg: Register address
      * @retval Read data
      */
    uint8_t IOE_Read(uint8_t Addr, uint8_t Reg)
    {
        return I2Cx_Read(Addr, Reg);
    }

    /**
      * @brief  IOE reads multiple data.
      * @param  Addr: I2C address
      * @param  Reg: Register address
      * @param  Buffer: Pointer to data buffer
      * @param  Length: Length of the data
      * @retval Number of read data
      */
    uint16_t IOE_ReadMultiple(uint8_t Addr, uint8_t Reg, uint8_t* Buffer, uint16_t Length)
    {
        return I2Cx_ReadMultiple(Addr, (uint16_t)Reg, I2C_MEMADD_SIZE_8BIT, Buffer, Length);
    }

    /**
      * @brief  IOE writes multiple data.
      * @param  Addr: I2C address
      * @param  Reg: Register address
      * @param  Buffer: Pointer to data buffer
      * @param  Length: Length of the data
      * @retval None
      */
    void IOE_WriteMultiple(uint8_t Addr, uint8_t Reg, uint8_t* Buffer, uint16_t Length)
    {
        I2Cx_WriteMultiple(Addr, (uint16_t)Reg, I2C_MEMADD_SIZE_8BIT, Buffer, Length);
    }

    /**
      * @brief  IOE delay
      * @param  Delay: Delay in ms
      * @retval None
      */
    void IOE_Delay(uint32_t Delay)
    {
        HAL_Delay(Delay);
    }
} // END of extern "C"

/* USER CODE END STM32TouchController */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
