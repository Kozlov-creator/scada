#pragma once

//Класс обертка для работы с текстурами

class CTexture
{
	public:
		//Инициализирует переменные Initializes variables
		CTexture();

		//Освобождает память Deallocates memory
		~CTexture();

		//Загружает изображение по указанному пути Loads image at specified path
		bool loadFromFile( SDL_Renderer* renderer, const std::string& path );

		//Создает изображение из строки шрифта Creates image from font string
		bool loadFromRenderedText( SDL_Renderer* renderer, const std::string& textureText, SDL_Color textColor, TTF_Font* Font_ttf );

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
		void render( int xy =0, const SDL_FRect *renderQuad = nullptr, SDL_FRect *clipRect = nullptr, double angle = 0.0, SDL_FPoint *center = nullptr, SDL_FlipMode flipRender = SDL_FLIP_NONE );

		//Получает размеры изображения Gets image dimensions
		float getWidth() const;
		float getHeight() const;
		SDL_Texture *getTexture() const;
		void setXY(float x, float y);

	private:
		//Фактическая текстура оборудования The actual hardware texture
		SDL_Texture *mTexture;

		//Размеры изображения Image dimensions
		float mWidth;
		float mHeight;
		float mX;
		float mY;

		bool createFromSurface(SDL_Renderer* renderer, SDL_Surface* surface);
};
