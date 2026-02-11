BIN=bin
SRC=src
INCLUDE=inc

SOURCES=$(notdir $(foreach dir,$(SRC),$(wildcard $(dir)/*.cpp)))
TARGET=$(addprefix ,$(shell basename `pwd`)) # Убрал BIN/ здесь
OBJECTS=$(addprefix $(BIN)/,$(SOURCES:.cpp=.o))
DEPS = $(OBJECTS:.o=.d)

CXXFLAGS=-std=c++17 -O0 -g $(addprefix -I,$(INCLUDE))
LDFLAGS=-lSDL3 -lSDL3_image -lSDL3_ttf # Перенес флаги SDL2 сюда

vpath %.cpp $(SRC)

all: $(BIN) $(TARGET)

$(TARGET): $(OBJECTS)
	g++ $(CXXFLAGS) $(LDFLAGS) $(OBJECTS) -o $@ # Упрощенная линковка

$(BIN)/%.o: %.cpp
	g++ $(CXXFLAGS) -c $< -o $@ -MMD -MF $(@:.o=.d) # Compile and generate dependencies

clean:
	$(RM) $(OBJECTS) $(TARGET) $(DEPS)

-include $(DEPS)

.PHONY: all clean
