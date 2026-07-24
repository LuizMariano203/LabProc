#ifndef KEY_H
#define KEY_H

#include <wiringPi.h>

#define boolean bool
#define byte unsigned char
#define OPEN LOW
#define CLOSED HIGH

typedef unsigned int uint;
typedef enum { IDLE, PRESSED, HOLD, RELEASED } KeyState;

const char NO_KEY = '\0';

class Key {
public:
	char keyChar;
	int keyCode;
	KeyState keyState;
	boolean hasChanged;

	Key();
	Key(char ch);
	void update(char ch, KeyState state, boolean changed);
};

#endif
