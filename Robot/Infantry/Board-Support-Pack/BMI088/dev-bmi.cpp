/**
 *******************************************************************************
 * @file    dev-bmi.cpp
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


/* ------- define ----------------------------------------------------------------------------------------------------*/

#define GRAVITY_EARTH (9.80665f)




/* ------- include ---------------------------------------------------------------------------------------------------*/

#include "dev-bmi.h"

#include "dev-bmi-def.h"

#include "gpio.h"

#include "tim.h"

#include <cstring>

#include "FreeRTOS.h"
#include "semphr.h"





/* ------- class prototypes-------------------------------------------------------------------------------------------*/





/* ------- macro -----------------------------------------------------------------------------------------------------*/

#define __SELECT_ACC()    _accCsPort->BSRR = _accCsPin << 16
#define __SELECT_GYRO()   _gyroCsPort->BSRR = _gyroCsPin << 16
#define __UNSELECT_ACC()  _accCsPort->BSRR = _accCsPin
#define __UNSELECT_GYRO() _gyroCsPort->BSRR = _gyroCsPin



/* ------- variables -------------------------------------------------------------------------------------------------*/





/* ------- function implement ----------------------------------------------------------------------------------------*/


Bmi088::Bmi088(SPI_HandleTypeDef* spiHandle, GPIO_TypeDef* accCsPort, GPIO_TypeDef* gyroCsPort, uint16_t accCsPin,
               uint16_t gyroCsPin, uint8_t* pDmaTxBuf, uint8_t* pDmaRxBuf)
    : _handle(spiHandle), _pDmaTxBuf(pDmaTxBuf), _pDmaRxBuf(pDmaRxBuf), _accCsPort(accCsPort), _gyroCsPort(gyroCsPort),
      _accCsPin(accCsPin), _gyroCsPin(gyroCsPin) {

    _afterTxCompleteSema       = xSemaphoreCreateBinary();
    _afterTransferCompleteSema = xSemaphoreCreateBinary();
}

BmiErr Bmi088::init(Bmi088AccRange accRange = Bmi088AccRange::RANGE_3G, Bmi088AccODR accODR = Bmi088AccODR::ODR_1600_HZ,
                    Bmi088AccWidth accWidth   = Bmi088AccWidth::NORMAL,
                    Bmi088GyroRange gyroRange = Bmi088GyroRange::RANGE_125_DPS,
                    Bmi088GyroWidth gyroWidth = Bmi088GyroWidth::ODR_1000HZ_BW_116HZ) {
    switch (accRange) {
        case Bmi088AccRange::RANGE_3G:
            _accRange = 3;
            break;
        case Bmi088AccRange::RANGE_6G:
            _accRange = 6;
            break;
        case Bmi088AccRange::RANGE_12G:
            _accRange = 12;
            break;
        case Bmi088AccRange::RANGE_24G:
            _accRange = 24;
            break;
    }

    switch (gyroRange) {
        case Bmi088GyroRange::RANGE_125_DPS:
            _gyroRange = 125;
            break;
        case Bmi088GyroRange::RANGE_250_DPS:
            _gyroRange = 250;
            break;
        case Bmi088GyroRange::RANGE_500_DPS:
            _gyroRange = 500;
            break;
        case Bmi088GyroRange::RANGE_1000_DPS:
            _gyroRange = 1000;
            break;
        case Bmi088GyroRange::RANGE_2000_DPS:
            _gyroRange = 2000;
            break;
    }

    /* 1. 启用加速度计的SPI接口 */
    __SELECT_ACC();
    vTaskDelay(10);
    __UNSELECT_ACC();
    vTaskDelay(100);

    /* 2. 每次上电必须使能加速度计 */

    writeGyroRegister(Bmi088GyroRegister::GYRO_SOFTRESET, 0xB6);
    vTaskDelay(100);
    writeAccRegister(Bmi088AccRegister::ACC_SOFTRESET, 0xB6);
    vTaskDelay(100);
    writeAccRegister(Bmi088AccRegister::ACC_PWR_CONF, 0x00);
    vTaskDelay(100);
    writeAccRegister(Bmi088AccRegister::ACC_PWR_CTRL, 0x04);
    vTaskDelay(100);

    /* 3. 读取ID验证 */
    if (readAccRegister(Bmi088AccRegister::ACC_CHIP_ID) != 0x1E) {
        return BmiErr::ERROR;
    }
    if (readGyroRegister(Bmi088GyroRegister::GYRO_CHIP_ID) != 0x0F) {
        return BmiErr::ERROR;
    }

    /* 4. 配置参数 */
    writeAccRegister(Bmi088AccRegister::ACC_RANGE, (uint8_t)accRange);

    writeAccRegister(Bmi088AccRegister::ACC_CONF, (uint8_t)accWidth << 4 | (uint8_t)accODR);

    writeGyroRegister(Bmi088GyroRegister::GYRO_RANGE, (uint8_t)gyroRange);

    writeGyroRegister(Bmi088GyroRegister::GYRO_BANDWIDTH, (uint8_t)gyroWidth);


    /* 5. 读取验证参数 */
    if (readAccRegister(Bmi088AccRegister::ACC_RANGE) != (uint8_t)accRange) {
        return BmiErr::ERROR;
    }
    if (readAccRegister(Bmi088AccRegister::ACC_CONF) != ((uint8_t)accWidth << 4 | (uint8_t)accODR)) {
        return BmiErr::ERROR;
    }
    if (readGyroRegister(Bmi088GyroRegister::GYRO_RANGE) != (uint8_t)gyroRange) {
        return BmiErr::ERROR;
    }
    if (readGyroRegister(Bmi088GyroRegister::GYRO_BANDWIDTH) != ((uint8_t)gyroWidth | 0x80)) {
        return BmiErr::ERROR;
    }

    /* 6. 开启中断触发信号 */
    writeAccRegister(Bmi088AccRegister::INT1_IO_CONF, 0x00);

    writeAccRegister(Bmi088AccRegister::INT1_INT2_MAP_DATA, 0x00);

    writeGyroRegister(Bmi088GyroRegister::GYRO_INT_CTRL, 0x80);

    writeGyroRegister(Bmi088GyroRegister::INT3_INT4_IO_CONF, 0x01);

    writeGyroRegister(Bmi088GyroRegister::INT3_INT4_IO_MAP, 0x01);

    _state             = BmiState::IDLE;

    /* 7. 获取任务句柄 */
    _processTaskHandle = xTaskGetCurrentTaskHandle();

    return BmiErr::SUCCESS;
}




