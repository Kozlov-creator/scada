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
#include <const_data.h>
#include <main.h>

// 1. РЕСУРСЫ: Уникальные текстуры (Ключ: "pump" -> Обьект CTexture с данными из файла pump.png)
std::map<std::string, CTexture> gSharedTextures;

// 2. ОБЪЕКТЫ: Описание элементов на экране
struct ScadaElement {
	std::string textureKey; // Имя текстуры из gSharedTextures
	SDL_FRect rect;         // Координаты на экране
};

// Карта всех объектов (Ключ: "pump_left", "bg_main" и т.д.)
std::map<std::string, ScadaElement> gSceneElements;

// 3. ТЕКСТ: Координаты текстовых полей
extern std::map<std::string, SDL_FRect> gTextConfig;


SDL_Surface *surfcursor = NULL;
SDL_Cursor *mousecursor = NULL;
std::vector<std::string> vstrValueMC;

//Окно для рендеринга
extern SDL_Window *gWindow;
//Рендер окна
extern SDL_Renderer *gRenderer;
//Глобальный шрифт
extern TTF_Font *gFont;
extern SDL_Texture *ClockFon;
//Scene textures
// Глобальное хранилище текстур
extern std::map<std::string, SDL_Texture*> gImageTexture;

extern std::vector<SDL_Texture*> vimageTexture;
extern std::vector<SDL_Texture*> vDateTimeTextures;
extern std::vector<std::string> vstrImage;
std::vector<SDL_FRect> vRectClipCoord;

//Подготовка глобальных контейнеров
// Кэш текстур для значений (vstrValue)
std::vector<CTexture> vValueTextures;
// Кэш текстур для даты/времени (vstrDate)
std::vector<CTexture> vDateTextures;
// Последние отрисованные строки (для сравнения)
std::vector<std::string> vLastValueStrings;
std::vector<std::string> vLastDateStrings;
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

	newTexture = IMG_LoadTexture(gRenderer,path.c_str());

	if(newTexture == nullptr)
	{
		std::cout<<"Can't load: "<<SDL_GetError()<<std::endl;
	}

	//Успешное возращение
	mTexture = newTexture;
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
	}
	else
	{
		//Color key image
		SDL_SetSurfaceColorKey( loadedSurface, true, SDL_MapSurfaceRGB( loadedSurface, 0, 0xFF, 0xFF ) );

		//Create texture from surface pixels
        newTexture = SDL_CreateTextureFromSurface( gRenderer, loadedSurface );
		if( newTexture == nullptr )
		{
			std::cout<< "Unable to create texture from "<<path.c_str() <<"SDL Error: "<< SDL_GetError() <<std::endl;
		}
		else
		{
			//Get image dimensions
			mWidth = loadedSurface->w;
			mHeight = loadedSurface->h;
		}

		//Избавьтесь от старой загруженной поверхности
		SDL_DestroySurface( loadedSurface );
	}

	//Return success
	mTexture = newTexture;
	return mTexture != nullptr;

}

