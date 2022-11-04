#include <SPI.h>
#include <MFRC522.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <Keypad.h>

const int Field_Number_1 = 1;
#define SS_PIN    22  
#define RST_PIN   27 
#define RED_LED 33 //indica che l'apertura dell'elettroserratura viene negata
#define GREEN_LED 32 //indica l'apertura dell'elettroserratura
#define Password_Length 5 // Lunghezza Password della tastiera 4+1
char Data[Password_Length];
char Master[Password_Length] = "2580"; //PW per apertura serratura
byte data_count = 0, master_count = 0;
bool Pass_is_good;
char customKey;
bool leggibile=true;
const byte RIGHE = 4;
const byte COLS = 4;
char hexaKeys[RIGHE][COLS] = {
  {'1', '2', '3', 'A'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'C'},
  {'*', '0', '#', 'D'}
};
byte rowPins[RIGHE] = {5, 17, 16, 4};
byte colPins[COLS] = {0, 2, 15, 12}; 
 
Keypad customKeypad = Keypad(makeKeymap(hexaKeys), rowPins, colPins, RIGHE, COLS);
 
MFRC522 rfid(SS_PIN, RST_PIN);

const char * ssid = ""; //modificare in base alla rete
const char * password = ""; //modificare in base alla password della rete
const char *host = "http://192.168.43.117/"; //modificare in base all host
String server = "http://maker.ifttt.com";
String eventName = "rfid_read";
String IFTTT_Key = "INSERIRE IFTTT KEY";
String IFTTTUrl="INSERIRE IFTTT URL";
String value1;
String value2;
String value3;
int count=0;

void setup() {
  
  Serial.begin(115200);
  SPI.begin(); // init SPI bus
  rfid.PCD_Init(); // init MFRC522
  pinMode(GREEN_LED, OUTPUT); // initialize pin as an output.
  pinMode(RED_LED, OUTPUT); // initialize pin as an output.
  digitalWrite(GREEN_LED, LOW); // lock the door
  digitalWrite(RED_LED, LOW); // lock the door
  WiFi.mode(WIFI_STA);
  internet();
  Serial.println("Tap an RFID/NFC tag on the RFID-RC522 reader");
}

void loop() {
  internet();
  if (rfid.PICC_IsNewCardPresent() && leggibile) { // new tag is available
    if (rfid.PICC_ReadCardSerial()) { // NUID has been readed
      MFRC522::PICC_Type piccType = rfid.PICC_GetType(rfid.uid.sak);
      HTTPClient http;
      String GetAddress, LinkGet, getData;
      String id = String(rfid.uid.uidByte[0])+String(rfid.uid.uidByte[1])+String(rfid.uid.uidByte[2])+String(rfid.uid.uidByte[3]);
      GetAddress = "scriptDatabase.php?uid="+id; 
      LinkGet = host + GetAddress; 
      http.begin(LinkGet);
      http.addHeader("Content-Type", "application/x-www-form-urlencoded");    //Specify content-type header
      int httpCodeGet = http.GET(); //--> Send the request
      String payloadGet = http.getString();
      http.end();
      if (payloadGet!="0 results" && payloadGet!="nessun uid passato") {
        Serial.print("The access is granted to user: ");
        Serial.print(payloadGet);
        Serial.println();
        sendDataToSheet(payloadGet,id);
        digitalWrite(GREEN_LED, HIGH);  // unlock the door for 2 seconds
        delay(2000);
        digitalWrite(GREEN_LED, LOW); // lock the door
      }
      else
      {
        Serial.print("The access is denied to user with UID: ");
        Serial.print(id);
        Serial.println();
        sendDataToSheet("unknown uid",id);
        digitalWrite(RED_LED, HIGH);  // unlock the door for 2 seconds
        delay(2000);
        digitalWrite(RED_LED, LOW); // lock the door
      }
      
      rfid.PICC_HaltA(); // halt PICC
      rfid.PCD_StopCrypto1(); // stop encryption on PCD
    }
  }
  else{
    customKey = customKeypad.getKey();
    if (customKey) {
    leggibile=false;
    
    Data[data_count] = customKey;
    if(count==0){
      Serial.print("Password: ");
      }
    count++;
    Serial.print(Data[data_count]);
    data_count++;
  }
  if (data_count == Password_Length - 1) {
    count=0;
    if (!strcmp(Data, Master)) {
      Serial.println("");
      Serial.println("right password!");
      
      sendDataToSheet("right password",0+"");
      Serial.println("The access is granted to USER");
      digitalWrite(GREEN_LED, HIGH);  // unlock the door for 2 seconds
      delay(2000);
      digitalWrite(GREEN_LED, LOW);
      //clear screen
    }
    else {
      Serial.println("");
      Serial.println("wrong password!");
      Serial.println("The access is denied to USER");
      sendDataToSheet("wrong password",0+"");
      digitalWrite(RED_LED, HIGH);  // unlock the door for 2 seconds
      delay(2000);
      digitalWrite(RED_LED, LOW); // lock the door
      
      //clear screen
      
  }
  clearData();
  leggibile=true;
    
    }
    }
  
 
}


void internet()
{
  if (WiFi.status() != WL_CONNECTED)
  {
    while (WiFi.status() != WL_CONNECTED)
    {
      WiFi.begin(ssid, password);
      delay(5000);
    }
  }
}
void clearData() {
  while (data_count != 0) {
    Data[data_count--] = 0;
  }
  return;
}

void sendDataToSheet(String name,String id)
{
  String url = server + "/trigger/" + eventName + "/with/key/" + IFTTT_Key + "?value1=" + name + "&value2="+id;  
  //Start to send data to IFTTT
  HTTPClient http;
  http.begin(url); //HTTP


  // start connection and send HTTP header
  int httpCode = http.GET();
  // httpCode will be negative on error
  if(httpCode > 0) {
    // HTTP header has been send and Server response header has been handled
    // file found at server
    if(httpCode == HTTP_CODE_OK) {
      String payload = http.getString();
    }
  } else {
    Serial.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
  }
  http.end();

}
