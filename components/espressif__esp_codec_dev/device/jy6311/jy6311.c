/**
  ******************************************************************************
  * @file    jy6311.c
  * @author  Clover
  * @brief   jy6311 drivers for Bare metal and RTOS
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

/* Includes ------------------------------------------------------------------*/
#include "jy6311.h"

// for ESP32
#include <string.h>
#include "jy6311_codec.h"
#include "esp_log.h"
#include "es_common.h"
#include "esp_codec_dev_os.h"

#define TAG          "JY6311"

/** @defgroup JY6311_Driver JY6311 Driver
  * @brief    JY6311 Driver for Bare metal and RTOS
  * @{
  */

#ifdef JY6311_MODULE_ENABLED

/* Private Constants ---------------------------------------------------------*/
/** @defgroup JY6311_Private_Constants JY6311 Private Constants
  * @brief    JY6311 Private Constants
  * @{
  */

// for ESP32
#define I2S_SLOT_NUMS           (2)
#define I2S_SLOT_WIDTH          (16)
#define I2S_FORMAT              (JY6311_I2S_FMT_I2S)

/**
  * @}
  */


/* Private Macros ------------------------------------------------------------*/
/** @defgroup JY6311_Private_Macros JY6311 Private Macros
  * @brief    JY6311 Private Macros
  * @{
  */

/**
  * @brief JY6311 DAC EQx Band Coff config macro function definition
  * @param  i2c_addr jy6311 7bit I2C address
  * @param  x DAC EQ Band number, range in [1, 2, 3]
  * @param  array_cfg dac eq coff config array pointer
  */
#define JY6311_DAC_EQx_BAND_COFF_CFG(i2c_addr, x, array_cfg)                            \
    do {                                                                                \
        __JY6311_DAC_EQ##x##_Coff_b0h_Set(i2c_addr, (uint8_t)((array_cfg)[0] >> 16));   \
        __JY6311_DAC_EQ##x##_Coff_b0m_Set(i2c_addr, (uint8_t)((array_cfg)[0] >> 8));    \
        __JY6311_DAC_EQ##x##_Coff_b0l_Set(i2c_addr, (uint8_t)(array_cfg)[0]);           \
        __JY6311_DAC_EQ##x##_Coff_b1h_Set(i2c_addr, (uint8_t)((array_cfg)[1] >> 16));   \
        __JY6311_DAC_EQ##x##_Coff_b1m_Set(i2c_addr, (uint8_t)((array_cfg)[1] >> 8));    \
        __JY6311_DAC_EQ##x##_Coff_b1l_Set(i2c_addr, (uint8_t)(array_cfg)[1]);           \
        __JY6311_DAC_EQ##x##_Coff_b2h_Set(i2c_addr, (uint8_t)((array_cfg)[2] >> 16));   \
        __JY6311_DAC_EQ##x##_Coff_b2m_Set(i2c_addr, (uint8_t)((array_cfg)[2] >> 8));    \
        __JY6311_DAC_EQ##x##_Coff_b2l_Set(i2c_addr, (uint8_t)(array_cfg)[2]);           \
        __JY6311_DAC_EQ##x##_Coff_a1h_Set(i2c_addr, (uint8_t)((array_cfg)[3] >> 16));   \
        __JY6311_DAC_EQ##x##_Coff_a1m_Set(i2c_addr, (uint8_t)((array_cfg)[3] >> 8));    \
        __JY6311_DAC_EQ##x##_Coff_a1l_Set(i2c_addr, (uint8_t)(array_cfg)[3]);           \
        __JY6311_DAC_EQ##x##_Coff_a2h_Set(i2c_addr, (uint8_t)((array_cfg)[4] >> 16));   \
        __JY6311_DAC_EQ##x##_Coff_a2m_Set(i2c_addr, (uint8_t)((array_cfg)[4] >> 8));    \
        __JY6311_DAC_EQ##x##_Coff_a2l_Set(i2c_addr, (uint8_t)(array_cfg)[4]);           \
    } while (0)

/**
  * @}
  */


/* Private Types -------------------------------------------------------------*/
/** @defgroup JY6311_Private_Types JY6311 Private Types
  * @brief    JY6311 Private Types
  * @{
  */

enum {
    JY6311_SMP_OSR_ADC = 0,             /*!< smp_osr_pll_out array idx ADC  */
    JY6311_SMP_OSR_DAC,                 /*!< smp_osr_pll_out array idx DAC  */
};

/**
  * @brief JY6311 PLL Divider factor struct definition
  * @note  FOUT = FIN * N /(M*P)
  */
struct pll_div {
    uint32_t freq_in;                   /*!< PLL input frequence            */
    uint32_t freq_out;                  /*!< PLL output frequence           */
    uint16_t n;                         /*!< PLL feedback-div factor N      */
    uint8_t p;                          /*!< PLL pre-div factor P           */
    uint8_t m;                          /*!< PLL post-div factor M          */
};

/**
  * @brief JY6311 register value struct definition
  */
struct reg_val {
    uint8_t reg;                        /*!< JY6311 register address        */
    uint8_t val;                        /*!< JY6311 register value          */
} ;

/**
  * @brief JY6311 sample rate to OSR and pll frequence out struct definition
  */
struct smp_osr_pll_out {
    JY6311_UserSmpRateETypeDef smp;     /*!< User sample rate in Hz         */
    JY6311_ADDA_OSRETypeDef osr;        /*!< ADC/DAC OSR                    */
    uint32_t pll_out;                   /*!< PLL frequence out              */
};

// for ESP32
typedef struct {
    audio_codec_if_t   base;
    jy6311_codec_cfg_t cfg;
    bool               is_open;
    bool               enabled;
    float              hw_gain;
} audio_codec_jy6311_t;

/**
  * @}
  */


/* Private Variables ---------------------------------------------------------*/
/** @defgroup JY6311_Private_Variables JY6311 Private Variables
  * @brief    JY6311 Private Variables
  * @{
  */

/**
  * @brief JY6311 PLL Divider factor array definition
  * @note  FOUT = FIN * N /(M*P), N[1, 256], P[1, 32], M[2, 16] and must be even
  */
static const struct pll_div jy6311_pll_div[] = {
    // 48k Series @6.144M
    {512000,    6144000, 192,  1, 16},
    {768000,    6144000, 128,  1, 16},
    {1024000,   6144000,  96,  1, 16},
    {1536000,   6144000,  64,  1, 16},
    {2048000,   6144000,  96,  2, 16},
    {3072000,   6144000,  96,  3, 16},
    {4096000,   6144000,  96,  4, 16},
    {4608000,   6144000,  64,  3, 16},
    {6000000,   6144000,  82,  5, 16},  //out: 6.15M
    {12000000,  6144000,  82, 10, 16},  //out: 6.15M
    {13000000,  6144000,  68,  9, 16},  //out: 6.139M
    {15360000,  6144000,  64, 10, 16},
    {16000000,  6144000,  92, 15, 16},  //out: 6.133M
    {19200000,  6144000,  82, 16, 16},  //out: 6.15M
    {19680000,  6144000,  95, 19, 16},  //out: 6.15M

    // 44.1k Series @5.6448M
    {705600,    5644800, 144,  1, 18},
    {1058400,   5644800,  96,  1, 18},
    {1411200,   5644800,  72,  1, 18},
    {1587600,   5644800,  64,  1, 18},
    {2116800,   5644800,  96,  2, 18},
    {2822400,   5644800,  72,  2, 18},
    {3175200,   5644800,  64,  2, 18},
    {4233600,   5644800,  96,  4, 18},
    {6000000,   5644800, 113,  6, 20},  //out: 5.65M
    {12000000,  5644800, 113, 12, 20},  //out: 5.65M
    {13000000,  5644800, 113, 13, 20},  //out: 5.65M
    {15360000,  5644800,  86, 13, 18},  //out: 5.6451M
    {16000000,  5644800, 113, 16, 20},  //out: 5.65M
    {19200000,  5644800,  53, 10, 18},  //out: 5.6533M
    {19680000,  5644800,  62, 12, 18},  //out: 5.6489M

    // 48k Series @12.288M
    {512000,   12288000, 192,  1,  8},
    {768000,   12288000, 128,  1,  8},
    {1024000,  12288000,  96,  1,  8},
    {1536000,  12288000,  64,  1,  8},
    {2048000,  12288000,  96,  2,  8},
    {3072000,  12288000,  96,  3,  8},
    {4096000,  12288000,  96,  4,  8},
    {4608000,  12288000,  64,  3,  8},
    {6000000,  12288000,  82,  5,  8},   //out: 12.3M
    {12000000, 12288000,  82, 10,  8},   //out: 12.3M
    {13000000, 12288000,  68,  9,  8},   //out: 12.278M
    {15360000, 12288000,  64, 10,  8},
    {16000000, 12288000,  92, 15,  8},   //out: 12.267M
    {19200000, 12288000,  82, 16,  8},   //out: 12.3M
    {19680000, 12288000,  95, 19,  8},   //out: 12.3M
};

/**
  * @brief jy6311 all register defalut value array
  */
