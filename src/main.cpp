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
#include <asio.hpp>

#include "globals.h"
#include "functions.h"

#include <cmath>

using asio::ip::udp;

asio::io_context io_context;
udp::socket global_socket(io_context, udp::endpoint(udp::v4(), 1234));

bool net_quit = false;
bool modbus_quit = false;

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
	udp_send_data(Scada::destIP, Scada::destPort, Scada::localPort, packet, 1);

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
				SensorData incoming;

				memcpy(&incoming, recv_buf, sizeof(SensorData));
				SDL_LockMutex(ctx->mutex);
				// Проверяем: изменился ли timestamp от контроллера?
				if (incoming.timestamp != Scada::lastMcuTimestamp) {
					Scada::lastMcuTimestamp = incoming.timestamp;
					Scada::lastUdpUpdateTimePC = SDL_GetTicks(); // Фиксируем время ПК
				}
				std::cout<< "incoming.timestamp = " << incoming.timestamp << "|| lastMcuTimestamp = " << Scada::lastMcuTimestamp << std::endl;
				Scada::shared_sensor_data = incoming;
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
	nCtx.mutex = Scada::data_mutex;



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

			Scada::data_mutex = SDL_CreateMutex();
			Scada::modbus_mutex = SDL_CreateMutex(); // Создаем мьютекс
			net_quit = false;
			modbus_quit = false;


			// Запускаем поток
			SDL_Thread* netThread = SDL_CreateThread(network_thread_func, "NetThread", &nCtx);
			SDL_Thread* modbusThread = SDL_CreateThread(modbus_thread_func, "ModbusThread", &mConfig);

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
								App::scene.saveConfig(IMAGES_CONF);
								std::cout << "Конфигурация сохранена вручную ('S')." << std::endl;
								break;
							}

							case SDLK_F2:
							{
								Scada::editMode = !Scada::editMode; // Переключаем режим (true <-> false)
								std::cout << "РЕЖИМ РЕДАКТИРОВАНИЯ: " << (Scada::editMode ? "ВКЛЮЧЕН" : "ВЫКЛЮЧЕН") << std::endl;

								if (Scada::editMode) {
								// Укажите имя вашего окна (например, window или Scada::gWindow)
								SDL_SetWindowTitle(Scada::gWindow, "SCADA [РЕДАКТИРОВАНИЕ] - F2 для выхода");
								std::cout << "Режим правки ВКЛ" << std::endl;}
								// Опционально: если выходим из режима редактирования, можно сразу сохранить конфиг
								else {
									SDL_SetWindowTitle(Scada::gWindow, "SCADA [РАБОТА] - F2 для правок");
									App::scene.saveConfig(IMAGES_CONF);
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
							if (!Scada::editMode && !Scada::isDragging && !selectedObjectName.empty())
							{
								if (selectedObjectName == "CLOCK_SYSTEM") {
								screen = (screen == 1) ? 2 : 1;
								}
								else if (selectedObjectName != "bg_main") {
								Scada::activeControlObject = selectedObjectName;
								Scada::showControlWindow = true;
								}
							}

							// 2. Если мы БЫЛИ в режиме редактирования и что-то двигали
							if (Scada::editMode && Scada::isDragging) {
								std::cout << "Объект " << selectedObjectName << " перемещен в x:"
								<< Scada::selectedRect->x << " y:" << Scada::selectedRect->y << std::endl;
							}

							// Сброс всех состояний захвата
						leftMouseButtonDown = false;
						Scada::isDragging = false;
						Scada::isResizing = false;
						Scada::selectedRect = nullptr;
						selectedObjectName = "";
						}

					}



					else if (e.type == SDL_EVENT_MOUSE_MOTION)
					{
						mousePos = { (float)e.motion.x, (float)e.motion.y };
						if (leftMouseButtonDown && Scada::selectedRect && Scada::editMode)
						{
							if (Scada::isResizing) {

								// ПРОВЕРКА: Если ширина больше высоты — тянем вбок, иначе — вниз
								if (Scada::selectedRect->w >= Scada::selectedRect->h) {
									// Горизонтальное растяжение
									float newW = mousePos.x - Scada::selectedRect->x;
									if (newW > 40.0f) {
										Scada::selectedRect->w = newW;
										 printf("Rect pointer: %p, New Width: %f\n", (void*)Scada::selectedRect, Scada::selectedRect->w);
									}
								} else {
									// Вертикальное растяжение
									float newH = mousePos.y - Scada::selectedRect->y;
									if (newH > 40.0f) {
										Scada::selectedRect->h = newH;
									}
								}
							}
							// Порог чувствительности 3 пикселя
							else {
								// Обычное ПЕРЕМЕЩЕНИЕ
								if (std::abs(e.motion.xrel) > 2 || std::abs(e.motion.yrel) > 2) {
								Scada::isDragging = true;
							}
							float gridSize = 2.0f; // Шаг сетки

							// Вычисляем новую позицию с учетом смещения клика
							float newX = mousePos.x - clickOffset.x;
							float newY = mousePos.y - clickOffset.y;

							// Применяем прилипание (математическое округление до ближайшего шага)
							Scada::selectedRect->x = std::round(newX / gridSize) * gridSize;
							Scada::selectedRect->y = std::round(newY / gridSize) * gridSize;
							}
						}
					}//else if (e.type == SDL_EVENT_MOUSE_MOTION)

					else if (e.type == SDL_EVENT_MOUSE_BUTTON_DOWN)
					{
						if (e.button.button == SDL_BUTTON_LEFT)
						{
							mousePos = { (float)e.button.x, (float)e.button.y };
							leftMouseButtonDown = true;
							Scada::isDragging = false;
							Scada::isResizing = false;
							Scada::selectedRect = nullptr;
							selectedObjectName = "";

							// 1. Проверка кнопок окна управления (оставляем как есть)
							if (Scada::showControlWindow) {
								SDL_FRect btnClose = { Scada::controlWindowRect.x + Scada::controlWindowRect.w - 40, Scada::controlWindowRect.y + 10, 30, 30 };
								if (SDL_PointInRectFloat(&mousePos, &btnClose)) { Scada::showControlWindow = false; break; }
								SDL_FRect btnOn = { Scada::controlWindowRect.x + 50, Scada::controlWindowRect.y + 150, 150, 80 };
								SDL_FRect btnOff = { Scada::controlWindowRect.x + 300, Scada::controlWindowRect.y + 150, 150, 80 };
								if (SDL_PointInRectFloat(&mousePos, &btnClose)) { Scada::showControlWindow = false; break; }
								if (SDL_PointInRectFloat(&mousePos, &btnOff))  { send_mcu_command(Scada::activeControlObject, 0); break; }
								if (SDL_PointInRectFloat(&mousePos, &btnOn))   { send_mcu_command(Scada::activeControlObject, 1); break; }
								if (SDL_PointInRectFloat(&mousePos, &Scada::controlWindowRect)) break;
							}

							// 2. ИСПОЛЬЗУЕМ КЛАСС: Ищем графический объект

							Scada::selectedRect = App::scene.findElementAt(mousePos.x, mousePos.y, selectedObjectName);


							// 4. Логика захвата (Resize / Drag)
							if (Scada::selectedRect) {
								// Проверяем, является ли выбранный объект текстовым
								// Допустим, у вас есть флаг или вы проверяете наличие имени в карте текста
								bool isText = App::scene.getTextConfig().count(selectedObjectName);

								float edgeSize = 15.0f;
								// Если это ТЕКСТ — запрещаем ресайз, только перетаскивание
								if (isText) {
									Scada::isResizing = false;
									clickOffset.x = mousePos.x - Scada::selectedRect->x;
									clickOffset.y = mousePos.y - Scada::selectedRect->y;
								}
								// Если это КАРТИНКА (или другой объект) — оставляем логику ресайза
								else {
								bool isVertical = (Scada::selectedRect->h > Scada::selectedRect->w);
								bool hitRight = (mousePos.x > (Scada::selectedRect->x + Scada::selectedRect->w - edgeSize));
								bool hitBottom = (mousePos.y > (Scada::selectedRect->y + Scada::selectedRect->h - edgeSize));
								// Проверяем попадание в край для ресайза
								if ((isVertical && hitBottom) || (!isVertical && hitRight)) {
									Scada::isResizing = true;
								} else {
									Scada::isResizing = false;
									// Смещение для корректного перетаскивания
									clickOffset.x = mousePos.x - Scada::selectedRect->x;
									clickOffset.y = mousePos.y - Scada::selectedRect->y;
									}
								}
							}
						}

						}//else if (e.type == SDL_EVENT_MOUSE_BUTTON_DOWN)
					}//	while( SDL_PollEvent( &e ) != 0 )

			//}//while( !quit )





				// --- ОБНОВЛЕНИЕ ГРАФИКИ ДАННЫМИ ---
				SDL_LockMutex(Scada::data_mutex);
				// Теперь Scada::shared_sensor_data содержит актуальные значения
				// Можно обновить тексты или координаты объектов:
				// label_temp.text = std::to_string(Scada::shared_sensor_data.temperature);
				SDL_UnlockMutex(Scada::data_mutex);

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
			SDL_DestroyMutex(Scada::data_mutex);
			SDL_WaitThread(modbusThread, nullptr);

			SDL_DestroyMutex(Scada::modbus_mutex);

		}
	}

	//Free resources and close SDL
	close();

	return 0;
}
