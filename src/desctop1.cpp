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

#include <iomanip>
#include <sstream>

std::string format_timestamp(uint32_t ms) {
    uint32_t seconds = ms / 1000;
    uint32_t h = (seconds / 3600) % 24;
    uint32_t m = (seconds / 60) % 60;
    uint32_t s = seconds % 60;

    std::ostringstream oss;
    oss << "[" << std::setfill('0') << std::setw(2) << h << ":"
    << std::setw(2) << m << ":" << std::setw(2) << s << "] ";
    return oss.str();
}

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
//======================================================================
/////////////////////////////////////////////////////////////////////
//Отрисовка фоновой сетки Чтобы оператор видел, куда "прилипает" объект, нарисуем точки или линии на заднем фоне. Это нужно делать в самом начале цикла отрисовки сцены (до объектов).

void render_grid() {
    if (!Scada::editMode) return; // Рисуем сетку только в режиме редактирования

    SDL_SetRenderDrawColor(Scada::gRenderer, 50, 50, 50, 255); // Темно-серый цвет
    int step = 10;
    int w, h;
    SDL_GetRenderOutputSize(Scada::gRenderer, &w, &h);

    for (int x = 0; x < w; x += step) {
        for (int y = 0; y < h; y += step) {
            SDL_RenderPoint(Scada::gRenderer, (float)x, (float)y);
        }
    }
}
//======================================================================
//Функция синхронизации (Data Merger), которая собирает данные из глобальной структуры и карты Modbus в одну общую карту. Вызываем её перед логикой алармов и отрисовки.
void sync_all_tags() {
    SDL_LockMutex(Scada::data_mutex); // Защищаем UDP структуру
    SDL_LockMutex(Scada::modbus_mutex); // Защищаем Modbus карту

    // 1. Забираем данные из глобальной структуры SensorData (UDP)
    // Имена ключей ("UDP_Temp") должны совпадать с именами в вашем файле конфига
    Scada::gLiveTags["UDP_Temp"] = std::to_string(Scada::shared_sensor_data.temperature);
    Scada::gLiveTags["UDP_Hum"]  = std::to_string(Scada::shared_sensor_data.humidity);
    Scada::gLiveTags["UDP_AccX"] = std::to_string(Scada::shared_sensor_data.accel_x);
    Scada::gLiveTags["UDP_ID"]   = std::to_string(Scada::shared_sensor_data.packet_id);

    // 2. Копируем всё из Modbus
   // for (auto const& [reg, val] : Scada::gModbusData) {
   //     Scada::gLiveTags[reg] = val;
   // }

    SDL_UnlockMutex(Scada::modbus_mutex);
    SDL_UnlockMutex(Scada::data_mutex);
}
//=========================================================================================
// Реализация мигания (Blinking)Мигание в SDL делается через проверку текущего времени системы. Если остаток от деления времени на период (например, 500 мс) меньше половины периода — рисуем текст, иначе — нет (или рисуем другим цветом).
bool is_blink_on() {
    // Меняет состояние каждые 500 мс
    return (SDL_GetTicks() % 1000) < 500;
}

//=========================================================================================
//Логика «Сдвига» (Push & Shift)Когда происходит новое событие, мы добавляем его в начало списка, а самый старый аларм удаляем. Таким образом, новые алармы всегда будут на «линии 0», а старые будут «уходить» вверх (или вниз, смотря как назначите индексы).
void add_to_alarm_log(std::string message) {
    SDL_LockMutex(Scada::alarm_mutex);

    // Вставляем новый аларм в начало (индекс 0)
    Scada::gAlarmMessages.insert(Scada::gAlarmMessages.begin(), message);

    // Если алармов больше, чем зон в конфиге (у вас их 4) — удаляем лишний
    if (Scada::gAlarmMessages.size() > 4) {
        Scada::gAlarmMessages.pop_back();
    }

    SDL_UnlockMutex(Scada::alarm_mutex);
}

