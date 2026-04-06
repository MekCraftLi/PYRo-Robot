/**
 *******************************************************************************
 * @file    iio-adapter.h
 * @brief   用于适配IIO Channel
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
 * @date    2026/2/10
 * @version 1.0
 *******************************************************************************
 */


/* Define to prevent recursive inclusion -----------------------------------------------------------------------------*/

#ifndef INFANTRY_CHASSIS_IIO_ADAPTER_H
#define INFANTRY_CHASSIS_IIO_ADAPTER_H



/*-------- 1. includes and imports -----------------------------------------------------------------------------------*/

#include "../../System/IIO-Channel/iio-device.h"
#include "dev-bmi.h"



/*-------- 2. enum and define ----------------------------------------------------------------------------------------*/




/*-------- 3. interface ----------------------------------------------------------------------------------------------*/

class IioBmi088: public IIODevice {
public:
    // 构造函数：传入设备名
    IIO_BMI088(const char* name, Bmi088& device): m_name(name), m_driver(device) {}

    // 实现核心接口
    bool ReadChannel(IIOType type, IIOChan chan, float& out_val) override {
        // 1. 获取 BSP 层最新的缓存数据 (Snapshot)
        // 注意：这里假设 GetData() 返回的是 float 格式的结构体
        const auto& data = m_driver->GetData();

        switch (type) {
            case IIOType::ACCEL:
                switch (chan) {
                case IIOChan::X: out_val = data.accel[0]; return true;
                case IIOChan::Y: out_val = data.accel[1]; return true;
                case IIOChan::Z: out_val = data.accel[2]; return true;
                default: return false;
                }

            case IIOType::ANGL_VEL: // Gyro
                switch (chan) {
                case IIOChan::X: out_val = data.gyro[0]; return true;
                case IIOChan::Y: out_val = data.gyro[1]; return true;
                case IIOChan::Z: out_val = data.gyro[2]; return true;
                default: return false;
                }

            case IIOType::TEMP:
                if (chan == IIOChan::NONE) {
                    out_val = data.temp;
                    return true;
                }
                return false;

            default:
                return false;
        }
    }

private:
    Bmi088& m_driver;
};


/*-------- 4. decorator ----------------------------------------------------------------------------------------------*/




/*-------- 5. factories ----------------------------------------------------------------------------------------------*/





#endif /*INFANTRY_CHASSIS_IIO_ADAPTER_H*/
