// LCD
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// RFID
#include <SPI.h>
#include <MFRC522.h>
#define SS_PIN 10
#define RST_PIN 9

// 압력센서
#define FSR A0

// 상태LED
#define LAMP_PREG 3
#define LAMP_ELDER 4



// LCD 설정
LiquidCrystal_I2C LCD(0x27, 16, 2);

// RFID 설정
MFRC522 rfid(SS_PIN, RST_PIN); // Instance of the class
MFRC522::MIFARE_Key key; 
// Init array that will store new NUID 
byte nuidPICC[4];

int SeatStatus = -1;
// 일반인 0 임산부 1 노약자 2로 지정했음

bool Sit = false; 


void setup() { 
  Serial.begin(9600);
  
  LCD.init();
  LCD.backlight();
  LCD.clear();
  LCD.home();
  LCD.print("Now Beginning...");
  
  SPI.begin(); // Init SPI bus
  rfid.PCD_Init(); // Init MFRC522 

  for (byte i = 0; i < 6; i++) {
    key.keyByte[i] = 0xFF;
  }

  pinMode(LAMP_PREG, OUTPUT);
  pinMode(LAMP_ELDER, OUTPUT);
  Serial.println(F("RFID 준비 완료"));
}
 
void loop() {

  int currentRFID = checkRFID();
  
  if ( currentRFID == 0 )         //카드 접촉이 없거나, 노약자나 임산부가 아닌 경우
  {
    if( checkPressure() )         // 앉으면?
    {
      // 앉음 상태 설정
      Serial.println("일반인 앉음");
      Sit = true;
      // 일반인 상태 설정
      SeatStatus = 0;
      // 일반인 앉음 / 램프 끄기
      digitalWrite(LAMP_ELDER, LOW);
      digitalWrite(LAMP_PREG, LOW);
      LCD.clear();
      LCD.print(" Public Sitting ");
    }
    else
    {
      Serial.println("일어섬or빈자리");
      // 빈자리 설정
      Sit = false;
      // 빈자리 상태 설정
      SeatStatus = -1;
      // 빈 좌석 안내하기/ LED끄기
      digitalWrite(LAMP_ELDER, LOW);
      digitalWrite(LAMP_PREG, LOW);
      LCD.clear();
      LCD.home();
      LCD.print("  Empty Seat... ");
    }
    delay(250);
  }

  // 카드를 대면
  else
  {
    Serial.println("카드 인식");

    if( Sit && SeatStatus == 0 ) // 누군가 앉은 상태인데, 임산부나 노약자가 아니라면?                
    {
      LCD.clear();
      LCD.home();
      LCD.print("Plesase, Give Up");
      LCD.setCursor(0, 1);
      LCD.print("Your Seat");
      Serial.print("자리양보 안내멘트 발생");
      while( checkPressure() )
      {
        Serial.println("양보 대기");
        delay(250);
      }
      Sit = false;
      delay(2000);
      while( !Sit )
      {
        LCD.clear();
        LCD.home();
        LCD.print("Take a seat!");
        Serial.println("노약자임산부 앉기 대기");
        if( checkPressure() )
        {
          Serial.println("노약자임산부 앉음");
          LCD.clear();
          break;
        }
        delay(250);
      }
    }
    
    if ( !Sit )
    {
      SeatStatus = currentRFID;
      if( SeatStatus == 1 )
      {
        Serial.print("임산부 착석 안내");
        
        // LCD 상태문구 변경
        // 임산부 램프 점등
        LCD.clear();
        LCD.home();
        LCD.print("Take a seat!");
        while( !checkPressure() )
        {
          // 앉는 것 확인하기
        }
        // 앉음
        Sit = true;
        digitalWrite(LAMP_ELDER, LOW);
        digitalWrite(LAMP_PREG, HIGH);
        LCD.clear();
        LCD.home();
        LCD.print("Pregnant Woman");
        LCD.setCursor(0, 1);
        LCD.print("is Sitting...");
        Serial.println("앉음");
        delay(2000);

        while( checkPressure() )
        {
          // 앉은 상태일 경우 대기
          Serial.println("착석중");
          int card = checkRFID();
          if( card > 0 )
          {
            if( SeatStatus == 1 )
            {
              Serial.print("현재 착석자가 임산부임을 안내");
            }
            else
            {
              Serial.print("현재 착석자가 노약자임을 안내");
            }
          }
          delay(250);
        }
        Serial.println("자리에서 일어남");
      }
      else if( SeatStatus == 2 )
      {
        Serial.print("노약자 착석 안내");
        // LCD 메시지 출력
        // 노약자 램프 켜기
        LCD.clear();
        LCD.home();
        LCD.print("Take a seat!");
        
        while( !checkPressure() )
        {
        }
        // 앉음
        Sit = true;

        LCD.clear();
        LCD.home();
        LCD.print("The Eldery");
        LCD.setCursor(0, 1);
        LCD.print("is Sitting...");
        Serial.println("앉음");
        digitalWrite(LAMP_ELDER, HIGH);
        digitalWrite(LAMP_PREG, LOW);
        delay(2000);
        while( checkPressure() )
        {
          // 앉은 상태일 경우 대기
          int card = checkRFID();

          Serial.println("착석중");
          if( card > 0 )
          {
            if( SeatStatus == 1 )
            {
              Serial.print("현재 착석자가 임산부임을 안내");
              digitalWrite(LAMP_ELDER, LOW);
              digitalWrite(LAMP_PREG, HIGH);
              delay(1000);
              digitalWrite(LAMP_PREG, LOW);
              delay(1000);
              digitalWrite(LAMP_PREG, HIGH);
            }
            else
            {
              Serial.print("현재 착석자가 노약자임을 안내");
              digitalWrite(LAMP_PREG, LOW);
              digitalWrite(LAMP_ELDER, HIGH);
              delay(1000);
              digitalWrite(LAMP_ELDER, LOW);
              delay(1000);
              digitalWrite(LAMP_ELDER, HIGH);
            }
          }
          delay(250);
        }
        Serial.println("자리에서 일어남");
        LCD.clear();
        digitalWrite(LAMP_ELDER, LOW);
        digitalWrite(LAMP_PREG, LOW);

      }
    }
  }



  

}



