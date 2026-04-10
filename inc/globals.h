#pragma once
#include "types.h"
#include "SceneManager.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>

// Подключаем структуры
#include "texture_class.h"



#define FONT_TTF "/usr/share/fonts/TTF/Hack-BoldItalic.ttf" //Arch
#define FILE_ALLERTMESSAGE "./logs/alert_message.txt"
#define FILE_MODBUS "/dev/shm/tags.csv"
#define FILE_IMAGE "./coordinate/typeImage.txt"
#define IMAGES_CONF "./coordinate/images.conf"

//Screen dimension constants
const int SCREEN_WIDTH = 1920;
const int SCREEN_HEIGHT = 1080;

//Number of data integers
const int TOTAL_DATA = 10;

namespace Tags {
    const std::string UDP_TEMP = "UDP_temp";
    const std::string UDP_HUM  = "UDP_hum";
    const std::string UDP_ID   = "UDP_ID";
    const std::string UDP_ACCEL   = "UDP_acel";
}

namespace App {
    inline SceneManager scene;      // Управление объектами
    inline SDL_Renderer* renderer;  // Глобальный рендерер
    inline bool editMode = false;
}

namespace Scada {
    // 1. Состояние системы
    inline bool editMode = false; //Реализация через флаг Edit Mode. В режиме работы (Runtime) перетаскивание должно быть запрещено, чтобы оператор случайно не «унес» насос с экрана.
    inline bool isResizing = false;
    inline bool isDragging = false;

    // 2. Графические ресурсы
    inline SDL_Renderer* gRenderer = nullptr; //Средство визуализации окна
    inline SDL_Window* gWindow = nullptr; //Окно, в которое будем отображать

    // Общий мап текстур (теперь один!)
    //РЕСУРСЫ: Уникальные текстуры (Ключ: "pump" -> Обьект CTexture с данными из файла pump.png)
    inline std::unordered_map<std::string, CTexture> gSharedTextures;

    // 3. Объекты сцены (теперь объединенная структура)
    inline std::unordered_map<std::string, SceneElement> gSceneElements;

    // Очередь отрисовки (Z-order)
    inline std::vector<RenderItem> gRenderOrder; // Список в порядке слоев

    // 4. Текстовые конфиги
//ТЕКСТ: Координаты текстовых полей
    inline std::unordered_map<std::string, SDL_FRect> gTextAlert;

    // 5. Данные Modbus
    inline std::unordered_map<std::string, std::string> gModbusData;
    inline SDL_Mutex* modbus_mutex = nullptr;

    // 6. Конфигурация сети
    inline std::string destIP = "127.0.0.1";
    inline int destPort = 1234;
    inline int localPort = 1235;

    inline std::vector<std::string> vstrValueMC;

    // Глобальный вектор для хранения строк, которые мы не умеем парсить (например, сетевые настройки)
    inline std::vector<std::string> gUnknownConfigLines;

    inline bool gUseSystemTime = false; // Глобальный флаг

    inline bool showControlWindow;    // Флаг: показано ли окно

    inline SDL_FRect controlWindowRect = { 700, 300, 500, 400 }; // Координаты окна по центру

    // Глобальные данные для обмена между потоками
    inline SensorData shared_sensor_data;
    inline SDL_Mutex* data_mutex;

    inline SDL_FRect *selectedRect = nullptr;

    inline std::string activeControlObject = ""; // Имя объекта (например, "pump_left")

    inline std::vector<std::string> vstrAlert;

    // Добавьте в Scada глобальную карту подготовленных строк
    inline std::unordered_map<std::string, std::string> gDisplayStrings;

    // Единая точка для всех данных в системе
    inline std::unordered_map<std::string, std::string> gLiveTags;

    inline std::vector<AlarmEntry> gAlarmLog;
    inline float alarmScrollPos = 0.0f; // Текущая X-позиция текста
    inline SDL_Mutex* alarm_mutex = SDL_CreateMutex();
    // Хранит время последнего успешного получения тега (в миллисекундах)
    inline std::unordered_map<std::string, long> gTagTimestamps;

    inline bool isModbusLinkLost = false; // Глобальный статус для файла
    inline bool isUdpLinkLost = false;    // Новый флаг для UDP

    inline uint32_t lastMcuTimestamp = 0;      // Тот, что пришел в структуре
    inline uint32_t lastUdpUpdateTimePC = 0;   // Время ПК (SDL_GetTicks)

}

//Scada::
