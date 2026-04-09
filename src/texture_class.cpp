#include <unordered_map>
#include <map>
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
#include <unistd.h>

#include <algorithm> // для std::clamp

#include "globals.h"
#include "functions.h"


SDL_Cursor *mousecursor = NULL;

TTF_Font *Font_ttf;


/////////////////////////////////////////////////////////////////////////////

CTexture::CTexture()
{
	//Инициализация
	mTexture = nullptr;
	mWidth = 0;
	mHeight = 0;
	mX = 0;
	mY = 0;
}

CTexture::~CTexture()
{
	//Освободить текстуры
	freeTexture();
}

void CTexture::setXY(float x, float y)
{
	mX = x;
	mY = y;
}

bool CTexture::loadTextureFromFile( std::string path )
{
	//Избавьтесь от ранее существовавшей текстуры
	freeTexture();

	//Конечная текстура
	SDL_Texture *newTexture = nullptr;

	newTexture = IMG_LoadTexture(Scada::gRenderer,path.c_str());

	if(newTexture == nullptr)
	{
		std::cout<<"Can't load: "<<SDL_GetError()<<std::endl;
	}

	//Успешное возращение
	mTexture = newTexture;
	return mTexture != nullptr;
}

bool CTexture::loadSVGAuto( std::string path )
{
	freeTexture();

	// 1. Получаем текущий размер области отрисовки (окна)
	int windowW, windowH;
	if (!SDL_GetRenderOutputSize(Scada::gRenderer, &windowW, &windowH)) {
		std::cout << "Error getting render size: " << SDL_GetError() << std::endl;
		return false;
	}

	// 2. Открываем файл через IOStream (стандарт SDL3)
	SDL_IOStream* io = SDL_IOFromFile(path.c_str(), "rb");
	if (!io) return false;

	// 3. Растеризуем SVG сразу в размер окна
	// Теперь картинка будет идеально четкой, без "мыла"
	SDL_Surface* loadedSurface = IMG_LoadSizedSVG_IO(io, windowW, windowH);
	SDL_CloseIO(io);

	if (!loadedSurface) {
		std::cout << "SVG Load Error: " << SDL_GetError() << std::endl;
		return false;
	}

	// 4. Создаем текстуру
	mTexture = SDL_CreateTextureFromSurface(Scada::gRenderer, loadedSurface);

	if (mTexture) {
		mWidth = loadedSurface->w;
		mHeight = loadedSurface->h;

		// Включаем режим смешивания для поддержки прозрачности SVG
		SDL_SetTextureBlendMode(mTexture, SDL_BLENDMODE_BLEND);
	}

	SDL_DestroySurface(loadedSurface);
	return mTexture != nullptr;
}


bool CTexture::loadFromFile( std::string path )
{
	//Избавьтесь от ранее существовавшей текстуры
	freeTexture();

	//Конечная текстура
	SDL_Texture *newTexture = nullptr;

	//Загрузить изображение по указанному пути
	SDL_Surface *loadedSurface = IMG_Load( path.c_str() );
	if( loadedSurface == nullptr )
	{
		std::cout<< "Unable to load image "<< path.c_str()<<" SDL_image Error: " << SDL_GetError() <<std::endl;
		return false;
	}
		// УДАЛЯЕМ SDL_SetSurfaceColorKey!
		// SVG сам управляет прозрачностью через альфа-канал.

		//Create texture from surface pixels
        newTexture = SDL_CreateTextureFromSurface( Scada::gRenderer, loadedSurface );
		if( newTexture == nullptr )
		{
			std::cout<< "Unable to create texture from "<<path.c_str() <<"SDL Error: "<< SDL_GetError() <<std::endl;
		}
		else
		{
			//Get image dimensions
			mWidth = loadedSurface->w;
			mHeight = loadedSurface->h;

			// Включаем поддержку прозрачности для текстуры
			SDL_SetTextureBlendMode(newTexture, SDL_BLENDMODE_BLEND);
		}

	//Избавьтесь от старой загруженной поверхности
	SDL_DestroySurface( loadedSurface );
	//Return success
	mTexture = newTexture;
	return mTexture != nullptr;

}

