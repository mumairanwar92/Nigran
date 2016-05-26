//Declaration for GPRS Connection
#include <SoftwareSerial.h>
#include <SPI.h>

char response[100];
int answer = 0;
#include<string.h>

SoftwareSerial GPRS(7,8); // configure software serial port

void setup()
{
  Serial.begin(9600);
  GPRS.begin(9600);
  SPI.begin();
  Serial.println("Starting...");
//  powerOn();
  while (sendATcommand("AT", "OK", 2000) == 0);
  while (sendATcommand("", "Call Ready", 5000) == 0);
  Serial.println("Done,Done,Done,Done,Done,Done,Done,Done,Done,Done,Done,Done,Done,Done,Done,Done,Done,Done,Done,Done,Done,Done,Done,Done,Done,Done");
  setupGPRS();

}

void loop()
{
  // put your main code here, to run repeatedly:
  httpSetParameter(1.1,2.2,3.3,4.4);
  delay(10000);
}



/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//HTTP Set Parameter
void httpSetParameter(float Temperature, float Humidity, float ppmv, int mq)
{
  //http://immunization.ipal.itu.edu.pk/index.php/ApiEngine/IPushdata?rfid=4536822837&mcode=255645435345&amount=223&rate=5.5
  const char *URL = "http://damp-crag-29984.herokuapp.com/server";
  char command[200];
  sprintf(command, "AT+HTTPPARA=\"URL\",\"%s?t=%f&h=%f&d=%f&mq=%d\"", URL, Temperature, Humidity, ppmv, mq);
  sendATcommand(command, "OK", 1000);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//send AT Commands    
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

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//Setup GPRS
void setupGPRS()
{
  Serial.println("setupGPRS.");
  sendATcommand("ATE0", "OK", 1000);
  sendATcommand("AT+IPR=9600", "OK", 1000);
  sendATcommand("AT+CMGF=1", "OK", 100);
  while (sendATcommand("AT+CSTT=\"wap.warid\"", "OK", 2000) == 0 );
  while (sendATcommand("AT+CIICR", "OK", 5000) == 0);
  sendATcommand("AT+CIFSR", ".", 2000);
  sendATcommand("AT+CIFSR=?", "OK", 2000);
  sendATcommand("AT+CDNSCFG=\"8.8.8.8\",\"8.8.4.4\"", "OK", 2000);
  sendATcommand("AT+SAPBR=3,1,\"CONTYPE\",\"GPRS\"", "OK", 2000);
  sendATcommand("AT+SAPBR=3,1,\"APN\",\"wap.warid\"", "OK", 2000);
  sendATcommand("AT+SAPBR=1,1", "OK", 5000);
  Serial.println("Setting completed!");
  
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//Power ON/OFF

void powerOn()
{
//  digitalWrite(onModulePin, LOW);
  delay(1000);
//  digitalWrite(onModulePin, HIGH);
  delay(2000);
//  digitalWrite(onModulePin, LOW);
  delay(4000);
}

//  sendATcommand("AT+CGATT=1", "OK", 5000);
void powerDown()
{
  sendATcommand("AT+CPOWD=1", "NORMAL POWER DOWN", 5000);
}
