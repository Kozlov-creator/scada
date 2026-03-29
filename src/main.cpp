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



asio::io_context io_context;
udp::socket global_socket(io_context, udp::endpoint(udp::v4(), 1234));


std::string destIP = "127.0.0.1";
int destPort = 1234;
int localPort = 1235;

bool editMode = false;//Реализация через флаг Edit Mode. В режиме работы (Runtime) перетаскивание должно быть запрещено, чтобы оператор случайно не «унес» насос с экрана.

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
void read_layout_config(const std::string& file_name);

//-----------------------------------------------------------------------------
// Аргументы: IP адрес МК, порт МК, порт ПК (исходящий), массив данных, длина данных
void udp_send_data(const std::string& dst_ip, uint16_t dst_port, uint16_t src_port, uint8_t* data, uint16_t len) {
	try {
		asio::ip::udp::endpoint remote_endpoint(asio::ip::make_address(dst_ip), dst_port);

		// Отправляем данные через наш основной сокет
		// global_socket должен быть доступен здесь (сделайте его extern или передайте ссылкой)
		global_socket.send_to(asio::buffer(data, len), remote_endpoint);

		// std::cout << "Команда отправлена на " << dst_ip << ":" << dst_port << std::endl;
	} catch (std::exception& e) {
		std::cerr << "Ошибка отправки UDP: " << e.what() << std::endl;
	}
}

