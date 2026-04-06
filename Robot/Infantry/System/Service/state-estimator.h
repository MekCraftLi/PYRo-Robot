/**
 *******************************************************************************
 * @file    app-imu.h
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
 * @date    2026/2/5
 * @version 1.0
 *******************************************************************************
 */


/* Define to prevent recursive inclusion -----------------------------------------------------------------------------*/

#ifndef INFANTRY_CHASSIS_APP_IMU_H
#define INFANTRY_CHASSIS_APP_IMU_H




/*-------- 1. includes and imports -----------------------------------------------------------------------------------*/

#ifdef __cplusplus

/* I. interface */

#include "../Thread/application-base.h"
#include "../crtp.h"

/* II. OS */


/* III. middlewares */
#include "semphr.h"


/* IV. drivers */
#include "Component/INS/pyro_ins.h"
#include "../../Board-Support-Pack/BMI088/dev-bmi.h"
#include "spi.h"
#include "tim.h"

/* V. standard lib */





/*-------- 2. enum ---------------------------------------------------------------------------------------------------*/




/*-------- 3. interface ---------------------------------------------------------------------------------------------*/


class ImuApp final : public NotifyApp, public Singleton<ImuApp> {
public:
    ImuApp();

    void init() override;

    void setIMUConfig(Bmi088AccRange, Bmi088AccODR, Bmi088AccWidth, Bmi088GyroRange, Bmi088GyroWidth);


    void run() override;

    ImuData data;

    /************ setter & getter ***********/


private:
    /* message interface */

    // 1. message queue

    // 2. mutex

    // 3. semphr
    xSemaphoreHandle _waitForTransmit;
    xSemaphoreHandle _waitForReceive;
    xSemaphoreHandle _waitForTransmitReceive;

    // 4. notify

    // 5. stream or message

    // 6. event group

    /* drivers */
    Bmi088 _bmi088;

    friend void HAL_SPI_TxRxCpltCallback(SPI_HandleTypeDef* hspi);
    friend void HAL_SPI_RxCpltCallback(SPI_HandleTypeDef* hspi);
    friend void HAL_SPI_TxCpltCallback(SPI_HandleTypeDef* hspi);
    friend void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin);

    friend bool waitForTransmit(uint32_t timeout);
    friend bool waitForReceive(uint32_t timeout);
    friend bool waitForTransmitReceive(uint32_t timeout );

};
#endif


#ifdef __cplusplus
extern "C" {
#endif

    /* C Interface */
    void HAL_SPI_TxRxCpltCallback(SPI_HandleTypeDef* hspi);
    void HAL_SPI_RxCpltCallback(SPI_HandleTypeDef* hspi);
    void HAL_SPI_TxRxCpltCallback(SPI_HandleTypeDef* hspi);

#ifdef __cplusplus
}
#endif






/*-------- 4. decorator ----------------------------------------------------------------------------------------------*/




/*-------- 5. factories ----------------------------------------------------------------------------------------------*/

#endif