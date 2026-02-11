#include <SDL3/SDL.h>
#include <fstream>
#include <string>
#include <vector>
#include <texture_class.h>
#include <iostream>
#include <main.h>

extern std::vector<SDL_FRect> vRectClipCoordDateTime;
extern std::vector<SDL_FRect> vRectClipCoord;
extern std::vector<SDL_FRect> vRectClipCoordAlertMessage;
extern std::vector<SDL_FRect> vRectClipValue;
extern std::vector<std::string> vstrImage;

////////////////////////////////////////////////////////////

void InputCoord()
{
SDL_FRect tempRect;
std::ifstream in;
std::string sCoord;
std::string sImage;

    in.open(FILE_IMAGE); //Считывание места положения изображений
    if (in.is_open())
    {
        for(in >> sImage; !in.eof();in >> sImage)
        {
            vstrImage.push_back(sImage);
        }
    }
    else {std::cout<<"file typeImage not found"<<std::endl;}
    in.close();


    in.open(FILE_RECTCOORD); //Считывание координат изображений
    if (in.is_open())
    {
        vRectClipCoord.clear();
        sCoord.clear();

            for(in >> sCoord; !in.eof();in >> sCoord)
            {
                tempRect.x = atoi(sCoord.c_str());
                in>>sCoord;
                in>>sCoord;
                tempRect.y = atoi(sCoord.c_str());

                vRectClipCoord.push_back(tempRect);
            }
    }
    else {std::cout<<"file rectcoorddatetime not found"<<std::endl;}
    in.close();

    in.open(FILE_RECTCOORDDATETIME); //Считывание координат даты времени
    if (in.is_open())
    {
        vRectClipCoordDateTime.clear();
        vRectClipValue.clear();
        sCoord.clear();
        for (int i=0; i<6;i++)
        {
            in >> sCoord;
            tempRect.x = atoi(sCoord.c_str());
            in>>sCoord;
            in>>sCoord;
            tempRect.y = atoi(sCoord.c_str());

            vRectClipCoordDateTime.push_back(tempRect);
        }

        for(in >> sCoord; !in.eof();in >> sCoord)
        {
            tempRect.x = atoi(sCoord.c_str());
            in>>sCoord;
            in>>sCoord;
            tempRect.y = atoi(sCoord.c_str());

            vRectClipValue.push_back(tempRect);
        }
    }
    else {std::cout<<"file rectcoorddatetime not found"<<std::endl;}
    in.close();

    in.open(FILE_RECTCOORDALERTMESSAGE); //Считывание координат даты времени
    if (in.is_open())
    {
        vRectClipCoordAlertMessage.clear();
        sCoord.clear();

        for(in >> sCoord; !in.eof();in >> sCoord)
        {
            tempRect.x = atoi(sCoord.c_str());
            in>>sCoord;
            in>>sCoord;
            tempRect.y = atoi(sCoord.c_str());

            vRectClipCoordAlertMessage.push_back(tempRect);
        }
    }
    else {std::cout<<"file rectcoordalertmessage not found"<<std::endl;}
    in.close();
}