uint8_t Bmi088::readAccRegister(Bmi088AccRegister reg) {
    __SELECT_ACC();
    _pDmaTxBuf[0] = static_cast<uint8_t>(reg) | 0x80;
    HAL_SPI_TransmitReceive_DMA(_handle, _pDmaTxBuf, _pDmaRxBuf, 3);
    xSemaphoreTake(_afterTransferCompleteSema, portMAX_DELAY);
    __UNSELECT_ACC();
    return _pDmaRxBuf[2];
}

uint8_t Bmi088::readGyroRegister(Bmi088GyroRegister reg) {
    __SELECT_GYRO();
    _pDmaTxBuf[0] = static_cast<uint8_t>(reg) | 0x80;
    HAL_SPI_TransmitReceive_DMA(_handle, _pDmaTxBuf, _pDmaRxBuf, 2);
    xSemaphoreTake(_afterTransferCompleteSema, portMAX_DELAY);
    __UNSELECT_GYRO();
    return _pDmaRxBuf[1];
}
void Bmi088::writeGyroRegister(Bmi088GyroRegister reg, uint8_t value) {
    __SELECT_GYRO();
    _pDmaTxBuf[0] = static_cast<uint8_t>(reg);
    _pDmaTxBuf[1] = value;
    HAL_SPI_Transmit_DMA(_handle, _pDmaTxBuf, 2);
    xSemaphoreTake(_afterTxCompleteSema, portMAX_DELAY);
    vTaskDelay(1); // 寄存器写入需要1us的延时，这里使用1ms
    __UNSELECT_GYRO();
}


void Bmi088::writeAccRegister(Bmi088AccRegister reg, uint8_t value) {
    __SELECT_ACC();
    _pDmaTxBuf[0] = static_cast<uint8_t>(reg);
    _pDmaTxBuf[1] = value;
    HAL_SPI_Transmit_DMA(_handle, _pDmaTxBuf, 2);
    xSemaphoreTake(_afterTxCompleteSema, portMAX_DELAY);
    vTaskDelay(1); // 寄存器写入需要1us的延时，这里使用1ms
    __UNSELECT_ACC();
}

void Bmi088::waitingForData(TickType_t timeout) { ulTaskNotifyTake(pdTRUE, timeout); }


