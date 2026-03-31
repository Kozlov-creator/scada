#include <map>
#include <fstream>
#include <iostream>
#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <vector>
#include <texture_class.h>
#include <string>
#include <main.h>
#include <chrono>
#include "data_struct.h" //  структура с сенсорами
#include <unordered_map>



// Глобальное хранилище координат


extern std::vector<RenderItem> gRenderOrder; // Список в порядке слоев
extern bool showControlWindow;    // Флаг: показано ли окно
extern std::string activeControlObject; // Имя объекта (например, "pump_left")
extern SDL_FRect controlWindowRect; // Координаты окна по центру

// 1. Хранилище уникальных текстур (Путь к файлу -> Объект класса)
extern std::map<std::string, CTexture> gSharedTextures;

// Хранилище всех текстур проекта
extern std::map<std::string, CTexture> gSceneTextures;
// Глобальное хранилище координат
// 2. ОБЪЕКТЫ: Описание элементов на экране

struct MKElement {
    std::string textureKey; // Имя текстуры из gSharedTextures
    SDL_FRect rect;         // Координаты на экране
};


extern std::unordered_map<std::string, SceneElement> gSceneElements;
extern std::unordered_map<std::string, std::string> gModbusData;

extern std::map<std::string, SDL_FRect> gTextConfig;
extern std::map<std::string, SDL_FRect> gTextAlert;
// Глобальные данные для обмена между потоками
extern SensorData shared_sensor_data;
extern SDL_Mutex* data_mutex;
extern SDL_Mutex* modbus_mutex;

//Подготовка глобальных контейнеров
extern std::vector<CTexture> vValueTextures;
// Кэш текстур для даты/времени (vstrDate)
extern std::vector<CTexture> vDateTextures;
// Последние отрисованные строки (для сравнения)
extern std::vector<std::string> vLastValueStrings;
extern std::vector<std::string> vLastDateStrings;

extern SDL_FRect *selectedRect;
extern bool editMode;

extern ProgressBar pressureBar;
extern std::string get_modbus_datetime();
extern std::string get_display_time();

//==============================================================================================
class TimingUtil
{
public:
    TimingUtil(std::string const& message)
    : start_(std::chrono::steady_clock::now()),
    message_(message)
    {
    }
    ~TimingUtil()
    {
        std::chrono::steady_clock::time_point finish = std::chrono::steady_clock::now();
        std::cout << message_ << " took "
        << std::chrono::duration_cast<std::chrono::milliseconds>(finish - start_).count()
        << " ns." << std::endl;
    }
private:
    std::chrono::steady_clock::time_point start_;
    std::string message_;
};


///////////////////////////////////////////////////////////////////////

extern SDL_Renderer *gRenderer;

extern std::vector<SDL_FRect> vRectClipCoord;
extern std::vector<SDL_FRect> vRectClipCoordDateTime;
extern std::vector<SDL_FRect> vRectClipCoordAlertMessage;
extern std::vector<SDL_FRect> vRectClipValue;

std::vector<std::string> vstrDate;
std::vector<std::string> vstrValue;
extern std::vector<std::string> vstrValueMC;
std::vector<std::string> vstrImage;
std::vector<std::string> vstrAlert;

std::vector<SDL_Texture*> vDateTimeTextures;
//std::vector<SDL_Texture*> vValueTextures;
std::vector<SDL_Texture*> vMessageTextures;


std::vector<SDL_Texture*> vimageTexture;

extern SDL_FRect fonClockRect;
SDL_FRect MessageRect;

SDL_Texture *ClockFon;

SDL_Texture *TestTexture = nullptr;
SDL_Surface *TestSurface = nullptr;

extern TTF_Font *gFont;

std::vector<CTexture> vectorTextureTextValue(4);
CTexture tempCTexture;