static const struct reg_val jy6311_reg_default_val [] = {
    // Power and PLL control register
    {SRST, 0x00},
    {PWR_CTRL1, 0x8c},
    {PWR_CTRL2, 0x48},
    {PLL_CTRL1, 0x48},
    {PLL_CFG1, 0x00},
    {PLL_CFG2, 0x01},
    {PLL_CFG3, 0x00},

    // Clock Control register
    {SYSCLK_CTRL, 0x00},
    {MODCLK_EN, 0x00},
    {ADDA_FS, 0x25},
    {ADC_OSR_CLK, 0x20},
    {DAC_CLK_DIV, 0x20},

    // I2S Control register
    {I2S_CTRL, 0x00},
    {I2S_FMT1, 0x00},
    {I2S_FMT2, 0x3f},
    {I2S_FMT3, 0x05},
    {I2S_TX_CTRL, 0x35},
    {I2S_RX_CTRL, 0x05},
    {I2S_TXM_CTRL, 0x11},
    {I2S_RXM_CTRL, 0x01},
    {I2S_PAD_CTRL1, 0x80},

    // ADC Digital control register
    {ADC_DIG_EN, 0x06},
    {ADC_HPF_COEF, 0x00},
    {ADC_DVC_CTRL, 0xbf},
    {ADC_SMUTE, 0x10},

    // ADC EQ Control register
    {ADC_EQ_CTRL, 0x00},
    {ADC_EQ1_b0_H, 0x00},
    {ADC_EQ1_b0_M, 0x00},
    {ADC_EQ1_b0_L, 0x00},
    {ADC_EQ1_b1_H, 0x00},
    {ADC_EQ1_b1_M, 0x00},
    {ADC_EQ1_b1_L, 0x00},
    {ADC_EQ1_b2_H, 0x00},
    {ADC_EQ1_b2_M, 0x00},
    {ADC_EQ1_b2_L, 0x00},
    {ADC_EQ1_a1_H, 0x00},
    {ADC_EQ1_a1_M, 0x00},
    {ADC_EQ1_a1_L, 0x00},
    {ADC_EQ1_a2_H, 0x00},
    {ADC_EQ1_a2_M, 0x00},
    {ADC_EQ1_a2_L, 0x00},

    // ADC DRC Control register
    {ADC_DRC_CTL, 0x02},
    {ADC_PEAK_CTL, 0x16},
    {ADC_ATK_TIME, 0x27},
    {ADC_DRC_THD1, 0x02},
    {ADC_DRC_THD2, 0x0a},
    {ADC_DRC_THD3, 0x16},
    {ADC_DRC_THD4, 0x0f},
    {ADC_DRC_SLP1, 0x72},
    {ADC_DRC_SLP2, 0x70},

    // DAC Digital control register
    {DAC_DIG_EN, 0x06},
    {DAC_DVC_CTRL, 0xbf},
    {DAC_SMUTE, 0x10},
    {DAC_DMIX_CTRL, 0x01},

    // DAC EQ Control register
    {DAC_EQ_CTRL, 0x00},
    {DAC_EQ1_b0_H, 0x00},
    {DAC_EQ1_b0_M, 0x00},
    {DAC_EQ1_b0_L, 0x00},
    {DAC_EQ1_b1_H, 0x00},
    {DAC_EQ1_b1_M, 0x00},
    {DAC_EQ1_b1_L, 0x00},
    {DAC_EQ1_b2_H, 0x00},
    {DAC_EQ1_b2_M, 0x00},
    {DAC_EQ1_b2_L, 0x00},
    {DAC_EQ1_a1_H, 0x00},
    {DAC_EQ1_a1_M, 0x00},
    {DAC_EQ1_a1_L, 0x00},
    {DAC_EQ1_a2_H, 0x00},
    {DAC_EQ1_a2_M, 0x00},
    {DAC_EQ1_a2_L, 0x00},
    {DAC_EQ2_b0_H, 0x00},
    {DAC_EQ2_b0_M, 0x00},
    {DAC_EQ2_b0_L, 0x00},
    {DAC_EQ2_b1_H, 0x00},
    {DAC_EQ2_b1_M, 0x00},
    {DAC_EQ2_b1_L, 0x00},
    {DAC_EQ2_b2_H, 0x00},
    {DAC_EQ2_b2_M, 0x00},
    {DAC_EQ2_b2_L, 0x00},
    {DAC_EQ2_a1_H, 0x00},
    {DAC_EQ2_a1_M, 0x00},
    {DAC_EQ2_a1_L, 0x00},
    {DAC_EQ2_a2_H, 0x00},
    {DAC_EQ2_a2_M, 0x00},
    {DAC_EQ2_a2_L, 0x00},
    {DAC_EQ3_b0_H, 0x00},
    {DAC_EQ3_b0_M, 0x00},
    {DAC_EQ3_b0_L, 0x00},
    {DAC_EQ3_b1_H, 0x00},
    {DAC_EQ3_b1_M, 0x00},
    {DAC_EQ3_b1_L, 0x00},
    {DAC_EQ3_b2_H, 0x00},
    {DAC_EQ3_b2_M, 0x00},
    {DAC_EQ3_b2_L, 0x00},
    {DAC_EQ3_a1_H, 0x00},
    {DAC_EQ3_a1_M, 0x00},
    {DAC_EQ3_a1_L, 0x00},
    {DAC_EQ3_a2_H, 0x00},
    {DAC_EQ3_a2_M, 0x00},
    {DAC_EQ3_a2_L, 0x00},

    // DAC DRC Control register
    {DAC_DRC_CTL, 0x02},
    {DAC_PEAK_CTL, 0x16},
    {DAC_ATK_TIME, 0x27},
    {DAC_DRC_THD1, 0x02},
    {DAC_DRC_THD2, 0x0a},
    {DAC_DRC_THD3, 0x16},
    {DAC_DRC_THD4, 0x0f},
    {DAC_DRC_SLP1, 0x72},
    {DAC_DRC_SLP2, 0x70},

    // ADC Analog Control register
    {ANA_ADC_CTRL1, 0x00},
    {ANA_ADC_CTRL2, 0x29},
    {ANA_ADC_CTRL3, 0xff},
    {ANA_ADC_CTRL4, 0xf4},
    {ANA_ADC_TUNE1, 0x7e},
    {ANA_ADC_TUNE2, 0x5a},

    // DAC Analog Control register
    {ANA_DAC_CTRL1, 0x20},
    {ANA_DAC_CTRL2, 0x93},  // To Do: not equal to SPEC value 0xb2
    {ANA_DAC_CTRL3, 0x03},  // To Do: not equal to SPEC value 0x0f
    {ANA_DAC_TUNE1, 0x04},
    {ANA_DAC_TUNE2, 0x50},
    {ANA_DAC_TUNE3, 0xaa},

    // Debug Register
    {ADDA_DEBUG, 0x80},
    {CHIP_VERSION, 0x12},
};

static const struct smp_osr_pll_out jy6311_smp_osr_pll_out[][2][11] = {
    // ADC
    {
        // Normal performance
        {
            {JY6311_USER_SR_8K,    JY6311_ADDA_OSR_64,   6144000},
            {JY6311_USER_SR_12K,   JY6311_ADDA_OSR_64,   6144000},
            {JY6311_USER_SR_16K,   JY6311_ADDA_OSR_64,   6144000},
            {JY6311_USER_SR_24K,   JY6311_ADDA_OSR_64,   6144000},
            {JY6311_USER_SR_32K,   JY6311_ADDA_OSR_64,   6144000},
            {JY6311_USER_SR_48K,   JY6311_ADDA_OSR_64,   6144000},
            {JY6311_USER_SR_96K,   JY6311_ADDA_OSR_64,   6144000},
            {JY6311_USER_SR_192K,  JY6311_ADDA_OSR_32,   6144000},
            {JY6311_USER_SR_11025, JY6311_ADDA_OSR_64,   5644800},
            {JY6311_USER_SR_22050, JY6311_ADDA_OSR_64,   5644800},
            {JY6311_USER_SR_44100, JY6311_ADDA_OSR_64,   5644800},
        },

        // High performance
        {
            {JY6311_USER_SR_8K,    JY6311_ADDA_OSR_128,  6144000},
            {JY6311_USER_SR_12K,   JY6311_ADDA_OSR_128,  6144000},
            {JY6311_USER_SR_16K,   JY6311_ADDA_OSR_128,  6144000},
            {JY6311_USER_SR_24K,   JY6311_ADDA_OSR_128,  6144000},
            {JY6311_USER_SR_32K,   JY6311_ADDA_OSR_64,   6144000},
            {JY6311_USER_SR_48K,   JY6311_ADDA_OSR_128,  6144000},
            {JY6311_USER_SR_96K,   JY6311_ADDA_OSR_64,   6144000},
            {JY6311_USER_SR_192K,  JY6311_ADDA_OSR_32,   6144000},
            {JY6311_USER_SR_11025, JY6311_ADDA_OSR_128,  5644800},
            {JY6311_USER_SR_22050, JY6311_ADDA_OSR_128,  5644800},
            {JY6311_USER_SR_44100, JY6311_ADDA_OSR_128,  5644800},
        },
    },

    // DAC
    {
        // Normal performance
        {
            {JY6311_USER_SR_8K,    JY6311_ADDA_OSR_128,  6144000},
            {JY6311_USER_SR_12K,   JY6311_ADDA_OSR_128,  6144000},
            {JY6311_USER_SR_16K,   JY6311_ADDA_OSR_128,  6144000},
            {JY6311_USER_SR_24K,   JY6311_ADDA_OSR_128,  6144000},
            {JY6311_USER_SR_32K,   JY6311_ADDA_OSR_128, 12288000},
            {JY6311_USER_SR_48K,   JY6311_ADDA_OSR_128,  6144000},
            {JY6311_USER_SR_96K,   JY6311_ADDA_OSR_128, 12288000},
            {JY6311_USER_SR_192K,  JY6311_ADDA_OSR_64,  12288000},
            {JY6311_USER_SR_11025, JY6311_ADDA_OSR_128,  5644800},
            {JY6311_USER_SR_22050, JY6311_ADDA_OSR_128,  5644800},
            {JY6311_USER_SR_44100, JY6311_ADDA_OSR_128,  5644800},
        },

        // High performance
        {
            {JY6311_USER_SR_8K,    JY6311_ADDA_OSR_256,  6144000},
            {JY6311_USER_SR_12K,   JY6311_ADDA_OSR_256,  6144000},
            {JY6311_USER_SR_16K,   JY6311_ADDA_OSR_256, 12288000},
            {JY6311_USER_SR_24K,   JY6311_ADDA_OSR_256,  6144000},
            {JY6311_USER_SR_32K,   JY6311_ADDA_OSR_128, 12288000},
            {JY6311_USER_SR_48K,   JY6311_ADDA_OSR_256, 12288000},
            {JY6311_USER_SR_96K,   JY6311_ADDA_OSR_128, 12288000},
            {JY6311_USER_SR_192K,  JY6311_ADDA_OSR_64,  12288000},
            {JY6311_USER_SR_11025, JY6311_ADDA_OSR_256,  5644800},
            {JY6311_USER_SR_22050, JY6311_ADDA_OSR_256,  5644800},
            {JY6311_USER_SR_44100, JY6311_ADDA_OSR_128,  5644800},  // To Do: 256 or 128 ?
        },
    },
};

/**
  * @brief jy6311 adc/dac osr real value array
  */
static const uint16_t jy6311_adda_osr[] = {32, 64, 128, 256};

/**
  * @brief jy6311 adc/dac DCLK and I2S BCLK divider real value array
  */
static const uint8_t jy6311_adda_dclk_div[] = {1, 2, 3, 4, 6, 8, 12, 16, 24};

// for ESP32
static const esp_codec_dev_vol_range_t vol_range = {
    .min_vol =
    {
        .vol = 0x0,
        .db_value = -95.5,
    },
    .max_vol =
    {
        .vol = 0xFF,
        .db_value = 32.0,
    },
};

/**
  * @brief JY6311 DAC EQ Coefficient array
  * @note  This parameter array is generated by JY6311 AEtool
  */
static const int dac_eq_filt_coef[] = {
    0xfe60a,  0xe033eb, 0xfe60a,  0xe03414, 0xfcc3e,
    0x1015df, 0xe085f6, 0xf66f4,  0xe085f6, 0xf7cd4,
    0x218bdc, 0xd3293e, 0x10f3fb, 0xf011ef, 0x59727,
};

/**
  * @brief JY6311 DAC DRC Coefficient array
  * @note  This parameter array is generated by JY6311 AEtool
  */
static const int dac_drc_filt_coef[] = {
    0x6,  0x7, 0xf,  0x18,
    0x16, 0xf, 0x62, 0xf,
};

static audio_codec_jy6311_t *jy6311_audio_codec;

/**
  * @}
  */


/* Private Function Prototypes -----------------------------------------------*/
/** @addtogroup JY6311_Private_Functions
  * @{
  */
static uint8_t jy6311_smpRateToRegOrIdx(uint32_t rate, bool reg);
static JY6311_StatusETypeDef jy6311_pllCfg(uint8_t i2c_addr, JY6311_PLLCLKSrcETypeDef clk_src,
                                                  uint32_t freq_in, uint32_t freq_out);
/**
  * @}
  */


/* Exported Variables --------------------------------------------------------*/
/* Exported Functions --------------------------------------------------------*/
/** @defgroup JY6311_Exported_Functions JY6311 Exported Functions
  * @brief    JY6311 Exported Functions
  * @{
  */

/** @defgroup JY6311_Exported_Functions_Group1 JY6311 I2C Read/Write Functions
  * @brief    JY6311 I2C Read/Write Functions
  * @{
  */

unsigned char jy6311_i2c_read_byte(unsigned char i2c_addr, unsigned char reg)
{
    int ret;
    uint8_t data = 0;
    JY6311_UNUSED(i2c_addr);

    if (jy6311_audio_codec == NULL || jy6311_audio_codec->cfg.ctrl_if == NULL ||
        jy6311_audio_codec->cfg.ctrl_if->read_reg == NULL) {
        return 0;
    }

    ret = jy6311_audio_codec->cfg.ctrl_if->read_reg(jy6311_audio_codec->cfg.ctrl_if, reg, sizeof(reg), &data, sizeof(data));
    return ret == ESP_CODEC_DEV_OK ? data : 0;
}