//-----------------------------------------------------------------------------
//Добавляем функцию отправки команды
void send_mcu_command(std::string objName, uint8_t cmd) {
	// Формируем пакет. Например: [ID_ОБЪЕКТА, КОМАНДА]
	// Для простоты отправим 1 байт: 1 - ПУСК, 0 - СТОП
	uint8_t packet[1] = { cmd };

	// IP адрес берем из remote_endpoint (из сетевого потока)
	// или задаем статически (ipaddr_pc)
	udp_send_data(destIP, destPort, localPort, packet, 1);

	std::cout << "UDP Command Sent: " << (int)cmd << " to " << objName << std::endl;
}
//-----------------------------------------------------------------------------
//Функция сетевого потока (UDP-клиентInputCoord) Эта функция будет постоянно слушать сеть.
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

	NetContext nCtx;
	nCtx.socket = &global_socket;
	nCtx.quitFlag = &net_quit;
	nCtx.mutex = data_mutex;

	read_layout_config(IMAGES_CONF); //Функция парсера (Безопасная) retcoord.cpp

	bool leftMouseButtonDown = false;
	int xpos=0;
	int ypos=0;
	float x=0;
	int m=0;
	int screen = 1;
	//Запуск SDL3 и создание окна
	std::cout<<"main"<<std::endl;



	if( !init() ) //texture_class.cpp
	{
		std::cout<< "Failed to initialize!" <<std::endl;
	}
	else
	{
		std::cout<<"init"<<std::endl;
		//Загрузка медиа данных
		if( !loadMedia() ) //texture_class.cpp
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
								std::cout << "Конфигурация сохранена вручную ('S')." << std::endl;
								break;
							}

							case SDLK_F2:
							{
								editMode = !editMode; // Переключаем режим (true <-> false)
								std::cout << "РЕЖИМ РЕДАКТИРОВАНИЯ: " << (editMode ? "ВКЛЮЧЕН" : "ВЫКЛЮЧЕН") << std::endl;

								if (editMode) {
								// Укажите имя вашего окна (например, window или gWindow)
								SDL_SetWindowTitle(gWindow, "SCADA [РЕДАКТИРОВАНИЕ] - F2 для выхода");
								std::cout << "Режим правки ВКЛ" << std::endl;}
								// Опционально: если выходим из режима редактирования, можно сразу сохранить конфиг
								else {
									SDL_SetWindowTitle(gWindow, "SCADA [РАБОТА] - F2 для правок");
									save_layout_config(IMAGES_CONF);
									std::cout << "Режим правки ВЫКЛ, конфиг сохранен" << std::endl;
								}
								break;
							}
						}
					}

					else if (e.type == SDL_EVENT_MOUSE_BUTTON_UP)
					{
						if (e.button.button == SDL_BUTTON_LEFT)
						{
							// Если это был быстрый клик (не тащили) и мы попали по объекту
							// 1. Если мы НЕ в режиме редактирования И это был просто клик (не перетаскивание)
							if (!editMode && !isDragging && !selectedObjectName.empty())
							{
								if (selectedObjectName == "CLOCK_SYSTEM") {
								screen = (screen == 1) ? 2 : 1;
								}
								else if (selectedObjectName != "bg_main") {
								activeControlObject = selectedObjectName;
								showControlWindow = true;
								}
							}

							// 2. Если мы БЫЛИ в режиме редактирования и что-то двигали
							if (editMode && isDragging) {
								std::cout << "Объект " << selectedObjectName << " перемещен в x:"
								<< selectedRect->x << " y:" << selectedRect->y << std::endl;
							}

							// Сброс всех состояний захвата
						leftMouseButtonDown = false;
						isDragging = false;
						selectedRect = nullptr;
						selectedObjectName = "";
						}
					}



					else if (e.type == SDL_EVENT_MOUSE_MOTION)
					{
						mousePos = { (float)e.motion.x, (float)e.motion.y };
						if (leftMouseButtonDown && selectedRect && editMode)
						{
							// Порог чувствительности 3 пикселя
							if (std::abs(e.motion.xrel) > 2 || std::abs(e.motion.yrel) > 2) {
								isDragging = true;
							}
							selectedRect->x = mousePos.x - clickOffset.x;
							selectedRect->y = mousePos.y - clickOffset.y;
						}
					}


					else if (e.type == SDL_EVENT_MOUSE_BUTTON_DOWN)
					{
						if (e.button.button == SDL_BUTTON_LEFT)
						{
							mousePos = { (float)e.button.x, (float)e.button.y };
							leftMouseButtonDown = true;
							isDragging = false;
							selectedRect = nullptr;
							selectedObjectName = "";

							// Сначала проверяем клик по кнопкам ОТКРЫТОГО окна (если оно есть)
							if (showControlWindow) {
								SDL_FRect btnClose = { controlWindowRect.x + controlWindowRect.w - 40, controlWindowRect.y + 10, 30, 30 };
								SDL_FRect btnOn = { controlWindowRect.x + 50, controlWindowRect.y + 150, 150, 80 };
								SDL_FRect btnOff = { controlWindowRect.x + 300, controlWindowRect.y + 150, 150, 80 };

								if (SDL_PointInRectFloat(&mousePos, &btnClose)) { showControlWindow = false; break; }
								if (SDL_PointInRectFloat(&mousePos, &btnOff))  { send_mcu_command(activeControlObject, 0); break; }
								if (SDL_PointInRectFloat(&mousePos, &btnOn))   { send_mcu_command(activeControlObject, 1); break; }

								// Если кликнули внутри окна, но не по кнопкам — блокируем клик сквозь окно
								if (SDL_PointInRectFloat(&mousePos, &controlWindowRect)) break;
							}

							// Если окно не перехватило клик, ищем объект на сцене
							if (SDL_PointInRectFloat(&mousePos, &fonClockRect)) {
								selectedObjectName = "CLOCK_SYSTEM";
							}
							else {
								for (auto& [name, element] : gSceneElements) {
									if (SDL_PointInRectFloat(&mousePos, &element.rect)) {
										selectedRect = &element.rect;
										selectedObjectName = name;
										break;
									}
								}
								if (!selectedRect) { // Если не нашли в картинках, ищем в тексте
									for (auto& [name, rect] : gTextConfig) {
										if (SDL_PointInRectFloat(&mousePos, &rect)) {
											selectedRect = &rect;
											selectedObjectName = name;
											break;
										}
									}
								}
							}

							if (selectedRect) {
								clickOffset.x = mousePos.x - selectedRect->x;
								clickOffset.y = mousePos.y - selectedRect->y;
							}
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
