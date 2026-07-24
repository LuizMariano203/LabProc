#ifndef KEYPAD_H
#define KEYPAD_H

#include "Key.hpp"
#include <wiringPi.h>
#include <stdio.h>

#define INPUT_PULLUP	0x02
#define bitWrite(x,n,b)	(b ? (x |= 1<<n) : (x &= ~(1<<n)))
#define bitRead(x,n)	((((x>>n)&1) == 1) ? 1 : 0)

#define OPEN LOW
#define CLOSED HIGH

typedef char KeypadEvent;
typedef unsigned int uint;
typedef unsigned long ulong;

typedef struct {
	byte rows;
	byte columns;
} KeypadSize;

#define LIST_MAX 10
#define MAPSIZE 10
#define makeKeymap(x) ((char*)x)

class Keypad : public Key {
public:

	Keypad(char *userKeymap, byte *row, byte *col, byte numRows, byte numCols);

	uint keyBitmap[MAPSIZE];
	Key keys[LIST_MAX];
	unsigned long holdStartTime;

	char getKey();
	bool getKeys();
	KeyState getState();
	void begin(char *userKeymap);
	bool isPressed(char keyChar);
	void setDebounceTime(uint);
	void setHoldTime(uint);
	void addEventListener(void (*listener)(char));
	int findInList(char keyChar);
	int findInList(int keyCode);
	char waitForKey();
	bool keyStateChanged();
	byte numKeys();

private:
	unsigned long lastScanTime;
	char *keyMap;
	byte *rowPinList;
	byte *colPinList;
	KeypadSize matrixSize;
	uint debounceMs;
	uint holdMs;
	bool singleKeyMode;

	void scanKeys();
	bool updateList();
	void nextKeyState(byte idx, boolean pressed);
	void transitionTo(byte idx, KeyState nextState);
	void (*eventListener)(char);
};

void setPinMode(byte pinNum, byte mode);
void writePin(byte pinNum, boolean level);
int  readPin(byte pinNum);
#endif