signed char jy6311_i2c_write_byte(unsigned char i2c_addr, unsigned char reg, unsigned char val)
{
    int ret;
    JY6311_UNUSED(i2c_addr);

    if (jy6311_audio_codec == NULL || jy6311_audio_codec->cfg.ctrl_if == NULL ||
        jy6311_audio_codec->cfg.ctrl_if->write_reg == NULL) {
        return -1;
    }

    ret = jy6311_audio_codec->cfg.ctrl_if->write_reg(jy6311_audio_codec->cfg.ctrl_if, reg, sizeof(reg), &val, sizeof(val));
    return ret == ESP_CODEC_DEV_OK ? 0 : -1;
}

/**
  * @brief  jy6311 update bits
  * @param  i2c_addr jy6311 7bit I2C address
  * @param  reg JY6311 register address to update
  * @param  mask update mask
  * @param  val update value
  * @return 0: Success; other: Failed
  */
int8_t jy6311_update_bits(uint8_t i2c_addr, uint8_t reg, uint8_t mask, uint8_t val)
{
    int8_t ret = 0;
	uint8_t val_old, val_new;

	val_old = JY6311_I2C_READ_BYTE(i2c_addr, reg);
	val_new = (val_old & ~mask) | (val & mask);
	if (val_new != val_old) {
		ret = JY6311_I2C_WRITE_BYTE(i2c_addr, reg, val_new);
	}

	return ret;
}

/**
  * @}
  */


/** @defgroup JY6311_Exported_Functions_Group2 JY6311 Init and DeInit Functions
  * @brief    JY6311 Init and DeInit Functions
  * @{
  */

/**
  * @brief  jy6311 init
  * @param  i2c_addr jy6311 7bit I2C address
  * @param  mod jy6311 Init module select
  * @return JY6311_StatusETypeDef
  */
JY6311_StatusETypeDef jy6311_init(uint8_t i2c_addr, JY6311_InitModETypeDef mod)
{
    JY6311_LOG_I("--->%s\n", __FUNCTION__);

    /* ADC Init */
    if (mod & JY6311_INIT_MOD_ADC) {
        // Optimization Config
        __JY6311_ADC_SDM_OpCurr1st_Set(i2c_addr, JY6311_ADC_SDM_OP_CURR_1st_240uA);
        __JY6311_ADC_SDM_OpCurr2nd_Set(i2c_addr, JY6311_ADC_SDM_OP_CURR_2nd_36uA);
        //__JY6311_ADC_PGA_OpAAFOutCurr_Set(i2c_addr, JY6311_ADC_PGA_OPAAF_OUT_CURR_100uA);

        //__JY6311_ADC_PGA_OpMICBiasCurr_Set(i2c_addr, JY6311_ADC_PGA_OPMIC_BIAS_CURR_5uA);
        //__JY6311_ADC_PGA_OpAAFBiasCurr_Set(i2c_addr, JY6311_ADC_PGA_OPAAF_BIAS_CURR_5uA);
        __JY6311_ADC_PGA_SDMBiasCurr_Set(i2c_addr, JY6311_ADC_PGA_SDM_BIAS_CURR_3uA);
        //__JY6311_ADC_PGA_VCMBiasCurr_Set(i2c_addr, JY6311_ADC_PGA_VCM_BIAS_CURR_4uA);

        // MICBIAS Enable
        //__JY6311_MicbiasRegBypass_Dis(i2c_addr);
        //__JY6311_MicbiasChopFreq_Set(i2c_addr, JY6311_MICBIAS_CHOP_FREQ_384KHz);
        //__JY6311_MicbiasChop_En(i2c_addr);
        __JY6311_MicbiasOutputVol_Set(i2c_addr, JY6311_MICBIAS_VOL_2V5);
        __JY6311_Micbias_En(i2c_addr);

        //__JY6311_Debug_En(i2c_addr);
        //__JY6311_DebugSel_ADC(i2c_addr);
        //__JY6311_DebugPat_Set(i2c_addr, JY6311_DBG_PAT_0dB_Sin);
        //__JY6311_ADC_HPF_Dis(i2c_addr);
    }

    /* DAC Init */
    if (mod & JY6311_INIT_MOD_DAC) {
        // Optimization Config
        //__JY6311_DAC_OutAutoRampUp_En(i2c_addr);
        __JY6311_DAC_OutChargeTime_Set(i2c_addr, JY6311_DAC_OUT_CHARGE_TIME_256ms);
        __JY6311_DAC_OutRampUp_Dis(i2c_addr);
        //__JY6311_DAC_OutRampTime_Set(i2c_addr, JY6311_DAC_OUT_RAMP_TIME_512ms);

        //__JY6311_DAC_OpdctOutStageCurr_Set(i2c_addr, JY6311_DAC_OPDCT_OUT_STA_CURR_60uA);
        //__JY6311_DAC_DCT_OpdctBiasCurr_Set(i2c_addr, JY6311_DAC_DCT_BIAS_CURR_4uA);
        //__JY6311_DAC_DCT_OpoutBiasCurr_Set(i2c_addr, JY6311_DAC_DCT_BIAS_CURR_7uA);
        //__JY6311_DAC_DCT_VcmbufBiasCurr_Set(i2c_addr, JY6311_DAC_DCT_BIAS_CURR_4uA);

		__JY6311_DAC_AutoMute_En(i2c_addr);
    }

    /* Common Init */
    __JY6311_I2S_PadDrvLvl_Set(i2c_addr, JY6311_PAD_DRV_LVL_2);
    __JY6311_DMICDAT_Pad_Dis(i2c_addr);
    //__JY6311_I2S_All_Pad_En(i2c_addr);

    /* Power On */
    //__JY6311_DLDO_OutputVol_Set(i2c_addr, JY6311_DLDO_VOL_1V2);
    //__JY6311_VRP_En(i2c_addr);
    //__JY6311_ALDO_VrefFilResBypass_Dis(i2c_addr);
    //__JY6311_ALDO_OutputVol_Set(i2c_addr, JY6311_ALDO_VOL_1V9);
    __JY6311_ALDO_En(i2c_addr);

    return JY6311_OK;
}

/**
  * @brief  jy6311 deinit
  * @param  i2c_addr jy6311 7bit I2C address
  * @param  mod jy6311 DeInit module select
  * @return JY6311_StatusETypeDef
  */
JY6311_StatusETypeDef jy6311_deinit(uint8_t i2c_addr, JY6311_InitModETypeDef mod)
{
    JY6311_LOG_I("--->%s\n", __FUNCTION__);

    /* ADC DeInit */
    if (mod & JY6311_INIT_MOD_ADC) {
        // MICBIAS Disable
        __JY6311_Micbias_Dis(i2c_addr);
    }

    /* DAC DeInit */
    if (mod & JY6311_INIT_MOD_DAC) {
        // Reserver for future
    }

    /* Common DeInit */
    if (mod == JY6311_INIT_MOD_ADC_DAC) {
        // Power Off
        __JY6311_VRP_Dis(i2c_addr);
        __JY6311_ALDO_Dis(i2c_addr);

        // I2S Global Disable
        __JY6311_I2S_Global_Dis(i2c_addr);

        // SYSCLK Disable
        __JY6311_SYSCLK_Dis(i2c_addr);
    }

    return JY6311_OK;
}

/**
  * @}
  */


/** @defgroup JY6311_Exported_Functions_Group3 JY6311 Play and Record Functions
  * @brief    JY6311 Play and Record Functions
  * @{
  */

/**
  * @brief  jy6311 play start
  * @param  i2c_addr jy6311 7bit I2C address
  * @return None
  */
void jy6311_play_start(uint8_t i2c_addr)
{
    JY6311_LOG_I("--->%s\n", __FUNCTION__);

    /* DAC Module Clock Enable */
    __JY6311_DAC_Path_Clk_En(i2c_addr);
    __JY6311_DAC_Analog_Clk_En(i2c_addr);
    __JY6311_DAC_Dig_Clk_En(i2c_addr);

    /* DAC digital config */
    //__JY6311_DAC_SDM_4bit(i2c_addr);
    //__JY6311_DAC_SDM_PreScale_Set(i2c_addr, JY6311_DAC_SDM_PRE_SCL_MINUS_0Dot6dBFS);
    __JY6311_DAC_DitherAmp_Set(i2c_addr, JY6311_DAC_DITHER_AMP_1_8LSB);
    //__JY6311_DAC_Dither_En(i2c_addr);
    __JY6311_DAC_Dig_En(i2c_addr);

    /* DAC analog config */
    //__JY6311_DAC_OutGain_Set(i2c_addr, JY6311_DAC_OUT_GAIN_0dB);
    //__JY6311_DAC_OutMode_Set(i2c_addr, JY6311_DAC_OUT_MODE_DIFFERENTIAL);
    //__JY6311_DAC_OutPath_Set(i2c_addr, JY6311_DAC_OUT_PATH_HEADPHONE);
    __JY6311_DAC_OutSrc_DAC_En(i2c_addr);
    //__JY6311_DAC_OutSrc_MIC_Dis(i2c_addr);
    __JY6311_DAC_Out_En(i2c_addr);
    __JY6311_DAC_DCT_En(i2c_addr);

    /* I2S Rx Enable */
    __JY6311_I2S_Rx_En(i2c_addr);
}

/**
  * @brief  jy6311 play stop
  * @param  i2c_addr jy6311 7bit I2C address
  * @return None
  */
void jy6311_play_stop(uint8_t i2c_addr)
{
    JY6311_LOG_I("--->%s\n", __FUNCTION__);

    /* DAC analog config */
    __JY6311_DAC_Out_Dis(i2c_addr);
    __JY6311_DAC_DCT_Dis(i2c_addr);

    /* DAC digital config */
    __JY6311_DAC_Dig_Dis(i2c_addr);

    /* DAC Module Clock Disable */
    __JY6311_DAC_ModClkAll_Dis(i2c_addr);

    /* I2S Rx Disable */
    __JY6311_I2S_Rx_Dis(i2c_addr);
}

/**
  * @brief  jy6311 record start
  * @param  i2c_addr jy6311 7bit I2C address
  * @return None
  */
void jy6311_record_start(uint8_t i2c_addr)
{
    JY6311_LOG_I("--->%s\n", __FUNCTION__);

    /* ADC Module Clock Enable */
    __JY6311_ADC_Path_Clk_En(i2c_addr);
    __JY6311_ADC_Analog_Clk_En(i2c_addr);
    __JY6311_ADC_Dig_Clk_En(i2c_addr);

    /* ADC analog config */
    //__JY6311_ADC_Channel_Set(i2c_addr, 0);
    __JY6311_ADC_SDM_Conv_Start(i2c_addr);
    __JY6311_ADC_SDM_En(i2c_addr);
    __JY6311_ADC_PGA_En(i2c_addr);

    /* DAC digital config */
    //__JY6311_ADC_DelayFunc_Set(i2c_addr, JY6311_ADC_DELAY_FUNC_BYPASS);
    //__JY6311_ADC_HPF_En(i2c_addr);
    //__JY6311_ADC_Dither_En(i2c_addr);
    __JY6311_ADC_Dig_En(i2c_addr);

    /* I2S Tx Enable */
    __JY6311_I2S_Tx_En(i2c_addr);
}

/**
  * @brief  jy6311 record stop
  * @param  i2c_addr jy6311 7bit I2C address
  * @return None
  */
