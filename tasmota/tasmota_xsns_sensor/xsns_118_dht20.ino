#ifdef USE_I2C
#ifdef USE_DHT20


#include <Wire.h>


#define XSNS_118    118
#define XI2C_94     94  //See I2CDEVICES.md


#define DHT20_ADDR  0x38


#define DHT20_TIMEOUT 1000 //ms


struct DHT20t{
    float   temperature = NAN;
    float   humidity = NAN;
    uint8_t valid = 0;
    uint8_t count = 0;
    char    name[6] = "DHT20";
} DHT20;


uint8_t DHT20_crc8(uint8_t *data, uint8_t len) {
    uint8_t crc = 0xFF;
    for (uint8_t i = 0; i < len; i++) {
        crc ^= data[i];
        for (uint8_t j = 0; j < 8; j++) {
            crc = (crc & 0x80) ? (crc << 1) ^ 0x31 : (crc << 1);
        }
    }
    return crc;
}


bool DHT20Read(void){
    if (DHT20.valid) {DHT20.valid--;}


    Wire.beginTransmission(DHT20_ADDR);
    Wire.write(0xAC);
    Wire.write(0x33);
    Wire.write(0x00);
    if (Wire.endTransmission() != 0) {return false;}


    unsigned long start = millis();
    while (millis() - start < DHT20_TIMEOUT) {
        Wire.requestFrom(DHT20_ADDR, 1);
        if (Wire.available()) {
            uint8_t status = Wire.read();
            if ((status & 0x80) == 0) break; // Bit 7 = 0 means measurement is ready
        }
        delay(5);
    }


    Wire.requestFrom(DHT20_ADDR, 7);
    if (Wire.available() < 7) {
        return false;
    }


    uint8_t data[7];
    for (int i = 0; i < 7; i++) {
        data[i] = Wire.read();
    }


    // CRC check
    if (DHT20_crc8(data, 6) != data[6]) {return false;}


    uint32_t raw_humi = ((uint32_t)data[1] << 12) | ((uint32_t)data[2] << 4) | ((data[3] & 0xF0) >> 4);
    DHT20.humidity = (raw_humi * 100.0) / 1048576.0;
    uint32_t raw_temp = ((uint32_t)(data[3] & 0x0F) << 16) | ((uint32_t)data[4] << 8) | data[5];
    DHT20.temperature = ((raw_temp * 200.0) / 1048576.0) - 50;


    if (isnan(DHT20.humidity) || isnan(DHT20.temperature)) {return false;}


    DHT20.valid = SENSOR_MAX_MISS;
    return true;
}


void DHT20Detect(void){
    Wire.begin(11, 12);
    if (!I2cSetDevice(DHT20_ADDR)) return;


    if (DHT20Read()) {
    I2cSetActiveFound(DHT20_ADDR, DHT20.name);
    DHT20.count = 1;
    }
}


void DHT20EverySecond(void){
    if(TasmotaGlobal.uptime & 1){
        if(!DHT20Read()){
            AddLogMissed(DHT20.name, DHT20.valid);
        }
    }
}


void DHT20Show(bool json){
    if(DHT20.valid){
        TempHumDewShow(json,(0 == TasmotaGlobal.tele_period), DHT20.name, DHT20.temperature, DHT20.humidity);
    }
}


bool Xsns118(uint32_t function){
    if(!I2cEnabled(XI2C_94)) {return false;}


    bool result = false;


    if(FUNC_INIT == function){
        DHT20Detect();
    }
    else if(DHT20.count){
        switch(function){
        case FUNC_EVERY_SECOND:
            DHT20EverySecond();
            break;
        case FUNC_JSON_APPEND:
            DHT20Show(1);
            break;
#ifdef USE_WEBSERVER
        case FUNC_WEB_SENSOR:
            DHT20Show(0);
            break;
#endif
        }
    }
    return result;
}


#endif //DHT20
#endif //I2C