bool CTexture::loadFromRenderedText( std::string textureText, SDL_Color textColor )
{
	//Избавьтесь от ранее существовавшей текстуры
	freeTexture();

	//Render text surface
	SDL_Surface *textSurface = TTF_RenderText_Solid( Font_ttf, textureText.c_str(), textureText.size(), textColor );
	if( textSurface != nullptr )
	{
		//Create texture from surface pixels
        mTexture = SDL_CreateTextureFromSurface( Scada::gRenderer, textSurface );
		if( mTexture == nullptr )
		{
			std::cout<< "Unable to create texture from rendered text! SDL Error: "<< SDL_GetError() <<std::endl;
		}
		else
		{
			//Get image dimensions
			mWidth = textSurface->w;
			mHeight = textSurface->h;
		}

		//Get rid of old surface
		SDL_DestroySurface( textSurface );
	}
	else
	{
		std::cout<< "Unable to render text surface! SDL_ttf Error: "<< SDL_GetError() <<std::endl;
	}

	
	//Return success
	return mTexture != nullptr;
}


void CTexture::freeTexture()
{
	//Free texture if it exists
	if( mTexture != nullptr )
	{
		SDL_DestroyTexture( mTexture );
		mTexture = nullptr;
		mWidth = 0;
		mHeight = 0;
	}
}

void CTexture::setColor( Uint8 red, Uint8 green, Uint8 blue )
{
	//Modulate texture rgb
	SDL_SetTextureColorMod( mTexture, red, green, blue );
}

void CTexture::setBlendMode( SDL_BlendMode blending )
{
	//Set blending function
	SDL_SetTextureBlendMode( mTexture, blending );
}
		
void CTexture::setAlpha( Uint8 alpha )
{
	//Modulate texture alpha
	SDL_SetTextureAlphaMod( mTexture, alpha );
}

//void CTexture::render( float x, float y, SDL_FRect *clipRect, double angle, SDL_FPoint *center, SDL_FlipMode flipRender )
void CTexture::render( int xy, const SDL_FRect *renderQuad, SDL_FRect *clipRect, double angle, SDL_FPoint *center, SDL_FlipMode flipRender )
{
	// 1. Создаем локальную копию прямоугольника.
	// Если renderQuad передан, копируем его. Если нет — берем внутренние координаты mX, mY.
	SDL_FRect finalQuad;

	if (renderQuad == nullptr) {
		finalQuad = { mX, mY, mWidth, mHeight };
	} else {
		finalQuad = *renderQuad; // Копируем данные из const указателя

		// Теперь мы можем спокойно менять finalQuad, не трогая оригинал в конфиге
		finalQuad.w = mWidth;
		finalQuad.h = mHeight;
		if (xy) {
			finalQuad.x = mX;
			finalQuad.y = mY;
		}
	}

	// 2. Если есть область обрезки (clip), корректируем размеры
	if (clipRect != nullptr) {
		finalQuad.w = clipRect->w;
		finalQuad.h = clipRect->h;
		// Если логика требует подмены координат на координаты клипа:
		finalQuad.x = clipRect->x;
		finalQuad.y = clipRect->y;
	}

	// 3. Отрисовываем, передавая адрес нашей локальной копии
	SDL_RenderTextureRotated(Scada::gRenderer, mTexture, clipRect, &finalQuad, angle, center, flipRender);
}

float CTexture::getWidth()
{
	return mWidth;
}

float CTexture::getHeight()
{
	return mHeight;
}

SDL_Texture* CTexture::getTexture()
{
	return mTexture;
}


//Функция загрузки изображений в текстуру//////////////////////////////////////////
void LoadImageTextureFromFile( std::string path, SDL_FRect *RectClipCoord )
{
	//Конечная текстура The final texture
	SDL_Texture *tempTexture = nullptr;
	float w, h;

	tempTexture = IMG_LoadTexture(Scada::gRenderer,path.c_str());
	if(tempTexture == nullptr)
	{
		std::cout<<"Can't load: "<<SDL_GetError()<<std::endl;
	}

	// Получаем размеры текстуры

		SDL_GetTextureSize(tempTexture, &w, &h);

	if (RectClipCoord != nullptr)
	{
		RectClipCoord->w = w;
		RectClipCoord->h = h;
	}


}

//Функция создание изображения из текста///////////////////////////////////////////////////////////
SDL_Texture *LoadFromRenderedText(std::string sstr, SDL_FRect &RectClipCoord)
{

	SDL_Color textColor = {0, 0, 0, 0xFF};
	SDL_Surface *textSurface = nullptr;
	SDL_Texture *tempTexture = nullptr;
	float w, h;

	// Render text surface
	textSurface = TTF_RenderText_Solid(Font_ttf, sstr.c_str(), sstr.size(), textColor);
	if (textSurface == nullptr) {
		std::cout<<"Unable to render text surface! SDL_ttf Error: "<< SDL_GetError()<<std::endl;
		return nullptr; // Важно: Выход из функции при ошибке
	}

	// Create texture from surface pixels
	tempTexture = SDL_CreateTextureFromSurface(Scada::gRenderer, textSurface);
	SDL_DestroySurface(textSurface);
	if (tempTexture == nullptr) {
		std::cout<<"Unable to create texture from rendered text! SDL Error: "<< SDL_GetError()<<std::endl;
		SDL_DestroySurface(textSurface); // Освобождаем поверхность, если создание текстуры не удалось
		return nullptr; // Важно: Выход из функции при ошибке
	}

	SDL_GetTextureSize(tempTexture, &w, &h);
	//Get rid of old surface
		RectClipCoord.w = w;
		RectClipCoord.h = h;

	return tempTexture;
}