void jy6311_record_stop(uint8_t i2c_addr)
{
    JY6311_LOG_I("--->%s\n", __FUNCTION__);

    /* DAC digital config */
    __JY6311_ADC_Dig_Dis(i2c_addr);

    /* ADC analog config */
    __JY6311_ADC_SDM_Conv_Reset(i2c_addr);
    __JY6311_ADC_SDM_Dis(i2c_addr);
    __JY6311_ADC_PGA_Dis(i2c_addr);

    /* ADC Module Clock Disable */
    __JY6311_ADC_ModClkAll_Dis(i2c_addr);

    /* I2S Tx Disable */
    __JY6311_I2S_Tx_Dis(i2c_addr);
}

/**
  * @brief  jy6311 play volume config
  * @param  i2c_addr jy6311 7bit I2C address
  * @param  vol jy6311 DAC digial volume
  * @param  vol range in [0x0, 0xff], which means [-95dB, 32dB], 0.5dB/Step, while 0 means Mute, 1 means -95dB
  * @return None
  */
void jy6311_play_vol_cfg(uint8_t i2c_addr, uint8_t vol)
{
    JY6311_LOG_I("--->%s\n", __FUNCTION__);

    // DAC Digital volume config
    __JY6311_DAC_DigVol_Set(i2c_addr, vol);
}

/**
  * @brief  jy6311 record gain config
  * @param  i2c_addr jy6311 7bit I2C address
  * @param  gain jy6311 ADC analog gain
  * @return None
  */
void jy6311_record_gain_cfg(uint8_t i2c_addr, JY6311_ADC_PGAGainETypeDef gain)
{
    JY6311_LOG_I("--->%s\n", __FUNCTION__);

    // ADC PGA gain config
    __JY6311_ADC_PGAGain_Set(i2c_addr, gain);
}

/**
  * @}
  */


/** @defgroup JY6311_Exported_Functions_Group4 JY6311 Common Config Functions
  * @brief    JY6311 Common Config Functions
  * @{
  */

/**
  * @brief  jy6311 i2s config
  * @param  i2c_addr jy6311 7bit I2C address
  * @param  cfg jy6311 i2s config struct pointer
  * @return JY6311_StatusETypeDef
  */
JY6311_StatusETypeDef jy6311_i2s_cfg(uint8_t i2c_addr, JY6311_I2SCfgTypeDef *cfg)
{
    JY6311_LOG_I("--->%s\n", __FUNCTION__);

    // Check cfg pointer to be valid
    if (cfg == NULL) {
        JY6311_LOG_E("I2S config pointer is NULL!\n");
        return JY6311_INVALID;
    }

    // Check slot numbers not to be 0
    if (!cfg->slot_nums) {
        JY6311_LOG_E("I2S config slot_nums is 0!\n");
        return JY6311_INVALID;
    }

    // Check slot numbers to be even in I2S/LJ mode
    if (cfg->fmt < JY6311_I2S_FMT_PCM_A && (cfg->slot_nums % 2)) {
        JY6311_LOG_E("I2S config slot_nums must be even in I2S/LJ format!\n");
        return JY6311_INVALID;
    }

    // I2S Global Disable first before config
    __JY6311_I2S_Global_Dis(i2c_addr);

    // I2S Config
    __JY6311_I2S_Role_Set(i2c_addr, cfg->role);
    __JY6311_I2S_LRCK_Period_Set(i2c_addr, cfg->slot_nums * cfg->slot_width / (cfg->fmt < JY6311_I2S_FMT_PCM_A ? 2 : 1));
    JY6311_FUNC_ALTER(cfg->bclk_invert, __JY6311_I2S_BCLK_Pol_Invert(i2c_addr), __JY6311_I2S_BCLK_Pol_Normal(i2c_addr));
    JY6311_FUNC_ALTER(cfg->lrCk_invert, __JY6311_I2S_LRCK_Pol_Invert(i2c_addr), __JY6311_I2S_LRCK_Pol_Normal(i2c_addr));
    __JY6311_I2S_Format_Set(i2c_addr, cfg->fmt);
    __JY6311_I2S_WordLen_Set(i2c_addr, cfg->word_len);
    __JY6311_I2S_Tx_LastWordState_HiZ(i2c_addr);
    __JY6311_I2S_Tx_IdleState_Zero(i2c_addr);

    //__JY6311_I2S_Tx_SlotNums_Set(cfg->slot_nums);
    //__JY6311_I2S_Rx_SlotNums_Set(cfg->slot_nums);

    // I2S Global Enable
    __JY6311_I2S_Global_En(i2c_addr);

    return JY6311_OK;
}

/**
  * @brief  jy6311 sysclk config
  * @param  i2c_addr jy6311 7bit I2C address
  * @param  cfg jy6311 sysclk config struct pointer
  * @return JY6311_StatusETypeDef
  */
