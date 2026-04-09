#include <SDL3/SDL.h>
#include <stdio.h>
#include <string>
#include <vector>
#include <fstream>
#include <iostream>
#include <sstream> // Для std::istringstream
#include <algorithm>
#include <unordered_map>

#include <iomanip>
#include <ctime>
#include <chrono>

#include "globals.h"
#include "functions.h"
#include "SceneManager.h"

std::vector<std::string> strv;


//Обновленная функция read_modbus//Теперь вместо того, чтобы складывать всё подряд в вектор, мы будем просто обновлять значения в карте по ключу-регистру.
void read_modbus(const std::string& file_name)
{
    std::ifstream openfile(file_name);
    if (!openfile.is_open()) return;

    // Временная карта, чтобы минимизировать время блокировки мьютекса
    std::unordered_map<std::string, std::string> tempData;
    std::string sline;

    while (std::getline(openfile, sline)) {
        if (sline.empty() || sline[0] == '#') continue;

        std::istringstream iss(sline);
        std::string regNum, value;

        if (std::getline(iss, regNum, ',') && std::getline(iss, value, ',')) {
            // Очистка пробелов (Trim)
            auto trim = [](std::string& s) {
                s.erase(0, s.find_first_not_of(" \t\r\n"));
                s.erase(s.find_last_not_of(" \t\r\n") + 1);
            };
            trim(regNum);
            trim(value);

            // Просто сохраняем в карту: ключ — регистр, значение — данные
            tempData[regNum] = value;
        }
    }
    openfile.close();

    // Обновляем глобальную карту под мьютексом
    if (Scada::modbus_mutex != nullptr) {
        SDL_LockMutex(Scada::modbus_mutex);
        for (auto const& [reg, val] : tempData) {
            // Для Modbus тегами будут номера регистров "7802", "7805" и т.д.
            Scada::gLiveTags[reg] = val;
        }
        SDL_UnlockMutex(Scada::modbus_mutex);
    }

}


//=====================================================================================
std::string get_modbus_datetime() {
    std::string dateStr = "--/--/--";
    std::string timeStr = "--:--:--";

    // Блокируем мьютекс на время чтения из глобальной карты
    SDL_LockMutex(Scada::modbus_mutex);

    // 1. Форматируем ДАТУ (Регистр 7793)
    if (Scada::gLiveTags.count("7793")) {
        std::string val = Scada::gLiveTags["7793"];
        // Дополняем нулями слева до 6 символов (ЧЧММСС)
        while (val.length() < 6) val.insert(0, "0");

        if (val.length() >= 6) {
            std::string day = val.substr(0, 2);
            std::string month = val.substr(2, 2);
            std::string year = val.substr(4, 2);
            // Превращаем номер месяца в название (если нужно)
            try {
                int mIdx = std::stoi(month);
                if (mIdx >= 1 && mIdx <= 12) month = MONTHS[mIdx - 1];
            } catch (...) {}

            dateStr = day + " " + month + " 20" + year;
        }
    }

    // 2. Форматируем ВРЕМЯ (Регистр 7794)
    if (Scada::gLiveTags.count("7794")) {
        std::string val = Scada::gLiveTags["7794"];
        // Отрезаем дробную часть, если она есть (например, .00)
        size_t dot = val.find('.');
        if (dot != std::string::npos) val = val.substr(0, dot);

        // Дополняем нулями слева до 6 символов (ЧЧММСС)
        while (val.length() < 6) val.insert(0, "0");

        timeStr = val.substr(0, 2) + ":" + val.substr(2, 2) + ":" + val.substr(4, 2);
    }

    SDL_UnlockMutex(Scada::modbus_mutex);

    return dateStr + "   " + timeStr;
}

//==========================================================================================
// Функция получения времени (Универсальная)//Перепишем функцию так, чтобы она сама решала, откуда брать данные. Для системного времени используем std::put_time.
std::string get_display_time() {
    if (Scada::gUseSystemTime) {
        // ЧИТАЕМ СИСТЕМНОЕ ВРЕМЯ ПК
        auto now = std::chrono::system_clock::now();
        std::time_t now_c = std::chrono::system_clock::to_time_t(now);
        std::tm* now_tm = std::localtime(&now_c);

        std::stringstream ss;
        // %d - день, %m - месяц числом, %Y - год 2024
       //ss << std::put_time(now_tm, "%d.%m.%Y  %H:%M:%S");
        ///месяц числом. конец

        ///Для текстового месяца
        // День
        ss << std::setfill('0') << std::setw(2) << now_tm->tm_mday << " ";
        // Месяц из массива (tm_mon идет от 0 до 11)
        ss << MONTHS[now_tm->tm_mon] << " ";
        // Год и время
        ss << (1900 + now_tm->tm_year) << "   ";
        ss << std::put_time(now_tm, "%H:%M:%S");
        ///для текстового месяца.конец
        return ss.str();
    } else {
        // ЧИТАЕМ ИЗ MODBUS (ваш старый метод)
         return get_modbus_datetime();
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

    Scada::vstrAlert = std::move(tempAlerts); // Теперь это корректный перенос данных
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