bool CTexture::loadFromRenderedText( std::string textureText, SDL_Color textColor )
{
	//Избавьтесь от ранее существовавшей текстуры
	freeTexture();

	//Render text surface
	SDL_Surface *textSurface = TTF_RenderText_Solid( gFont, textureText.c_str(), textureText.size(), textColor );
	if( textSurface != nullptr )
	{
		//Create texture from surface pixels
        mTexture = SDL_CreateTextureFromSurface( gRenderer, textSurface );
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
void CTexture::render( int xy, SDL_FRect *renderQuad, SDL_FRect *clipRect, double angle, SDL_FPoint *center, SDL_FlipMode flipRender )
{
	//Set rendering space and render to screen
	//SDL_FRect renderQuad = { x, y, mWidth, mHeight };
	if ( renderQuad == nullptr )
	{SDL_FRect temprenderQuad = { mX, mY, mWidth, mHeight }; renderQuad=&temprenderQuad;}
	else
	{
		renderQuad -> w = mWidth;
		renderQuad -> h = mHeight;
		if (xy)
		{
			renderQuad -> x = mX;
			renderQuad -> y = mY;
		}

	}

	//Set clip rendering dimensions
	if( clipRect != nullptr )
	{
		renderQuad -> w = clipRect->w;
		renderQuad -> h = clipRect->h;
		renderQuad -> x = clipRect -> x;
		renderQuad -> y = clipRect -> y;
	}

	//Render to screenrenderQuad
	SDL_RenderTextureRotated( gRenderer, mTexture, clipRect, renderQuad, angle, center, flipRender );
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
void LoadImageTextureFromFile( std::string path, SDL_FRect *vRectClipCoord )
{
	//Конечная текстура The final texture
	SDL_Texture *tempTexture = nullptr;
	float w, h;

	tempTexture = IMG_LoadTexture(gRenderer,path.c_str());
	if(tempTexture == nullptr)
	{
		std::cout<<"Can't load: "<<SDL_GetError()<<std::endl;
	}

	// Получаем размеры текстуры

		SDL_GetTextureSize(tempTexture, &w, &h);

	//SDL_GetTextureProperties(tempTexture);
	//if (SDL_QueryTexture(tempTexture, NULL, NULL, &w, &h) != 0) {
	//	std::cout << "SDL_QueryTexture failed: " << SDL_GetError() << std::endl;
	//	SDL_DestroyTexture(tempTexture); // Освобождаем память, если не удалось получить размеры
	//	return;
	//}

	if (vRectClipCoord != nullptr)
	{
		vRectClipCoord->w = w;
		vRectClipCoord->h = h;
	}

	vimageTexture.push_back(tempTexture); // Добавляем новую текстуру в вектор
}

//Функция создание изображения из текста///////////////////////////////////////////////////////////
SDL_Texture *LoadFromRenderedText(std::string sstr, SDL_FRect &vRectClipCoord)
{

	SDL_Color textColor = {0, 0, 0, 0xFF};
	SDL_Surface *textSurface = nullptr;
	SDL_Texture *tempTexture = nullptr;
	float w, h;

	// Render text surface
	textSurface = TTF_RenderText_Solid(gFont, sstr.c_str(), sstr.size(), textColor);
	if (textSurface == nullptr) {
		std::cout<<"Unable to render text surface! SDL_ttf Error: "<< SDL_GetError()<<std::endl;
		return nullptr; // Важно: Выход из функции при ошибке
	}

	// Create texture from surface pixels
	tempTexture = SDL_CreateTextureFromSurface(gRenderer, textSurface);
	SDL_DestroySurface(textSurface);
	if (tempTexture == nullptr) {
		std::cout<<"Unable to create texture from rendered text! SDL Error: "<< SDL_GetError()<<std::endl;
		SDL_DestroySurface(textSurface); // Освобождаем поверхность, если создание текстуры не удалось
		return nullptr; // Важно: Выход из функции при ошибке
	}

	 //Получаем размеры текстуры (безопаснее, чем прямое обращение к textSurface->w/h)
	//if (SDL_QueryTexture(tempTexture, NULL, NULL, &w, &h) != 0) {
	//	std::cerr << "SDL_QueryTexture failed: " << SDL_GetError() << std::endl;
	//	SDL_DestroyTexture(tempTexture);
	//	return nullptr;
	//}
	SDL_GetTextureSize(tempTexture, &w, &h);
	//Get rid of old surface
		vRectClipCoord.w = w;
		vRectClipCoord.h = h;

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
		gWindow = SDL_CreateWindow( " SCADA", SCREEN_WIDTH, SCREEN_HEIGHT, 0 );//| SDL_WINDOW_BORDERLESS|SDL_WINDOW_FULLSCREEN);
		if( gWindow == NULL )
		{
			std::cout<< "Window could not be created! SDL Error: "<< SDL_GetError() << std::endl;
			success = false;
		}
		else
		{
			//Создание синхронизированного средства визуализации для окна Create vsynced renderer for window
			gRenderer = SDL_CreateRenderer( gWindow, NULL );
			if( gRenderer == NULL )
			{
				std::cout<< "Renderer could not be created! SDL Error: "<< SDL_GetError() << std::endl;
				success = false;
			}
			else
			{
				//VSync означает, что экран будет обновляться одновременно с обновлением монитора
				if ( SDL_SetRenderVSync ( gRenderer, 1 ) == false )
				{
					SDL_Log( "Не удалось включить VSync! Ошибка SDL: %s\n", SDL_GetError() );
				}

				//Initialize renderer color
				SDL_SetRenderDrawColor( gRenderer, 129, 191, 254, 0xFF );

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
	if (gSharedTextures.count(name)) return true;

	// 2. Используем метод вашего класса CTexture
	// Он сам создаст SDL_Texture, заполнит mWidth/mHeight и удалит surface
	if (gSharedTextures[name].loadFromFile(path)) {
		return true;
	}

	// Если загрузка не удалась, удаляем пустой ключ из карты, чтобы не занимал место
	gSharedTextures.erase(name);
	return false;
}


//Функция загрузка изображений /////////////////////////////////////////////////////////////
bool loadMedia()
{
	//Text rendering color
	SDL_Color textColor = { 0, 0, 0, 0xFF };
	
	//Loading success flag
	bool success = true;

	// Загрузка шрифта
	gFont = TTF_OpenFont( font_ttf, 20 );
	if( gFont == NULL )
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
	read_alert(file_allertmessage);


	// ИНИЦИАЛИЗАЦИЯ КЭША ТЕКСТУР Резервируем место под максимальное кол-во элементов (например, 100)
	vValueTextures.resize(100);
	vLastValueStrings.resize(100, "");

	vDateTextures.resize(20);
	vLastDateStrings.resize(20, "");
	vstrValueMC.resize(4, ""); // Создает 4 пустые строки

	//Загрузка изображений
	// Проходим по всем объектам, которые считал парсер в gSceneElements
	for (auto& [objName, element] : gSceneElements) {

		std::string texName = element.textureKey;

		// Если текстура с таким именем еще не загружена в gSharedTextures
		if (gSharedTextures.find(texName) == gSharedTextures.end()) {
			std::string path = "./image/" + texName + ".png";

			if (!gSharedTextures[texName].loadFromFile(path)) {
				std::cout << "Ошибка загрузки файла: " << path << std::endl;
				success = false;
			}
		}
		// 2. БЕРЕМ РАЗМЕРЫ ИЗ КЛАССА ТЕКСТУРЫ
		// Теперь нам не нужен SDL_GetTextureSize, так как класс CTexture уже всё знает
		element.rect.w = (float)gSharedTextures[texName].getWidth();
		element.rect.h = (float)gSharedTextures[texName].getHeight();
	}
	return success;
}

//Function close//////////////////////////////////////////////////////////////////////
void close()
{
	std::string coordstr;
	std::ofstream inrect;
	int coordx=0,coordy=0;
	inrect.open(FILE_RECTCOORD);

	if (inrect.is_open())
	{
		for (int s=0;s<vRectClipCoord.size();s++)
		{

			coordstr += std::to_string(vRectClipCoord.at(s).x);
			coordstr += " , ";
			coordstr += std::to_string(vRectClipCoord.at(s).y);

			inrect<< coordstr.c_str()<<std::endl; // populate data file
			coordstr.clear();

		}

		inrect.close();
	}

	//Free global font
	TTF_CloseFont( gFont );
	gFont = NULL;
	SDL_DestroyCursor(mousecursor);
	//Destroy window	
	SDL_DestroyRenderer( gRenderer );
	SDL_DestroyWindow( gWindow );
	gWindow = NULL;
	gRenderer = NULL;

	//Quit SDL subsystems
	TTF_Quit();
	//IMG_Quit();
	SDL_Quit();
}


