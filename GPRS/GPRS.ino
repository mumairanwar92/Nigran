#include <SoftwareSerial.h>
#include <AddicoreRFID.h>
#include <EEPROM.h>
#include <SPI.h>

/*---------------------------------------*/

#define GPRS_Tx 8
#define GPRS_Rx 7

#define onModulePin 9
#define chipSelectPin 10
/*---------------------------------------*/
SoftwareSerial GPRS(GPRS_Rx, GPRS_Tx);
AddicoreRFID myRFID;
/*---------------------------------------*/
int answer = 0;
char response[100];
#define	uchar	unsigned char
#define	uint	unsigned int
int cardStored = memoryEEPROM();
long tagValue = 0;
short maxTags = 200;
/*---------------------------------------*/
void setup()
{
  pinMode(onModulePin, OUTPUT);
  delay(100);
  GPRS.begin(9600);
  Serial.begin(9600);
  SPI.begin();
  pinMode(chipSelectPin, OUTPUT);             // Set digital pin 10 as OUTPUT to connect it to the RFID /ENABLE pin
  digitalWrite(chipSelectPin, LOW);         // Activate the RFID readers
  myRFID.AddicoreRFID_Init();

  Serial.println("Starting...");
  powerOn();
  while (sendATcommand("AT", "OK", 1000) == 0);
  sendATcommand("", "Call Ready", 5000);
  Serial.println("Done");
  setupGPRS();
}

/*---------------------------------------*/
void loop ()
{

  tagValue = 0;
  long ATMCode = 3110333107;
  /*  Serial.print("Card Stored: ");
    Serial.println(cardStored);
    //Value = ReadCard();
  */
  Serial.println("Reading Card......");
  while (! tagValue )
    tagValue = readCard();
  Serial.println(tagValue);
  if (compareTag(tagValue))
  {
    Serial.println("CardRegistered");
  }
  else
  {
    if (authenRequest(tagValue, ATMCode))
    {
      writeTag2EEPROM(tagValue);       /*  Store Card in Local EEPROM   */
      Serial.println("Card Locally Stored.");
    }
    else
    {
      Serial.println("Card Not Registered");
    }
  }
  delay(1000);
}

/*---------------------------------------*/
long readCard()
{
  long Value;
  uchar status;
  uchar str[16];
  str[1] = 0x4400;
  //Find tags, return tag type
  status = myRFID.AddicoreRFID_Request(PICC_REQIDL, str);
  if (! (status == MI_OK))
    return 0;
  status = myRFID.AddicoreRFID_Anticoll(str);
  if (status == MI_OK)
  {
    Value = 0;
    for (byte i = 0; i < 4; i++) {
      Value = (Value << 8) + str[i];
    }
    return Value;
  }
  else
    return 0;
  //  myRFID.AddicoreRFID_Halt();		   //Command tag into hibernation
}

void printEEPROM()
{
  for (int i = 0; i < 5; i++)
    readTagFromEEPROM(i);
}

/*---------------------------------------*/
bool compareTag(long tagValue)
{
  long temp = 0;
  if (cardStored == 0)
  {
    Serial.print("No Card in EEPROM.");
    return false;
  }
  for (int i = 0; i < maxTags; i++)
  {
    if (tagValue == readTagFromEEPROM(i))
      return true;
  }
  return false;
}

/*---------------------------------------*/
long readTagFromEEPROM(int address)
{
  long tagValue = 0;
  for (int j = 0; j < 4; j++)
  {
    tagValue = (tagValue << 8) + EEPROM.read(j + (address * 4));
  }
  return tagValue;
}

/*---------------------------------------*/
void writeTag2EEPROM(long tagValue)
{
  int address = checkEmptyEEPROMAddress();
  int j = 0;
  if (address == -1)
    return;
  for (int i = ((address * 4) + 3); i >= address * 4; i--)
  {
    EEPROM.write( i, (tagValue >> (j * 8)) & 255);
    j++;
  }
  cardStored++;
  //  for(int i = 0;i<3; i++)
  //    Serial.print(EEPROM.read(i));
  //  Serial.println();
}

/*---------------------------------------*/
int memoryEEPROM()
{
  int Stored = 0;
  long value = 0;
  int i = 0, j = 0;
  for (i = 0; i < maxTags; i++)
  {
    for (j = 0; j < 4; j++)
    {
      value = (value << 8) + EEPROM.read(j + (i * 4));
    }
    if (value != 0xFFFFFFFF)
    {
      Stored++;
    }
  }
  return Stored;
}

/*---------------------------------------*/
int checkEmptyEEPROMAddress()
{
  long value = 0;
  int i = 0, j = 0;
  for (i = 0; i < maxTags; i++)
  {
    for (j = 0; j < 4; j++)
    {
      value = (value << 8) + EEPROM.read(j + (i * 4));
    }
    if (value == 0xFFFFFFFF)
    {
      return (i);
    }
  }
  return -1;
}

