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
		bool loadFromFile( std::string path );

		bool loadSVGAuto( std::string path );

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
		void render( int xy =0, const SDL_FRect *renderQuad = nullptr, SDL_FRect *clipRect = nullptr, double angle = 0.0, SDL_FPoint *center = nullptr, SDL_FlipMode flipRender = SDL_FLIP_NONE );

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


class ProgressBar {
public:
	SDL_FRect rect;      // Позиция и размер {x, y, w, h}
	float minVal = 0.0f;
	float maxVal = 40.0f;

	ProgressBar(float x, float y, float w, float h);

	void draw(SDL_Renderer* renderer, float currentValue);
};

