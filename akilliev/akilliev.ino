#include "lsm6dsm.h"
#include "deneyap.h"
#include <LiquidCrystal_I2C.h>
#include "uRTCLib.h"
#include <MFRC522.h>
#include <ESP32Servo.h>

uRTCLib rtc(0x68);
LSM6DSM IMU;
LiquidCrystal_I2C lcd(0x27, 20, 4);
LiquidCrystal_I2C lcd1(0x3F, 20, 4);
float sicaklik_degeri = 0.0;
MFRC522 mfrc522(D8, D4);
int servoPin = D1;
Servo motor;
String authorizedID = "3352030c"; // Yetkili kart ID'si
int buzzerPin = D12;
int fanPin = D13;

float accelX, accelY, accelZ, accAngleY, accAngleX; // IMU değeri değişken tanımlaması yapıldı
float threshold = 20;                               // Sarsıntı eşiği

String fnc_user_ID() {
  String user_ID = "";
  for (byte i = 0; i < mfrc522.uid.size; i++) {
    user_ID += String(mfrc522.uid.uidByte[i] < 0x10 ? "0" : "") + String(mfrc522.uid.uidByte[i], HEX);
  }
  return user_ID;
}

void setup() {
  IMU.begin();
  
  Serial.begin(115200);
  lcd.init();
  lcd1.init();
  lcd1.backlight();
  lcd.backlight();

  #ifdef ARDUINO_ARCH_ESP8266
    URTCLIB_WIRE.begin(0, 2);
  #else
    URTCLIB_WIRE.begin();
  #endif

  SPI.begin();
  mfrc522.PCD_Init();
  motor.attach(servoPin);
  pinMode(buzzerPin, OUTPUT);
  digitalWrite(buzzerPin, LOW); // Buzzer başlangıçta kapalı
}

void loop() {
  imudeprem();
  rtc.refresh();
  sicaklik_degeri = IMU.readTempC();
  lcd.setCursor(0, 0);
  lcd.print("Sicaklik:");
  lcd.setCursor(10, 0);
  lcd.print(sicaklik_degeri);
  lcd.setCursor(0, 2);
  lcd.print("Gaz Durumu:");
  lcd.setCursor(0, 1);
  lcd.print("Nem:");
  lcd.setCursor(5, 1);
  lcd.print("Kuru");
  lcd.setCursor(12, 2);
  lcd.print("Temiz");
  lcd1.setCursor(0, 0);
  lcd1.print("Saat:");
  lcd1.setCursor(5, 0);
  lcd1.print(rtc.hour());
  lcd1.setCursor(7, 0);
  lcd1.print(":");
  lcd1.setCursor(8, 0);
  lcd1.print(rtc.minute());
  lcd1.setCursor(0, 1);
  lcd1.print("Tarih:");
  lcd1.setCursor(6, 1);
  lcd1.print(rtc.day());
  lcd1.setCursor(8, 1);
  lcd1.print("/");
  lcd1.setCursor(9, 1);
  lcd1.print(rtc.month());
  lcd1.setCursor(10, 1);
  lcd1.print("/");
  lcd1.setCursor(11, 1);
  lcd1.print(rtc.year());
  lcd1.setCursor(0, 2);
  lcd1.print("Ses Duzeyi:");
  lcd1.setCursor(11, 2);
  lcd1.print("Normal");
  lcd1.setCursor(0, 3);
  lcd1.print("Kapi Durumu:");
  lcd1.setCursor(12, 3);
  lcd1.print("Kapali");

  if(sicaklik_degeri>40){
    digitalWrite(fanPin,HIGH);
  }else{
    digitalWrite(fanPin,LOW);
  }



  if (analogRead(A1) > 2000) {
    lcd.setCursor(12, 2);
    lcd.print("Zehirli!!!");
    digitalWrite(D12,HIGH);
    delay(1000); // Buzzer'ı 1 saniye çal
    digitalWrite(D12,LOW);
    lcd.init();
  }

  if (analogRead(A0) < 2000) {
    lcd.setCursor(5, 1);
    lcd.print("Islak!!!");
    digitalWrite(D12,HIGH);
    delay(500); // Buzzer'ı 0.5 saniye çal
    digitalWrite(D12,LOW);
    lcd.init();
  }

  if (digitalRead(D0) == LOW) {
    lcd.setCursor(0, 3);
    lcd.print("YANGIN VAR!!!!");
    digitalWrite(D12,HIGH);
    delay(3000); // Buzzer'ı 3 saniye çal
    digitalWrite(D12,LOW);
    lcd.init();
  }

  if (analogRead(A2) > 1400) {
    lcd1.setCursor(11, 2);
    lcd1.print("Yuksek");
    digitalWrite(D12,HIGH);
    delay(500); // Buzzer'ı 0.5 saniye çal
    digitalWrite(D12,LOW);
    lcd1.init();
  }

  if (mfrc522.PICC_IsNewCardPresent() && mfrc522.PICC_ReadCardSerial()) {
    String strID = fnc_user_ID();
    Serial.print("Kart UID: ");
    Serial.println(strID);

    if (strID.equals(authorizedID)) {
      lcd1.setCursor(12, 3);
      lcd1.print("Acik!!!");
      motor.write(180);
      delay(500);
      motor.write(0);
    } else {
      // Yetkisiz kart okunduğunda buzzer'ı aktif et
      digitalWrite(buzzerPin, HIGH);
      delay(1000); // 1 saniye buzzer çal
      digitalWrite(buzzerPin, LOW);
    }
  }


}

void imudeprem() {
  accelX = IMU.readFloatAccelX(); // sensörden gelen X, Y, Z verileri okundu
  accelY = IMU.readFloatAccelY();
  accelZ = IMU.readFloatAccelZ();
  accAngleX = atan(accelX / sqrt(pow(accelY, 2) + pow(accelZ, 2) + 0.001)) * 180 / PI; // Matematik işlemi ile sensör verileri -90-+90 arasında açı değerlerine dönüştürüldü.
  accAngleY = atan(-1 * accelY / sqrt(pow(accelX, 2) + pow(accelZ, 2) + 0.001)) * 180 / PI;

  Serial.print(" AngleY: " + String(accAngleY)); // AngelY de okunan değere göre eşik değerinizi belirleyebilirsiniz.
  Serial.println(" AngleX: " + String(accAngleX));
  
  if (abs(accAngleX) > threshold || abs(accAngleY) > threshold) {
    digitalWrite(buzzerPin, HIGH); // Buzzer'ı aktif et
    lcd.setCursor(0,3);
    lcd.print("DEPREM!!!!");
    delay(1000);                   // Buzzer'ın 1 saniye ötmesi
    digitalWrite(buzzerPin, LOW);  // Buzzer'ı kapat
    lcd.init();
  }
}
