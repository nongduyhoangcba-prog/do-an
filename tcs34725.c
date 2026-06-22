#include "tcs34725.h"

extern I2C_HandleTypeDef hi2c1; // Ðam bao ban dang dùng I2C1 trong CubeMX

void TCS34725_WriteRegister(uint8_t reg, uint8_t value) {
    uint8_t data[2] = {TCS34725_COMMAND_BIT | reg, value};
    HAL_I2C_Master_Transmit(&hi2c1, TCS34725_ADDRESS, data, 2, 100);
}

uint8_t TCS34725_ReadRegister8(uint8_t reg) {
    uint8_t value;
    uint8_t command = TCS34725_COMMAND_BIT | reg;
    HAL_I2C_Master_Transmit(&hi2c1, TCS34725_ADDRESS, &command, 1, 100);
    HAL_I2C_Master_Receive(&hi2c1, TCS34725_ADDRESS, &value, 1, 100);
    return value;
}

uint16_t TCS34725_ReadRegister16(uint8_t reg) {
    uint8_t data[2];
    uint8_t command = TCS34725_COMMAND_BIT | reg;
    HAL_I2C_Master_Transmit(&hi2c1, TCS34725_ADDRESS, &command, 1, 100);
    HAL_I2C_Master_Receive(&hi2c1, TCS34725_ADDRESS, data, 2, 100);
    return (data[1] << 8) | data[0]; 
}

uint8_t TCS34725_Init(void) {
    // 1. Kiem tra ID xem có dúng module không 
    // [UPDATE]: Ðã thêm di?u ki?n ch?p nh?n mã 0x4D 
    uint8_t id = TCS34725_ReadRegister8(TCS34725_ID);
    if ((id != 0x44) && (id != 0x10) && (id != 0x4D)) {
        return 0; // Loi: Không tìm thay cam bien
    }

    // 2. Chinh Integration Time (24ms)
    TCS34725_WriteRegister(TCS34725_ATIME, 0xF6);
    
    // 3. Chinh Gain khuech dai (16X) 
    TCS34725_WriteRegister(TCS34725_CONTROL, 0x02);

    // 4. Bat nguon module
    TCS34725_WriteRegister(TCS34725_ENABLE, TCS34725_ENABLE_PON);
    HAL_Delay(3);
    TCS34725_WriteRegister(TCS34725_ENABLE, TCS34725_ENABLE_PON | TCS34725_ENABLE_AEN);
    
    return 1; // Khoi tao thành công
}

void TCS34725_GetRawData(uint16_t *r, uint16_t *g, uint16_t *b, uint16_t *c) {
    *c = TCS34725_ReadRegister16(TCS34725_CDATAL);
    *r = TCS34725_ReadRegister16(TCS34725_RDATAL);
    *g = TCS34725_ReadRegister16(TCS34725_GDATAL);
    *b = TCS34725_ReadRegister16(TCS34725_BDATAL);
}
