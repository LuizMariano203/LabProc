#include <stdio.h>
#include <signal.h>
#include <string>

#include <wiringPi.h>
#include <wiringPiI2C.h>
#include <pcf8574.h>
#include <lcd.h>
#include <softTone.h>
#include <softPwm.h>

#include "Keypad.hpp"

const std::string VALID_PASSWORD = "1234";

const int PASSWORD_MAX_LEN = 8;
const int ATTEMPTS_LIMIT = 3;

const unsigned int LOCKOUT_DURATION_MS = 30000;
const unsigned int UNLOCK_DURATION_MS = 10000;
const unsigned int SENSOR_POLL_MS = 300;

const float DOOR_OPEN_THRESHOLD_CM = 10.0f;

#define SERVO_PIN 18

#define ANGLE_LOCKED 0
#define ANGLE_UNLOCKED 90

#define LCD_ADDRESS 0x27

#define BASE 64
#define RS   (BASE + 0)
#define RW   (BASE + 1)
#define EN   (BASE + 2)
#define LED  (BASE + 3)
#define D4   (BASE + 4)
#define D5   (BASE + 5)
#define D6   (BASE + 6)
#define D7   (BASE + 7)

int lcdFd = -1;

const byte ROWS = 4;
const byte COLS = 4;

char keyLayout[ROWS][COLS] = {
	{'1', '2', '3', 'A'},
	{'4', '5', '6', 'B'},
	{'7', '8', '9', 'C'},
	{'*', '0', '#', 'D'}
};

byte rowPins[ROWS] = {16, 20, 21, 26};
byte colPins[COLS] = {19, 13, 6, 5};

Keypad keypad(
	makeKeymap(keyLayout),
	rowPins,
	colPins,
	ROWS,
	COLS
);

#define BUZZER_PIN 4

#define TRIG_PIN 14
#define ECHO_PIN 15

#define SENSOR_TIMEOUT_US 30000

volatile sig_atomic_t keepRunning = 1;

std::string enteredPassword;

int failCount = 0;

unsigned int lockoutEnd = 0;
unsigned int unlockDeadline = 0;
unsigned int nextSensorPoll = 0;
unsigned int tempMsgEnd = 0;
unsigned int nextLockoutRefresh = 0;

bool tempMsgActive = false;
bool doorWasOpen = false;
bool alertActive = false;
bool doorUnlocked = false;

void initServo() {
	if (softPwmCreate(SERVO_PIN, 0, 200) != 0) {
		printf("Erro ao inicializar o controle do servo (SoftPWM).\n");
	}
}

void setServoAngle(int angle) {
	if (angle < 0) angle = 0;
	if (angle > 180) angle = 180;

	int pulseWidth = 5 + (angle * 20 / 180);
	softPwmWrite(SERVO_PIN, pulseWidth);
}

void lockDoor() {
	setServoAngle(ANGLE_LOCKED);
	doorUnlocked = false;
	printf("Fechadura: TRANCADA (Servo em %d deg)\n", ANGLE_LOCKED);
}

void unlockDoor() {
	setServoAngle(ANGLE_UNLOCKED);
	doorUnlocked = true;
	printf("Fechadura: DESTRANCADA (Servo em %d deg)\n", ANGLE_UNLOCKED);
}

void showTwoLines(const char* line1, const char* line2) {
	lcdClear(lcdFd);

	lcdPosition(lcdFd, 0, 0);
	lcdPrintf(lcdFd, "%-16.16s", line1);

	lcdPosition(lcdFd, 0, 1);
	lcdPrintf(lcdFd, "%-16.16s", line2);
}

void showPasswordPrompt() {
	lcdClear(lcdFd);

	lcdPosition(lcdFd, 0, 0);
	lcdPrintf(lcdFd, "Digite a senha:");

	lcdPosition(lcdFd, 0, 1);

	for (size_t i = 0; i < enteredPassword.length(); i++) {
		lcdPutchar(lcdFd, '*');
	}
}

void showTempMessage(const char* line1, const char* line2, unsigned int durationMs) {
	showTwoLines(line1, line2);

	tempMsgActive = true;
	tempMsgEnd = millis() + durationMs;
}

