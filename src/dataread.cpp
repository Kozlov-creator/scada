#include <SDL3/SDL.h>
#include <stdio.h>
#include <string>
#include <main.h>
#include <vector>
#include <fstream>
#include <iostream>
#include <sstream> // Для std::istringstream
#include <algorithm>


// Статический массив месяцев (быстрее чем switch)
static const std::string MONTHS[] = {
    "января", "февраля", "марта", "апреля", "мая", "июня",
    "июля", "августа", "сентября", "октября", "ноября", "декабря"
};



std::vector<std::string> strv;

extern std::vector<std::string> vstrDate;
extern std::vector<std::string> vstrValue;
extern std::vector<std::string> vstrAlert;

// Глобальные данные для обмена между потоками
extern SDL_Mutex* modbus_mutex;

//---------------------------------------------------------------------------------
//---------------------------------------------------------------------------------
std::ifstream open_file(std::string file_name, std::string sData)
{
    std::ifstream in;

    in.open(file_name);

    if (in.is_open())
    {
        //Load data
        while (getline(in,sData)) strv.push_back(sData);
    }
    else{std::cout<<file_name<<" not found"<<std::endl;}
    in.close();
    return in;
}
//---------------------------------------------------------------------------------
//Безопасная версия read_modbus
void read_modbus(const std::string& file_name)
{
    std::ifstream openfile(file_name);
    if (!openfile.is_open()) {
        // Не спамим в консоль каждый кадр, если файла нет
        return;
    }

    // Временные векторы, чтобы не блокировать основной поток надолго
    std::vector<std::string> tempDate;
    std::vector<std::string> tempValue;
    std::string sline;

    while (std::getline(openfile, sline)) {
        if (sline.empty()) continue;

        std::istringstream iss(sline);
        std::string regNum, value;

        if (std::getline(iss, regNum, ':') && std::getline(iss, value)) {
            // Trim (очистка пробелов)
            auto trim = [](std::string& s) {
                s.erase(0, s.find_first_not_of(" \t\r\n"));
                s.erase(s.find_last_not_of(" \t\r\n") + 1);
            };
            trim(regNum);
            trim(value);

            // Обработка даты (Регистр 7793)
            if (regNum == "7793" && value.length() >= 6) {
                // День
                tempDate.push_back(value.substr(0, 2));

                // Месяц
                try {
                    int monthIdx = std::stoi(value.substr(2, 2));
                    if (monthIdx >= 1 && monthIdx <= 12) {
                        tempDate.push_back(MONTHS[monthIdx - 1]);
                    } else {
                        tempDate.push_back("unknown");
                    }
                } catch (...) { tempDate.push_back("error"); }

                // Год
                tempDate.push_back(value.substr(4, 2));
            }
            // Обработка времени (Регистр 7794)
            else if (regNum == "7794") {
                if (value.find('.') != std::string::npos) {
                    value = value.substr(0, value.find('.')); // Отрезаем дробную часть
                }

                if (value.length() < 6) value.insert(0, 6 - value.length(), '0');

                tempDate.push_back(value.substr(0, 2) + ":");
                tempDate.push_back(value.substr(2, 2) + ":");
                tempDate.push_back(value.substr(4, 2));
            }
            // Все остальные регистры
            else {///Ограничение размера векторов. Чтобы векторы не росли бесконечно (например, если файл Modbus стал огромным или произошла ошибка дублирования), добавим жесткий лимит
                if (tempValue.size() < 50) {
                    tempValue.push_back(regNum + " " + value);
                } else {
                    // Если нужно удалять старые и добавлять новые (FIFO):
                    // tempValue.erase(tempValue.begin());
                    // tempValue.push_back(regNum + " " + value);
                    break; // Или просто перестаем читать, если данных слишком много
                }
               }
        }
    }
    openfile.close();

    // КРИТИЧЕСКИЙ УЧАСТОК: Обновляем глобальные векторы под мьютексом
    if (modbus_mutex != nullptr) {
        SDL_LockMutex(modbus_mutex);
        vstrDate = std::move(tempDate);   // move эффективнее копирования
        vstrValue = std::move(tempValue);
        SDL_UnlockMutex(modbus_mutex);
    }
}
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
int read_alert(std::string file_name)
{ std::ifstream openfile(file_name, std::ios::binary | std::ios::ate);
    if (!openfile.is_open()) return 1;

    int numLinesToRead = 4;
    int linesRead = 0;
    std::string line;
    std::vector<std::string> tempAlerts;

    long position = openfile.tellg();
    long fileSize = position;

    while (position > 0 && linesRead < numLinesToRead) {
        position--;
        openfile.seekg(position);

        // Нашли начало строки или начало файла
        if ((openfile.peek() == '\n' || position == 0) && position + 1 != fileSize) {
            long currentPos = openfile.tellg();

            if (position == 0) openfile.seekg(0);
            else openfile.seekg(position + 1);

            if (std::getline(openfile, line)) {
                if (!line.empty()) {
                    tempAlerts.push_back(line);
                    linesRead++;
                }
            }
            // Возвращаемся на позицию поиска, чтобы не зациклиться
            openfile.seekg(currentPos);
        }
    }

    // ВАЖНО: т.к. мы читали с конца, самая новая строка — первая в векторе.
    // Если нужно, чтобы порядок был хронологический (старые сверху),
    // можно перевернуть вектор: std::reverse(tempAlerts.begin(), tempAlerts.end());

    vstrAlert = std::move(tempAlerts); // Теперь это корректный перенос данных
    openfile.close();
    return 0;
}

// Функция для получения размера файла
int getFileSize(std::ifstream& file)
{
    file.seekg(0, std::ios::end); // Перемещаемся в конец файла
    int fileSize = file.tellg(); // Получаем текущую позицию (размер файла)
    file.seekg(0, std::ios::beg); // Возвращаемся в начало файла
    return fileSize;
}
