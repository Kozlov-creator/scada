#include <map>
#include <SDL3/SDL.h>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <texture_class.h>
#include <iostream>
#include <main.h>


extern std::string destIP;
extern int destPort;
extern int localPort;

// Глобальный вектор для хранения строк, которые мы не умеем парсить (например, сетевые настройки)
std::vector<std::string> gUnknownConfigLines;

extern bool editMode;//Реализация через флаг Edit Mode. В режиме работы (Runtime) перетаскивание должно быть запрещено, чтобы оператор случайно не «унес» насос с экрана.

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
    if (!file.is_open()) {
        std::cerr << "Ошибка: не удалось открыть файл конфигурации: " << file_name << std::endl;
        return;
    }

    gUnknownConfigLines.clear(); // Очищаем перед загрузкой
    std::string line;

    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#') {
            gUnknownConfigLines.push_back(line); // Сохраняем комментарии и пустые строки
            continue;
        }

        std::istringstream iss(line);
        std::string type;

         if (!(iss >> type)) continue; // Считываем ТИП один раз

            if (type == "NET:") {
                std::string netParam;
                if (iss >> netParam) {
                    if (netParam == "DEST_IP:") {
                        iss >> destIP;
                        std::cout << "UDP Dest IP: " << destIP << std::endl;
                    }
                    else if (netParam == "DEST_PORT:") {
                        iss >> destPort;
                        std::cout << "UDP Dest Port: " << destPort << std::endl;
                    }
                    else if (netParam == "LOCAL_PORT:") {
                        iss >> localPort;
                        std::cout << "UDP Local Port: " << localPort << std::endl;
                    }
                }
                 gUnknownConfigLines.push_back(line); // Сохраняем, чтобы не потерять при записи
                continue; // Переходим к следующей строке
            }

            // 1. Проверяем одиночные параметры (без имени объекта)
            //Режим редактирования
            if (type == "EDIT_MODE:") {
                int mode;
                if (iss >> mode) {
                    editMode = (mode != 0);
                    std::cout << "Режим редактирования: " << (editMode ? "ВКЛ" : "ВЫКЛ") << std::endl;
                }
                // НЕ используем return, идем дальше
                continue;
            }

          std::string name;
        // Читаем тип (IMG: или TXT:) и имя объекта
        if (iss >> name) {
            // Убираем двоеточие из имени "clock_fon:" -> "clock_fon"
            if (!name.empty() && name.back() == ':') {
                name.pop_back();
            }

            float x, y, w, h;
            if (type == "IMG:") {
                std::string fileName; // Имя файла (без .png)
                if (iss >> fileName >> x >> y) {
                    // 1. Сохраняем объект в карту сцены
                    gSceneElements[name] = { fileName, {x, y, 0.0f, 0.0f} };
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
            else {
                gUnknownConfigLines.push_back(line);
            }

        }
        else {
            // Если тип неизвестен (например, "NET:"), сохраняем строку целиком
            gUnknownConfigLines.push_back(line);
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

    // 1. Сначала записываем всё, что мы не трогали (включая NET: настройки и комментарии)
    for (const auto& unknownLine : gUnknownConfigLines) {
        if (!unknownLine.empty()) {
            file << unknownLine << "\n";
        }
    }

    file << "\n# --- Auto-saved Dynamic Elements ---\n";

    // 2. Сохраняем актуальный EDIT_MODE
    file << "EDIT_MODE: " << (editMode ? "1" : "0") << "\n\n";

    // 1. Сохраняем ГРАФИЧЕСКИЕ ОБЪЕКТЫ (IMG:)
    for (auto const& [objName, element] : gSceneElements) {
        file << "IMG: " << objName << ": "
        << element.textureKey << " "
        << (int)element.rect.x << " "
        << (int)element.rect.y << "\n";
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