//Функция инициализации SDL/////////////////////////////////////////////////////////
bool init()
{
	//Initialization flag
	bool success = true;

	//Initialize SDL
	if( SDL_Init( SDL_INIT_VIDEO ) < 0 )
	{
		std::cout<< "SDL could not initialize! SDL Error: "<< SDL_GetError() <<std::endl;
		success = false;
	}
	else
	{
		//Set texture filtering to linear
		//if( !SDL_SetHint( SDL_HINT_RENDER_SCALE_QUALITY, "1" ) )
		//{
		//	printf( "Warning: Linear texture filtering not enabled!" );
		//}

		//Create window
		Scada::gWindow = SDL_CreateWindow( " SCADA", SCREEN_WIDTH, SCREEN_HEIGHT, 0 );//| SDL_WINDOW_BORDERLESS|SDL_WINDOW_FULLSCREEN);
		if( Scada::gWindow == NULL )
		{
			std::cout<< "Window could not be created! SDL Error: "<< SDL_GetError() << std::endl;
			success = false;
		}
		else
		{
			//Создание синхронизированного средства визуализации для окна Create vsynced renderer for window
			Scada::gRenderer = SDL_CreateRenderer( Scada::gWindow, NULL );
			if( Scada::gRenderer == NULL )
			{
				std::cout<< "Renderer could not be created! SDL Error: "<< SDL_GetError() << std::endl;
				success = false;
			}
			else
			{
				//VSync означает, что экран будет обновляться одновременно с обновлением монитора
				if ( SDL_SetRenderVSync ( Scada::gRenderer, 1 ) == false )
				{
					SDL_Log( "Не удалось включить VSync! Ошибка SDL: %s\n", SDL_GetError() );
				}

				//Initialize renderer color
				SDL_SetRenderDrawColor( Scada::gRenderer, 129, 191, 254, 0xFF );

				//Initialize PNG loading
				//int imgFlags = IMG_INIT_PNG;
				//if( !( IMG_Init( imgFlags ) & imgFlags ) )
				//{
				//	printf( "SDL_image could not initialize! SDL_image Error: %s\n", SDL_GetError() );
				//	success = false;
				//}

				 //Initialize SDL_ttf
				if( TTF_Init() == -1 )
				{
					std::cout << "SDL_ttf could not initialize! SDL_ttf Error: " << SDL_GetError() << std::endl;
					success = false;
				}

			}
		}
	}

	return success;
}
//Оптимальная функция загрузки Эта функция проверяет, не загружена ли текстура ранее, чтобы не дублировать данные в видеопамяти.
bool LoadTexture(const std::string& name, const std::string& path) {
	// Если такая текстура уже есть, не загружаем заново
	if (Scada::gSharedTextures.count(name)) return true;

	// 2. Используем метод вашего класса CTexture
	// Он сам создаст SDL_Texture, заполнит mWidth/mHeight и удалит surface
	if (Scada::gSharedTextures[name].loadFromFile(path)) {
		return true;
	}

	// Если загрузка не удалась, удаляем пустой ключ из карты, чтобы не занимал место
	Scada::gSharedTextures.erase(name);
	return false;
}


