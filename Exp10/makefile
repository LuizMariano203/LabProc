CXX = g++
CXXFLAGS = -Wall -Wextra -std=c++17

CPPFLAGS = -I/usr/local/include
LDFLAGS = -L/usr/local/lib

WIRINGPI_LIBS = -lwiringPi
LCD_LIBS = -lwiringPi -lwiringPiDev

KEYPAD_FILES = Keypad.cpp Key.cpp

all: \
	teste_lcd \
	teste_teclado \
	teste_teclado_lcd \
	teste_buzzer \
	teste_sensor \
	fechadura


teste_lcd: teste_lcd.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) \
	teste_lcd.cpp \
	-o teste_lcd \
	$(LDFLAGS) \
	$(LCD_LIBS)


teste_teclado: teste_teclado.cpp $(KEYPAD_FILES)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) \
	teste_teclado.cpp $(KEYPAD_FILES) \
	-o teste_teclado \
	$(LDFLAGS) \
	$(WIRINGPI_LIBS)


teste_teclado_lcd: teste_teclado_lcd.cpp $(KEYPAD_FILES)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) \
	teste_teclado_lcd.cpp $(KEYPAD_FILES) \
	-o teste_teclado_lcd \
	$(LDFLAGS) \
	$(LCD_LIBS)


teste_buzzer: teste_buzzer.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) \
	teste_buzzer.cpp \
	-o teste_buzzer \
	$(LDFLAGS) \
	$(WIRINGPI_LIBS)


teste_sensor: teste_sensor.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) \
	teste_sensor.cpp \
	-o teste_sensor \
	$(LDFLAGS) \
	$(WIRINGPI_LIBS)


fechadura: fechadura.cpp $(KEYPAD_FILES)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) \
	fechadura.cpp $(KEYPAD_FILES) \
	-o fechadura \
	$(LDFLAGS) \
	$(LCD_LIBS)


clean:
	rm -f \
	teste_lcd \
	teste_teclado \
	teste_teclado_lcd \
	teste_buzzer \
	teste_sensor \
	teste_senha \
	fechadura


.PHONY: all clean