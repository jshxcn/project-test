/**
  ******************************************************************************
  * File Name          : STM32TouchController.hpp
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

#ifndef STM32TOUCHCONTROLLER_HPP
#define STM32TOUCHCONTROLLER_HPP

#include <platform/driver/touch/TouchController.hpp>

extern "C" {
#include "stm32f4xx_hal.h"
    /* Include IOExpander(STMPE811) component Driver */
#include "../stmpe811/stmpe811.h"
    /* Include TouchScreen component drivers */
#include "../ts3510/ts3510.h"
#include "../exc7200/exc7200.h"

    /** @defgroup STM324x9I_EVAL_TS_Exported_Constants
    * @{
    */
#define TS_SWAP_NONE                    0x00
#define TS_SWAP_X                       0x01
#define TS_SWAP_Y                       0x02
#define TS_SWAP_XY                      0x04

    /* Interrupt sources pins definition */
#define TS_INT_PIN                      0x0010

#define TS_I2C_ADDRESS                   0x82
#define TS3510_I2C_ADDRESS               0x80
#define EXC7200_I2C_ADDRESS              0x08

    typedef enum
    {
        TS_OK       = 0x00,
        TS_ERROR    = 0x01,
        TS_TIMEOUT  = 0x02
    } TS_StatusTypeDef;

    /** @defgroup STM324x9I_EVAL_TS_Exported_Types
      * @{
      */
    typedef struct
    {
        uint16_t TouchDetected;
        uint16_t x;
        uint16_t y;
        uint16_t z;
    } TS_StateTypeDef;

}
/**
 * @class STM32TouchController
 *
 * @brief This class specializes TouchController Interface.
 *
 * @sa touchgfx::TouchController
 */

class STM32TouchController : public touchgfx::TouchController
{
public:

    STM32TouchController() {}

    /**
      * @fn virtual void STM32TouchController::init() = 0;
      *
      * @brief Initializes touch controller.
      *
      *        Initializes touch controller.
      */
    virtual void init();

    /**
    * @fn virtual bool STM32TouchController::sampleTouch(int32_t& x, int32_t& y) = 0;
    *
    * @brief Checks whether the touch screen is being touched, and if so, what coordinates.
    *
    *        Checks whether the touch screen is being touched, and if so, what coordinates.
    *
    * @param [out] x The x position of the touch
    * @param [out] y The y position of the touch
    *
    * @return True if a touch has been detected, otherwise false.
    */
    virtual bool sampleTouch(int32_t& x, int32_t& y);

protected:
    bool isInitialized;
};

#endif // STM32TOUCHCONTROLLER_HPP

/* USER CODE END STM32TouchController */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
