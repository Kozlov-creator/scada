#include <map>
#include <SDL3/SDL.h>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <texture_class.h>
#include <iostream>
#include <main.h>

// 1. Хранилище уникальных текстур (Путь к файлу -> Объект класса)
extern std::map<std::string, CTexture> gSharedTextures;
// Глобальное хранилище координат
// 2. ОБЪЕКТЫ: Описание элементов на экране
struct ScadaElement {
    std::string textureKey; // Имя текстуры из gSharedTextures
    SDL_FRect rect;         // Координаты на экране
};

// Карта всех объектов (Ключ: "pump_left", "bg_main" и т.д.)
extern std::map<std::string, ScadaElement> gSceneElements;
// Хранилище для текста (Имя поля: Координаты)
std::map<std::string, SDL_FRect> gTextConfig;
std::map<std::string, SDL_FRect> gTextAlert;

////////////////////////////////////////////////////////////
//Функция парсера (Безопасная) Эта функция проигнорирует пустые строки и комментарии, корректно распарсив данные.Универсальный парсер Обновим функцию чтения, чтобы она понимала, куда записывать данные.
void read_layout_config(const std::string& file_name) {
    std::ifstream file(file_name);
    std::string line;

    if (!file.is_open()) {
        std::cerr << "Ошибка: не удалось открыть файл конфигурации: " << file_name << std::endl;
        return;
    }

    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#') continue;

        std::istringstream iss(line);
        std::string type, name;

        // Читаем тип (IMG: или TXT:) и имя объекта
        if (iss >> type >> name) {
            // Убираем двоеточие из имени "clock_fon:" -> "clock_fon"
            if (!name.empty() && name.back() == ':') {
                name.pop_back();
            }

            float x, y, w, h;
            if (type == "IMG:") {
                std::string fileName; // Имя файла (без .png)
                if (iss >> fileName >> x >> y >> w >> h) {
                    // 1. Сохраняем объект в карту сцены
                    gSceneElements[name] = { fileName, {x, y, w, h} };
                }
            }

            else if (type == "TXT_ALRT:") {
                if (iss >> x >> y >> w >> h) {
                    gTextAlert[name] = { x, y, w, h };
                }
            }

            else if (type == "TXT_MK:") {
                if (iss >> x >> y >> w >> h) {
                    gTextConfig[name] = { x, y, w, h };
                }
            }
        }
    }
    file.close();
}
//--------------------------------------------------------------------------------------------
//Функция сохранения save_layout_config. Эта функция перезаписывает файл, сохраняя актуальные координаты всех объектов, которые вы передвинули мышкой.
void save_layout_config(const std::string& file_name) {
    std::ofstream file(file_name);
    if (!file.is_open()) {
        std::cerr << "Ошибка: не удалось открыть файл для сохранения: " << file_name << std::endl;
        return;
    }

    file << "# SCADA Layout Configuration (Auto-saved)\n";
    file << "# Формат: ТИП ИМЯ_ОБЪЕКТА: [ИМЯ_ФАЙЛА] X Y W H\n\n";

    // 1. Сохраняем ГРАФИЧЕСКИЕ ОБЪЕКТЫ (IMG:)
    for (auto const& [objName, element] : gSceneElements) {
        file << "IMG: " << objName << ": "
        << element.textureKey << " "
        << (int)element.rect.x << " "
        << (int)element.rect.y << " "
        << (int)element.rect.w << " "
        << (int)element.rect.h << "\n";
    }

    file << "\n"; // Разделитель секций

    // 2. Сохраняем ТЕКСТОВЫЕ ЗОНЫ (TXT:)
    for (auto const& [textName, rect] : gTextConfig) {
        file << "TXT_MK: " << textName << ": "
        << (int)rect.x << " "
        << (int)rect.y << " "
        << (int)rect.w << " "
        << (int)rect.h << "\n";
    }

    // 2. Сохраняем ТЕКСТОВЫЕ ЗОНЫ (TXT:)
    for (auto const& [textName, rect] : gTextAlert) {
        file << "TXT_ALRT: " << textName << ": "
        << (int)rect.x << " "
        << (int)rect.y << " "
        << (int)rect.w << " "
        << (int)rect.h << "\n";
    }

    file.close();
}
//-----------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------
void InputCoord()
{
    read_layout_config(IMAGES_CONF);
}
