/**
 *******************************************************************************
 * @file    dev-bmi-cmd.h
 * @brief   简要描述
 *******************************************************************************
 * @attention
 *
 * none
 *
 *******************************************************************************
 * @note
 *
 * none
 *
 *******************************************************************************
 * @author  MekLi
 * @date    2025/11/26
 * @version 1.0
 *******************************************************************************
 */


/* Define to prevent recursive inclusion -----------------------------------------------------------------------------*/

#ifndef H723VG_BMI08X_DEV_BMI_CMD_H
#define H723VG_BMI08X_DEV_BMI_CMD_H



/*-------- 1. includes and imports -----------------------------------------------------------------------------------*/

#include <cstdint>



/*-------- 2. enum and define ----------------------------------------------------------------------------------------*/

enum class Bmi088Chip {
    NONE,
    ACC,
    GYRO,
};

enum class Bmi088AccRange: uint8_t {
    RANGE_3G = 0x00,
    RANGE_6G = 0x01,
    RANGE_12G = 0x02,
    RANGE_24G = 0x03
};

enum class Bmi088AccODR: uint8_t {
    ODR_12_5_HZ = 0x05,
    ODR_25_HZ = 0x06,
    ODR_50_HZ = 0x07,
    ODR_100_HZ = 0x08,
    ODR_200_HZ = 0x09,
    ODR_400_HZ = 0x0A,
    ODR_800_HZ = 0x0B,
    ODR_1600_HZ = 0x0C,
};

enum class Bmi088AccWidth: uint8_t {
    OSR4 = 0x08,
    OSR2 = 0x09,
    NORMAL = 0x0A,
};

enum class Bmi088GyroRange: uint8_t {
    RANGE_2000_DPS = 0x00,
    RANGE_1000_DPS = 0x01,
    RANGE_500_DPS = 0x02,
    RANGE_250_DPS = 0x03,
    RANGE_125_DPS = 0x04,
};

enum class Bmi088GyroWidth: uint8_t {
    ODR_2000HZ_BW_532HZ = 0x00,
    ODR_2000HZ_BW_230HZ = 0x01,
    ODR_1000HZ_BW_116HZ = 0x02,
    ODR_400HZ_BW_47HZ = 0x03,
    ODR_200HZ_BW_64HZ = 0x04,
    ODR_100HZ_BW_32HZ = 0x05,
    ODR_200HZ_BW_23HZ = 0x06,
    ODR_100HZ_BW_12HZ = 0x07,
};




enum class Bmi160Cmd : uint8_t {
    CHIP_ID      = 0x00,
    ERR          = 0x02,
    PMU_STATUS   = 0x03,
    DATA         = 0x04,
    SENSOR_TIME  = 0x18,
    STATUS       = 0x1B,
    INT_STATUS   = 0x1C,
    TEMPERATURE  = 0x1D,
    FIFO_LEN     = 0x22,
    FIFO_DATA    = 0x24,
    ACC_CONF     = 0x40,
    ACC_RANGE    = 0x41,
    GYR_CONF     = 0x42,
    GYR_RANGE    = 0x43,
    MAG_CONF     = 0x44,
    FIFO_DOWNS   = 0x45,
    FIFO_CONFIG  = 0x46,
    MAG_IF       = 0x4B,
    INT_EN       = 0x50,
    INT_OUT_CTRL = 0x53,
    INT_LATCH    = 0x54,
    INT_MAP      = 0x55,
    INT_DATA     = 0x58,
    INT_LOW_HIGH = 0x5A,
    INT_MOTION   = 0x5F,
    INT_TAP      = 0x63,
    INT_ORIENT   = 0x65,
    INT_FLAG     = 0x67,
    FOC_CONF     = 0x69,
    CONF         = 0x6A,
    IF_CONF      = 0x6B,
    PMU_TRIGGER  = 0x6C,
    SELF_TEST    = 0x6D,
    NV_CONF      = 0x70,
    OFFSET       = 0x71,
    STEP_CNT     = 0x78,
    STEP_CONF    = 0x7A,
    CMD          = 0x7E,
};

enum class Bmi088AccRegister : uint8_t {
    ACC_CHIP_ID             = 0x00,
    ACC_ERR_REG             = 0x02,
    ACC_STATUS              = 0x03,
    ACC_DATA                = 0x12,
    SENSORTIME_DATA         = 0x18,
    ACC_INT_STAT_1          = 0x1D,
    TEMPERATURE_SENSOR_DATA = 0x22,
    FIFO_LENGTH             = 0x24,
    FIFO_DATA               = 0x26,
    ACC_CONF                = 0x40,
    ACC_RANGE               = 0x41,
    FIFO_WTM                = 0x46,
    FIFO_CONFIG_0           = 0x48,
    FIFO_CONFIG_1           = 0x49,
    INT1_IO_CONF            = 0x53,
    INT2_IO_CONF            = 0x54,
    INT1_INT2_MAP_DATA      = 0x58,
    ACC_SELF_TEST           = 0x6D,
    ACC_PWR_CONF            = 0x7C,
    ACC_PWR_CTRL            = 0x7D,
    ACC_SOFTRESET           = 0x7E
};

enum class Bmi088GyroRegister : uint8_t {
    GYRO_CHIP_ID      = 0x00,
    RATE_DATA         = 0x02,
    GYRO_INT_STAT_1   = 0x0A,
    FIFO_STATUS       = 0x0E,
    GYRO_RANGE        = 0x0F,
    GYRO_BANDWIDTH    = 0x10,
    GYRO_LPM1         = 0x11,
    GYRO_SOFTRESET    = 0x14,
    GYRO_INT_CTRL     = 0x15,
    INT3_INT4_IO_CONF = 0x16,
    INT3_INT4_IO_MAP  = 0x18,
    FIFO_WM_ENABLE    = 0x1E,
    FIFO_EXT_INT_S    = 0x34,
    GYRO_SELF_TEST    = 0x3C,
    FIFO_CONFIG_0     = 0x3D,
    FIFO_CONFIG_1     = 0x3E,
    FIFO_DATA         = 0x3F
};




/*-------- 3. interface ----------------------------------------------------------------------------------------------*/




/*-------- 4. decorator ----------------------------------------------------------------------------------------------*/




/*-------- 5. factories ----------------------------------------------------------------------------------------------*/





#endif /*H723VG_BMI08X_DEV_BMI_CMD_H*/