//Функция загрузка изображений /////////////////////////////////////////////////////////////
bool loadMedia()
{
	SDL_Surface *surfcursor = NULL;
	//Text rendering color
	SDL_Color textColor = { 0, 0, 0, 0xFF };
	
	//Loading success flag
	bool success = true;

	// Загрузка шрифта
	Font_ttf = TTF_OpenFont( FONT_TTF, 20 );
	if( Font_ttf == NULL )
	{
		std::cout << "Failed to load font! SDL_ttf Error: " << SDL_GetError() << std::endl;
		success = false;
	}

	 // Настройка курсора
	surfcursor = IMG_Load("./image/cursor53x66.png");
	if(surfcursor == NULL)
	{
		std::cout << "Can't load: " << SDL_GetError() << std::endl;
	}
	mousecursor= SDL_CreateColorCursor(surfcursor, 1 ,1);

	if (mousecursor == NULL)
	{
		std::cout << "Can't load: " << SDL_GetError() << std::endl;
	}
	SDL_SetCursor(mousecursor);

	//Предварительная загрузка данных (Алерты) Считывание данных об изображения из файла//////////////////////////////////
	read_alert(FILE_ALLERTMESSAGE);


	// ИНИЦИАЛИЗАЦИЯ КЭША ТЕКСТУР Резервируем место под максимальное кол-во элементов (например, 100)
	Scada::vstrValueMC.resize(4, ""); // Создает 4 пустые строки

	App::scene.loadConfig(IMAGES_CONF); //Функция парсера (Безопасная) retcoord.cpp

	//Загрузка изображений
	// Загрузка всех графических объектов (теперь только SVG)
	for (auto& [objName, element] : App::scene.getElements()) {

		std::string texName = element.textureKey;

		// 1. ЗАГРУЗКА ТЕКСТУРЫ (если еще не в памяти)
		if (Scada::gSharedTextures.find(texName) == Scada::gSharedTextures.end()) {
			// Теперь путь всегда к .svg
			std::string path = "./image/" + texName + ".svg";

			if (!Scada::gSharedTextures[texName].loadFromFile(path)) {
				std::cout << "Ошибка загрузки файла: " << path << std::endl;
				success = false;
				continue; // Пропускаем объект, если файл не найден
			}
		}

		// 2. ОПРЕДЕЛЕНИЕ РАЗМЕРОВ
		// Если в конфиге размеры 0 (новый объект), берем родной размер SVG
		if (element.rect.w <= 0.0f || element.rect.h <= 0.0f) {
			element.rect.w = (float)Scada::gSharedTextures[texName].getWidth();
			element.rect.h = (float)Scada::gSharedTextures[texName].getHeight();

			std::cout << "Объект [" << objName << "] инициализирован размером SVG: "
			<< element.rect.w << "x" << element.rect.h << std::endl;
		}
		else {
			// Если размеры > 0, значит они кастомные (из конфига) — не трогаем их
			std::cout << "Объект [" << objName << "] загружен с сохраненным размером: "
			<< element.rect.w << "x" << element.rect.h << std::endl;
		}
	}

	// 3. ОБНОВЛЯЕМ ОЧЕРЕДЬ ОТРИСОВКИ (Z-Order)
	// После того как все элементы загружены, один раз строим список слоев
	App::scene.refreshRenderOrder();



	return success;
}

//Function close//////////////////////////////////////////////////////////////////////
void close()
{
	//Free global font
	TTF_CloseFont( Font_ttf );
	Font_ttf = NULL;
	SDL_DestroyCursor(mousecursor);
	//Destroy window	
	SDL_DestroyRenderer( Scada::gRenderer );
	SDL_DestroyWindow( Scada::gWindow );
	Scada::gWindow = NULL;
	Scada::gRenderer = NULL;

	//Quit SDL subsystems
	TTF_Quit();
	//IMG_Quit();
	SDL_Quit();
}

//===========================================================================================


ProgressBar::ProgressBar(float x, float y, float w, float h): rect{x, y, w, h}
{

}

void ProgressBar::draw(SDL_Renderer* renderer, float currentValue) {
		// 1. Нормализуем значение от 0.0 до 1.0 (защита от выхода за границы)
		float ratio = (currentValue - minVal) / (maxVal - minVal);
		ratio = std::clamp(ratio, 0.0f, 1.0f);

		// 2. Рисуем подложку (фон)
		SDL_SetRenderDrawColor(renderer, 40, 40, 40, 255); // Темно-серый
		SDL_RenderFillRect(renderer, &rect);

		// 3. Рассчитываем цвет (зеленый -> желтый -> красный)
		uint8_t r = (ratio > 0.5f) ? 255 : (uint8_t)(ratio * 2 * 255);
		uint8_t g = (ratio < 0.5f) ? 255 : (uint8_t)((1.0f - ratio) * 2 * 255);
		SDL_SetRenderDrawColor(renderer, r, g, 0, 255);

		// 4. Рисуем заполнение (ширина зависит от ratio)
		SDL_FRect fillRect = { rect.x, rect.y, rect.w * ratio, rect.h };
		SDL_RenderFillRect(renderer, &fillRect);

		// 5. Рисуем контур
		SDL_SetRenderDrawColor(renderer, 200, 200, 200, 255);
		SDL_RenderRect(renderer, &rect);
}