int initLcd() {
	int fd = wiringPiI2CSetup(LCD_ADDRESS);

	if (fd < 0) {
		printf("Erro: dispositivo I2C 0x%X nao encontrado.\n", LCD_ADDRESS);
		return -1;
	}

	pcf8574Setup(BASE, LCD_ADDRESS);

	for (int pin = BASE; pin < BASE + 8; pin++) {
		pinMode(pin, OUTPUT);
	}

	digitalWrite(LED, HIGH);
	digitalWrite(RW, LOW);

	return lcdInit(2, 16, 4, RS, EN, D4, D5, D6, D7, 0, 0, 0, 0);
}

void stopBuzzer() {
	softToneWrite(BUZZER_PIN, 0);
}

void playSuccessSound() {
	softToneWrite(BUZZER_PIN, 2000);
	delay(150);
	stopBuzzer();
}

void playErrorSound() {
	softToneWrite(BUZZER_PIN, 700);
	delay(500);
	stopBuzzer();
}

void playAlertSound() {
	for (int i = 0; i < 3; i++) {
		softToneWrite(BUZZER_PIN, 1500);
		delay(180);

		stopBuzzer();
		delay(120);
	}
}

long readPulseWithTimeout(int pin, int expectedLevel, unsigned int timeoutUs) {
	unsigned int waitStart = micros();

	while (digitalRead(pin) != expectedLevel) {
		if (micros() - waitStart > timeoutUs) {
			return 0;
		}
	}

	unsigned int pulseStart = micros();

	while (digitalRead(pin) == expectedLevel) {
		if (micros() - pulseStart > timeoutUs) {
			return 0;
		}
	}

	return static_cast<long>(micros() - pulseStart);
}

float readDistanceCm() {
	digitalWrite(TRIG_PIN, LOW);
	delayMicroseconds(2);

	digitalWrite(TRIG_PIN, HIGH);
	delayMicroseconds(10);
	digitalWrite(TRIG_PIN, LOW);

	long pulseUs = readPulseWithTimeout(ECHO_PIN, HIGH, SENSOR_TIMEOUT_US);

	if (pulseUs == 0) {
		return -1.0f;
	}

	return pulseUs * 34300.0f / 2000000.0f;
}

void processSensor(unsigned int now) {
	if (now < nextSensorPoll) {
		return;
	}

	nextSensorPoll = now + SENSOR_POLL_MS;

	float dist = readDistanceCm();

	if (dist < 0) {
		printf("Sensor: timeout de leitura.\n");
		return;
	}

	bool isOpen = dist > DOOR_OPEN_THRESHOLD_CM;

	printf("Distancia: %.2f cm | Estado: %s\n", dist, isOpen ? "ABERTA" : "TRANCADA");

	if (isOpen && !doorWasOpen) {
		if (now < unlockDeadline) {
			printf("Abertura autorizada detectada.\n");
			showTempMessage("Porta aberta", "Acesso autorizado", 2000);
		} else {
			printf("ALERTA: abertura nao autorizada.\n");
			alertActive = true;
			showTempMessage("ALERTA!", "Abertura indevida", 3000);
			playAlertSound();
		}
	}

	if (!isOpen && doorWasOpen) {
		printf("Fechadura voltou ao estado trancado.\n");

		alertActive = false;
		unlockDeadline = 0;

		lockDoor();

		if (now >= lockoutEnd) {
			showTempMessage("Fechadura", "TRANCADA", 1500);
		}
	}

	doorWasOpen = isOpen;
}

void clearPassword() {
	enteredPassword.clear();
}

void validatePassword(unsigned int now) {
	if (enteredPassword == VALID_PASSWORD) {
		printf("ACESSO LIBERADO\n");

		failCount = 0;
		unlockDeadline = now + UNLOCK_DURATION_MS;

		unlockDoor();
		playSuccessSound();

		showTempMessage("Acesso liberado", "Abra em 10 seg.", 2500);
	} else {
		failCount++;

		printf("ACESSO NEGADO - tentativa %d de %d\n", failCount, ATTEMPTS_LIMIT);
		playErrorSound();

		if (failCount >= ATTEMPTS_LIMIT) {
			lockoutEnd = now + LOCKOUT_DURATION_MS;
			nextLockoutRefresh = 0;

			showTwoLines("Sist. bloqueado", "Aguarde 30 seg.");
			tempMsgActive = false;

			printf("Sistema bloqueado por 30 segundos.\n");
		} else {
			char attemptBuf[17];
			snprintf(attemptBuf, sizeof(attemptBuf), "Tentativa %d de %d", failCount, ATTEMPTS_LIMIT);
			showTempMessage("Acesso negado", attemptBuf, 2000);
		}
	}

	clearPassword();
}

