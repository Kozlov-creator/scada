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
#include <dot_class.h>
#include <const_data.h>
#include <main.h>
#include "data_struct.h" //  структура с сенсорами
#include <asio.hpp>

using asio::ip::udp;

// Глобальное хранилище координат
// 2. ОБЪЕКТЫ: Описание элементов на экране
struct ScadaElement {
	std::string textureKey; // Имя текстуры из gSharedTextures
	SDL_FRect rect;         // Координаты на экране
};

// Карта всех объектов (Ключ: "pump_left", "bg_main" и т.д.)
extern std::map<std::string, ScadaElement> gSceneElements;
extern std::map<std::string, SDL_FRect> gTextConfig;

// Глобальные данные для обмена между потоками
SensorData shared_sensor_data;
SDL_Mutex* data_mutex;
SDL_Mutex* modbus_mutex;
bool net_quit = false;
bool modbus_quit = false;

//Окно, в которое мы будем отображать
SDL_Window *gWindow = NULL;

//Средство визуализации окна
SDL_Renderer *gRenderer = NULL;

//Глобально используемый шрифт
TTF_Font *gFont = NULL;

SDL_FRect *selectedRect = NULL;


SDL_FRect fonClockRect={1490,5,400,50};


bool showControlWindow = false;    // Флаг: показано ли окно
std::string activeControlObject = ""; // Имя объекта (например, "pump_left")
SDL_FRect controlWindowRect = { 700, 300, 500, 400 }; // Координаты окна по центру


// Структура для передачи параметров в поток (если нужно)
struct ThreadConfig {
	std::string fileName;
	bool* quitFlag;
};

//Создаем структуру контекста Объедините флаги и объекты в одну структуру, чтобы не плодить глобальные переменные.
struct NetContext {
	asio::ip::udp::socket* socket; // Указатель на сокет
	bool* quitFlag;                // Указатель на флаг выхода
	SDL_Mutex* mutex;              // Мьютекс для данных
};


void save_layout_config(const std::string& file_name);
void InputCoord();

//-----------------------------------------------------------------------------
//Функция сетевого потока (UDP-клиент) Эта функция будет постоянно слушать сеть.
int network_thread_func(void* ptr) {
	// 1. Извлекаем контекст
	NetContext* ctx = (NetContext*)ptr;

	// 2. Буфер для данных
	char recv_buf[sizeof(SensorData)];
	asio::ip::udp::endpoint remote_endpoint;

	// 3. Устанавливаем таймаут (используем сокет из контекста!)
	/*struct timeval tv;
	tv.tv_sec = 1;
	tv.tv_usec = 0;
	setsockopt(ctx->socket->native_handle(), SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));*/

	// 4. Цикл (проверяем флаг через указатель из контекста)
	while (!(*ctx->quitFlag)) {
		try {
			asio::error_code ec;
			// Читаем через сокет, который живет в main
			size_t len = ctx->socket->receive_from(asio::buffer(recv_buf), remote_endpoint, 0, ec);

			if (ec == asio::error::operation_aborted || ec == asio::error::bad_descriptor) {
				// Если сокет закрыт или произошла ошибка - выходим из цикла
				break;
			}

			if (!ec && len == sizeof(SensorData)) {
				SDL_LockMutex(ctx->mutex);
				memcpy(&shared_sensor_data, recv_buf, sizeof(SensorData));
				SDL_UnlockMutex(ctx->mutex);
			}
		} catch (...) { /* ошибка сети */   break; // При любой системной ошибке сокета — выход

		}
	}
	return 0;
}


//-----------------------------------------------------------------------------
// Поток для чтения файла-----------------------------------------------------------
int modbus_thread_func(void* data) {
	 ThreadConfig* config = (ThreadConfig*)data;

	 while (!(*config->quitFlag)) {
		 read_modbus(config->fileName);
		// Читаем данные во временные векторы

		SDL_Delay(500); // Читаем файл 2 раза в секунду, а не 60! чтобы не нагружать диск
	}
	return 0;
}
//-----------------------------------------------------------------------------
////////////////////////////////////////////////////////////////

