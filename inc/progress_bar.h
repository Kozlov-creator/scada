#pragma once

class ProgressBar {
public:
    SDL_FRect rect;      // Позиция и размер {x, y, w, h}
    float minVal = 0.0f;
    float maxVal = 40.0f;

    ProgressBar(float x, float y, float w, float h);

    void draw(SDL_Renderer* renderer, float currentValue);
};