JY6311_StatusETypeDef jy6311_sysclk_cfg(uint8_t i2c_addr, JY6311_SysclkCfgTypeDef *cfg)
{
    #define JY6311_DCLK_DIV_INVALID     (0xff)
    uint8_t adc_smp_idx = 0, dac_smp_idx = 0;
    uint32_t sysclk_freq, i2s_lrck_freq;
    uint16_t adc_dclk_div_real, dac_dclk_div_real;//, pll_out_multi_times;
    uint8_t i, adc_osr_reg = 0, dac_osr_reg = 0, adc_dclk_div_reg = JY6311_DCLK_DIV_INVALID, dac_dclk_div_reg = JY6311_DCLK_DIV_INVALID;

    JY6311_LOG_I("--->%s\n", __FUNCTION__);

    // Check cfg pointer to be valid
    if (cfg == NULL) {
        JY6311_LOG_E("SYSCLK config pointer is NULL!\n");
        return JY6311_INVALID;
    }

    // Check play and record sample rate to be valid
    if (cfg->play_smp_rate == JY6311_USER_SR_NONE && cfg->rec_smp_rate == JY6311_USER_SR_NONE) {
        JY6311_LOG_E("Both play_smp_rate[%d] and rec_smp_rate[%d] are config to JY6311_USER_SR_NONE!\n",
            cfg->play_smp_rate, cfg->rec_smp_rate);
        return JY6311_INVALID;
    }

    // Check play and record sample rate to be the same 48K/44.1K series
    if (cfg->play_smp_rate != JY6311_USER_SR_NONE && cfg->rec_smp_rate != JY6311_USER_SR_NONE) {
        if (!(cfg->play_smp_rate % 1000) ^ !(cfg->rec_smp_rate % 1000)) {
            JY6311_LOG_E("play_smp_rate[%d] and rec_smp_rate[%d] isn't the same 48K/44.1K series!\n",
                cfg->play_smp_rate, cfg->rec_smp_rate);
            return JY6311_INVALID;
        }
    }

    // SYSCLK Disable first before config
    __JY6311_SYSCLK_Dis(i2c_addr);

    // Calc DAC DCLK Div without PLL if possible
    if (cfg->play_smp_rate != JY6311_USER_SR_NONE) {
        // Decide DAC OSR
        dac_smp_idx = jy6311_smpRateToRegOrIdx(cfg->play_smp_rate, false);
        dac_osr_reg = jy6311_smp_osr_pll_out[JY6311_SMP_OSR_DAC][cfg->performance][dac_smp_idx].osr;

        if (!(cfg->extclk_freq % cfg->play_smp_rate)) {
            uint32_t dac_multi_times = cfg->extclk_freq / cfg->play_smp_rate;
            if (!(dac_multi_times % jy6311_adda_osr[dac_osr_reg])) {
                dac_dclk_div_real = dac_multi_times / jy6311_adda_osr[dac_osr_reg];
                for (i = 0; i < JY6311_ARRAY_SIZE(jy6311_adda_dclk_div); i++) {
                    if (jy6311_adda_dclk_div[i] == dac_dclk_div_real) {
                        dac_dclk_div_reg = i;
                        break;
                    }
                }
            }
        }
    }

    // Calc ADC DCLK Div without PLL if possible
    if (cfg->rec_smp_rate != JY6311_USER_SR_NONE) {
        // Decide ADC OSR
        adc_smp_idx = jy6311_smpRateToRegOrIdx(cfg->rec_smp_rate, false);
        adc_osr_reg = jy6311_smp_osr_pll_out[JY6311_SMP_OSR_ADC][cfg->performance][adc_smp_idx].osr;

        if (!(cfg->extclk_freq % cfg->rec_smp_rate)) {
            uint32_t adc_multi_times = cfg->extclk_freq / cfg->rec_smp_rate;
            if (!(adc_multi_times % jy6311_adda_osr[adc_osr_reg])) {
                adc_dclk_div_real = adc_multi_times / jy6311_adda_osr[adc_osr_reg];
                for (i = 0; i < JY6311_ARRAY_SIZE(jy6311_adda_dclk_div); i++) {
                    if (jy6311_adda_dclk_div[i] == adc_dclk_div_real) {
                        adc_dclk_div_reg = i;
                        break;
                    }
                }
            }
        }
    }

    if (cfg->rec_smp_rate == JY6311_USER_SR_NONE) {             /* Only use DAC */
        if (dac_dclk_div_reg != JY6311_DCLK_DIV_INVALID) {      // Don't need to use PLL
            // SYSCLK Source select MCLK/BCLK
            sysclk_freq = cfg->extclk_freq;
            __JY6311_SYSCLK_Src_Set(i2c_addr, cfg->extclk_src);
            JY6311_LOG_D("SYSCLK Source select [%s] @%" PRIu32 "Hz.\n", cfg->extclk_src == JY6311_EXT_CLK_SRC_MCLK ? "MCLK" : "BCLK",
                cfg->extclk_freq);
        } else {                                                // Need to use PLL
            // Get PLL out frequence
            sysclk_freq = jy6311_smp_osr_pll_out[JY6311_SMP_OSR_DAC][cfg->performance][dac_smp_idx].pll_out;

            // Calc DAC DCLK Div
            dac_dclk_div_real = sysclk_freq / (cfg->play_smp_rate * jy6311_adda_osr[dac_osr_reg]);
            for (i = 0; i < JY6311_ARRAY_SIZE(jy6311_adda_dclk_div); i++) {
                if (jy6311_adda_dclk_div[i] == dac_dclk_div_real) {
                    dac_dclk_div_reg = i;
                    break;
                }
            }
            if (i == JY6311_ARRAY_SIZE(jy6311_adda_dclk_div)) {
                JY6311_LOG_E("DAC DCLK divide don't support [%d]!\n", dac_dclk_div_real);
                return JY6311_ERROR;
            }

            // PLL Config
            if (jy6311_pllCfg(i2c_addr, (JY6311_PLLCLKSrcETypeDef)cfg->extclk_src, cfg->extclk_freq, sysclk_freq) != JY6311_OK) {
                JY6311_LOG_E("PLL config error!\n");
                return JY6311_ERROR;
            }

            // SYSCLK Source select PLL
            __JY6311_SYSCLK_Src_Set(i2c_addr, JY6311_SYSCLK_SRC_PLL);
            JY6311_LOG_D("SYSCLK Source select [PLL] @%" PRIu32 "Hz.\n", sysclk_freq);
        }

        // DAC SR/OSR/DCLK_DIV Config
        __JY6311_DAC_SampleRate_Set(i2c_addr, jy6311_smpRateToRegOrIdx(cfg->play_smp_rate, true));
        __JY6311_DAC_OverSampleRatio_Set(i2c_addr, dac_osr_reg);
        __JY6311_DAC_DCLK_Div_Set(i2c_addr, dac_dclk_div_reg);
        i2s_lrck_freq = cfg->play_smp_rate;
    } else if (cfg->play_smp_rate == JY6311_USER_SR_NONE) {     /* Only use ADC */
        if (adc_dclk_div_reg != JY6311_DCLK_DIV_INVALID) {      // Don't need to use PLL
            // SYSCLK Source select MCLK/BCLK
            sysclk_freq = cfg->extclk_freq;
            __JY6311_SYSCLK_Src_Set(i2c_addr, cfg->extclk_src);
            JY6311_LOG_D("SYSCLK Source select [%s] @%" PRIu32 "Hz.\n", cfg->extclk_src == JY6311_EXT_CLK_SRC_MCLK ? "MCLK" : "BCLK",
                cfg->extclk_freq);
        } else {                                                // Need to use PLL
            // Get PLL out frequence
            sysclk_freq = jy6311_smp_osr_pll_out[JY6311_SMP_OSR_ADC][cfg->performance][adc_smp_idx].pll_out;

            // Calc ADC DCLK Div
            adc_dclk_div_real = sysclk_freq / (cfg->rec_smp_rate * jy6311_adda_osr[adc_osr_reg]);
            for (i = 0; i < JY6311_ARRAY_SIZE(jy6311_adda_dclk_div); i++) {
                if (jy6311_adda_dclk_div[i] == adc_dclk_div_real) {
                    adc_dclk_div_reg = i;
                    break;
                }
            }
            if (i == JY6311_ARRAY_SIZE(jy6311_adda_dclk_div)) {
                JY6311_LOG_E("ADC DCLK divide don't support [%d]!\n", adc_dclk_div_real);
                return JY6311_ERROR;
            }

            // PLL Config
            if (jy6311_pllCfg(i2c_addr, (JY6311_PLLCLKSrcETypeDef)cfg->extclk_src, cfg->extclk_freq, sysclk_freq) != JY6311_OK) {
                JY6311_LOG_E("PLL config error!\n");
                return JY6311_ERROR;
            }

            // SYSCLK Source select PLL
            __JY6311_SYSCLK_Src_Set(i2c_addr, JY6311_SYSCLK_SRC_PLL);
            JY6311_LOG_D("SYSCLK Source select [PLL] @%" PRIu32 "Hz.\n", sysclk_freq);
        }

        // ADC SR/OSR/DCLK_DIV Config
        __JY6311_ADC_SampleRate_Set(i2c_addr, jy6311_smpRateToRegOrIdx(cfg->rec_smp_rate, true));
        __JY6311_ADC_OverSampleRatio_Set(i2c_addr, adc_osr_reg);
        __JY6311_ADC_DCLK_Div_Set(i2c_addr, adc_dclk_div_reg);
        i2s_lrck_freq = cfg->rec_smp_rate;
    } else {                                                        /* Both use ADC/DAC */
        if (dac_dclk_div_reg != JY6311_DCLK_DIV_INVALID && adc_dclk_div_reg != JY6311_DCLK_DIV_INVALID) {// Don't need to use PLL
            // SYSCLK Source select MCLK/BCLK
            sysclk_freq = cfg->extclk_freq;
            __JY6311_SYSCLK_Src_Set(i2c_addr, cfg->extclk_src);
            JY6311_LOG_D("SYSCLK Source select [%s] @%" PRIu32 "Hz.\n", cfg->extclk_src == JY6311_EXT_CLK_SRC_MCLK ? "MCLK" : "BCLK",
                cfg->extclk_freq);
        } else {                                                                                         // Need to use PLL
            // Get PLL out frequence
            sysclk_freq = JY6311_MAX(jy6311_smp_osr_pll_out[JY6311_SMP_OSR_DAC][cfg->performance][dac_smp_idx].pll_out,
                                     jy6311_smp_osr_pll_out[JY6311_SMP_OSR_ADC][cfg->performance][adc_smp_idx].pll_out);

            // Calc DAC DCLK Div
            dac_dclk_div_real = sysclk_freq / (cfg->play_smp_rate * jy6311_adda_osr[dac_osr_reg]);
            for (i = 0; i < JY6311_ARRAY_SIZE(jy6311_adda_dclk_div); i++) {
                if (jy6311_adda_dclk_div[i] == dac_dclk_div_real) {
                    dac_dclk_div_reg = i;
                    break;
                }
            }
            if (i == JY6311_ARRAY_SIZE(jy6311_adda_dclk_div)) {
                JY6311_LOG_E("DAC DCLK divide don't support [%d]!\n", dac_dclk_div_real);
                return JY6311_ERROR;
            }

            // Calc ADC DCLK Div
            adc_dclk_div_real = sysclk_freq / (cfg->rec_smp_rate * jy6311_adda_osr[adc_osr_reg]);
            for (i = 0; i < JY6311_ARRAY_SIZE(jy6311_adda_dclk_div); i++) {
                if (jy6311_adda_dclk_div[i] == adc_dclk_div_real) {
                    adc_dclk_div_reg = i;
                    break;
                }
            }
            if (i == JY6311_ARRAY_SIZE(jy6311_adda_dclk_div)) {
                JY6311_LOG_E("ADC DCLK divide don't support [%d]!\n", adc_dclk_div_real);
                return JY6311_ERROR;
            }

            // PLL Config
            if (jy6311_pllCfg(i2c_addr, (JY6311_PLLCLKSrcETypeDef)cfg->extclk_src, cfg->extclk_freq, sysclk_freq) != JY6311_OK) {
                JY6311_LOG_E("PLL config error!\n");
                return JY6311_ERROR;
            }

            // SYSCLK Source select PLL
            __JY6311_SYSCLK_Src_Set(i2c_addr, JY6311_SYSCLK_SRC_PLL);
            JY6311_LOG_D("SYSCLK Source select [PLL] @%" PRIu32 "Hz.\n", sysclk_freq);
        }

        // DAC SR/OSR/DCLK_DIV Config
        __JY6311_DAC_SampleRate_Set(i2c_addr, jy6311_smpRateToRegOrIdx(cfg->play_smp_rate, true));
        __JY6311_DAC_OverSampleRatio_Set(i2c_addr, dac_osr_reg);
        __JY6311_DAC_DCLK_Div_Set(i2c_addr, dac_dclk_div_reg);

        // ADC SR/OSR/DCLK_DIV Config
        __JY6311_ADC_SampleRate_Set(i2c_addr, jy6311_smpRateToRegOrIdx(cfg->rec_smp_rate, true));
        __JY6311_ADC_OverSampleRatio_Set(i2c_addr, adc_osr_reg);
        __JY6311_ADC_DCLK_Div_Set(i2c_addr, adc_dclk_div_reg);

        i2s_lrck_freq = JY6311_MAX(cfg->play_smp_rate, cfg->rec_smp_rate);
    }

    // I2S BCLK Divide config in Master mode if need
    if (cfg->i2s_lrck_period) {
        if (sysclk_freq % (i2s_lrck_freq * cfg->i2s_lrck_period)) {
            JY6311_LOG_W("I2S BCLK can't divide for SYSCLK in integer multiples, please check it again!\n");
        }

        // Calc I2S BCLK Div and set
        uint8_t i2s_bclk_div_real = sysclk_freq / (i2s_lrck_freq * cfg->i2s_lrck_period);
        for (i = 0; i < JY6311_ARRAY_SIZE(jy6311_adda_dclk_div); i++) {
            if (jy6311_adda_dclk_div[i] == i2s_bclk_div_real) {
                __JY6311_I2S_BCLK_Div_Set(i2c_addr, i);
                JY6311_LOG_W("I2S BCLK divide set to [%d].\n", i2s_bclk_div_real);
                break;
            }
        }
        if (i == JY6311_ARRAY_SIZE(jy6311_adda_dclk_div)) {
            JY6311_LOG_W("I2S BCLK divide don't support [%d], set failed!\n", i2s_bclk_div_real);
        }
    }

    // SYSCLK Enable
    __JY6311_SYSCLK_En(i2c_addr);

    return JY6311_OK;
}

/**
  * @brief  jy6311 pdm interface config
  * @param  i2c_addr jy6311 7bit I2C address
  * @param  en pdm enable config
  * @param  cfg jy6311 pdm config struct pointer
  * @return JY6311_StatusETypeDef
  */
JY6311_StatusETypeDef jy6311_pdm_cfg(uint8_t i2c_addr, bool en, JY6311_PDMCfgTypeDef *cfg)
{
    JY6311_LOG_I("--->%s\n", __FUNCTION__);

    // Check cfg pointer to be valid
    if (en && cfg == NULL) {
        JY6311_LOG_E("PDM config pointer is NULL!\n");
        return JY6311_INVALID;
    }

    if (en) {
        // PDM Interface Enable config
        JY6311_FUNC_ALTER(cfg->timing_invert, __JY6311_PDM_Timing_Invert(i2c_addr), __JY6311_PDM_Timing_Normal(i2c_addr));
        if (cfg->clk_io == JY6311_PDMCLK_IO_MCLK) {
            __JY6311_MCLK_IO_PinFunc_Set(i2c_addr, JY6311_MCLK_IO_PIN_FUNC_OUTPUT_PDMCLK);
        } else if (cfg->clk_io == JY6311_PDMCLK_IO_DEV_ID) {
            __JY6311_DeviceID_PinFunc_OutPDMCLK(i2c_addr);
        } else {
            JY6311_LOG_E("PDMCLK IO config error: %d\n", cfg->clk_io);
            return JY6311_INVALID;
        }
        __JY6311_DMICDAT_Pad_En(i2c_addr);
        __JY6311_PDM_En(i2c_addr);
    } else {
        // PDM Interface Disable config
        __JY6311_MCLK_IO_PinFunc_Set(i2c_addr, JY6311_MCLK_IO_PIN_FUNC_INPUT);
        __JY6311_DeviceID_PinFunc_Input(i2c_addr);
        __JY6311_DMICDAT_Pad_Dis(i2c_addr);
        __JY6311_PDM_Dis(i2c_addr);
    }

    return JY6311_OK;
}

/**
  * @}
  */


/** @defgroup JY6311_Exported_Functions_Group5 JY6311 EQ and DRC Config Functions
  * @brief    JY6311 EQ and DRC Config Functions
  * @{
  */

/**
  * @brief  jy6311 adc eq config
  * @param  i2c_addr jy6311 7bit I2C address
  * @param  en adb eq enable config
  * @param  array_cfg[] adc eq coff config array pointer
  * @param  array_size adc eq coff config array size in int32_t unit
  * @return JY6311_StatusETypeDef
  */