bool processLockoutState(unsigned int now) {
	if (now >= lockoutEnd) {
		return false;
	}

	if (now >= nextLockoutRefresh) {
		unsigned int remainingMs = lockoutEnd - now;
		unsigned int secsLeft = (remainingMs + 999) / 1000;

		char line2Buf[17];
		snprintf(line2Buf, sizeof(line2Buf), "Restam %u seg.", secsLeft);

		showTwoLines("Sist. bloqueado", line2Buf);

		nextLockoutRefresh = now + 1000;
	}

	return true;
}

void finishLockoutIfNeeded(unsigned int now) {
	static bool wasLockedOut = false;

	bool lockedOut = now < lockoutEnd;

	if (lockedOut) {
		wasLockedOut = true;
		return;
	}

	if (wasLockedOut) {
		wasLockedOut = false;
		failCount = 0;
		lockoutEnd = 0;

		printf("Bloqueio encerrado.\n");
		showTempMessage("Bloqueio encerr.", "Digite a senha", 1500);
	}
}

void processUnlockTimeout(unsigned int now) {
	if (doorUnlocked && now >= unlockDeadline) {
		printf("Tempo limite atingido. Trancando automaticamente.\n");
		lockDoor();
		unlockDeadline = 0;
	}
}

void processKey(char key, unsigned int now) {
	printf("Tecla detectada: %c\n", key);

	switch (key) {
		case '0': case '1': case '2': case '3': case '4':
		case '5': case '6': case '7': case '8': case '9':
			if (static_cast<int>(enteredPassword.length()) < PASSWORD_MAX_LEN) {
				enteredPassword += key;
				tempMsgActive = false;
				showPasswordPrompt();
			}
			break;

		case '*':
			if (!enteredPassword.empty()) {
				enteredPassword.pop_back();
			}
			tempMsgActive = false;
			showPasswordPrompt();
			break;

		case 'D':
			clearPassword();
			tempMsgActive = false;
			showPasswordPrompt();
			break;

		case '#':
			if (enteredPassword.empty()) {
				showTempMessage("Senha vazia", "Digite a senha", 1500);
				break;
			}
			validatePassword(now);
			break;

		default:
			break;
	}
}

void handleSignal(int) {
	keepRunning = 0;
}

void finishProgram() {
	stopBuzzer();
	lockDoor();
	delay(300);

	if (lcdFd >= 0) {
		showTwoLines("Sistema", "encerrado");
		delay(800);
		lcdClear(lcdFd);
	}

	printf("\nPrograma encerrado.\n");
}

int main() {
	signal(SIGINT, handleSignal);
	signal(SIGTERM, handleSignal);

	printf("Inicializando fechadura eletronica...\n");

	if (wiringPiSetupGpio() == -1) {
		printf("Erro ao inicializar wiringPi.\n");
		return 1;
	}

	initServo();
	lockDoor();

	lcdFd = initLcd();

	if (lcdFd == -1) {
		printf("Erro ao inicializar LCD.\n");
		return 1;
	}

	keypad.setDebounceTime(50);

	pinMode(BUZZER_PIN, OUTPUT);

	if (softToneCreate(BUZZER_PIN) != 0) {
		printf("Erro ao inicializar buzzer.\n");
		return 1;
	}

	pinMode(TRIG_PIN, OUTPUT);
	pinMode(ECHO_PIN, INPUT);

	digitalWrite(TRIG_PIN, LOW);
	stopBuzzer();

	showTwoLines("Fechadura", "Inicializando...");
	delay(1500);

	showPasswordPrompt();

	printf("Sistema iniciado.\n");
	printf("# confirma, * apaga e D limpa.\n");
	printf("Pressione Ctrl+C para encerrar.\n");

	while (keepRunning) {
		unsigned int now = millis();

		processUnlockTimeout(now);
		processSensor(now);
		finishLockoutIfNeeded(now);

		if (processLockoutState(now)) {
			delay(10);
			continue;
		}

		if (tempMsgActive && now >= tempMsgEnd) {
			tempMsgActive = false;
			showPasswordPrompt();
		}

		char key = keypad.getKey();

		if (key) {
			processKey(key, now);
		}

		delay(10);
	}

	finishProgram();

	return 0;
}
