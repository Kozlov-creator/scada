#include <map>
#include <stdio.h>
#include <sstream>
#include <fstream>
#include <iostream>
#include <unistd.h>
#include <algorithm> // для std::clamp

#include "globals.h"
#include "functions.h"

//#include <texture_class.h>
//#include <SDL3/SDL.h>
//#include <SDL3_image/SDL_image.h>
//#include <unordered_map>
//#include <vector>
//#include <string>

SDL_Cursor *mousecursor = NULL;

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

bool CTexture::loadFromFile( SDL_Renderer* renderer, const std::string& path )
{
	//Загрузить изображение по указанному пути
	SDL_Surface *loadedSurface = IMG_Load( path.c_str() );
	if( loadedSurface == nullptr )
	{
		std::cout<< "Unable to load image "<< path.c_str()<<" SDL_image Error: " << SDL_GetError() <<std::endl;
		return false;
	}

	//Избавьтесь от старой загруженной поверхности
	bool success = createFromSurface(renderer, loadedSurface);
	SDL_DestroySurface(loadedSurface);
	return success;

}

bool CTexture::loadFromRenderedText( SDL_Renderer* renderer, const std::string& textureText, SDL_Color textColor, TTF_Font* Font_ttf )
{
	//Render text surface
	SDL_Surface *textSurface = TTF_RenderText_Solid( Font_ttf, textureText.c_str(), textureText.size(), textColor );
	if( textSurface == nullptr )
	{
		std::cout<< "Unable to render text surface! SDL_ttf Error: "<< SDL_GetError() <<std::endl;
		return false;
	}
	
	//Избавьтесь от старой загруженной поверхности
	bool success = createFromSurface(renderer, textSurface);
	SDL_DestroySurface(textSurface);
	return success;

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

float CTexture::getWidth() const
{
	return mWidth;
}

float CTexture::getHeight() const
{
	return mHeight;
}

SDL_Texture* CTexture::getTexture() const
{
	return mTexture;
}

// В private:
bool CTexture::createFromSurface(SDL_Renderer* renderer, SDL_Surface* surface) {
	//Избавьтесь от ранее существовавшей текстуры
	freeTexture();
	mTexture = SDL_CreateTextureFromSurface(renderer, surface);
	if (mTexture != nullptr) {
		mWidth = (float)surface->w;
		mHeight = (float)surface->h;
		SDL_SetTextureBlendMode(mTexture, SDL_BLENDMODE_BLEND);
	}
	return mTexture != nullptr;
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
	if (Scada::gSharedTextures[name].loadFromFile(Scada::gRenderer, path)) {
		return true;
	}

	// Если загрузка не удалась, удаляем пустой ключ из карты, чтобы не занимал место
	Scada::gSharedTextures.erase(name);
	return false;
}

//============================================================================================
//Загрузка системных ресурсов (Шрифты)
bool loadFonts(TTF_Font*& font, const std::string& path, int size) {
	// 1. Очищаем старый шрифт, если он там был
	if (font != nullptr) {
		TTF_CloseFont(font);
	}

	// 2. Загружаем новый прямо в переданную переменную
	font = TTF_OpenFont(path.c_str(), size);

	if (font == nullptr) {
		std::cout << "Ошибка загрузки: " << SDL_GetError() << std::endl;
		return false;
	}
	return true;
}
//============================================================================================
//Настройка интерфейса (Курсор)
bool initCustomCursor(SDL_Cursor*& cursor, const std::string& path) {
	SDL_Surface* surf = IMG_Load(path.c_str());
	if (!surf) {
		std::cerr << "Can't load cursor image: " << SDL_GetError() << std::endl;
		return false;
	}

	// Если старый курсор был — удаляем его из памяти
	if (cursor != nullptr) {
		SDL_DestroyCursor(cursor);// Обязательно освобождаем!
	}
	// Создаем аппаратный курсор (данные копируются в видеокарту)
	cursor = SDL_CreateColorCursor(surf, 1, 1);

	// ОЧИСТКА: Удаляем поверхность из оперативной памяти
	// Она больше не нужна, так как курсор уже в видеопамяти
	SDL_DestroySurface(surf);

	if (!cursor) {
		std::cerr << "CreateCursor Error: " << SDL_GetError() << std::endl;
		return false;
	}

	SDL_SetCursor(cursor);
	return true;
}
//============================================================================================
//Загрузка игровых ассетов (Текстуры и сцена) Здесь мы отделяем логику парсинга от логики загрузки.
bool loadSceneAssets() {
	read_alert(FILE_ALLERTMESSAGE);
	Scada::vstrValueMC.resize(4, "");

	if (!App::scene.loadConfig(IMAGES_CONF)) return false;

	for (auto& [objName, element] : App::scene.getElements()) {
		std::string texName = element.textureKey;

		// Если текстуры нет в кэше — грузим
		if (Scada::gSharedTextures.find(texName) == Scada::gSharedTextures.end()) {
			std::string path = "./image/" + texName + ".svg";
			if (!Scada::gSharedTextures[texName].loadFromFile(Scada::gRenderer, path)) {
				return false;
			}
		}

		// Авто-размер, если не задан в конфиге
		if (element.rect.w <= 0.0f || element.rect.h <= 0.0f) {
			element.rect.w = Scada::gSharedTextures[texName].getWidth();
			element.rect.h = Scada::gSharedTextures[texName].getHeight();
		}
	}

	App::scene.refreshRenderOrder();
	return true;
}


//===========================================================================================
//Функция загрузка изображений /////////////////////////////////////////////////////////////
bool loadMedia()
{
	bool success = true;

	// Загружаем шрифт прямо в глобальную переменную Scada
	if (!loadFonts(Scada::gMainFont, FONT_TTF, 20)) {
		success = false;
	}

	// Загружаем курсор
	if (!initCustomCursor(Scada::gMouseCursor, "./image/cursor53x66.png")) {
		success = false;
	}

	// Загружаем всё остальное (алерты, конфиги, SVG)
	if (!loadSceneAssets()) {
		success = false;
	}

	return success;
}

//Function close//////////////////////////////////////////////////////////////////////
void cleanup()
{
	//Free global font
	if (Scada::gMainFont) {
		TTF_CloseFont(Scada::gMainFont);
		Scada::gMainFont = nullptr;
	}

	SDL_DestroyCursor(mousecursor);
	//Destroy window	
	SDL_DestroyRenderer( Scada::gRenderer );
	SDL_DestroyWindow( Scada::gWindow );

	Scada::gWindow = NULL;
	Scada::gRenderer = NULL;

	//Quit SDL subsystems
	TTF_Quit();
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


