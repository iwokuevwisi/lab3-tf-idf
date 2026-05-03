# Компилятор
CXX = g++

# Флаги компиляции
CXXFLAGS = -std=c++23 -Wall -Wextra -O0

# Цель по умолчанию
TARGET = TF-IDF

# Исходный файл
SRC = main.cpp

# Объектный файл
OBJ = main.o

# Правило по умолчанию
all: $(TARGET)

# Сборка исполняемого файла
$(TARGET): $(OBJ)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJ)

# Компиляция исходного файла
$(OBJ): $(SRC)
	$(CXX) $(CXXFLAGS) -c $(SRC) -o $(OBJ)

# Запуск программы
run: $(TARGET)
	./$(TARGET)

# Очистка
clean:
	rm -f $(OBJ) $(TARGET)

# Полная пересборка
rebuild: clean all

# Отладка (с флагами для gdb)
debug: CXXFLAGS += -g -O0 -DDEBUG
debug: clean all

# Объявляем цели, которые не являются файлами
.PHONY: all run clean rebuild debug test