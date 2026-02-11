#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include <SDL3_ttf/SDL_ttf.h>

#include <stdio.h>
#include <string>
#include <sstream>
#include <vector>
#include <fstream>
#include <iostream>

#include <texture_class.h>
#include <dot_class.h>
#include <const_data.h>
#include <main.h>


//Окно, в которое мы будем отображать
SDL_Window *gWindow = NULL;

//Средство визуализации окна
SDL_Renderer *gRenderer = NULL;

//Глобально используемый шрифт
TTF_Font *gFont = NULL;

SDL_FRect *selectedRect = NULL;

std::vector<SDL_FRect> vRectClipCoord;
std::vector<SDL_FRect> vRectClipCoordDateTime;
std::vector<SDL_FRect> vRectClipCoordAlertMessage;
std::vector<SDL_FRect> vRectClipValue;

SDL_FRect fonClockRect={1490,5,400,50};

void InputCoord();


////////////////////////////////////////////////////////////////

int main( int argc, char *args[] )
{
	SDL_FPoint mousePos;
	SDL_Point clickOffset;

	InputCoord();

	bool leftMouseButtonDown = false;
	int xpos=0;
	int ypos=0;
	float x=0;
	int m=0;
	int screen = 1;
	//Запуск SDL3 и создание окна
	std::cout<<"main"<<std::endl;



	if( !init() )
	{
		std::cout<< "Failed to initialize!" <<std::endl;
	}
	else
	{
		std::cout<<"init"<<std::endl;
		//Загрузка медиа данных
		if( !loadMedia() )
		{
			std::cout<< "Failed to load media!" << std::endl;
		}

		else
		{
			std::cout<<"load media"<<std::endl;
			//Флаг основного цикла
			bool quit = false;

			//Обработчик событий
			SDL_Event e;
			SDL_zero( e );

			//Цвет отображения текста
			SDL_Color textColor = { 0, 0, 0, 0xFF };
			SDL_Color highlightColor = { 0xFF, 0, 0, 0xFF };

			//Текущая точка ввода
			int currentData = 0;
			int data = 101;


			//SDL_Thread* threadID = SDL_CreateThread(threadFunction, "ReadFunction",(void*)data);

			//Основной цикл
			while( !quit )
			{
				//Обрабатывать события в очереди
				while( SDL_PollEvent( &e ) != 0 )
				{
					//Запрос пользователя для выхода
					if( e.type == SDL_EVENT_QUIT )
					{
						quit = true;
					}
					else if( e.type == SDL_EVENT_KEY_DOWN )
					{
						switch( e.key.key )
						{
							//Предыдущий ввод данных
							case SDLK_UP:
							std::cout<<"SDLK_UP\n";
							break;

							//Next data entry
							case SDLK_DOWN:
							std::cout<<"SDLK_DOWN\n";
							break;

							//Decrement input point
							case SDLK_LEFT:
							std::cout<<"SDLK_LEFT\n";
							break;

							//Increment input point
							case SDLK_RIGHT:
							std::cout<<"SDLK_RIGHT\n";
							break;
						}
					}
					else if (e.type == SDL_EVENT_MOUSE_BUTTON_UP)
					{
						if (leftMouseButtonDown && e.button.button == SDL_BUTTON_LEFT)
								{
									leftMouseButtonDown = false;
									selectedRect = NULL;
								}
					}

					else if( e.type == SDL_EVENT_MOUSE_MOTION)
					{
						mousePos = { e.motion.x, e.motion.y };

						if (leftMouseButtonDown && selectedRect != NULL)
						{
							selectedRect->x = mousePos.x - clickOffset.x;

							selectedRect->y = mousePos.y - clickOffset.y;

						}
					}
					else if (e.type == SDL_EVENT_MOUSE_BUTTON_DOWN)
					{
						if(!leftMouseButtonDown && e.button.button == SDL_BUTTON_LEFT)
					{
						leftMouseButtonDown = true;

						if (SDL_PointInRectFloat(&mousePos,&fonClockRect))
						{
							if (screen == 1)
							{screen = 2;}
							else {screen =1;}
						}

						for (int x=0; x<vRectClipCoord.size(); x++)
						{
							if (SDL_PointInRectFloat(&mousePos,&vRectClipCoord.at(x)))
							{
								selectedRect = &vRectClipCoord.at(x);
								clickOffset.x = mousePos.x - vRectClipCoord.at(x).x;
								clickOffset.y = mousePos.y - vRectClipCoord.at(x).y;

								break;
							}
						}
						for (int x=0; x<vRectClipCoordDateTime.size(); x++)
						{
							if (SDL_PointInRectFloat(&mousePos,&vRectClipCoordDateTime.at(x)))
							{
								selectedRect = &vRectClipCoordDateTime.at(x);
								clickOffset.x = mousePos.x - vRectClipCoordDateTime.at(x).x;
								clickOffset.y = mousePos.y - vRectClipCoordDateTime.at(x).y;

								break;
							}
						}
						for (int x=0; x<vRectClipValue.size(); x++)
						{
							if (SDL_PointInRectFloat(&mousePos,&vRectClipValue.at(x)))
							{
								selectedRect = &vRectClipValue.at(x);
								clickOffset.x = mousePos.x - vRectClipValue.at(x).x;
								clickOffset.y = mousePos.y - vRectClipValue.at(x).y;

								break;
							}
						}
						}
					}
				}

			switch (screen)
			{
				case 1:
					desctop1();
					break;

				case 2:
					desctop2();
					break;
				default:
					break;
			}
			SDL_DelayNS( 16666666 );
			}

			//Дождитесь завершения потока
			//SDL_WaitThread( threadID, NULL );

		}
	}

	//Free resources and close SDL
	close();

	return 0;
}
