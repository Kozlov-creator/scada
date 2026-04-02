#include <map>
#include <fstream>
#include <iostream>
#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <vector>
#include <texture_class.h>
#include <string>
#include <chrono>
#include <unordered_map>

#include "globals.h"
#include "functions.h"


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


//----------------------------------------------------------------------
//Функция отрисовки DrawControlPopup. Эту функцию нужно вызывать в самом конце desctop1(), чтобы окно рисовалось поверх всех остальных элементов.
void DrawControlPopup() {

    CTexture tempCTexture;

    if (!Scada::showControlWindow) return;

    // Затеняем задний план (полупрозрачный черный фон)
    SDL_SetRenderDrawBlendMode(Scada::gRenderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(Scada::gRenderer, 0, 0, 0, 150);
    SDL_FRect fullScreen = { 0, 0, 1920, 1080 };
    SDL_RenderFillRect(Scada::gRenderer, &fullScreen);

    // Рисуем основное тело окна
    SDL_SetRenderDrawColor(Scada::gRenderer, 50, 50, 50, 255); // Темно-серый
    SDL_RenderFillRect(Scada::gRenderer, &Scada::controlWindowRect);
    SDL_SetRenderDrawColor(Scada::gRenderer, 200, 200, 200, 255); // Рамка
    SDL_RenderRect(Scada::gRenderer, &Scada::controlWindowRect);

    // Заголовок окна
    std::string title = "УПРАВЛЕНИЕ: " + Scada::activeControlObject;
    tempCTexture.loadFromRenderedText(title, {255, 255, 255, 255});
    SDL_FRect titleRect = { Scada::controlWindowRect.x + 20, Scada::controlWindowRect.y + 20, 400, 40 };
    tempCTexture.render(0, &titleRect, nullptr, 0.0, nullptr, SDL_FLIP_NONE);

    // Кнопка "ВКЛЮЧИТЬ" (Зеленая)
    SDL_FRect btnOn = { Scada::controlWindowRect.x + 50, Scada::controlWindowRect.y + 150, 150, 80 };
    SDL_SetRenderDrawColor(Scada::gRenderer, 0, 150, 0, 255);
    SDL_RenderFillRect(Scada::gRenderer, &btnOn);
    tempCTexture.loadFromRenderedText("ПУСК", {255, 255, 255, 255});
    tempCTexture.render(0, &btnOn, nullptr, 0.0, nullptr, SDL_FLIP_NONE);

    // Кнопка "ВЫКЛЮЧИТЬ" (Красная)
    SDL_FRect btnOff = { Scada::controlWindowRect.x + 300, Scada::controlWindowRect.y + 150, 150, 80 };
    SDL_SetRenderDrawColor(Scada::gRenderer, 150, 0, 0, 255);
    SDL_RenderFillRect(Scada::gRenderer, &btnOff);
    tempCTexture.loadFromRenderedText("СТОП", {255, 255, 255, 255});
    tempCTexture.render(0, &btnOff, nullptr, 0.0, nullptr, SDL_FLIP_NONE);

    // Кнопка "ЗАКРЫТЬ" (Маленький крестик в углу)
    SDL_FRect btnClose = { Scada::controlWindowRect.x + Scada::controlWindowRect.w - 40, Scada::controlWindowRect.y + 10, 30, 30 };
    SDL_SetRenderDrawColor(Scada::gRenderer, 200, 0, 0, 255);
    SDL_RenderFillRect(Scada::gRenderer, &btnClose);
}


void read_image_config(const std::string);
/////////////////////////////////////////////////////////////////////

// Подготовка данных для вывода. Добавил обновление вектора строк Scada::vstrValueMC данными из сетевой структуры. Это нужно делать до начала цикла отрисовки или внутри него, защитив мьютексом.
void update_interface_values() {
    SDL_LockMutex(Scada::data_mutex);
    // Преобразуем сырые данные в строки для отображения
    // Допустим, Scada::vstrValueMC[0] - температура, [1] - влажность и т.д.
    Scada::vstrValueMC.at(0) = "Temp: " + std::to_string(Scada::shared_sensor_data.temperature) + " C";
    Scada::vstrValueMC.at(1) = "Hum:  " + std::to_string(Scada::shared_sensor_data.humidity) + " %";
    Scada::vstrValueMC.at(2) = "AccX: " + std::to_string(Scada::shared_sensor_data.accel_x);
    Scada::vstrValueMC.at(3) = "ID:   " + std::to_string(Scada::shared_sensor_data.packet_id);
    SDL_UnlockMutex(Scada::data_mutex);
}

/////////////////////////////////////////////////////////////////////
void desctop1()
{
    ProgressBar pressureBar(100.0f, 150.0f, 200.0f, 30.0f);
    SDL_Color sdlcolor;
    CTexture tempCTexture;
    // Очистка экрана
    SDL_SetRenderDrawColor(Scada::gRenderer, 129, 191, 254, 255);
    SDL_RenderClear(Scada::gRenderer);

    // ОТРИСОВКА ГРАФИКИ (IMG:)
    // Проходим по всем объектам из конфига

    // ЕДИНЫЙ ЦИКЛ ОТРИСОВКИ ГРАФИКИ
    for (auto& item : App::scene.getRenderOrder()) {
        SceneElement* el = item.element;
        std::string texKey = el->textureKey;

        if (item.name.find("truba") != std::string::npos) {
            // Рисуем как растягиваемую трубу (9-grid)
            float flange = 4.0f;
            SDL_RenderTexture9Grid(Scada::gRenderer, Scada::gSharedTextures[texKey].getTexture(),
                                   nullptr, flange, 0, flange, 0, 1.0f, &el->rect);
        } else {
            // Обычная отрисовка текстуры
            Scada::gSharedTextures[el->textureKey].render(0, &el->rect);
        }
    }

    // ОТРИСОВКА ДАННЫХ МК (TXT:)
    // Обновляем строки из сетевой структуры (под мьютексом)
    update_interface_values();

    sdlcolor = {255, 255, 255, 255}; // Белый для текста

    // ОТРИСОВКА ГРАФИКИ (IMG:)
    // Проходим по всем объектам из конфига
    int count=0;
   for (auto const& [objName, rect] : App::scene.getTextConfig()) {
              // Рисуем, передавая прямоугольник конкретного объекта
            tempCTexture.loadFromRenderedText(Scada::vstrValueMC[count], sdlcolor);
            tempCTexture.render(0, &App::scene.getTextConfig()[objName], nullptr, 0.0, nullptr, SDL_FLIP_NONE);
            count++;
    }


   //  Получаем строку (либо из системы, либо из Modbus)
   std::string currentTime = get_display_time();

   //  Загружаем в текстуру
   tempCTexture.loadFromRenderedText(currentTime, sdlcolor);
   //  Создаем прямоугольник отрисовки
   // x = 10, y = 10, ширину и высоту берем из самой текстуры
   SDL_FRect textRect = { 10.0f, 10.0f, (float)tempCTexture.getWidth(), (float)tempCTexture.getHeight() };

   // Вызываем рендер
   // Первый аргумент (xy) 0, второй — адрес прямоугольника
   tempCTexture.render(0, &textRect);
    // ОТРИСОВКА АЛЕРТОВ (TXT:)
    // Используем метод render_alerts, но адаптированный под Scada::gTextAlert
    for (size_t i = 0; i < Scada::vstrAlert.size(); i++) {
        std::string key = "alert_line_" + std::to_string(i);
        if (Scada::gTextAlert.count(key)) {
            tempCTexture.loadFromRenderedText(Scada::vstrAlert[i], sdlcolor);
            tempCTexture.render(0, &Scada::gTextAlert[key], nullptr, 0.0, nullptr, SDL_FLIP_NONE);
        }
    }

    pressureBar.draw(Scada::gRenderer, Scada::shared_sensor_data.temperature); // Отрисовка бара

    DrawControlPopup();

    if (Scada::editMode) {
        // Устанавливаем цвет рамки (ярко-зеленый)
        // RGBA: 0, 255, 0, 255
        SDL_SetRenderDrawColor(Scada::gRenderer, 0, 255, 0, 255);

        // Рисуем рамки для всех графических объектов
        for (auto& [name, element] : App::scene.getElements()) {
            SDL_RenderRect(Scada::gRenderer, &element.rect);
        }

        // Рисуем рамки для текстовых зон (другим цветом, желтым)
        SDL_SetRenderDrawColor(Scada::gRenderer, 255, 255, 0, 255);
        for (auto& [name, rect] : App::scene.getTextConfig()) {
            SDL_RenderRect(Scada::gRenderer, &rect);
        }

        // Подсветим КРАСНЫМ тот объект, который сейчас тащим
        if (Scada::selectedRect != nullptr) {
            SDL_SetRenderDrawColor(Scada::gRenderer, 255, 0, 0, 255);
            // Рисуем рамку чуть толще (просто рисуем две с небольшим смещением)
            SDL_RenderRect(Scada::gRenderer, Scada::selectedRect);

            SDL_FRect boldRect = { Scada::selectedRect->x - 1, Scada::selectedRect->y - 1, Scada::selectedRect->w + 2, Scada::selectedRect->h + 2 };
            SDL_RenderRect(Scada::gRenderer, &boldRect);
        }
    }

    // Вывод на экран
    SDL_RenderPresent(Scada::gRenderer);
}
/////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////

void desctop2()
{
   /*
    //Обновить экран
    SDL_RenderPresent( Scada::gRenderer );
    */
};