void Bmi088::getImuData(ImuData& data) {
    int16_t rawAccelx   = _rawAccData[1] << 8 | _rawAccData[0];
    int16_t rawAccely   = _rawAccData[3] << 8 | _rawAccData[2];
    int16_t rawAccelz   = _rawAccData[5] << 8 | _rawAccData[4];


    float factor        = (static_cast<float>(_accRange)) / 32768.0f * GRAVITY_EARTH;

    data.accel.x        = factor * rawAccelx;
    data.accel.y        = factor * rawAccely;
    data.accel.z        = factor * rawAccelz;

    int16_t rawGyrox    = _rawGyroData[1] << 8 | _rawGyroData[0];
    int16_t rawGyroy    = _rawGyroData[3] << 8 | _rawGyroData[2];
    int16_t rawGyroz    = _rawGyroData[5] << 8 | _rawGyroData[4];

    factor              = M_PI * static_cast<float>(_gyroRange) / 32767.0f / 180.0f;

    data.rate.x         = factor * rawGyrox;
    data.rate.y         = factor * rawGyroy;
    data.rate.z         = factor * rawGyroz;

    uint16_t tempUInt11 = (_rawTempData[0] << 3) | (_rawTempData[1] >> 5);

    auto tempInt11      = static_cast<int16_t>(tempUInt11);
    if (tempInt11 > 1023) {
        tempInt11 -= 2048;
    }

    data.temperature = (static_cast<float>(tempInt11) * 0.125f) + 23.0f;

    data.timestamp   = _timestamp;
}


void Bmi088::onTxComplete() {
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    xSemaphoreGiveFromISR(_afterTxCompleteSema, &xHigherPriorityTaskWoken);
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

void Bmi088::onTransferComplete() {

    switch (_state) {
        case BmiState::READING_GYRO: {
            _state = BmiState::READING_ACCEL;
            __UNSELECT_GYRO();
            _rawGyroData[0] = _pDmaRxBuf[1];
            _rawGyroData[1] = _pDmaRxBuf[2];
            _rawGyroData[2] = _pDmaRxBuf[3];
            _rawGyroData[3] = _pDmaRxBuf[4];
            _rawGyroData[4] = _pDmaRxBuf[5];
            _rawGyroData[5] = _pDmaRxBuf[6];
            __SELECT_ACC();
            _pDmaTxBuf[0] = static_cast<uint8_t>(Bmi088AccRegister::ACC_DATA) | 0x80;
            HAL_SPI_TransmitReceive_DMA(_handle, _pDmaTxBuf, _pDmaRxBuf, 8);
        } break;

        case BmiState::READING_ACCEL: {
            _state = BmiState::READING_TEMP;
            __UNSELECT_ACC();
            _rawAccData[0] = _pDmaRxBuf[2];
            _rawAccData[1] = _pDmaRxBuf[3];
            _rawAccData[2] = _pDmaRxBuf[4];
            _rawAccData[3] = _pDmaRxBuf[5];
            _rawAccData[4] = _pDmaRxBuf[6];
            _rawAccData[5] = _pDmaRxBuf[7];
            __SELECT_ACC();
            _pDmaTxBuf[0] = static_cast<uint8_t>(Bmi088AccRegister::TEMPERATURE_SENSOR_DATA) | 0x80;
            HAL_SPI_TransmitReceive_DMA(_handle, _pDmaTxBuf, _pDmaRxBuf, 4);
        } break;

        case BmiState::READING_TEMP: {
            _state = BmiState::IDLE;
            __UNSELECT_ACC();
            _rawTempData[0]                     = _pDmaRxBuf[2];
            _rawTempData[1]                     = _pDmaRxBuf[3];
            BaseType_t xHigherPriorityTaskWoken = pdFALSE;

            vTaskNotifyGiveFromISR(_processTaskHandle, &xHigherPriorityTaskWoken);


        } break;

        default: {
            BaseType_t xHigherPriorityTaskWoken = pdFALSE;
            xSemaphoreGiveFromISR(_afterTransferCompleteSema, &xHigherPriorityTaskWoken);
            portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
        }
    }
}

void Bmi088::onExti() {

    if (_state == BmiState::IDLE) {
        __SELECT_GYRO();
        _timestamp    = xTaskGetTickCountFromISR();
        _pDmaTxBuf[0] = static_cast<uint8_t>(Bmi088GyroRegister::RATE_DATA) | 0x80;
        HAL_SPI_TransmitReceive_DMA(_handle, _pDmaTxBuf, _pDmaRxBuf, 7);
        _state = BmiState::READING_GYRO;
    }
}