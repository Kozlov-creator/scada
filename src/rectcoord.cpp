#include <map>
#include <SDL3/SDL.h>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <texture_class.h>
#include <iostream>
#include <main.h>
#include <filesystem>
#include <algorithm>
#include <unordered_map>

namespace fs = std::filesystem;

extern std::string destIP;
extern int destPort;
extern int localPort;

bool gUseSystemTime = false; // Глобальный флаг
// Глобальный вектор для хранения строк, которые мы не умеем парсить (например, сетевые настройки)
std::vector<std::string> gUnknownConfigLines;

extern bool editMode;//Реализация через флаг Edit Mode. В режиме работы (Runtime) перетаскивание должно быть запрещено, чтобы оператор случайно не «унес» насос с экрана.

// 1. Хранилище уникальных текстур (Путь к файлу -> Объект класса)
extern std::map<std::string, CTexture> gSharedTextures;
// Глобальное хранилище координат
// 2. ОБЪЕКТЫ: Описание элементов на экране

extern std::unordered_map<std::string, SceneElement> gSceneElements;

std::vector<RenderItem> gRenderOrder; // Список в порядке слоев

// Хранилище для текста (Имя поля: Координаты)
std::map<std::string, SDL_FRect> gTextConfig;
std::map<std::string, SDL_FRect> gTextAlert;
//////////////////////////////////////////////////////////////////////////////////////////////////////////////
//Функция сортировки (вызывается 1 раз) Создайте функцию, которая собирает всё в один список и сортирует:
void refresh_render_order() {
    gRenderOrder.clear();

    // Если у вас один общий map gSceneElements:
    for (auto& pair : gSceneElements) {
        gRenderOrder.emplace_back(pair.first, &pair.second);
    }

    // Сортировка по слою (используем поле прямо из структуры объекта)
    std::sort(gRenderOrder.begin(), gRenderOrder.end(), [](const RenderItem& a, const RenderItem& b) {
        return a.element->layer < b.element->layer;
    });
}

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
            gUnknownConfigLines.push_back(line);
            continue;
        }

        std::istringstream iss(line);
        std::string type;
        if (!(iss >> type)) continue;

        // Обработка сетевых настроек и режима редактирования (без изменений)
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
        if (type == "EDIT_MODE:") { /* ... ваш код ... */ continue; }
        // Внутри цикла while в парсере:
        if (type == "USE_SYSTEM_TIME:") {
            int val;
            if (iss >> val) gUseSystemTime = (val != 0);
            continue;
        }

        std::string name;
        if (iss >> name) {
            if (!name.empty() && name.back() == ':') name.pop_back();

            // 1. ОБЪЕДИНЕННАЯ ОБРАБОТКА ГРАФИКИ (SVG и старые PNG)
            if (type == "IMG:") {
                std::string fileName;
                float x = 0, y = 0, w = 0, h = 0;
                int layer = 0;

                // Считываем имя файла и координаты
                if (iss >> fileName >> x >> y) {
                    // Пробуем считать дополнительные параметры, если они есть
                    if (!(iss >> w >> h >> layer)) {
                        // Если их нет в строке — используем значения по умолчанию
                        w = 0.0f;
                        h = 0.0f;
                        layer = 0;
                    }

                    // Записываем всё в единую карту объектов
                    gSceneElements[name] = { fileName, {x, y, w, h}, layer };
                }
            }
            // 2. ТЕКСТОВЫЕ ЗОНЫ
            else if (type == "TXT_ALRT:") {
                float x, y, w, h;
                if (iss >> x >> y >> w >> h) gTextAlert[name] = { x, y, w, h };
            }
            else if (type == "TXT_MK:") {
                float x, y, w, h;
                if (iss >> x >> y >> w >> h) gTextConfig[name] = { x, y, w, h };
            }

            else {
                gUnknownConfigLines.push_back(line);
            }


        }
    }
    file.close();

     // ПОСЛЕ закрытия файла и загрузки всех текстур:
   // refresh_render_order();
}
//--------------------------------------------------------------------------------------------
//Функция сохранения save_layout_config. Эта функция перезаписывает файл, сохраняя актуальные координаты всех объектов, которые вы передвинули мышкой.
void save_layout_config(const std::string& file_name) {

    // 1. Создаем бэкап перед началом записи
    if (fs::exists(file_name)) {
        std::string backup_name = file_name + ".bak";
        try {
            // copy_options::overwrite_existing позволяет обновлять старый бэкап
            fs::copy(file_name, backup_name, fs::copy_options::overwrite_existing);
            std::cout << "Бэкап создан: " << backup_name << std::endl;
        } catch (const fs::filesystem_error& e) {
            std::cerr << "Критическая ошибка при создании бэкапа: " << e.what() << std::endl;
            return; // Прекращаем, чтобы не испортить оригинал
        }
    }


      // 2. Открываем файл для записи
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

    // 2. Сохраняем актуальный USE_SYSTEM_TIME
    file << "USE_SYSTEM_TIME: " << (gUseSystemTime ? "1" : "0") << "\n\n";

    // 3. ЕДИНЫЙ ЦИКЛ ДЛЯ ВСЕХ ГРАФИЧЕСКИХ ОБЪЕКТОВ (теперь только IMG:)
    for (auto const& [objName, element] : gSceneElements) {
        file << "IMG: " << objName << ": "
        << element.textureKey << " "
        << element.rect.x << " "
        << element.rect.y << " "
        << element.rect.w << " "
        << element.rect.h << " "
        << element.layer << "\n";
    }

    file << "\n"; // Разделитель секций

    // 4. Сохраняем ТЕКСТОВЫЕ ЗОНЫ (TXT:) - без изменений
    for (auto const& [textName, rect] : gTextConfig) {
        file << "TXT_MK: " << textName << ": "
        << (int)rect.x << " " << (int)rect.y << " "
        << (int)rect.w << " " << (int)rect.h << "\n";
    }

    for (auto const& [textName, rect] : gTextAlert) {
        file << "TXT_ALRT: " << textName << ": "
        << (int)rect.x << " " << (int)rect.y << " "
        << (int)rect.w << " " << (int)rect.h << "\n";
    }

    file.close();
    std::cout << "Конфигурация успешно сохранена в формате IMG:" << std::endl;
}
//-----------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------
