#include <fstream>
#include <sstream>
#include <algorithm>
#include <iostream>
#include "globals.h"

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
            else if (type == "TXT_MK:") {
                float x, y, w, h;
                if (iss >> x >> y >> w >> h) m_textConfig[name] = { x, y, w, h };
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
    for (auto& [name, rect] : m_textConfig) {
        if (SDL_PointInRectFloat(&p, &rect)) {
            outName = name;
            return &rect; // Возвращаем указатель на рект текста
        }
    }
    return nullptr;
}
