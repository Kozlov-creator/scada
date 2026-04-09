#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include "types.h"

class SceneManager {
public:
    // Основные методы
    bool loadConfig(const std::string& path);
    void saveConfig(const std::string& path);
    void refreshRenderOrder();

    // Поиск объекта под мышкой (с учетом слоев)
    SDL_FRect* findElementAt(float x, float y, std::string& outName);

    // Доступ к текстовым конфигам
    std::unordered_map<std::string, TextElement>& getTextConfig() { return m_textConfig; }

    // Доступ к данным
    std::unordered_map<std::string, SceneElement>& getElements() { return m_elements; }
    std::vector<RenderItem>& getRenderOrder() { return m_renderOrder; }

private:
    std::unordered_map<std::string, SceneElement> m_elements;
    std::vector<RenderItem> m_renderOrder;
    std::vector<std::string> m_unknownLines; // Для комментариев и NET: настроек
    std::unordered_map<std::string, TextElement> m_textConfig;
};