JY6311_StatusETypeDef jy6311_adc_eq_cfg(uint8_t i2c_addr, bool en, int32_t array_cfg[], uint8_t array_size)
{
    JY6311_LOG_I("--->%s\n", __FUNCTION__);

    if (!en) {
        // ADC EQ Band 1 Disable
        __JY6311_ADC_EQ1_Dis(i2c_addr);
        return JY6311_OK;
    }

    // Check array cfg pointer and array size to be valid
    if (array_cfg == NULL || array_size != JY6311_EQ_BAND_COFF_NUMS) {
        JY6311_LOG_E("ADC EQ config array pointer is NULL or array size isn't equal to %d!\n", JY6311_EQ_BAND_COFF_NUMS);
        return JY6311_INVALID;
    }

    // b0 coff config
    __JY6311_ADC_EQ1_Coff_b0h_Set(i2c_addr, (uint8_t)(array_cfg[0] >> 16));
    __JY6311_ADC_EQ1_Coff_b0m_Set(i2c_addr, (uint8_t)(array_cfg[0] >> 8));
    __JY6311_ADC_EQ1_Coff_b0l_Set(i2c_addr, (uint8_t)array_cfg[0]);

    // b1 coff config
    __JY6311_ADC_EQ1_Coff_b1h_Set(i2c_addr, (uint8_t)(array_cfg[1] >> 16));
    __JY6311_ADC_EQ1_Coff_b1m_Set(i2c_addr, (uint8_t)(array_cfg[1] >> 8));
    __JY6311_ADC_EQ1_Coff_b1l_Set(i2c_addr, (uint8_t)array_cfg[1]);

    // b2 coff config
    __JY6311_ADC_EQ1_Coff_b2h_Set(i2c_addr, (uint8_t)(array_cfg[2] >> 16));
    __JY6311_ADC_EQ1_Coff_b2m_Set(i2c_addr, (uint8_t)(array_cfg[2] >> 8));
    __JY6311_ADC_EQ1_Coff_b2l_Set(i2c_addr, (uint8_t)array_cfg[2]);

    // a1 coff config
    __JY6311_ADC_EQ1_Coff_a1h_Set(i2c_addr, (uint8_t)(array_cfg[3] >> 16));
    __JY6311_ADC_EQ1_Coff_a1m_Set(i2c_addr, (uint8_t)(array_cfg[3] >> 8));
    __JY6311_ADC_EQ1_Coff_a1l_Set(i2c_addr, (uint8_t)array_cfg[3]);

    // a2 coff config
    __JY6311_ADC_EQ1_Coff_a2h_Set(i2c_addr, (uint8_t)(array_cfg[4] >> 16));
    __JY6311_ADC_EQ1_Coff_a2m_Set(i2c_addr, (uint8_t)(array_cfg[4] >> 8));
    __JY6311_ADC_EQ1_Coff_a2l_Set(i2c_addr, (uint8_t)array_cfg[4]);

    // ADC EQ Band 1 Enable
    __JY6311_ADC_EQ1_En(i2c_addr);

    // ADC DSP Clock Enable
    __JY6311_ADC_DSP_Clk_En(i2c_addr);

    return JY6311_OK;
}

/**
  * @brief  jy6311 adc drc config
  * @param  i2c_addr jy6311 7bit I2C address
  * @param  en adc drc enable config
  * @param  array_cfg[] adc drc coff config array pointer
  * @param  array_size adc drc coff config array size in int32_t unit
  * @return JY6311_StatusETypeDef
  */
JY6311_StatusETypeDef jy6311_adc_drc_cfg(uint8_t i2c_addr, bool en, int32_t array_cfg[], uint8_t array_size)
{
    JY6311_LOG_I("--->%s\n", __FUNCTION__);

    if (!en) {
        // ADC DRC Disable
        __JY6311_ADC_DRC_Dis(i2c_addr);
        return JY6311_OK;
    }

    // Check array cfg pointer and array size to be valid
    if (array_cfg == NULL || array_size != JY6311_DRC_COFF_NUMS) {
        JY6311_LOG_E("ADC DRC config array pointer is NULL or array size isn't equal to %d!\n", JY6311_DRC_COFF_NUMS);
        return JY6311_INVALID;
    }

    // ADC DRC Coff config
    JY6311_WRITE_REG(i2c_addr, ADC_PEAK_CTL, (uint8_t)array_cfg[0]);
    JY6311_WRITE_REG(i2c_addr, ADC_ATK_TIME, (uint8_t)array_cfg[1]);
    JY6311_WRITE_REG(i2c_addr, ADC_DRC_THD1, (uint8_t)array_cfg[2]);
    JY6311_WRITE_REG(i2c_addr, ADC_DRC_THD2, (uint8_t)array_cfg[3]);
    JY6311_WRITE_REG(i2c_addr, ADC_DRC_THD3, (uint8_t)array_cfg[4]);
    JY6311_WRITE_REG(i2c_addr, ADC_DRC_THD4, (uint8_t)array_cfg[5]);
    JY6311_WRITE_REG(i2c_addr, ADC_DRC_SLP1, (uint8_t)array_cfg[6]);
    JY6311_WRITE_REG(i2c_addr, ADC_DRC_SLP2, (uint8_t)array_cfg[7]);

    // ADC DRC Enable
    __JY6311_ADC_DRC_En(i2c_addr);

    // ADC DSP Clock Enable
    __JY6311_ADC_DSP_Clk_En(i2c_addr);

    return JY6311_OK;
}

/**
  * @brief  jy6311 dac eq config
  * @param  i2c_addr jy6311 7bit I2C address
  * @param  en dac eq enable config
  * @param  array_cfg[] dac eq coff config array pointer
  * @param  array_size dac eq coff config array size in int32_t unit
  * @return JY6311_StatusETypeDef
  */
JY6311_StatusETypeDef jy6311_dac_eq_cfg(uint8_t i2c_addr, bool en, int32_t array_cfg[], uint8_t array_size)
{
    JY6311_LOG_I("--->%s\n", __FUNCTION__);

    if (!en) {
        // DAC EQ Band 1/2/3 all Disable
        __JY6311_DAC_EQ1_Dis(i2c_addr);
        __JY6311_DAC_EQ2_Dis(i2c_addr);
        __JY6311_DAC_EQ3_Dis(i2c_addr);
        return JY6311_OK;
    }

    // Check array cfg pointer and array size to be valid
    if (array_cfg == NULL || !array_size || (array_size % JY6311_EQ_BAND_COFF_NUMS) ||
        array_size > JY6311_EQ_BAND_COFF_NUMS * JY6311_DAC_EQ_BAND_NUMS) {
        JY6311_LOG_E("DAC EQ config array pointer is NULL or array size is invalid!\n");
        return JY6311_INVALID;
    }

    // DAC EQ Band 1 Enable and coff config
    if (array_size >= JY6311_EQ_BAND_COFF_NUMS) {
        JY6311_DAC_EQx_BAND_COFF_CFG(i2c_addr, 1, array_cfg);
        __JY6311_DAC_EQ1_En(i2c_addr);
    }

    // DAC EQ Band 2 Enable and coff config
    if (array_size >= JY6311_EQ_BAND_COFF_NUMS * 2) {
        JY6311_DAC_EQx_BAND_COFF_CFG(i2c_addr, 2, array_cfg + JY6311_EQ_BAND_COFF_NUMS * 1);
        __JY6311_DAC_EQ2_En(i2c_addr);
    }

    // DAC EQ Band 3 Enable and coff config
    if (array_size >= JY6311_EQ_BAND_COFF_NUMS * 3) {
        JY6311_DAC_EQx_BAND_COFF_CFG(i2c_addr, 3, array_cfg + JY6311_EQ_BAND_COFF_NUMS * 2);
        __JY6311_DAC_EQ3_En(i2c_addr);
    }

    // DAC DSP Clock Enable
    __JY6311_DAC_DSP_Clk_En(i2c_addr);

    return JY6311_OK;
}

/**
  * @brief  jy6311 dac drc config
  * @param  i2c_addr jy6311 7bit I2C address
  * @param  en dac drc enable config
  * @param  array_cfg[] dac drc coff config array pointer
  * @param  array_size dac drc coff config array size in int32_t unit
  * @return JY6311_StatusETypeDef
  */
JY6311_StatusETypeDef jy6311_dac_drc_cfg(uint8_t i2c_addr, bool en, int32_t array_cfg[], uint8_t array_size)
{
    JY6311_LOG_I("--->%s\n", __FUNCTION__);

    if (!en) {
        // DAC DRC Disable
        __JY6311_DAC_DRC_Dis(i2c_addr);
        return JY6311_OK;
    }

    // Check array cfg pointer and array size to be valid
    if (array_cfg == NULL || array_size != JY6311_DRC_COFF_NUMS) {
        JY6311_LOG_E("DAC DRC config array pointer is NULL or array size isn't equal to %d!\n", JY6311_DRC_COFF_NUMS);
        return JY6311_INVALID;
    }

    // DAC DRC Coff config
    JY6311_WRITE_REG(i2c_addr, DAC_PEAK_CTL, (uint8_t)array_cfg[0]);
    JY6311_WRITE_REG(i2c_addr, DAC_ATK_TIME, (uint8_t)array_cfg[1]);
    JY6311_WRITE_REG(i2c_addr, DAC_DRC_THD1, (uint8_t)array_cfg[2]);
    JY6311_WRITE_REG(i2c_addr, DAC_DRC_THD2, (uint8_t)array_cfg[3]);
    JY6311_WRITE_REG(i2c_addr, DAC_DRC_THD3, (uint8_t)array_cfg[4]);
    JY6311_WRITE_REG(i2c_addr, DAC_DRC_THD4, (uint8_t)array_cfg[5]);
    JY6311_WRITE_REG(i2c_addr, DAC_DRC_SLP1, (uint8_t)array_cfg[6]);
    JY6311_WRITE_REG(i2c_addr, DAC_DRC_SLP2, (uint8_t)array_cfg[7]);

    // DAC DRC Enable
    __JY6311_DAC_DRC_En(i2c_addr);

    // DAC DSP Clock Enable
    __JY6311_DAC_DSP_Clk_En(i2c_addr);

    return JY6311_OK;
}

/**
  * @}
  */


/** @defgroup JY6311_Exported_Functions_Group6 JY6311 Debug Functions
  * @brief    JY6311 Debug Functions
  * @{
  */

/**
  * @brief  jy6311 all register read for debug
  * @param  i2c_addr jy6311 7bit I2C address
  * @print_ print_all print all registers value or only print the registers value which not equal to default
  * @return None
  */
void jy6311_all_regs_read(uint8_t i2c_addr, bool print_all)
{
    uint8_t reg_val;

    for (uint16_t i = 0; i < JY6311_ARRAY_SIZE(jy6311_reg_default_val); i++) {
        reg_val = JY6311_I2C_READ_BYTE(i2c_addr, jy6311_reg_default_val[i].reg);
        if (print_all || reg_val != jy6311_reg_default_val[i].val) {
            JY6311_LOG_D("[0x%02x]: 0x%02x\n", jy6311_reg_default_val[i].reg, reg_val);
        }
    }
}

/**
  * @}
  */

/**
  * @}
  */


/* Private Functions ---------------------------------------------------------*/
/** @defgroup JY6311_Private_Functions JY6311 Private Functions
  * @brief    JY6311 Private Functions
  * @{
  */

/** @defgroup JY6311_Private_Functions_Group1 JY6311 Private Misc Functions
  * @brief    JY6311 Private Misc Functions
  * @{
  */

/**
  * @brief  jy6311 sample rate real value to register value
  * @param  rate sample rate real
  * @param  reg return register flag, if true return register value, else return jy6311_smp_osr_pll_out array index
  * @return register value
  */
static uint8_t jy6311_smpRateToRegOrIdx(uint32_t rate, bool reg)
{
    switch (rate) {
        case 8000:
            return JY6311_ADDA_SR_8K;

        case 12000:
            return JY6311_ADDA_SR_12K;

        case 16000:
            return JY6311_ADDA_SR_16K;

        case 24000:
            return JY6311_ADDA_SR_24K;

        case 32000:
            return JY6311_ADDA_SR_32K;

        case 48000:
            return JY6311_ADDA_SR_48K;

        case 96000:
            return JY6311_ADDA_SR_96K;

        case 192000:
            return JY6311_ADDA_SR_192K;

        case 11025:
            return reg ? JY6311_ADDA_SR_11025 : JY6311_ADDA_SR_192K + 1;

        case 22050:
            return reg ? JY6311_ADDA_SR_22050 : JY6311_ADDA_SR_192K + 2;

        case 44100:
            return reg ? JY6311_ADDA_SR_44100 : JY6311_ADDA_SR_192K + 3;

        default:
            JY6311_LOG_E("jy6311 don't support sample rate real value: %" PRIu32 "!\n", rate);
            return 0;
    }
}