//=========================================================================================
//Рендеринг бегущей строки
void render_alarm_log() {
      CTexture tempCTexture;
    SDL_LockMutex(Scada::alarm_mutex);

    for (int i = 0; i < Scada::gAlarmMessages.size(); ++i) {
        std::string zoneName = "alert_line_" + std::to_string(i);

        if (Scada::gTextAlert.count(zoneName)) {
            SDL_FRect rect = Scada::gTextAlert[zoneName];

            // Логика мигания: только для самой свежей аварии (индекс 0)
            // или для всех, если они не подтверждены
            if (i == 0 && !is_blink_on()) {
                continue; // Пропускаем отрисовку этого кадра (эффект исчезновения)
            }

            SDL_Color alarmColor = {255, 0, 0, 255}; // Красный

            tempCTexture.loadFromRenderedText(Scada::gAlarmMessages[i], alarmColor);
            tempCTexture.render(0, &rect);
        }
    }

    SDL_UnlockMutex(Scada::alarm_mutex);
}
//=================================================================================================
// Подготовка данных для вывода. Добавил обновление вектора строк Scada::vstrValueMC данными из сетевой структуры. Это нужно делать до начала цикла отрисовки или внутри него, защитив мьютексом.
void update_interface_values() {
     CTexture tempCTexture;
     SDL_Color sdlcolor = {255, 255, 255, 255}; // Белый для текста

     sync_all_tags(); // Сначала объединяем

     // Теперь просто бежим по конфигу и ищем совпадения имен
     for (auto& [name, element] : App::scene.getTextConfig()) {
          std::string displayStr = "0";
         if (Scada::gLiveTags.count(name)) {
             displayStr = Scada::gLiveTags[name];

             // Алармы (теперь работают для обоих контроллеров одинаково)
             float val = std::stof(displayStr);

             if (name == "7078") { if (val == 1.0f ){ displayStr = "УЧЁТ";} else displayStr = "НЕ УЧЁТ";}

             if (val > element.alarmHigh && !element.isAlarmed) {// Только в момент перехода в аварию
                 element.isAlarmed = true;

                 // Формируем строку: [Время] Имя: Значение > Порог
                 std::string timeStr = format_timestamp(Scada::shared_sensor_data.timestamp);
                 std::string msg = timeStr + name + ": " + displayStr + " > " + std::to_string(element.alarmHigh);

                 add_to_alarm_log(msg); // Функция с insert(begin), которую мы писали ранее
             }
              element.isAlarmed = (val > element.alarmHigh);

             // 2. Добавляем суффикс на основе unitType
             if (element.unitType == "M3H")       displayStr += " m3/h";
             else if (element.unitType == "TEMP") displayStr += " C";
             else if (element.unitType == "PERC") displayStr += " %";
             else if (element.unitType == "TH") displayStr += " t/h";
             else if (element.unitType == "PRES") displayStr += " MPa";
             else if (element.unitType == "DENS") displayStr += " kg/m3";
             else if (element.unitType == "NONE") displayStr += "      "; //чтобы можно было перемещать

         }

         if (element.isAlarmed) {
             sdlcolor = {255, 0, 0, 255}; // Красный при аварии
         } else { sdlcolor = {255, 255, 255, 255};} // Белый для текста


         // 3. Рендерим, обращаясь к .rect внутри структуры
         tempCTexture.loadFromRenderedText(displayStr, sdlcolor);
         tempCTexture.render(0, &element.rect, nullptr, 0.0, nullptr, SDL_FLIP_NONE);

         // НОВАЯ ЛОГИКА: Отображение имени тега при перемещении
         // Если этот объект сейчас выбран в редакторе
         if (Scada::editMode || Scada::selectedRect == &element.rect) {
             SDL_Color yellow = {255, 255, 0, 180}; // Желтый полупрозрачный
             render_grid();
             // Загружаем имя тега (например, "7802" или "UDP_Temp")
             tempCTexture.loadFromRenderedText("[" + name + "]", yellow);

             // Рисуем имя чуть выше самого объекта
             SDL_FRect nameRect = {
                 element.rect.x,
                 element.rect.y - 20.0f, // Смещение вверх
                 (float)tempCTexture.getWidth(),
                 15.0f
             };
             tempCTexture.render(0, &nameRect);

             // Опционально: рисуем тонкую рамку вокруг перемещаемого объекта
             SDL_SetRenderDrawColor(Scada::gRenderer, 255, 255, 0, 255);
             SDL_RenderRect(Scada::gRenderer, &element.rect);
         }
     }
    /* for (auto& [objName, element] : App::scene.getTextConfig()) {
        // 1. Получаем значение из данных (Modbus или UDP)
        std::string displayStr = "0";
        if (Scada::gLiveTags.count(objName)) {
            float val = std::stof(Scada::gLiveTags[objName]); // Преобразуем строку в число

            // Проверка порога
            if (val > element.alarmHigh) {
                if (!element.isAlarmed) {
                    element.isAlarmed = true;
                    // Здесь можно записать в лог: "Авария: Тег такой-то превышен!"
                }
            } else {
                element.isAlarmed = false;
            }
            displayStr = Scada::gModbusData[objName];
        }   else if (objName == "UDP_temp") {  displayStr = std::to_string(Scada::shared_sensor_data.temperature);  }
            else if (objName == "UDP_hum") { displayStr = std::to_string(Scada::shared_sensor_data.humidity);  }
            else if (objName == "UDP_acel") { displayStr = std::to_string(Scada::shared_sensor_data.accel_x);   }
            else if (objName == "UDP_ID") { displayStr = std::to_string(Scada::shared_sensor_data.packet_id); }


        // 2. Добавляем суффикс на основе unitType
        if (element.unitType == "M3H")       displayStr += " m3/h";
        else if (element.unitType == "TEMP") displayStr += " C";
        else if (element.unitType == "PERC") displayStr += " %";




    }

    SDL_UnlockMutex(Scada::modbus_mutex);
    SDL_UnlockMutex(Scada::data_mutex);*/
}

//========================================================================================================
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
   /* for (size_t i = 0; i < Scada::vstrAlert.size(); i++) {
        std::string key = "alert_line_" + std::to_string(i);
        if (Scada::gTextAlert.count(key)) {
            tempCTexture.loadFromRenderedText(Scada::vstrAlert[i], sdlcolor);
            tempCTexture.render(0, &Scada::gTextAlert[key], nullptr, 0.0, nullptr, SDL_FLIP_NONE);
        }
    }*/
   render_alarm_log();
   // pressureBar.draw(Scada::gRenderer, Scada::shared_sensor_data.temperature); // Отрисовка бара

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
        for (auto& [name, element] : App::scene.getTextConfig()) {
            SDL_RenderRect(Scada::gRenderer, &element.rect);
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
