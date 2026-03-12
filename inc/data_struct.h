#ifndef __DATA_STRUCT_H__
#define __DATA_STRUCT_H__

#include <stdint.h>

//Важно: используйте #pragma pack(push, 1), чтобы компилятор не вставлял пустые байты для выравнивания.
#pragma pack(push, 1)
struct SensorData {
    uint32_t timestamp;  // Время с момента запуска МК (ms)
    float    temperature; // Температура (4 байта, IEEE 754)
    uint16_t humidity;    // Влажность
    int16_t  accel_x;     // Ускорение X
    int16_t  accel_y;     // Ускорение Y
    int16_t  accel_z;     // Ускорение Z
    uint16_t packet_id;   // Номер пакета (для отслеживания потерь в UDP)
};
#pragma pack(pop)

#endif //__DATA_STRUCT_H__
