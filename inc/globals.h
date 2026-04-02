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
#define FILE_MODBUS "./logs/readModbus6.txt"
#define FILE_IMAGE "./coordinate/typeImage.txt"
#define IMAGES_CONF "./coordinate/images.conf"

//Screen dimension constants
const int SCREEN_WIDTH = 1920;
const int SCREEN_HEIGHT = 1080;

//Number of data integers
const int TOTAL_DATA = 10;

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
}

//Scada::