/*---------------------------------------*/
void setupGPRS()
{
  Serial.println("setupGPRS.");
  sendATcommand("ATE0", "OK", 1000);
  sendATcommand("AT+IPR=9600", "OK", 1000);
  sendATcommand("AT+CMGF=1", "OK", 100);
  while (sendATcommand("AT+CSTT=\"zongwap\"", "OK", 2000) == 0 );
  while (sendATcommand("AT+CIICR", "OK", 5000) == 0);
  sendATcommand("AT+CIFSR", ".", 2000);
  sendATcommand("AT+CIFSR=?", "OK", 2000);
  sendATcommand("AT+CDNSCFG=\"8.8.8.8\",\"8.8.4.4\"", "OK", 2000);
  sendATcommand("AT+CGATT=1", "OK", 5000);
  sendATcommand("AT+SAPBR=3,1,\"CONTYPE\",\"GPRS\"", "OK", 2000);
  sendATcommand("AT+SAPBR=3,1,\"APN\",\"zongwap\"", "OK", 2000);
  sendATcommand("AT+SAPBR=1,1", "OK", 5000);
}

/*---------------------------------------*/
bool authenRequest(long tagValue, long ATMCode)
{
  bool flag = false;
  while (sendATcommand("AT+HTTPINIT", "OK", 2000) == 0);

  /* http://immunization.ipal.itu.edu.pk/index.php/ApiEngine/Auth?rfid=3792724323&mcode=03234595939 */

  const char *URL = "http://immunization.ipal.itu.edu.pk/index.php/ApiEngine/Auth?";
  char command[300];
  sprintf(command, "AT+HTTPPARA=\"URL\",\"%srfid=%lu&mcode=0%lu\"", URL, tagValue, ATMCode);
  answer = 0;
  sendATcommand(command, "OK", 1000);
  sendATcommand("AT+HTTPACTION=0", "+HTTPACTION:0,200,", 10000);
  Serial.print("Response:" );
  Serial.println(response);
  delay(5000);
  sendATcommand("AT+HTTPREAD=0,100", "OK", 5000);
  Serial.print("Response:" );
  Serial.println(response);
  if (strstr(response, "true") != NULL)
    flag = true;
  else if (strstr(response, "false") != NULL)
    flag = false;
  while (sendATcommand("AT+HTTPTERM", "OK", 1000) == 0);
  return flag;
}

/*---------------------------------------*/
void sendTransaction(long tagValue, long ATMCode, long waterAmount, unsigned int flowRate)
{
  while (sendATcommand("AT+HTTPINIT", "OK", 2000) == 0);
  httpSetParameter(tagValue, ATMCode, waterAmount, flowRate);
  sendATcommand("AT+HTTPACTION=0", "+HTTPACTION:0,200,", 10000);
  Serial.print("Response:" );
  Serial.println(response);
  delay(1000);
  sendATcommand("AT+HTTPREAD=0,100", "OK", 5000);
  Serial.print("Response:" );
  Serial.println(response);
  while (sendATcommand("AT+HTTPTERM", "OK", 1000) == 0);
}

/*---------------------------------------*/
void httpSetParameter(long tagValue, long ATMCode, long waterAmount, unsigned int flowRate)
{
  /*http://immunization.ipal.itu.edu.pk/index.php/ApiEngine/IPushdata?rfid=4536822837&mcode=255645435345&amount=223&rate=5.5*/
  const char *URL = "http://immunization.ipal.itu.edu.pk/index.php/ApiEngine/IPushdata?";
  char command[200];
  sprintf(command, "AT+HTTPPARA=\"URL\",\"%srfid=%lu&mcode=0%lu&amount=%i&rate=0\"", URL, tagValue, ATMCode, waterAmount, flowRate);
  sendATcommand(command, "OK", 1000);
}

/*---------------------------------------*/
void powerOn()
{
  digitalWrite(onModulePin, LOW);
  delay(1000);
  digitalWrite(onModulePin, HIGH);
  delay(2000);
  digitalWrite(onModulePin, LOW);
  delay(4000);
}

/*---------------------------------------*/
void powerDown()
{
  sendATcommand("AT+CPOWD=1", "NORMAL POWER DOWN", 5000);
}

/*---------------------------------------*/
int sendATcommand(char* ATcommand, char* expected_answer1, unsigned int timeout)
{
  GPRS.begin(9600);
  Serial.println(ATcommand);
  unsigned int x = 0;
  answer = 0;
  unsigned long previous;
  memset(response, '\0', 100);    // Initialize the string
  delay(100);
  while ( GPRS.available() > 0) GPRS.read();   // Clean the input buffer
  GPRS.println(ATcommand);    // Send the AT command
  previous = millis();
  do {
    if (GPRS.available() != 0) {
      while (GPRS.available())
      {
        response[x] = GPRS.read();
        x++;
      }
    }
    if (strstr(response, expected_answer1) != NULL)
    {
      answer = 1;
    }
    else
      answer = 0;
  }
  while ((answer == 0) && ((millis() - previous) < timeout));
  while (GPRS.available())
  {
    response[x] = GPRS.read();
    x++;
  }
  if ((millis() - previous) > timeout)
    Serial.println("TimeOut.");
  Serial.println(response);
  return answer;
}
