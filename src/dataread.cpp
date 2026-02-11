#include <stdio.h>
#include <string>
#include <main.h>
#include <vector>
#include <fstream>
#include <iostream>
#include <sstream> // Для std::istringstream


std::vector<std::string> strv;

extern std::vector<std::string> vstrDate;
extern std::vector<std::string> vstrValue;
extern std::vector<std::string> vstrAlert;





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


void read_modbus(std::string& file_name) // file_name по const reference
{
int findsym = 0;
int count = 0;
std::string sline;

std::ifstream openfile(file_name);

if (!openfile.is_open()) {
    std::cerr << "Не удалось открыть файл: " << file_name << std::endl;
    return; // Выходим, если не удалось открыть файл
}


    vstrDate.clear();
     std::cout << "vstrDate size = " << vstrDate.size() << std::endl;
    //vstrDate.reserve(10); // Предварительное выделение памяти
    vstrValue.clear();
    //vstrValue.reserve(10); // Предварительное выделение памяти

int xy = 0, yx = 0;

    while (std::getline(openfile, sline)) { // Читаем файл построчно

         std::istringstream iss(sline); // Создаем поток из строки
         std::string registerNumber, value, stringTemp;
         if (std::getline(iss, registerNumber, ':') && std::getline(iss, value)) { // Разделяем строку на registerNumber и value
             // Удаляем лишние пробелы в начале и конце value и registerNumber
             size_t first = value.find_first_not_of(' ');
             size_t last = value.find_last_not_of(' ');
             value = value.substr(first, (last - first + 1));
             first = registerNumber.find_first_not_of(' ');
             last = registerNumber.find_last_not_of(' ');
             registerNumber = registerNumber.substr(first, (last - first + 1));

             if (registerNumber == "7793")
             {
                 size_t findsym = value.find('.');
                 if (findsym != std::string::npos) {
                     value.erase(findsym, 7); // Удаляем дробную часть
                 }

                stringTemp = value.substr(0,2);
                vstrDate.push_back(stringTemp); //день
                stringTemp = value.substr(2,2);
                int month = std::stoi(stringTemp); // Преобразуем в число
                switch (month) {
                    case 1:
                        stringTemp = "января";
                        break;
                    case 2:
                        stringTemp = "февраля";
                        break;
                    case 3:
                        stringTemp = "марта";
                        break;
                    case 4:
                        stringTemp = "апреля";
                        break;
                    case 5:
                        stringTemp = "мая";
                        break;
                    case 6:
                        stringTemp = "июня";
                        break;
                    case 7:
                        stringTemp = "июля";
                        break;
                    case 8:
                        stringTemp = "августа";
                        break;
                    case 9:
                        stringTemp = "сентября";
                        break;
                    case 10:
                        stringTemp = "октября";
                        break;
                    case 11:
                        stringTemp = "ноября";
                        break;
                    case 12:
                        stringTemp = "декабря";
                        break;
                    default:
                        stringTemp = "default";
                }
                vstrDate.push_back(stringTemp);//месяц
                 stringTemp = value.substr(4,2);
                 vstrDate.push_back(stringTemp);//год
yx++;
             }

             if (registerNumber == "7794")
             {
                 size_t findsym = value.find('.');
                 if (findsym != std::string::npos) {
                     value.erase(findsym, 7); // Удаляем дробную часть
                 }
                 std::cout << "value.length() = " << value.length() << std::endl;

                 if (value.length()==6)
                 {  stringTemp = value.substr(0,2);
                    stringTemp += ":";
                    vstrDate.push_back(stringTemp);
                    stringTemp = value.substr(2,2);
                    stringTemp += ":";
                    vstrDate.push_back(stringTemp);
                    stringTemp = value.substr(4,2);
                    vstrDate.push_back(stringTemp);
                }
                 else
                 {
                    value.insert(0, "0");
                    stringTemp = value.substr(0,2);
                    stringTemp += ":";
                    vstrDate.push_back(stringTemp);
                    stringTemp = value.substr(2,2);
                    stringTemp += ":";
                    vstrDate.push_back(stringTemp);
                    stringTemp = value.substr(4,2);
                    vstrDate.push_back(stringTemp);
                }}


                     else {
                         vstrValue.push_back(registerNumber + " " + value); xy++;}

                         std::cout << "value dataread vstrDate = " << vstrDate.at(yx-1) << std::endl;
std::cout << "value dataread = " << vstrValue.at(xy-1) << std::endl;

/*////IMC------------------------------------------------------------------------------------------------
             if (registerNumber == "7001" || registerNumber == "7002" || registerNumber == "7003" ||
                 registerNumber == "7004" || registerNumber == "7005" || registerNumber == "7006")
             {
                 size_t findsym = value.find('.');

             if (findsym != std::string::npos) {
                 value.erase(findsym, 7); // Удаляем дробную часть
                 }

             if (value.size() == 1) {
                 value.insert(0, "0");
             }
             if (registerNumber == "7004" || registerNumber == "7005") {
                 value += ":";}

             if (registerNumber == "7003") {
                 int month = std::stoi(value); // Преобразуем в число
                 switch (month) {
                     case 1:
                         value = "января";
                         break;
                     case 2:
                         value = "февраля";
                         break;
                     case 3:
                         value = "марта";
                         break;
                     case 4:
                         value = "апреля";
                         break;
                     case 5:
                         value = "мая";
                         break;
                     case 6:
                         value = "июня";
                         break;
                     case 7:
                         value = "июля";
                         break;
                     case 8:
                         value = "августа";
                         break;
                     case 9:
                         value = "сентября";
                         break;
                     case 10:
                         value = "октября";
                         break;
                     case 11:
                         value = "ноября";
                         break;
                     case 12:
                         value = "декабря";
                         break;
                     default:
                         value = "default";
                 }
             }
             vstrDate.push_back(value);
                 } else {
                     vstrValue.push_back(registerNumber + " " + value);
                }*/
////end IMC-------------------------------------------------------------------------------------------
         }
    }
    std::cout << "dataread while end" << std::endl;
openfile.close();

}

int read_alert(std::string file_name)
{
    std::ifstream openfile;
    int numLinesToRead = 4; // Количество строк для чтения с конца
    int linesRead = 0;
    std::string line;


    openfile.open(file_name);
    if (!openfile.is_open()) {
        std::cerr << "Не удалось открыть файл: " << file_name << std::endl;
        return 1;
    }

    int fileSize = getFileSize(openfile);
    int position = fileSize;

    //vstrAlert.clear();
    vstrAlert.resize(0);
    vstrAlert.shrink_to_fit();
        // Читаем файл с конца, пока не прочитаем нужное количество строк или не достигнем начала файла
        while (position > 0 && linesRead < numLinesToRead) {
            // Ищем предыдущий символ новой строки
            position--;
            openfile.seekg(position);

            if ((openfile.peek() == '\n' || position == 0) && position+1 != fileSize) {

                // Если нашли новую строку или достигли начала файла, читаем строку
                if (position == 0) {
                    openfile.seekg(0); // Если начало файла, читаем с начала
                } else {
                    openfile.seekg(position + 1); // Если нашли новую строку, читаем после неё
                }

                getline(openfile, line);

                vstrAlert.push_back(line);

                linesRead++;
            }
        }

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
