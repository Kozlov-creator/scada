#ifndef __MAIN_H__
#define __MAIN_H__

#define font_ttf "/usr/share/fonts/TTF/Hack-BoldItalic.ttf" //Arch
#define FILE_RECTCOORD "./coordinate/rectcoord.txt"
#define FILE_RECTCOORDDATETIME "./coordinate/rectcoorddatetime.txt"
#define FILE_RECTCOORDALERTMESSAGE "./coordinate/rectcoordalertmessage.txt"
#define file_allertmessage "./logs/alert_message.txt"
#define FILE_MODBUS "./logs/readModbus6.txt"
#define FILE_IMAGE "./coordinate/typeImage.txt"
#define IMAGE_CLOCKFON "./image/clock_fon.png"



int threadFunction(void*);

std::ifstream open_file(std::string, std::string);
void read_modbus(std::string&);
int getFileSize(std::ifstream &);
int read_alert(std::string);

void desctop1();
void desctop2();

#endif //__MAIN_H__
