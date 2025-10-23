/**
  ******************************************************************************
  * @file    jy6311_user_cfg.h
  * @author  Clover
  * @brief   Header file for jy6311 driver user config
  *
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2024 Amplore.
  * All rights reserved.</center></h2>
  *
  * This software is licensed by Amplore under BSD 3-Clause license,
  * the "License"; You may not use this file except in compliance with the
  * License. You may obtain a copy of the License at:
  *                        opensource.org/licenses/BSD-3-Clause
  *
  ******************************************************************************
  */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef _JY6311_USER_CFG_H_
#define _JY6311_USER_CFG_H_

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* Includes ------------------------------------------------------------------*/
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>


/** @addtogroup JY6311_Driver
  * @{
  */


/* Exported Constants --------------------------------------------------------*/
/** @addtogroup JY6311_Exported_Constants
  * @{
  */

#define JY6311_MODULE_ENABLED               /*!< JY6311 Module Enable   */

/**
  * @brief JY6311 debug level macro definition
  */
#define JY6311_DBG_NONE            0        /*!< Debug None             */
#define JY6311_DBG_ERROR           1        /*!< Debug Error            */
#define JY6311_DBG_WARNING         2        /*!< Debug Warning          */
#define JY6311_DBG_INFO            3        /*!< Debug Information      */
#define JY6311_DBG_LOG             4        /*!< Debug Log              */

/**
  * @brief JY6311 debug level user select
  * @note The debug information which is less than or equal to this debug level will be printed
  */
#define JY6311_DBG_LVL             JY6311_DBG_LOG

/**
  * @}
  */


/* Exported Macros -----------------------------------------------------------*/
/** @addtogroup JY6311_Exported_Macros
  * @{
  */

/**
  * @brief JY6311 debug print user macro definition
  */
#define JY6311_DBG_PRINT_USER(fmt, ...)             printf(fmt, ##__VA_ARGS__)


/**
  * @brief  JY6311 I2C read byte function macro definition
  * @param  i2c_addr JY6311 7bit I2C address
  * @param  reg JY6311 register address to read
  * @return register value
  */
#define JY6311_I2C_READ_BYTE(i2c_addr, reg)         jy6311_i2c_read_byte(i2c_addr, reg)

/**
  * @brief  JY6311 I2C write byte function macro definition
  * @param  i2c_addr JY6311 7bit I2C address
  * @param  reg JY6311 register address to write
  * @param  val register value
  * @return 0: Success; other: Failed
  */
#define JY6311_I2C_WRITE_BYTE(i2c_addr, reg, val)   jy6311_i2c_write_byte(i2c_addr, reg, val)

/**
  * @}
  */


/* Exported Types ------------------------------------------------------------*/
/** @addtogroup JY6311_Exported_Types
  * @{
  */

#if 0
/**
  * @brief JY6311 data type definition
  */
typedef   signed       char int8_t;
typedef   signed short  int int16_t;
typedef   signed        int int32_t;
typedef unsigned       char uint8_t;
typedef unsigned short  int uint16_t;
typedef unsigned        int uint32_t;
#endif

/**
  * @}
  */


/* Exported Variables --------------------------------------------------------*/
/* Exported Functions --------------------------------------------------------*/
/** @addtogroup JY6311_Exported_Functions
  * @{
  */
unsigned char jy6311_i2c_read_byte(unsigned char i2c_addr, unsigned char reg);
signed char jy6311_i2c_write_byte(unsigned char i2c_addr, unsigned char reg, unsigned char val);
/**
  * @}
  */


/**
  * @}
  */


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _JY6311_USER_CFG_H_ */

/************************* (C) COPYRIGHT Amplore *****END OF FILE***********/