SDL_Color sdlcolor;
//----------------------------------------------------------------------
//Функция отрисовки DrawControlPopup. Эту функцию нужно вызывать в самом конце desctop1(), чтобы окно рисовалось поверх всех остальных элементов.
void DrawControlPopup() {
    if (!showControlWindow) return;

    // 1. Затеняем задний план (полупрозрачный черный фон)
    SDL_SetRenderDrawBlendMode(gRenderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(gRenderer, 0, 0, 0, 150);
    SDL_FRect fullScreen = { 0, 0, 1920, 1080 };
    SDL_RenderFillRect(gRenderer, &fullScreen);

    // 2. Рисуем основное тело окна
    SDL_SetRenderDrawColor(gRenderer, 50, 50, 50, 255); // Темно-серый
    SDL_RenderFillRect(gRenderer, &controlWindowRect);
    SDL_SetRenderDrawColor(gRenderer, 200, 200, 200, 255); // Рамка
    SDL_RenderRect(gRenderer, &controlWindowRect);

    // 3. Заголовок окна
    std::string title = "УПРАВЛЕНИЕ: " + activeControlObject;
    tempCTexture.loadFromRenderedText(title, {255, 255, 255, 255});
    SDL_FRect titleRect = { controlWindowRect.x + 20, controlWindowRect.y + 20, 400, 40 };
    tempCTexture.render(0, &titleRect, nullptr, 0.0, nullptr, SDL_FLIP_NONE);

    // 4. Кнопка "ВКЛЮЧИТЬ" (Зеленая)
    SDL_FRect btnOn = { controlWindowRect.x + 50, controlWindowRect.y + 150, 150, 80 };
    SDL_SetRenderDrawColor(gRenderer, 0, 150, 0, 255);
    SDL_RenderFillRect(gRenderer, &btnOn);
    tempCTexture.loadFromRenderedText("ПУСК", {255, 255, 255, 255});
    tempCTexture.render(0, &btnOn, nullptr, 0.0, nullptr, SDL_FLIP_NONE);

    // 5. Кнопка "ВЫКЛЮЧИТЬ" (Красная)
    SDL_FRect btnOff = { controlWindowRect.x + 300, controlWindowRect.y + 150, 150, 80 };
    SDL_SetRenderDrawColor(gRenderer, 150, 0, 0, 255);
    SDL_RenderFillRect(gRenderer, &btnOff);
    tempCTexture.loadFromRenderedText("СТОП", {255, 255, 255, 255});
    tempCTexture.render(0, &btnOff, nullptr, 0.0, nullptr, SDL_FLIP_NONE);

    // 6. Кнопка "ЗАКРЫТЬ" (Маленький крестик в углу)
    SDL_FRect btnClose = { controlWindowRect.x + controlWindowRect.w - 40, controlWindowRect.y + 10, 30, 30 };
    SDL_SetRenderDrawColor(gRenderer, 200, 0, 0, 255);
    SDL_RenderFillRect(gRenderer, &btnClose);
}


void read_image_config(const std::string);
/////////////////////////////////////////////////////////////////////

/*void render_alerts() {
    sdlcolor = {0xFF, 0xFF, 0xFF, 0xFF}; // Белый текст для алертов

    for (size_t i = 0; i < vstrAlert.size(); i++) {
        // Формируем имя ключа: alert_line_0, alert_line_1...
        std::string key = "alert_line_" + std::to_string(i);

        // Проверяем, есть ли такая область в конфиге
        if (gImageConfig.count(key)) {
            // Умная отрисовка: используем кэш текстур для алертов (если создали его ранее)
            // или обычную отрисовку:
            tempCTexture.loadFromRenderedText(vstrAlert[i], sdlcolor);
            tempCTexture.render(0, &gImageConfig[key], NULL, 0, NULL, SDL_FLIP_NONE);
        }
    }
}*/


/*void render_images() {
    sdlcolor = {0xFF, 0xFF, 0xFF, 0xFF}; // Белый текст для алертов

    for (size_t i = 0; i < vstrImage.size(); i++) {
        // Формируем имя ключа: alert_line_0, alert_line_1...
        std::string key = "clock_fon" + std::to_string(i);

        // Проверяем, есть ли такая область в конфиге
        if (gImageConfig.count(key)) {
            // Умная отрисовка: используем кэш текстур для алертов (если создали его ранее)
            // или обычную отрисовку:
            tempCTexture.loadFromRenderedText(vstrImage[i], sdlcolor);
            tempCTexture.render(0, &gImageConfig[key], NULL, 0, NULL, SDL_FLIP_NONE);
        }
    }
}*/

/*void render_scada_objects() {
    for (auto const& [name, rect] : gImageConfig) {
        // Проверяем, есть ли такая текстура в нашем кэше (карта gSceneTextures)
        if (gSceneTextures.count(name)) {
            // Передаем указатель на rect из конфига прямо в ваш метод
            gSceneTextures[name].render(0, const_cast<SDL_FRect*>(&rect), nullptr, 0.0, nullptr, SDL_FLIP_NONE);
        }
    }
}*/
// Подготовка данных для вывода. Добавьте обновление вектора строк vstrValueMC данными из сетевой структуры. Это нужно делать до начала цикла отрисовки или внутри него, защитив мьютексом.

void update_interface_values() {
    SDL_LockMutex(data_mutex);
    // Преобразуем сырые данные в строки для отображения
    // Допустим, vstrValueMC[0] - температура, [1] - влажность и т.д.
    vstrValueMC.at(0) = "Temp: " + std::to_string(shared_sensor_data.temperature) + " C";
    vstrValueMC.at(1) = "Hum:  " + std::to_string(shared_sensor_data.humidity) + " %";
    vstrValueMC.at(2) = "AccX: " + std::to_string(shared_sensor_data.accel_x);
    vstrValueMC.at(3) = "ID:   " + std::to_string(shared_sensor_data.packet_id);
    SDL_UnlockMutex(data_mutex);
}

/////////////////////////////////////////////////////////////////////
void desctop1()
{
    // 1. Очистка экрана
    SDL_SetRenderDrawColor(gRenderer, 129, 191, 254, 255);
    SDL_RenderClear(gRenderer);

    // 2. ОТРИСОВКА ГРАФИКИ (IMGPNG:)
    // Проходим по всем объектам из конфига

    // 2. ЕДИНЫЙ ЦИКЛ ОТРИСОВКИ ГРАФИКИ
    for (auto& item : gRenderOrder) {
        SceneElement* el = item.element;
        std::string texKey = el->textureKey;

        if (item.name.find("truba") != std::string::npos) {
            // Рисуем как растягиваемую трубу (9-grid)
            float flange = 10.0f;
            SDL_RenderTexture9Grid(gRenderer, gSharedTextures[texKey].getTexture(),
                                   nullptr, flange, 0, flange, 0, 1.0f, &el->rect);
        } else {
            // Обычная отрисовка текстуры
            gSharedTextures[el->textureKey].render(0, &el->rect);
        }
    }
   /* for (auto const& [objName, img] : gSceneElementsPNG) {
        // Ключ для поиска текстуры — это имя файла из конфига
        std::string texKey = img.textureKey;

        if (gSharedTextures.count(texKey)) {
            // Рисуем, передавая прямоугольник конкретного объекта
            gSharedTextures[texKey].render(0, const_cast<SDL_FRect*>(&img.rect), nullptr, 0.0, nullptr, SDL_FLIP_NONE);
        }
    }

    // 2. ОТРИСОВКА ГРАФИКИ (IMGSVG:)
    // Проходим по всем объектам из конфига
    for (auto const& [objName, img] : gSceneElementsSVG) {
        // Ключ для поиска текстуры — это имя файла из конфига
        std::string texKey = img.textureKey;

        if (gSharedTextures.count(texKey)) {
            // Рисуем, передавая прямоугольник конкретного объекта
            // ПРОВЕРКА: Если это труба (можно определять по имени или ключу)
            if (objName.find("truba") != std::string::npos) {

                float flange = 10.0f; // Размер фланца, который не должен искажаться

                // Получаем указатель на текстуру из вашего класса-обертки
                SDL_Texture* rawTex = gSharedTextures[texKey].getTexture();

                // Рисуем с сохранением пропорций краев
                SDL_RenderTexture9Grid(gRenderer, rawTex, nullptr,
                                       flange, 0, flange, 0, // Отступы слева/справа
                                       1.0f, &img.rect);
            }
           else {
         gSharedTextures[texKey].render(0, const_cast<SDL_FRect*>(&img.rect), nullptr, 0.0, nullptr, SDL_FLIP_NONE);}
        }
    }*/

    // 3. ОТРИСОВКА ДАННЫХ МК (TXT:)
    // Обновляем строки из сетевой структуры (под мьютексом)
    update_interface_values();

    sdlcolor = {255, 255, 255, 255}; // Белый для текста

    // 2. ОТРИСОВКА ГРАФИКИ (IMG:)
    // Проходим по всем объектам из конфига
    int count=0;
   for (auto const& [objName, rect] : gTextConfig) {
              // Рисуем, передавая прямоугольник конкретного объекта
            tempCTexture.loadFromRenderedText(vstrValueMC[count], sdlcolor);
            tempCTexture.render(0, &gTextConfig[objName], nullptr, 0.0, nullptr, SDL_FLIP_NONE);
            count++;
    }

    // Пример вывода температуры в конкретную зону из конфига
   /* if (gTextConfig.count("temp_label")) {
        // Берем уже готовую строку из vstrValueMC[0]
        tempCTexture.loadFromRenderedText(vstrValueMC[0], sdlcolor);
        tempCTexture.render(0, &gTextConfig["temp_label"], nullptr, 0.0, nullptr, SDL_FLIP_NONE);
    }*/

   // 1. Получаем строку (либо из системы, либо из Modbus)
   std::string currentTime = get_display_time();

   // 2. Загружаем в текстуру
   tempCTexture.loadFromRenderedText(currentTime, sdlcolor);
   // 2. Создаем прямоугольник отрисовки
   // x = 10, y = 10, ширину и высоту берем из самой текстуры
   SDL_FRect textRect = { 10.0f, 10.0f, (float)tempCTexture.getWidth(), (float)tempCTexture.getHeight() };

   // 3. Вызываем рендер
   // Первый аргумент (xy) у вас 0, второй — адрес нашего прямоугольника
   tempCTexture.render(0, &textRect);
    // 4. ОТРИСОВКА АЛЕРТОВ (TXT:)
    // Используем ваш метод render_alerts, но адаптированный под gTextAlert
    for (size_t i = 0; i < vstrAlert.size(); i++) {
        std::string key = "alert_line_" + std::to_string(i);
        if (gTextAlert.count(key)) {
            tempCTexture.loadFromRenderedText(vstrAlert[i], sdlcolor);
            tempCTexture.render(0, &gTextAlert[key], nullptr, 0.0, nullptr, SDL_FLIP_NONE);
        }
    }

    pressureBar.draw(gRenderer, shared_sensor_data.temperature); // Отрисовка бара

    DrawControlPopup();

    if (editMode) {
        // 1. Устанавливаем цвет рамки (например, ярко-зеленый)
        // RGBA: 0, 255, 0, 255
        SDL_SetRenderDrawColor(gRenderer, 0, 255, 0, 255);

        // 2. Рисуем рамки для всех графических объектов
        for (auto& [name, element] : gSceneElements) {
            SDL_RenderRect(gRenderer, &element.rect);
        }

        // 3. Рисуем рамки для текстовых зон (другим цветом, например, желтым)
        SDL_SetRenderDrawColor(gRenderer, 255, 255, 0, 255);
        for (auto& [name, rect] : gTextConfig) {
            SDL_RenderRect(gRenderer, &rect);
        }

        // 4. Подсветим КРАСНЫМ тот объект, который мы сейчас тащим
        if (selectedRect != nullptr) {
            SDL_SetRenderDrawColor(gRenderer, 255, 0, 0, 255);
            // Рисуем рамку чуть толще (просто рисуем две с небольшим смещением)
            SDL_RenderRect(gRenderer, selectedRect);

            SDL_FRect boldRect = { selectedRect->x - 1, selectedRect->y - 1, selectedRect->w + 2, selectedRect->h + 2 };
            SDL_RenderRect(gRenderer, &boldRect);
        }
    }


    // 5. Вывод на экран
    SDL_RenderPresent(gRenderer);
}
/////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////

void desctop2()
{
   /* std::string sstr {"sstr.cstr"};
    fonClockRect.x=1490;
    fonClockRect.y=5;
    fonClockRect.w=400;
    fonClockRect.h=50;

    SDL_SetRenderDrawColor( gRenderer, 0xFF, 0xDF, 0xFF, 0xFF ); //8DCAFF 8E88E1 00b3ff Цвет фона
    SDL_RenderClear( gRenderer ); //Очистить текущую цель рендеринга с помощью цвета рисования.

    SDL_RenderTextureRotated(gRenderer,ClockFon,NULL,&fonClockRect,0,NULL,SDL_FLIP_NONE);
    SDL_SetRenderDrawColor(gRenderer,0x76,0x70,0x70,0xFF); //Цвет фона


    SDL_Color textColor = {0, 0, 0, 0xFF};
    TestSurface = TTF_RenderText_Solid(gFont, sstr.c_str(), sstr.size(), textColor);
    TestTexture = SDL_CreateTextureFromSurface(gRenderer, TestSurface);
    SDL_DestroySurface(TestSurface);
    TestSurface=nullptr;


        //SDL_QueryTexture(TestTexture, NULL, NULL, &w, &h);
        vRectClipCoordAlertMessage.at(0).w = 80;
        vRectClipCoordAlertMessage.at(0).h = 24;

     SDL_RenderTextureRotated(gRenderer, TestTexture, NULL, &vRectClipCoordAlertMessage.at(0), 0, NULL, SDL_FLIP_NONE);




    //Обновить экран
    SDL_RenderPresent( gRenderer );
     SDL_DestroyTexture(TestTexture);*/
};
