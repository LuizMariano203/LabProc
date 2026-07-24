#include "Keypad.hpp"

Keypad::Keypad(char *userKeymap, byte *row, byte *col, byte numRows, byte numCols) {
	rowPinList = row;
	colPinList = col;
	matrixSize.rows = numRows;
	matrixSize.columns = numCols;

	begin(userKeymap);

	setDebounceTime(50);
	setHoldTime(500);
	eventListener = 0;

	lastScanTime = 0;
	singleKeyMode = false;
}

void Keypad::begin(char *userKeymap) {
	keyMap = userKeymap;
}

char Keypad::getKey() {
	singleKeyMode = true;
	if (getKeys() && keys[0].hasChanged && keys[0].keyState == PRESSED) {
		return keys[0].keyChar;
	}

	singleKeyMode = false;
	return NO_KEY;
}

bool Keypad::getKeys() {
	bool keyActivity = false;

	if ((millis() - lastScanTime) > debounceMs) {
		scanKeys();
		keyActivity = updateList();
		lastScanTime = millis();
	}
	return keyActivity;
}

void Keypad::scanKeys() {
	for (byte r = 0; r < matrixSize.rows; r++) {
		setPinMode(rowPinList[r], INPUT_PULLUP);
	}

	for (byte c = 0; c < matrixSize.columns; c++) {
		setPinMode(colPinList[c], OUTPUT);
		writePin(colPinList[c], LOW);
		for (byte r = 0; r < matrixSize.rows; r++) {
			bitWrite(keyBitmap[r], c, !readPin(rowPinList[r]));
		}
		writePin(colPinList[c], HIGH);
		setPinMode(colPinList[c], INPUT);
	}
}

bool Keypad::updateList() {
	bool anyActivity = false;

	for (byte i = 0; i < LIST_MAX; i++) {
		if (keys[i].keyState == IDLE) {
			keys[i].keyChar = NO_KEY;
			keys[i].keyCode = -1;
			keys[i].hasChanged = false;
		}
	}

	for (byte r = 0; r < matrixSize.rows; r++) {
		for (byte c = 0; c < matrixSize.columns; c++) {
			boolean pressed = bitRead(keyBitmap[r], c);
			int keyCode = r * matrixSize.columns + c;
			int idx = findInList(keyCode);

			if (idx > -1) {
				nextKeyState(idx, pressed);
				continue;
			}

			if (!pressed) continue;

			char keyChar = keyMap[keyCode];
			for (byte i = 0; i < LIST_MAX; i++) {
				if (keys[i].keyChar == NO_KEY) {
					keys[i].keyChar = keyChar;
					keys[i].keyCode = keyCode;
					keys[i].keyState = IDLE;
					nextKeyState(i, pressed);
					break;
				}
			}
		}
	}

	for (byte i = 0; i < LIST_MAX; i++) {
		if (keys[i].hasChanged) anyActivity = true;
	}
	return anyActivity;
}

void Keypad::nextKeyState(byte idx, boolean pressed) {
	keys[idx].hasChanged = false;
	switch (keys[idx].keyState) {
		case IDLE:
			if (pressed == CLOSED) {
				transitionTo(idx, PRESSED);
				holdStartTime = millis();
			}
			break;
		case PRESSED:
			if ((millis() - holdStartTime) > holdMs)
				transitionTo(idx, HOLD);
			else if (pressed == OPEN)
				transitionTo(idx, RELEASED);
			break;
		case HOLD:
			if (pressed == OPEN) {
				transitionTo(idx, RELEASED);
			}
			break;
		case RELEASED:
			transitionTo(idx, IDLE);
			break;
	}
}

bool Keypad::isPressed(char keyChar) {
	for (byte i = 0; i < LIST_MAX; i++) {
		if (keys[i].keyChar == keyChar && keys[i].keyState == PRESSED && keys[i].hasChanged) {
			return true;
		}
	}
	return false;
}

int Keypad::findInList(char keyChar) {
	for (byte i = 0; i < LIST_MAX; i++) {
		if (keys[i].keyChar == keyChar) {
			return i;
		}
	}
	return -1;
}

int Keypad::findInList(int keyCode) {
	for (byte i = 0; i < LIST_MAX; i++) {
		if (keys[i].keyCode == keyCode) {
			return i;
		}
	}
	return -1;
}

char Keypad::waitForKey() {
	char waitKey;
	while ((waitKey = getKey()) == NO_KEY);
	return waitKey;
}

KeyState Keypad::getState() {
	return keys[0].keyState;
}

bool Keypad::keyStateChanged() {
	return keys[0].hasChanged;
}

byte Keypad::numKeys() {
	return LIST_MAX;
}

void Keypad::setDebounceTime(uint debounce) {
	debounceMs = debounce < 1 ? 1 : debounce;
}

void Keypad::setHoldTime(uint hold) {
	holdMs = hold;
}

void Keypad::addEventListener(void (*listener)(char)) {
	eventListener = listener;
}

void Keypad::transitionTo(byte idx, KeyState nextState) {
	keys[idx].keyState = nextState;
	keys[idx].hasChanged = true;

	if (singleKeyMode) {
		if (eventListener != NULL && idx == 0) {
			eventListener(keys[0].keyChar);
		}
	} else if (eventListener != NULL) {
		eventListener(keys[idx].keyChar);
	}
}

void setPinMode(byte pinNum, byte mode) {
	if (mode == INPUT_PULLUP) {
		pinMode(pinNum, INPUT);
		pullUpDnControl(pinNum, PUD_UP);
	} else {
		pinMode(pinNum, mode);
	}
}

void writePin(byte pinNum, boolean level) {
	digitalWrite(pinNum, level);
}

int readPin(byte pinNum) {
	return digitalRead(pinNum);
}