/**
  * @brief  jy6311 pll config
  * @param  i2c_addr jy6311 7bit I2C address
  * @param  clk_src PLLCLK source
  * @param  freq_in PLLCLK input frequence
  * @param  freq_out PLLCLK output frequence
  * @return JY6311_StatusETypeDef
  */
static JY6311_StatusETypeDef jy6311_pllCfg(uint8_t i2c_addr, JY6311_PLLCLKSrcETypeDef clk_src,
                                                  uint32_t freq_in, uint32_t freq_out)
{
    uint8_t p = 1, m = 2;
    uint16_t i, n = 1;

    // Check PLL frequence out to be valid
    if (!freq_out) {
        JY6311_LOG_E("PLLCLK frequence out [%" PRIu32 "] config error!\n", freq_out);
        return JY6311_INVALID;
    }

    // Check PLL frequence in to be valid
    if (freq_in < 512000 || freq_in > 24576000) {
		JY6311_LOG_E("PLLCLK source input freq only support [512K,24M], while now %" PRIu32 "!\n", freq_in);
        return JY6311_INVALID;
    }

    // FOUT = FIN * N /(M*P), N[1, 256], P[1, 32], M[2, 16] and must be even
    for (i = 0; i < JY6311_ARRAY_SIZE(jy6311_pll_div); i++) {
        if (jy6311_pll_div[i].freq_in == freq_in && jy6311_pll_div[i].freq_out == freq_out) {
            n = jy6311_pll_div[i].n;
            p = jy6311_pll_div[i].p;
            m = jy6311_pll_div[i].m;
            JY6311_LOG_D("PLLCLK freq_in match:%" PRIu32 ", freq_out:%" PRIu32 ".\n", freq_in, freq_out);
            break;
        }
    }
    if (i == JY6311_ARRAY_SIZE(jy6311_pll_div)) {
		JY6311_LOG_E("Don't match PLLCLK freq_in[%" PRIu32 "] and freq_out[%" PRIu32 "] table, "
            "please add a configuration item in jy6311_pll_div array for this!\n", freq_in, freq_out);
		return JY6311_FAILED;
    }

    // PLLCLK Disable first before config
    __JY6311_PLLCLK_Dis(i2c_addr);
    __JY6311_PLLLDO_Dis(i2c_addr);
    __JY6311_PLL_Dis(i2c_addr);

    // PLL Clock Source Select
    __JY6311_PLLCLK_Src_Set(i2c_addr, clk_src);
    JY6311_LOG_D("PLLCLK input source select %s.\n", clk_src == JY6311_PLLCLK_SRC_MCLK ? "MCLK" : "BCLK");

    // PLL DIV param Config
    __JY6311_PLL_FeedbackDiv_Set(i2c_addr, n);
    __JY6311_PLL_PreDiv_Set(i2c_addr, p);
    __JY6311_PLL_PostDiv_Set(i2c_addr, m);

    // PLLCLK Enable
    __JY6311_PLLCLK_En(i2c_addr);
    __JY6311_PLLLDO_En(i2c_addr);
    __JY6311_PLL_En(i2c_addr);

    return JY6311_OK;
}

/**
  * @}
  */


/** @defgroup JY6311_Private_Functions_Group2 JY6311 ESP32 Interface Functions
  * @brief    JY6311 ESP32 Interface Functions
  * @{
  */

static int jy6311_set_mute(const audio_codec_if_t *h, bool mute)
{
    audio_codec_jy6311_t *codec = (audio_codec_jy6311_t *) h;

    JY6311_LOG_I("--->%s\n", __FUNCTION__);

    if (codec == NULL || codec->is_open == false) {
        return ESP_CODEC_DEV_INVALID_ARG;
    }

    JY6311_FUNC_ALTER(mute, __JY6311_DAC_OutSrc_DAC_Dis(JY6311_CODEC_DEFAULT_ADDR),
        __JY6311_DAC_OutSrc_DAC_En(JY6311_CODEC_DEFAULT_ADDR));
    JY6311_LOG_D("mute %s\n.", mute ? "enabled" : "disabled");

    return ESP_CODEC_DEV_OK;
}

static int jy6311_set_vol(const audio_codec_if_t *h, float db_value)
{
    int reg;
    audio_codec_jy6311_t *codec = (audio_codec_jy6311_t *) h;

    JY6311_LOG_I("--->%s\n", __FUNCTION__);

    if (codec == NULL) {
        return ESP_CODEC_DEV_INVALID_ARG;
    }
    if (codec->is_open == false) {
        return ESP_CODEC_DEV_WRONG_STATE;
    }
    JY6311_LOG_D("user set db_value: %f, codec->hw_gain: %f\n", db_value, codec->hw_gain);

    //db_value -= codec->hw_gain;
    reg = esp_codec_dev_vol_calc_reg(&vol_range, db_value);
    JY6311_LOG_D("Set volume reg:%x db:%d\n", reg, (int)db_value);
    jy6311_play_vol_cfg(JY6311_CODEC_DEFAULT_ADDR, reg);

    return ESP_CODEC_DEV_OK;
}

static int jy6311_set_mic_gain(const audio_codec_if_t *h, float db)
{
    audio_codec_jy6311_t *codec = (audio_codec_jy6311_t *) h;
    JY6311_ADC_PGAGainETypeDef gain_db = JY6311_ADC_PGA_GAIN_0dB;

    JY6311_LOG_I("--->%s\n", __FUNCTION__);

    if (codec == NULL) {
        return ESP_CODEC_DEV_INVALID_ARG;
    }
    if (codec->is_open == false) {
        return ESP_CODEC_DEV_WRONG_STATE;
    }

#if 1
    db += 3;
    JY6311_LOG_D("add 3db to %fdB\n", db);
#endif

    if (db < 6) {
        gain_db = JY6311_ADC_PGA_GAIN_0dB;
    } else if (db < 9) {
        gain_db = JY6311_ADC_PGA_GAIN_6dB;
    } else if (db < 12) {
        gain_db = JY6311_ADC_PGA_GAIN_9dB;
    } else if (db < 15) {
        gain_db = JY6311_ADC_PGA_GAIN_12dB;
    } else if (db < 18) {
        gain_db = JY6311_ADC_PGA_GAIN_15dB;
    } else if (db < 21) {
        gain_db = JY6311_ADC_PGA_GAIN_18dB;
    } else if (db < 24) {
        gain_db = JY6311_ADC_PGA_GAIN_21dB;
    } else if (db < 27) {
        gain_db = JY6311_ADC_PGA_GAIN_24dB;
    } else if (db < 30) {
        gain_db = JY6311_ADC_PGA_GAIN_27dB;
    } else if (db < 33) {
        gain_db = JY6311_ADC_PGA_GAIN_30dB;
    } else if (db < 36) {
        gain_db = JY6311_ADC_PGA_GAIN_33dB;
    } else if (db < 39) {
        gain_db = JY6311_ADC_PGA_GAIN_36dB;
    } else if (db < 42) {
        gain_db = JY6311_ADC_PGA_GAIN_39dB;
    } else {
        gain_db = JY6311_ADC_PGA_GAIN_42dB;
    }

    JY6311_LOG_D("set mic gain: %fdB.\n", db);
    jy6311_record_gain_cfg(JY6311_CODEC_DEFAULT_ADDR, gain_db);

    return ESP_CODEC_DEV_OK;
}

static void jy6311_pa_power(audio_codec_jy6311_t *codec, es_pa_setting_t pa_setting)
{
    int16_t pa_pin = codec->cfg.pa_pin;

    JY6311_LOG_I("--->%s\n", __FUNCTION__);

    if (pa_pin == -1 || codec->cfg.gpio_if == NULL) {
        return;
    }

    if (pa_setting & ES_PA_SETUP) {
        codec->cfg.gpio_if->setup(pa_pin, AUDIO_GPIO_DIR_OUT, AUDIO_GPIO_MODE_FLOAT);
    }
    if (pa_setting & ES_PA_ENABLE) {
        codec->cfg.gpio_if->set(pa_pin, codec->cfg.pa_reverted ? false : true);
    }
    if (pa_setting & ES_PA_DISABLE) {
        codec->cfg.gpio_if->set(pa_pin, codec->cfg.pa_reverted ? true : false);
    }
}

static int jy6311_open(const audio_codec_if_t *h, void *cfg, int cfg_size)
{
    audio_codec_jy6311_t *codec = (audio_codec_jy6311_t *) h;
    jy6311_codec_cfg_t *codec_cfg = (jy6311_codec_cfg_t *) cfg;

    JY6311_LOG_I("--->%s\n", __FUNCTION__);

    if (codec == NULL || codec_cfg == NULL || codec_cfg->ctrl_if == NULL || cfg_size != sizeof(jy6311_codec_cfg_t)) {
        return ESP_CODEC_DEV_INVALID_ARG;
    }

    if (codec_cfg->codec_mode == ESP_CODEC_DEV_WORK_MODE_NONE) {
        JY6311_LOG_E("codec_mode is ESP_CODEC_DEV_WORK_MODE_NONE!");
        return ESP_CODEC_DEV_INVALID_ARG;
    }

    if (codec_cfg->codec_mode == ESP_CODEC_DEV_WORK_MODE_LINE) {
        JY6311_LOG_E("The codec jy6311 doesn't support ESP_CODEC_DEV_WORK_MODE_LINE mode");
        return ESP_CODEC_DEV_NOT_SUPPORT;
    }

    memcpy(&codec->cfg, cfg, sizeof(jy6311_codec_cfg_t));
    if (codec->cfg.mclk_div == 0) {
        codec->cfg.mclk_div = MCLK_DEFAULT_DIV;
        JY6311_LOG_D("mclk_div is 0, use default %d.\n", MCLK_DEFAULT_DIV);
    }

    // jy6311 init
    if (codec_cfg->codec_mode == ESP_CODEC_DEV_WORK_MODE_ADC) {
        jy6311_init(JY6311_CODEC_DEFAULT_ADDR, JY6311_INIT_MOD_ADC);
    } else if (codec_cfg->codec_mode == ESP_CODEC_DEV_WORK_MODE_DAC) {
        jy6311_init(JY6311_CODEC_DEFAULT_ADDR, JY6311_INIT_MOD_DAC);

        // jy6311 dac eq/drc config
        jy6311_dac_eq_cfg(JY6311_CODEC_DEFAULT_ADDR, true, (int32_t *)dac_eq_filt_coef, JY6311_ARRAY_SIZE(dac_eq_filt_coef));
        jy6311_dac_drc_cfg(JY6311_CODEC_DEFAULT_ADDR, true, (int32_t *)dac_drc_filt_coef, JY6311_ARRAY_SIZE(dac_drc_filt_coef));
    } else if (codec_cfg->codec_mode == ESP_CODEC_DEV_WORK_MODE_BOTH) {
        jy6311_init(JY6311_CODEC_DEFAULT_ADDR, JY6311_INIT_MOD_ADC_DAC);

        // jy6311 dac eq/drc config
        jy6311_dac_eq_cfg(JY6311_CODEC_DEFAULT_ADDR, true, (int32_t *)dac_eq_filt_coef, JY6311_ARRAY_SIZE(dac_eq_filt_coef));
        jy6311_dac_drc_cfg(JY6311_CODEC_DEFAULT_ADDR, true, (int32_t *)dac_drc_filt_coef, JY6311_ARRAY_SIZE(dac_drc_filt_coef));
    }

    if (codec_cfg->codec_mode & ESP_CODEC_DEV_WORK_MODE_ADC) {
        // jy6311 pdm config
        JY6311_PDMCfgTypeDef pdm_cfg;
        memset((void *)&pdm_cfg, 0, sizeof(pdm_cfg));
        pdm_cfg.timing_invert = false;
        pdm_cfg.clk_io = JY6311_PDMCLK_IO_MCLK;
        jy6311_pdm_cfg(JY6311_CODEC_DEFAULT_ADDR, codec_cfg->digital_mic, &pdm_cfg);

        // jy6311 adc right channel cofnig
        if (!codec_cfg->no_dac_ref) {
            __JY6311_I2S_TxR_MixerSrcDACLOOP_En(JY6311_CODEC_DEFAULT_ADDR);
        }
        __JY6311_I2S_TxR_MixerSrcADCDO_Dis(JY6311_CODEC_DEFAULT_ADDR);
    }

    jy6311_pa_power(codec, ES_PA_SETUP | ES_PA_DISABLE);//ES_PA_ENABLE
    codec->is_open = true;

    return ESP_CODEC_DEV_OK;
}

