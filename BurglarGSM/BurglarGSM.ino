/*
  Simple SMS Burglar Alarm using Mini PIR Sensor and SIM800 Development Kit
  By: Tiktak 
  
  Hardware: 
  Gizduino with Atmega 328P 16MHz (Arduino Uno Compatible)
  PIR Sensor : D8
  GSM RX -> D3
  GSM TX -> D2
  
  Compiler: Arduino IDE 1.0.6
  Required Library: Software Serial
  GSM Snippet from https://www.cooking-hacks.com
  
*/

#include <SoftwareSerial.h>
SoftwareSerial GSM(2, 3);

#define PRINT(x) Serial.println(x)

const int gsmTimeout = 30;
const int gsmMaxBuffer = 128;
char gsmBuffer[gsmMaxBuffer] = "";
char aux_string[30];
int8_t answer;

boolean arm = false;
boolean replied = true;
boolean burglar;


char* myphone = "09xxxxxxxxx";
char* intromessage = "Ang alarma ay handa na!";
char* message = "May pumasok sa Bahay Mo!";
char* armmessage = "Pinagana mo ang alarma!";
char* disarmmessage = "Pinatay mo ang alarma!";

char* armAlarm = "paganahin";
char* disarmAlarm = "patayin";

void setup() {
  GSM.begin(9600);
  Serial.begin(9600);    
  pinMode(13,OUTPUT);
  pinMode(8,INPUT);

  _networkConnect();
  _initModem();
 
  _sendSMS(intromessage,myphone);
    
}

void loop() {

    _pollGSM();
  
  burglar = digitalRead(8);
  digitalWrite(13,burglar);
  
  if(burglar == true){
    if(arm == true){
      PRINT("Alarm armed, Sending SMS");
       delay(1000);
       _sendSMS(message,myphone);
    }   
  }
    
 
  if(strstr(gsmBuffer,armAlarm) != NULL){
    arm = true;
    memset(gsmBuffer,NULL,gsmMaxBuffer);
    
    replied = false;
    while(replied == false){
      PRINT("Sending arm Reply");
      _sendSMS(armmessage,myphone);
      replied = true;
      PRINT("Replied");
    } 
  }
  
  if(strstr(gsmBuffer,disarmAlarm) != NULL){
   
    arm = false;
    memset(gsmBuffer,NULL,gsmMaxBuffer);
     
    replied = false;
    while(replied == false){
       PRINT("Sending disarm Reply");
      _sendSMS(disarmmessage,myphone);
       replied = true;
       PRINT("Replied");
    }
  }  


}

void _pollGSM()
{
   if(GSM.available()){
     byte charsRead = GSM.readBytesUntil('\n',gsmBuffer,gsmMaxBuffer);
     if(charsRead){
       gsmBuffer[charsRead] = 0; //Terminate String
       _gsmSerialHandleLine(String(gsmBuffer));
       
     }
   }
}

void _requestSmsAtIndex(const String &index)
{
  GSM.print("AT+CMGR=" + index + "\r");
}

boolean _isNewSms(const String &line)
{
  // +CMTI: "SM",11
  return line.indexOf("+CMTI") == 0 &&
         line.length() > 12;
}

boolean _isSms(const String &line)
{
  return line.indexOf("+CMGR") == 0;
}

void _receiveTextMessage(const String &line)
{
  String index = line.substring(12);
  index.trim();
  GSM.flush();
  _requestSmsAtIndex(index);
}

boolean _gsmReadBytesOrDisplayError(char numberOfBytes)
{
  if(!_gsmWaitForBytes(numberOfBytes, gsmTimeout))
  {
    PRINT("Error reading bytes from device!");
    return false;
  }

  while(GSM.available())
  {
    char next = GSM.read();
    //Serial.print(next);
  }
  
  return true;
}

char _gsmWaitForBytes(char numberOfBytes, int timeout)
{

  while(GSM.available() < numberOfBytes)
  {
    delay(200);
    timeout -= 1;
    if(timeout == 0)
    {
      return 0;
    }
  }
  return 1;
}

boolean _readAndPrintSms()
{
  // Line contains the +CMGR response, now we need to read the message
  
  byte charsRead = GSM.readBytesUntil('\n', gsmBuffer, gsmMaxBuffer);
  
  if(charsRead > 1)
  {
    charsRead--; // Skip newline
    gsmBuffer[charsRead] = 0; // Terminate string
    _gsmReadBytesOrDisplayError(4); // OK\r\n
    PRINT(gsmBuffer);
    
  }
}

void _gsmSerialHandleLine(const String &s)
{
  //PRINT(s);

  if(_isNewSms(s)) // +CMTI: "SM",1
  {
    _receiveTextMessage(s);
  }
  else if(_isSms(s))
  {
    _readAndPrintSms();
  }
  else
  {
    // Currently unhandled!
  }
}

void _initModem(){
  
  PRINT("Deleting Messages");
  _sendToModem("AT+CMGDA=\"DEL ALL\"","OK",10000);
  PRINT("Setting Storage Preference");  
  _sendToModem("AT+CNMI=3,1","OK",1000);
  PRINT("Setting Modem to Text Mode");  
  _sendToModem("AT+CMGF=1","OK",1000);
}

void _networkConnect()
{
    PRINT("Connecting to Cell Network");

    while( (_sendToModem("AT+CREG?", "+CREG: 0,1", 500) || _sendToModem("AT+CREG?", "+CREG: 0,5", 500)) == 0 ); 
}

int _sendToModem(char* AT, char* resp, unsigned int timeout){

    unsigned int x=0,  answer=0;
    char response[100];
    unsigned long previous;

    memset(response, '\0', 100);
    delay(100);
    
    while( GSM.available() > 0) GSM.read();   
    GSM.println(AT);    

    x = 0;
    previous = millis();
    do{
        if(GSM.available() != 0){    
            response[x] = GSM.read();
            x++;

            if (strstr(response, resp) != NULL) {
                answer = 1;
            }
        }
    }while((answer == 0) && ((millis() - previous) < timeout));    

    return answer;
}

int _sendSMS(char* message, char* number)
{
  sprintf(aux_string,"AT+CMGS=\"%s\"", number);
  answer = _sendToModem(aux_string, ">", 2000); 
  if(answer == 1)
   {
     GSM.print(message);
     GSM.write(0x1A);
     answer = _sendToModem("","OK",20000);
     if(answer == 1)
      {
        PRINT("Message Sent");
      }else
      {
        PRINT("Message not Sent");
      }
   } 
}
