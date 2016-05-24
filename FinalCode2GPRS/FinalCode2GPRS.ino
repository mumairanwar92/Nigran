#include <SPI.h>
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//Declarations for String Fuctions to be used in HTTP section
String mainquery, querystring;
char response[100];
int answer = 0;
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//Declaration for GPRS Connection
#include <SoftwareSerial.h>
SoftwareSerial GPRS(0, 1); // configure software serial port

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//Declaration for Dust Sensor
#include<string.h>
byte buff[2];
int pin = 3;//DSM501A input D8
unsigned long duration;
unsigned long starttime;
unsigned long endtime;
unsigned long sampletime_ms = 30000;
unsigned long lowpulseoccupancy = 0;
float ratio = 0;
float concentration = 0;
int temp = 20; 
int i=0;
float ppmv;
short mg;

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//Declaration for MQ7
const int DOUTpin=4;
const int AOUTpin=5;
int value=0;
int limit=0;

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//Decation for DHT11
#include <DHT.h>
#define DHT11_PIN 6
#define DHTTYPE DHT11
DHT dht(DHT11_PIN,DHTTYPE);
float Temperature;
float Humidity;

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//Setup for whole (Combined) Skecth

void setup()
{
  Serial.begin(9600);
  /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  //DSM501 Dust/Smoke
  pinMode(pin,INPUT);                                                   //Setting pin#3 to take input from DSM501A
  starttime = millis(); 
  
  /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  //MQ7 Gases
  pinMode(DOUTpin, INPUT);                                              //Setting pin#5 to get digital data from MQ7
  pinMode(AOUTpin, INPUT);                                              //Setting pin#6 to give a blink at specific time!
  
  /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  //DHT11 Temp/Himidity
  dht.begin();                                                          //Other cinfigs already setted up in decalration, particularly for DHT!
  Temperature=0.0;
  Humidity=0.0;
  
  /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  //GPRS Communication
  GPRS.begin(9600);
  SPI.begin();
  /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  //Data to be appended in Query String!
  mainquery = String("AT+HTTPPARA=\"URL\",\"http://3c5688f3.ngrok.io/server");
  querystring = String();
  querystring += (mainquery);
  
  Serial.println("Starting...");
//  powerOn();
  while (sendATcommand("AT", "OK", 1000) == 0);
  sendATcommand("", "Call Ready", 5000);
  Serial.println("Done");
  setupGPRS();
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//Loop for the whole (Combined) Sketch
void loop()
{
  /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////  
  //DSM501 Code
  duration = pulseIn(pin, LOW);
  lowpulseoccupancy += duration;
  endtime = millis();
  
  /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////  
  //MQ7 Code
  value= analogRead(AOUTpin);
  limit= digitalRead(DOUTpin);
  
  /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////   
  //DHT11 Code
  float val=dht.read(DHT11_PIN);
  Temperature = dht.readTemperature();
  Humidity = dht.readHumidity();

  /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////   
  //Now starting again the counter for DSM501A, so that millis could start the time & PulseIn could detect a particle again!
  if ((endtime-starttime) > sampletime_ms)
  {
    ratio = (lowpulseoccupancy-endtime+starttime + sampletime_ms)/(sampletime_ms);  // Integer percentage 0=>100
    concentration = 1.1*pow(ratio,3)-3.8*pow(ratio,2)+520*ratio+0.62; // using spec sheet curve
    ppmv=(concentration*0.0283168/100/1000) *  (0.08205*temp)/0.01;
    mg = (concentration*0.0283168/100/1000);
    //Serial.print("PPMV:");
    //Serial.println(ppmv);
    //Serial.print("MG/m3:");
    //Serial.println(mg);
  /*    
  Conversion Calculator
  "mg/m3 to ppm" or "ppm to mg/m3"
  The conversion equation is based on 25 ยบC and 1 atmosphere:
  X ppm   =   (Y mg/m3)(24.45)/(molecular weight)
  or
  Y mg/m3   =   (X ppm)(molecular weight)/24.45
  */
    //Serial.print("lowpulseoccupancy:");
    //Serial.print(lowpulseoccupancy);
    //Serial.print("    ratio:");
    //Serial.print(ratio);
    //Serial.print("    DSM501A:");
    //Serial.println(concentration);
    
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////   
    //Data is supposed to be transfered after 30 sec when DSM values are available too ...
/*    GPRS.println("AT+CSQ"); // Signal quality check
    delay(1000);
    GPRS.println("AT+CGATT?"); //Attach or Detach from GPRS Support
    delay(1000);
    GPRS.println("AT+SAPBR=3,1,\"CONTYPE\",\"GPRS\"");//setting the SAPBR, the connection type is using gprs
    delay(1000);
    GPRS.println("AT+SAPBR=3,1,\"APN\",\"warid\"");//setting the APN, Access point name string
    delay(4000);
    GPRS.println("AT+SAPBR=1,1");//setting the SAPBR
    delay(2000);
    GPRS.println("AT+HTTPINIT"); //init the HTTP request
    delay(2000);
    httpSetParameter(Temperature,Humidity,ppmv,value);
*/
/*    
    //Appending parameters to query string:
    querystring += String("?t=");
    querystring += Temperature;
    querystring += String("&h=");
    querystring += Humidity;
    querystring += String("&d=");
    querystring += ppmv;
    querystring += String("&mq=");
    querystring += value;
*/
/*
Serial.println(Temperature);
Serial.println(Humidity);
Serial.println(ppmv);
Serial.println(querystring);
Serial.println(mainquery);
*/    
/*    //Sending out the query string:
    GPRS.println(querystring);
    delay(5000);
    GPRS.println("AT+HTTPACTION=0");//submit the request 
    delay(10000);//the delay is very important, the delay time is base on the return from the website, if the return datas are very large, the time required longer.
    querystring = String(mainquery); //resetting the main query, so that latest parameters can be appended again!
    
    GPRS.println("AT+HTTPREAD");// read the data from the website you access
    delay(10000);
    ShowSerialData();
*/    
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    //Now resetting the variables for DSM again
    lowpulseoccupancy = 0;
    starttime = millis();
  }
}


void ShowSerialData()
{
  while(GPRS.available()!=0)
    Serial.print(char(GPRS.read()));
    Serial.print("\n");
}


/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//HTTP Set Parameter
void httpSetParameter(float Temperature, float Humidity, float ppmv, int mq)
{
  //http://immunization.ipal.itu.edu.pk/index.php/ApiEngine/IPushdata?rfid=4536822837&mcode=255645435345&amount=223&rate=5.5
  const char *URL = "http://http://3c5688f3.ngrok.io/server?";
  char command[200];
  sprintf(command, "AT+HTTPPARA=\"URL\",\"%st=%f&h=%f&d=%f&mq=%d\"", URL, Temperature, Humidity, ppmv, mq);
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
  while (sendATcommand("AT+CSTT=\"warid\"", "OK", 2000) == 0 );
  while (sendATcommand("AT+CIICR", "OK", 5000) == 0);
  sendATcommand("AT+CIFSR", ".", 2000);
  sendATcommand("AT+CIFSR=?", "OK", 2000);
  sendATcommand("AT+CDNSCFG=\"8.8.8.8\",\"8.8.4.4\"", "OK", 2000);
  sendATcommand("AT+CGATT=1", "OK", 5000);
  sendATcommand("AT+SAPBR=3,1,\"CONTYPE\",\"GPRS\"", "OK", 2000);
  sendATcommand("AT+SAPBR=3,1,\"APN\",\"warid\"", "OK", 2000);
  sendATcommand("AT+SAPBR=1,1", "OK", 5000);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//Power ON/OFF
/*
void powerOn()
{
  digitalWrite(onModulePin, LOW);
  delay(1000);
  digitalWrite(onModulePin, HIGH);
  delay(2000);
  digitalWrite(onModulePin, LOW);
  delay(4000);
}

void powerDown()
{
  sendATcommand("AT+CPOWD=1", "NORMAL POWER DOWN", 5000);
}
*/
