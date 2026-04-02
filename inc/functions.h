#pragma once
#include <string>


// Прототипы функций
void save_layout_config(const std::string& file_name);

std::string get_modbus_datetime();
std::string get_display_time();

int threadFunction(void*);

std::ifstream open_file(std::string, std::string);
void read_modbus(const std::string&);
int getFileSize(std::ifstream &);
int read_alert(std::string);

void desctop1();
void desctop2();


SDL_Texture *LoadFromRenderedText (std::string, SDL_FRect&);

//void LoadFromRenderedText (std::string, SDL_Rect&, SDL_Texture&);

void LoadImageTextureFromFile( std::string , SDL_FRect* );

//Запускает SDL2 и создает окно Starts up SDL2 and creates window
bool init();

//Загрузка медиа данных Loads media
bool loadMedia();

//Освобождает медиа данные и отключает SDL2 Frees media and shuts down SDL2
void close();