int checkRFID()
{
    // Reset the loop if no new card present on the sensor/reader. This saves the entire process when idle.
  if ( ! rfid.PICC_IsNewCardPresent())
  {
    return 0;
  }

  // Verify if the NUID has been readed
  if ( ! rfid.PICC_ReadCardSerial())
  {
    return 0;
  }
  Serial.print(F("PICC type: "));
  MFRC522::PICC_Type piccType = rfid.PICC_GetType(rfid.uid.sak);
  Serial.println(rfid.PICC_GetTypeName(piccType));

  // Check is the PICC of Classic MIFARE type
  if (piccType != MFRC522::PICC_TYPE_MIFARE_MINI &&  
    piccType != MFRC522::PICC_TYPE_MIFARE_1K &&
    piccType != MFRC522::PICC_TYPE_MIFARE_4K) {
    Serial.println(F("Your tag is not of type MIFARE Classic."));
    return 0;
  }
    Serial.println(F("check"));
    // Store NUID into nuidPICC array
    for (byte i = 0; i < 4; i++) {
      nuidPICC[i] = rfid.uid.uidByte[i];
    }

    unsigned long res = printDec(rfid.uid.uidByte, rfid.uid.size);
    Serial.print(F("결과 : "));
    Serial.println(res);
    
    if (res == 2725315134 ) 
    { 
      // NFC 태그가 임산부 태그라면
      delay(1000);
      return 1;
    }
    else if (res == 739855926 )
    {
      // NFC태그가 노약자 태그라면
      delay(1000);
      return 2;
    }
    else
    {
      // 그 외의 태그인 경우
      return 0;
    }
  

  // Halt PICC
  rfid.PICC_HaltA();

  // Stop encryption on PCD
  rfid.PCD_StopCrypto1();
}




/**
 * Helper routine to dump a byte array as hex values to Serial. 
 */
void printHex(byte *buffer, byte bufferSize) {
  for (byte i = 0; i < bufferSize; i++) {
    //Serial.print(buffer[i] < 0x10 ? " 0" : " ");
    //Serial.print(buffer[i], HEX);
  }
      String hexstring="";
    for (byte i = 0; i < 4; i++) {
        hexstring += String(rfid.uid.uidByte[i], HEX);
    }
    //Serial.println(hexstring);
  delay(1000);
}

/**
 * Helper routine to dump a byte array as dec values to Serial.
 */
unsigned long printDec(byte *buffer, byte bufferSize) {
  for (byte i = 0; i < bufferSize; i++) {
    //Serial.print(buffer[i] < 0x10 ? "0" : "");
    //Serial.print(buffer[i], DEC);
    
  }
  String decstring="";
    for (byte i = 0; i < 4; i++) {
        decstring += String(rfid.uid.uidByte[i], DEC);
    }
    unsigned long res = decstring.toInt();
    return res;
}



bool checkPressure()
{
  if(analogRead(FSR) < 50)
  {
    return false;
  }
  else
    return true;
}
