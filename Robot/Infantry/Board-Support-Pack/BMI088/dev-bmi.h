/**
 *******************************************************************************
 * @file    dev-bmi.h
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

#ifndef H723VG_BMI08X_DEV_BMI_H
#define H723VG_BMI08X_DEV_BMI_H



/*-------- 1. includes and imports -----------------------------------------------------------------------------------*/

#include "dev-bmi-def.h"
#include "gpio.h"
#include "spi.h"

#include "FreeRTOS.h"
#include "semphr.h"



/*-------- 2. enum and define ----------------------------------------------------------------------------------------*/

#define BMI_DMA_BUF_LEN 64




/*-------- 3. interface ----------------------------------------------------------------------------------------------*/

enum class BmiErr {
    NONE,
    SUCCESS,
    PARAM_ERROR,
    ADDR_NULL,
    ERROR_IN_HAL,
    COMM_TIMEOUT,
    ERROR,
};

enum class BmiState {
    NONE,
    IDLE,
    READING_ACCEL,
    READING_GYRO,
    READING_TEMP,
};

typedef struct {
    struct {
        float x;
        float y;
        float z;
    } rate;

    struct {
        float x;
        float y;
        float z;
    } accel;

    float temperature;
    uint32_t timestamp;
} ImuData;


class Bmi088 {
  public:
    Bmi088(SPI_HandleTypeDef* spiHandle, GPIO_TypeDef* _accCsPort, GPIO_TypeDef* _gyroCsPort, uint16_t _accCsPin,
           uint16_t _gyroCsPin, uint8_t* _pDmaTxBuf, uint8_t* _pDmaRxBuf);
    BmiErr init(Bmi088AccRange, Bmi088AccODR, Bmi088AccWidth, Bmi088GyroRange, Bmi088GyroWidth);

    uint8_t readAccRegister(Bmi088AccRegister);
    uint8_t readGyroRegister(Bmi088GyroRegister);
    void writeGyroRegister(Bmi088GyroRegister reg, uint8_t value);
    void writeAccRegister(Bmi088AccRegister reg, uint8_t value);
    static void waitingForData(TickType_t timeout);
    void getImuData(ImuData& data);
    void onExti();
    void onTxComplete();
    void onTransferComplete();



    /* setter & getter */

    uint8_t* getDmaTxBuf() { return _pDmaTxBuf; }

    uint8_t* getDmaRxBuf() { return _pDmaRxBuf; }

    struct __attribute__((packed)) AngularVelocity {
        int16_t x;
        int16_t y;
        int16_t z;
    } _angularVelocity{};

    struct __attribute__((packed)) Acceleration {
        int16_t x;
        int16_t y;
        int16_t z;
    } _acceleration{};


  private:
    SPI_HandleTypeDef* _handle                  = nullptr;
    xSemaphoreHandle _afterTxCompleteSema       = nullptr;
    xSemaphoreHandle _afterTransferCompleteSema = nullptr;
    uint8_t* _pDmaTxBuf                         = nullptr;
    uint8_t* _pDmaRxBuf                         = nullptr;
    uint8_t _rawGyroData[6]                     = {0};
    uint8_t _rawAccData[6]                      = {0};
    uint8_t _rawTempData[6]                     = {0};
    uint32_t _timestamp                         = 0;
    uint16_t _accRange                          = 0;
    uint16_t _gyroRange                         = 0;
    GPIO_TypeDef* _accCsPort                    = nullptr;
    GPIO_TypeDef* _gyroCsPort                   = nullptr;
    GPIO_TypeDef* _psCurrentPort                = nullptr;
    uint16_t _accCsPin                          = 0;
    uint16_t _gyroCsPin                         = 0;
    uint16_t _psCurrentPin                      = 0;
    BmiState _state                             = BmiState::NONE;
    TaskHandle_t _processTaskHandle             = nullptr;
};



/*-------- 4. decorator ----------------------------------------------------------------------------------------------*/




/*-------- 5. factories ----------------------------------------------------------------------------------------------*/





#endif /*H723VG_BMI08X_DEV_BMI_H*/
