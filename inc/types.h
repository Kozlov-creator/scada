#pragma once

#include <SDL3/SDL.h>
#include <stdint.h>
#include <asio.hpp>

using asio::ip::udp;

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

// Структура для передачи параметров в поток (если нужно)
struct ThreadConfig {
    std::string fileName;
    bool* quitFlag;
};

//Создаем структуру контекста Объедините флаги и объекты в одну структуру, чтобы не плодить глобальные переменные.
struct NetContext {
    asio::ip::udp::socket* socket; // Указатель на сокет
    bool* quitFlag;                // Указатель на флаг выхода
    SDL_Mutex* mutex;              // Мьютекс для данных
};

// Статический массив месяцев (быстрее чем switch)
static const std::string MONTHS[] = {
    "января", "февраля", "марта", "апреля", "мая", "июня",
    "июля", "августа", "сентября", "октября", "ноября", "декабря"
};

struct SceneElement {
    std::string textureKey; // Имя файла (без расширения)
    SDL_FRect rect;         // Позиция и размер
    int layer = 0;          // Слой отрисовки
};

struct RenderItem {
    std::string name;
    SceneElement* element; // Теперь тип совпадает!

    RenderItem(std::string n, SceneElement* el)
    : name(n), element(el) {}
};
