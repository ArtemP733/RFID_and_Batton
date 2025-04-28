#include <SPI.h>
#include <MFRC522.h>
#include <Servo.h>

// Пины подключения
#define RST_PIN 9
#define SS_PIN 10

#define GREEN_LED 5      // Зелёный светодиод (открыто)
#define RED_LED 6        // Красный светодиод (закрыто)
#define BUZZER_PIN 7     // Баззер
#define SERVO_PIN 3      // Серво
#define OPEN_BUTTON 2    // Кнопка открытия/закрытия
#define ADD_TAG_BUTTON 4 // Кнопка добавления карт

// Создание объектов
MFRC522 rfid(SS_PIN, RST_PIN);
Servo servo;

// Хранение UID карт (макс. 5 карт)
byte storedUIDs[5][4];
int uidCount = 0;

bool isLocked = true;
bool addMode = false;

void setup() {
  Serial.begin(9600);
  SPI.begin();
  rfid.PCD_Init();
  
  // Подключение серво
  servo.attach(SERVO_PIN);
  servo.write(0); // Закрытое положение

  // Настройка пинов
  pinMode(GREEN_LED, OUTPUT);
  pinMode(RED_LED, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(OPEN_BUTTON, INPUT_PULLUP);
  pinMode(ADD_TAG_BUTTON, INPUT_PULLUP);

  // Начальное состояние
  digitalWrite(GREEN_LED, LOW);
  digitalWrite(RED_LED, HIGH);

  Serial.println("Система готова!");
}

void loop() {
  if (digitalRead(ADD_TAG_BUTTON) == LOW) {
    enterAddMode();
  }

  if (digitalRead(OPEN_BUTTON) == LOW) {
    toggleLock();
  }

  if (rfid.PICC_IsNewCardPresent() && rfid.PICC_ReadCardSerial()) {
    if (addMode) {
      addCard(rfid.uid.uidByte, rfid.uid.size);
    } else {
      processCard(rfid.uid.uidByte, rfid.uid.size);
    }
    rfid.PICC_HaltA();
  }

  delay(100);
}

void toggleLock() {
  if (isLocked) {
    openLock();
  } else {
    closeLock();
  }
}

void openLock() {
  Serial.println("Замок открыт.");
  tone(BUZZER_PIN, 1000, 200); // Сигнал открытия
  digitalWrite(GREEN_LED, HIGH);
  digitalWrite(RED_LED, LOW);
  servo.write(90); // Открыть замок
  isLocked = false;

  delay(5000); // Ждать 5 секунд перед закрытием
  closeLock();
}

void closeLock() {
  Serial.println("Замок закрыт.");
  tone(BUZZER_PIN, 500, 200); // Сигнал закрытия
  digitalWrite(GREEN_LED, LOW);
  digitalWrite(RED_LED, HIGH);
  servo.write(0); // Закрыть замок
  isLocked = true;
}

void enterAddMode() {
  Serial.println("Режим добавления карт.");
  tone(BUZZER_PIN, 2000, 500);
  addMode = true;
}

void addCard(byte *uid, byte size) {
  if (uidCount >= 5) {
    Serial.println("Список UID заполнен!");
    addMode = false; // Выход из режима добавления
    return;
  }

  for (int i = 0; i < uidCount; i++) {
    if (compareUID(uid, storedUIDs[i], size)) {
      Serial.println("Карта уже существует.");
      addMode = false; // Выход из режима добавления
      return;
    }
  }

  for (byte i = 0; i < size; i++) {
    storedUIDs[uidCount][i] = uid[i];
  }
  uidCount++;
  Serial.println("Карта добавлена.");
  tone(BUZZER_PIN, 1500, 200);

  addMode = false; // Выход из режима добавления после успешного добавления
  Serial.println("Выход из режима добавления.");
}

void processCard(byte *uid, byte size) {
  for (int i = 0; i < uidCount; i++) {
    if (compareUID(uid, storedUIDs[i], size)) {
      Serial.println("Доступ разрешён.");
      openLock();
      return;
    }
  }
  Serial.println("Доступ запрещён.");
  failedAccess();
}

void failedAccess() {
  for (int i = 0; i < 3; i++) {
    tone(BUZZER_PIN, 1000, 200);
    delay(300);
  }
  digitalWrite(RED_LED, HIGH);
  delay(1000);
  digitalWrite(RED_LED, LOW);
}

bool compareUID(byte *uid1, byte *uid2, byte size) {
  for (byte i = 0; i < size; i++) {
    if (uid1[i] != uid2[i]) {
      return false;
    }
  }
  return true;
}
