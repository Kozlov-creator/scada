#include <fstream>
#include <sstream>
#include <iostream>
#include <filesystem>
#include <algorithm>
#include "globals.h"
#include <map>


namespace fs = std::filesystem;

bool SceneManager::loadConfig(const std::string& path) {
    m_elements.clear();
    m_unknownLines.clear();
    std::ifstream file(path);
    std::string line;

     if (!file.is_open()) return false;

    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#') {
            Scada::gUnknownConfigLines.push_back(line);
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
                    iss >> Scada::destIP;
                    std::cout << "UDP Dest IP: " << Scada::destIP << std::endl;
                }
                else if (netParam == "DEST_PORT:") {
                    iss >> Scada::destPort;
                    std::cout << "UDP Dest Port: " << Scada::destPort << std::endl;
                }
                else if (netParam == "LOCAL_PORT:") {
                    iss >> Scada::localPort;
                    std::cout << "UDP Local Port: " << Scada::localPort << std::endl;
                }
            }
            Scada::gUnknownConfigLines.push_back(line); // Сохраняем, чтобы не потерять при записи
            continue; // Переходим к следующей строке
        }
        if (type == "EDIT_MODE:") { /* ... ваш код ... */ continue; }
        // Внутри цикла while в парсере:
        if (type == "USE_SYSTEM_TIME:") {
            int val;
            if (iss >> val) Scada::gUseSystemTime = (val != 0);
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
                    m_elements[name] = { fileName, {x, y, w, h}, layer };
                }
            }
            // 2. ТЕКСТОВЫЕ ЗОНЫ
            else if (type == "TXT_ALRT:") {
                float x, y, w, h;
                if (iss >> x >> y >> w >> h) Scada::gTextAlert[name] = { x, y, w, h };
            }
            else if (type == "TXT_MB:" || type == "TXT_MK:") {
               float x, y, w, h;
               float alarm;
               bool isAlarm;
               std::string unit;
               int prec = 1;
               if (iss >> x >> y >> w >> h) {
                   if (!(iss >> unit)) unit = "NONE"; // Если в файле нет юнита, пишем NONE
                   if (!(iss >> alarm)) alarm = 1.0f; // Если в файле нет юнита, пишем NONE
                    if (!(iss >> isAlarm)) isAlarm = false; // Если в файле нет юнита, пишем NONE
                    if (iss >> prec) {} // Если есть в файле, считываем точность
                     // Заполняем структуру
                    m_textConfig[name] = { {x, y, w, h}, unit, alarm, isAlarm, prec };

               }
           }

            else {
                Scada::gUnknownConfigLines.push_back(line);
            }


        }
    }
    return true;
}

void SceneManager::refreshRenderOrder() {
    m_renderOrder.clear();
    for (auto& pair : m_elements) {
        m_renderOrder.emplace_back(pair.first, &pair.second);
    }
    std::sort(m_renderOrder.begin(), m_renderOrder.end(), [](const RenderItem& a, const RenderItem& b) {
        return a.element->layer < b.element->layer;
    });
}

SDL_FRect* SceneManager::findElementAt(float x, float y, std::string& outName) {
    SDL_FPoint p = {x, y};
    // Идем с конца (верхние слои первые) Сначала ищем в графике (учитывая Z-order)
    for (auto it = m_renderOrder.rbegin(); it != m_renderOrder.rend(); ++it) {
        if (SDL_PointInRectFloat(&p, &it->element->rect)) {
            outName = it->name;
            return &(it->element->rect);
        }
    }

    //Если не нашли, ищем в тексте
    for (auto& [name, element] : m_textConfig) {
        if (SDL_PointInRectFloat(&p, &element.rect)) {
            outName = name;
            return &element.rect; // Возвращаем указатель на рект текста
        }
    }
    return nullptr;
}


void SceneManager::saveConfig(const std::string& file_name) {

    // Создаем бэкап перед началом записи
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


    // Открываем файл для записи
    std::ofstream file(file_name);
    if (!file.is_open()) {
        std::cerr << "Ошибка: не удалось открыть файл для сохранения: " << file_name << std::endl;
        return;
    }

    //Сначала записываем всё, что мы не трогали (включая NET: настройки и комментарии)
    for (const auto& unknownLine : Scada::gUnknownConfigLines) {
        if (!unknownLine.empty()) {
            file << unknownLine << "\n";
        }
    }

    // Сохраняем актуальный EDIT_MODE
    file << "EDIT_MODE: " << (Scada::editMode ? "1" : "0") << "\n\n";
    // Сохраняем актуальный USE_SYSTEM_TIME
    file << "USE_SYSTEM_TIME: " << (Scada::gUseSystemTime ? "1" : "0") << "\n\n";

    file << std::fixed << std::setprecision(1); // Устанавливаем 1 знак для всех последующих float

    // --- ГРУППИРОВКА И СОРТИРОВКА IMG ---
    file << "# --- Images ---\n";
    // Копируем в std::map для автоматической сортировки по имени (ключу)
    std::map<std::string, SceneElement> sortedImages(m_elements.begin(), m_elements.end());
    for (auto const& [objName, element] : sortedImages) {
        file << "IMG: " << objName << ": " << element.textureKey << " "
        << element.rect.x << " " << element.rect.y << " "
        << element.rect.w << " " << element.rect.h << " "
        << element.layer << "\n";
    }
    // --- ГРУППИРОВКА И СОРТИРОВКА ТЕКСТА (MK и MB отдельно) ---
    std::map<std::string, TextElement> sortedMK;
    std::map<std::string, TextElement> sortedMB;

    for (auto const& [name, element] : m_textConfig) {
        if (name.find("UDP_") == 0) sortedMK[name] = element;
        else sortedMB[name] = element;
    }

    file << "\n# --- Modbus Tags ---\n";
    for (auto const& [name, element] : sortedMB) {
        file << "TXT_MB: " << name << " " << element.rect.x << " " << element.rect.y << " "
        << element.rect.w << " " << element.rect.h << " "
        << element.unitType << " " << element.alarmHigh << " "
        << element.isAlarmed << " " << element.precision << "\n";
    }

    file << "\n# --- UDP Tags ---\n";
    for (auto const& [name, element] : sortedMK) {
        file << "TXT_MK: " << name << " " << element.rect.x << " " << element.rect.y << " "
        << element.rect.w << " " << element.rect.h << " "
        << element.unitType << " " << element.alarmHigh << " "
        << element.isAlarmed << " " << element.precision << "\n";
    }

    // --- ГРУППИРОВКА И СОРТИРОВКА ALERTS ---
    file << "\n# --- Alerts ---\n";
    std::map<std::string, SDL_FRect> sortedAlerts(Scada::gTextAlert.begin(), Scada::gTextAlert.end());
    for (auto const& [textName, rect] : sortedAlerts) {
        file << "TXT_ALRT: " << textName << ": "
        << (int)rect.x << " " << (int)rect.y << " "
        << (int)rect.w << " " << (int)rect.h << "\n";
    }

    file.close();
    std::cout << "Конфигурация успешно сохранена в формате IMG:" << std::endl;
}
