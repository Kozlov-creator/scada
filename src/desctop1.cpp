#include <fstream>
#include <iostream>
#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <vector>
#include <texture_class.h>
#include <string>
#include <main.h>
#include <chrono>

class TimingUtil
{
public:
    TimingUtil(std::string const& message)
    : start_(std::chrono::steady_clock::now()),
    message_(message)
    {
    }
    ~TimingUtil()
    {
        std::chrono::steady_clock::time_point finish = std::chrono::steady_clock::now();
        std::cout << message_ << " took "
        << std::chrono::duration_cast<std::chrono::milliseconds>(finish - start_).count()
        << " ns." << std::endl;
    }
private:
    std::chrono::steady_clock::time_point start_;
    std::string message_;
};

///////////////////////////////////////////////////////////////////////

extern SDL_Renderer *gRenderer;

extern std::vector<SDL_FRect> vRectClipCoord;
extern std::vector<SDL_FRect> vRectClipCoordDateTime;
extern std::vector<SDL_FRect> vRectClipCoordAlertMessage;
extern std::vector<SDL_FRect> vRectClipValue;

std::vector<std::string> vstrDate;
std::vector<std::string> vstrValue;
std::vector<std::string> vstrImage;
std::vector<std::string> vstrAlert;

std::vector<SDL_Texture*> vDateTimeTextures;
std::vector<SDL_Texture*> vValueTextures;
std::vector<SDL_Texture*> vMessageTextures;


std::vector<SDL_Texture*> vimageTexture;

extern SDL_FRect fonClockRect;
SDL_FRect MessageRect;

SDL_Texture *ClockFon;

SDL_Texture *TestTexture = nullptr;
SDL_Surface *TestSurface = nullptr;

extern TTF_Font *gFont;

std::vector<CTexture> vectorTextureTextValue(4);
CTexture tempCTexture;

SDL_Color sdlcolor;

/////////////////////////////////////////////////////////////////////

void desctop1()
{
    //TimingUtil t("My test");
//------------------------------------------------------------------------//
                    //Очистить экран рендера
				SDL_SetRenderDrawColor( gRenderer, 129, 191, 254, 0xFF ); //8DCAFF 8E88E1 00b3ff Цвет фона
                SDL_RenderClear( gRenderer ); //Очистить текущую цель рендеринга с помощью цвета рисования.
//------------------------------------------------------------------------//
                tempCTexture.loadFromFile(IMAGE_CLOCKFON);
                tempCTexture.setXY(1490, 10);
                tempCTexture.render(1,NULL,NULL, 0, NULL, SDL_FLIP_NONE);
//------------------------------------------------------------------------//

                MessageRect.x=1;
                MessageRect.y=900;
                MessageRect.w=1910;
                MessageRect.h=175;

                SDL_SetRenderDrawColor(gRenderer,0x76,0x70,0x70,0xFF); //Цвет фона
                SDL_RenderFillRect(gRenderer, &MessageRect);
                SDL_RenderRect(gRenderer, &MessageRect);//рисуем квадрат заданным цветом

/////////////////////////////////////////////////////////////////////////////

                sdlcolor={0x0a,0x0b,0x0c,0xff};
                for (int i = 0; i < vstrAlert.size(); i++) {
                    tempCTexture.loadFromRenderedText(vstrAlert.at(i), sdlcolor);
                    tempCTexture.render(0,&vRectClipCoordAlertMessage.at(i),NULL, 0, NULL, SDL_FLIP_NONE);
                 }

//------------------------------------------------------------------------//
std::cout << "desctop1 vstrDate begin" << std::endl;
std::string snameFile = FILE_MODBUS;
read_modbus(snameFile);
                sdlcolor={0x0a,0x0b,0x0c,0xff};
                for (int i = 0; i < vstrDate.size(); i++) {
                     tempCTexture.loadFromRenderedText(vstrDate.at(i), sdlcolor);
                     tempCTexture.render(0,&vRectClipCoordDateTime.at(i),NULL, 0, NULL, SDL_FLIP_NONE);
                }
                std::cout << "desctop1 vstrDate end" << std::endl;
//------------------------------------------------------------------------//

                sdlcolor={0xa0,0xb0,0xc0,0xFF};
                for (int i = 0; i < 4; i++) {
                    tempCTexture.loadFromRenderedText(vstrValue.at(i), sdlcolor);
                    tempCTexture.render(0,&vRectClipValue.at(i),NULL, 0, NULL, SDL_FLIP_NONE);
                }

//-------------------------------------------------------------------------//

                for (int i=0; i < vstrImage.size();i++)
                {
                    SDL_RenderTextureRotated(gRenderer,vimageTexture.at(i),NULL,&vRectClipCoord.at(i),0,NULL,SDL_FLIP_NONE);
                }

//-------------------------------------------------------------------------//
                //Обновить экран
               SDL_RenderPresent( gRenderer );
}

/////////////////////////////////////////////////////////////////////////////////////////

void desctop2()
{
    std::string sstr {"sstr.cstr"};
    fonClockRect.x=1490;
    fonClockRect.y=5;
    fonClockRect.w=400;
    fonClockRect.h=50;

    SDL_SetRenderDrawColor( gRenderer, 0xFF, 0xDF, 0xFF, 0xFF ); //8DCAFF 8E88E1 00b3ff Цвет фона
    SDL_RenderClear( gRenderer ); //Очистить текущую цель рендеринга с помощью цвета рисования.

    SDL_RenderTextureRotated(gRenderer,ClockFon,NULL,&fonClockRect,0,NULL,SDL_FLIP_NONE);
    SDL_SetRenderDrawColor(gRenderer,0x76,0x70,0x70,0xFF); //Цвет фона


    SDL_Color textColor = {0, 0, 0, 0xFF};
    TestSurface = TTF_RenderText_Solid(gFont, sstr.c_str(), sstr.size(), textColor);
    TestTexture = SDL_CreateTextureFromSurface(gRenderer, TestSurface);
    SDL_DestroySurface(TestSurface);
    TestSurface=nullptr;


        //SDL_QueryTexture(TestTexture, NULL, NULL, &w, &h);
        vRectClipCoordAlertMessage.at(0).w = 80;
        vRectClipCoordAlertMessage.at(0).h = 24;

     SDL_RenderTextureRotated(gRenderer, TestTexture, NULL, &vRectClipCoordAlertMessage.at(0), 0, NULL, SDL_FLIP_NONE);




    //Обновить экран
    SDL_RenderPresent( gRenderer );
     SDL_DestroyTexture(TestTexture);
};