static int jy6311_close(const audio_codec_if_t *h)
{
    audio_codec_jy6311_t *codec = (audio_codec_jy6311_t *) h;
    JY6311_InitModETypeDef deinit_mod = JY6311_INIT_MOD_NONE;

    JY6311_LOG_I("--->%s\n", __FUNCTION__);

    if (codec == NULL) {
        return ESP_CODEC_DEV_INVALID_ARG;
    }

    if (codec->cfg.codec_mode & ESP_CODEC_DEV_WORK_MODE_ADC) {
        deinit_mod |= JY6311_INIT_MOD_ADC;
    }

    if (codec->cfg.codec_mode & ESP_CODEC_DEV_WORK_MODE_DAC) {
        deinit_mod |= JY6311_INIT_MOD_DAC;

        // jy6311 dac eq/drc config
        jy6311_dac_eq_cfg(JY6311_CODEC_DEFAULT_ADDR, false, NULL, 0);
        jy6311_dac_drc_cfg(JY6311_CODEC_DEFAULT_ADDR, false, NULL, 0);
    }

    if (codec->is_open) {
        jy6311_deinit(JY6311_CODEC_DEFAULT_ADDR, deinit_mod);
        jy6311_pa_power(codec, ES_PA_DISABLE);
        codec->is_open = false;
    }

    return ESP_CODEC_DEV_OK;
}

static int jy6311_set_fs(const audio_codec_if_t *h, esp_codec_dev_sample_info_t *fs)
{
    audio_codec_jy6311_t *codec = (audio_codec_jy6311_t *) h;
    JY6311_I2SCfgTypeDef i2s_cfg;
    JY6311_SysclkCfgTypeDef sysclk_cfg;

    JY6311_LOG_I("--->%s\n", __FUNCTION__);

    if (codec == NULL || codec->is_open == false || fs == NULL) {
        return ESP_CODEC_DEV_INVALID_ARG;
    }

    memset((void *)&sysclk_cfg, 0, sizeof(sysclk_cfg));
    memset((void *)&i2s_cfg, 0, sizeof(i2s_cfg));

    // jy6311 sysclk config
    if (codec->cfg.codec_mode == ESP_CODEC_DEV_WORK_MODE_ADC) {
        sysclk_cfg.rec_smp_rate = (JY6311_UserSmpRateETypeDef)fs->sample_rate;
    } else if (codec->cfg.codec_mode == ESP_CODEC_DEV_WORK_MODE_DAC) {
        sysclk_cfg.play_smp_rate = (JY6311_UserSmpRateETypeDef)fs->sample_rate;
    } else if (codec->cfg.codec_mode == ESP_CODEC_DEV_WORK_MODE_BOTH) {
        sysclk_cfg.rec_smp_rate = (JY6311_UserSmpRateETypeDef)fs->sample_rate;
        sysclk_cfg.play_smp_rate = (JY6311_UserSmpRateETypeDef)fs->sample_rate;
    } else {
        JY6311_LOG_E("The codec jy6311 doesn't support [%d] codec mode", codec->cfg.codec_mode);
        return ESP_CODEC_DEV_INVALID_ARG;
    }

    sysclk_cfg.extclk_freq = fs->sample_rate * codec->cfg.mclk_div;
    sysclk_cfg.extclk_src = codec->cfg.use_mclk ? JY6311_EXT_CLK_SRC_MCLK : JY6311_EXT_CLK_SRC_BCLK;
    sysclk_cfg.performance = JY6311_ADDA_PERF_NORMAL;
    sysclk_cfg.i2s_lrck_period = codec->cfg.master_mode ? I2S_SLOT_WIDTH * I2S_SLOT_NUMS : 0;
    jy6311_sysclk_cfg(JY6311_CODEC_DEFAULT_ADDR, &sysclk_cfg);

    // jy6311 i2c config
    switch (fs->bits_per_sample) {
        case 16:
        default:
            i2s_cfg.word_len = JY6311_I2S_WORD_LEN_16bit;
            break;

        case 20:
            i2s_cfg.word_len = JY6311_I2S_WORD_LEN_20bit;
            break;

        case 24:
            i2s_cfg.word_len = JY6311_I2S_WORD_LEN_24bit;
            break;

        case 32:
            i2s_cfg.word_len = JY6311_I2S_WORD_LEN_32bit;
            break;
    }

    i2s_cfg.bclk_invert = codec->cfg.invert_bclk;
    i2s_cfg.lrCk_invert = codec->cfg.invert_lrck;
    i2s_cfg.slot_nums = I2S_SLOT_NUMS;
    i2s_cfg.slot_width = I2S_SLOT_WIDTH;
    i2s_cfg.role = codec->cfg.master_mode ? JY6311_I2S_ROLE_MASTER : JY6311_I2S_ROLE_SLAVE;
    i2s_cfg.fmt = I2S_FORMAT;
    jy6311_i2s_cfg(JY6311_CODEC_DEFAULT_ADDR, &i2s_cfg);

    return ESP_CODEC_DEV_OK;
}

static int jy6311_enable(const audio_codec_if_t *h, bool enable)
{
    audio_codec_jy6311_t *codec = (audio_codec_jy6311_t *) h;

    JY6311_LOG_I("--->%s\n", __FUNCTION__);

    if (codec == NULL) {
        return ESP_CODEC_DEV_INVALID_ARG;
    }
    if (codec->is_open == false) {
        return ESP_CODEC_DEV_WRONG_STATE;
    }
    if (enable == codec->enabled) {
        JY6311_LOG_D("Codec now has in %s status, return directly\n", enable ? "enabled" : "disabled");
        return ESP_CODEC_DEV_OK;
    }

    if (enable) {
        // jy6311 record start
        if (codec->cfg.codec_mode & ESP_CODEC_DEV_WORK_MODE_ADC) {
            jy6311_record_start(JY6311_CODEC_DEFAULT_ADDR);
        }

        // jy6311 play start
        if (codec->cfg.codec_mode & ESP_CODEC_DEV_WORK_MODE_DAC) {
            jy6311_play_start(JY6311_CODEC_DEFAULT_ADDR);
            esp_codec_dev_sleep(50);    // wait for codec output stable
            jy6311_pa_power(codec, ES_PA_ENABLE);
            esp_codec_dev_sleep(120);   // wait for PA output stable
        }

        jy6311_all_regs_read(JY6311_CODEC_DEFAULT_ADDR, false);

    } else {
        // jy6311 record stop
        if (codec->cfg.codec_mode & ESP_CODEC_DEV_WORK_MODE_ADC) {
            jy6311_record_stop(JY6311_CODEC_DEFAULT_ADDR);
        }

        // jy6311 play stop
        if (codec->cfg.codec_mode & ESP_CODEC_DEV_WORK_MODE_DAC) {
            jy6311_pa_power(codec, ES_PA_DISABLE);
            esp_codec_dev_sleep(10);    // wait for PA close stable
            jy6311_play_stop(JY6311_CODEC_DEFAULT_ADDR);
        }
    }

    codec->enabled = enable;
    JY6311_LOG_D("Codec set to be %s\n", enable ? "enabled" : "disabled");

    return ESP_CODEC_DEV_OK;
}

static int jy6311_set_reg(const audio_codec_if_t *h, int reg, int value)
{
    audio_codec_jy6311_t *codec = (audio_codec_jy6311_t *) h;

    JY6311_LOG_I("--->%s\n", __FUNCTION__);

    if (codec == NULL) {
        return ESP_CODEC_DEV_INVALID_ARG;
    }
    if (codec->is_open == false) {
        return ESP_CODEC_DEV_WRONG_STATE;
    }

    return jy6311_i2c_write_byte(JY6311_CODEC_DEFAULT_ADDR, reg, value);
}

static int jy6311_get_reg(const audio_codec_if_t *h, int reg, int *value)
{
    audio_codec_jy6311_t *codec = (audio_codec_jy6311_t *) h;

    JY6311_LOG_I("--->%s\n", __FUNCTION__);

    if (codec == NULL) {
        return ESP_CODEC_DEV_INVALID_ARG;
    }
    if (codec->is_open == false) {
        return ESP_CODEC_DEV_WRONG_STATE;
    }

    *value = jy6311_i2c_read_byte(JY6311_CODEC_DEFAULT_ADDR, reg);

    return ESP_CODEC_DEV_OK;
}

static void jy6311_dump(const audio_codec_if_t *h)
{
    audio_codec_jy6311_t *codec = (audio_codec_jy6311_t *) h;

    JY6311_LOG_I("--->%s\n", __FUNCTION__);

    if (codec == NULL || codec->is_open == false) {
        return;
    }

    jy6311_all_regs_read(JY6311_CODEC_DEFAULT_ADDR, false);
}

const audio_codec_if_t *jy6311_codec_new(jy6311_codec_cfg_t *codec_cfg)
{
    JY6311_LOG_I("--->%s\n", __FUNCTION__);

    if (codec_cfg == NULL || codec_cfg->ctrl_if == NULL) {
        JY6311_LOG_E("Wrong codec config");
        return NULL;
    }
    if (codec_cfg->ctrl_if->is_open(codec_cfg->ctrl_if) == false) {
        JY6311_LOG_E("Control interface not open yet");
        return NULL;
    }

    audio_codec_jy6311_t *codec = (audio_codec_jy6311_t *) calloc(1, sizeof(audio_codec_jy6311_t));
    if (codec == NULL) {
        CODEC_MEM_CHECK(codec);
        return NULL;
    }

    codec->base.open = jy6311_open;
    codec->base.enable = jy6311_enable;
    codec->base.set_fs = jy6311_set_fs;
    codec->base.set_vol = jy6311_set_vol;
    codec->base.set_mic_gain = jy6311_set_mic_gain;
    codec->base.mute = jy6311_set_mute;
    codec->base.set_reg = jy6311_set_reg;
    codec->base.get_reg = jy6311_get_reg;
    codec->base.dump_reg = jy6311_dump;
    codec->base.close = jy6311_close;
    codec->hw_gain = esp_codec_dev_col_calc_hw_gain(&codec_cfg->hw_gain);
    jy6311_audio_codec = codec;

    do {
        int ret = codec->base.open(&codec->base, codec_cfg, sizeof(jy6311_codec_cfg_t));
        if (ret != 0) {
            JY6311_LOG_E("Open fail");
            break;
        }
        return &codec->base;
    } while (0);

    if (codec) {
        free(codec);
    }

    return NULL;
}

/**
  * @}
  */

/**
  * @}
  */


#endif  /* JY6311_MODULE_ENABLED */


/**
  * @}
  */


/************************* (C) COPYRIGHT Amplore *****END OF FILE***********/
