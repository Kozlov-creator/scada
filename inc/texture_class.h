#ifndef __TEXTURE_CLASS_H__
#define __TEXTURE_CLASS_H__


//Класс обертка для работы с текстурами

class CTexture
{
	public:
		//Инициализирует переменные Initializes variables
		CTexture();

		//Освобождает память Deallocates memory
		~CTexture();

		//Загружает изображение по указанному пути Loads image at specified path
		bool loadFromFile( std::string path );

		bool loadTextureFromFile( std::string path);


		//Создает изображение из строки шрифта Creates image from font string
		bool loadFromRenderedText( std::string textureText, SDL_Color textColor );


		//Освобождает текстуру Deallocates texture
		void freeTexture();

		//Установить цветовую модуляцию Set color modulation
		void setColor( Uint8 red, Uint8 green, Uint8 blue );

		//Установить смешивание Set blending
		void setBlendMode( SDL_BlendMode blending );

		//Установить альфа-модуляцию Set alpha modulation
		void setAlpha( Uint8 alpha );

		//Визуализирует текстуру в заданной точке Renders texture at given point
		//void render( float x, float y, SDL_FRect *clipRect = nullptr, double angle = 0.0, SDL_FPoint *center = nullptr, SDL_FlipMode flipRender = SDL_FLIP_NONE );
		void render( int xy =0, SDL_FRect *renderQuad = nullptr, SDL_FRect *clipRect = nullptr, double angle = 0.0, SDL_FPoint *center = nullptr, SDL_FlipMode flipRender = SDL_FLIP_NONE );

		//Получает размеры изображения Gets image dimensions
		float getWidth();
		float getHeight();
		SDL_Texture *getTexture();
		void setXY(float x, float y);

	private:
		//Фактическая текстура оборудования The actual hardware texture
		SDL_Texture *mTexture;

		//Размеры изображения Image dimensions
		float mWidth;
		float mHeight;
		float mX;
		float mY;
};


// ФУНКЦИИ

SDL_Texture *LoadFromRenderedText (std::string, SDL_FRect&);

//void LoadFromRenderedText (std::string, SDL_Rect&, SDL_Texture&);

void LoadImageTextureFromFile( std::string , SDL_FRect* );

//Запускает SDL2 и создает окно Starts up SDL2 and creates window
bool init();

//Загрузка медиа данных Loads media
bool loadMedia();

//Освобождает медиа данные и отключает SDL2 Frees media and shuts down SDL2
void close();


#endif //__TEXTURE_CLASS_H__