int main( int argc, char *args[] )
{
	SDL_FPoint mousePos;
	SDL_Point clickOffset;

	asio::io_context io_context;
	udp::socket global_socket(io_context, udp::endpoint(udp::v4(), 1234));

	NetContext nCtx;
	nCtx.socket = &global_socket;
	nCtx.quitFlag = &net_quit;
	nCtx.mutex = data_mutex;

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

			// --- ВНУТРИ main, перед while(!quit) ---
			ThreadConfig mConfig = { FILE_MODBUS, &modbus_quit };

			data_mutex = SDL_CreateMutex();
			modbus_mutex = SDL_CreateMutex(); // Создаем мьютекс
			net_quit = false;
			modbus_quit = false;


			// Запускаем поток
			SDL_Thread* netThread = SDL_CreateThread(network_thread_func, "NetThread", &nCtx);
			SDL_Thread* modbusThread = SDL_CreateThread(modbus_thread_func, "ModbusThread", &mConfig);

			bool isDragging = false; // Глобально или в main
			std::string selectedObjectName = ""; // Имя выбранного объекта
			//Основной цикл
			while( !quit )
			{
				//Обрабатывать события в очереди
				while( SDL_PollEvent( &e ) != 0 )
				{
					//Запрос пользователя для выхода
					if( e.type == SDL_EVENT_QUIT )
					{	quit = true;
						net_quit = true; // Сигнал сетевому потоку на выход
						*mConfig.quitFlag = true;

						// ПРИНУДИТЕЛЬНО закрываем сокет.
						// Это заставит receive_from в потоке немедленно вернуть ошибку ec.
						// Мягко останавливаем асио
						global_socket.close();
						io_context.stop();

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

							case SDLK_S:
							{ // Нажмите 'S' для сохранения
								save_layout_config(IMAGES_CONF);
								std::cout << "Конфигурация успешно сохранена!" << std::endl;
							}
						}
					}
					else if (e.type == SDL_EVENT_MOUSE_BUTTON_UP)
					{
						if (leftMouseButtonDown && e.button.button == SDL_BUTTON_LEFT)
								{
									leftMouseButtonDown = false;
									selectedRect = NULL;

									// Если мы НЕ тащили объект, значит это КЛИК
									if (!isDragging && !selectedObjectName.empty()) {

										// ЛОГИКА УПРАВЛЕНИЯ ПО ИМЕНАМ
										if (selectedObjectName == "pump_left") {
											std::cout << "ОТКРЫВАЕМ ОКНО: Управление левым насосом" << std::endl;
											// Здесь вызывайте вашу функцию: OpenPumpControl(1);
										}
										else if (selectedObjectName == "pump_right") {
											std::cout << "ОТКРЫВАЕМ ОКНО: Управление правым насосом" << std::endl;
										}
										else if (selectedObjectName == "bg_main") {
											std::cout << "Клик по фону - ничего не делаем" << std::endl;
										}
									}

									leftMouseButtonDown = false;
									selectedRect = nullptr;
									selectedObjectName = "";

								}
					}

					else if( e.type == SDL_EVENT_MOUSE_MOTION)
					{
						mousePos = { e.motion.x, e.motion.y };

						if (leftMouseButtonDown && selectedRect != NULL)
						{
							isDragging = true; // Мы начали двигать объект
							selectedRect->x = mousePos.x - clickOffset.x;

							selectedRect->y = mousePos.y - clickOffset.y;

						}
					}
					else if (e.type == SDL_EVENT_MOUSE_BUTTON_DOWN)
					{
						if (showControlWindow) {
							// Проверяем кнопки ПУСК/СТОП/ЗАКРЫТЬ
							SDL_FRect btnClose = { controlWindowRect.x + controlWindowRect.w - 40, controlWindowRect.y + 10, 30, 30 };
							SDL_FRect btnOn = { controlWindowRect.x + 50, controlWindowRect.y + 150, 150, 80 };

							if (SDL_PointInRectFloat(&mousePos, &btnClose)) {
								showControlWindow = false;
							}
							else if (SDL_PointInRectFloat(&mousePos, &btnOn)) {
								std::cout << "ОТПРАВЛЯЕМ UDP КОМАНДУ ВКЛ ДЛЯ: " << activeControlObject << std::endl;
								// Здесь ваш udp_send_data(...);
							}
							break; // Важно: не даем клику пройти к объектам на фоне!
						}

						if (!leftMouseButtonDown && e.button.button == SDL_BUTTON_LEFT)
						{
							leftMouseButtonDown = true;
							selectedRect = nullptr; // Сбрасываем выбор

							isDragging = false; // Пока еще не двигаем
							selectedObjectName = "";

							// Ищем среди картинок
							for (auto& [name, element] : gSceneElements) {
								if (SDL_PointInRectFloat(&mousePos, &element.rect)) {
									selectedRect = &element.rect;
									selectedObjectName = name; // Запоминаем имя!
									clickOffset.x = mousePos.x - element.rect.x;
									clickOffset.y = mousePos.y - element.rect.y;
									break;
								}
							}

							// 1. Проверка клика по фону часов (как у вас было)
							if (SDL_PointInRectFloat(&mousePos, &fonClockRect))
							{
								screen = (screen == 1) ? 2 : 1;
								// Если фон часов — это просто кнопка перехода, выходим
								// Если его тоже надо двигать — не вызывайте return
								break;
							}

							// 2. Ищем среди ГРАФИЧЕСКИХ ОБЪЕКТОВ (IMG:)
							for (auto& [name, element] : gSceneElements)
							{
								if (SDL_PointInRectFloat(&mousePos, &element.rect))
								{
									selectedRect = &element.rect;
									clickOffset.x = mousePos.x - element.rect.x;
									clickOffset.y = mousePos.y - element.rect.y;
									std::cout << "Selected IMG: " << name << std::endl;
									break; // Нашли объект, выходим из циклов
								}
							}

							// 3. Ищем среди ТЕКСТОВЫХ ЗОН (TXT:)
							for (auto& [name, rect] : gTextConfig)
							{
								if (SDL_PointInRectFloat(&mousePos, &rect))
								{
									selectedRect = &rect;
									clickOffset.x = mousePos.x - rect.x;
									clickOffset.y = mousePos.y - rect.y;
									std::cout << "Selected TXT: " << name << std::endl;
									break;
								}
							}
						}

						if (!isDragging && !selectedObjectName.empty()) {
							showControlWindow = true;
							activeControlObject = selectedObjectName;
						}
					}

				}

				// --- ОБНОВЛЕНИЕ ГРАФИКИ ДАННЫМИ ---
				SDL_LockMutex(data_mutex);
				// Теперь shared_sensor_data содержит актуальные значения
				// Можно обновить тексты или координаты объектов:
				// label_temp.text = std::to_string(shared_sensor_data.temperature);
				SDL_UnlockMutex(data_mutex);

			// Отрисовка десктопов
			switch (screen)
			{
				case 1:	desctop1();	break;
				case 2:	desctop2();	break;

			}
			SDL_DelayNS( 16666666 ); // ~60 FPS
			}

			//Дождитесь завершения потока
			//SDL_WaitThread( threadID, NULL );

			// --- ЗАВЕРШЕНИЕ ---
			// 2. Теперь поток точно выйдет из цикла и завершится.
			// Ждем его здесь:

			if (netThread) SDL_DetachThread(netThread);// "грязный", но рабочий хак для диагностики:

			// 3. И только теперь удаляем мьютексы
			SDL_DestroyMutex(data_mutex);
			SDL_WaitThread(modbusThread, nullptr);

			SDL_DestroyMutex(modbus_mutex);

		}
	}

	//Free resources and close SDL
	close();

	return 0;
}
