#include "Key.hpp"

Key::Key() : keyChar(NO_KEY), keyCode(-1), keyState(IDLE), hasChanged(false) {}

Key::Key(char ch) : keyChar(ch), keyCode(-1), keyState(IDLE), hasChanged(false) {}

void Key::update(char ch, KeyState state, boolean changed) {
	keyChar = ch;
	keyState = state;
	hasChanged = changed;
}
