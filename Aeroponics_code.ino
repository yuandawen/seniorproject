
#include <DHT.h>
#include <SPI.h>
#include <Ethernet.h>
#include <UTFT.h>
#include <URTouch.h>"
#include <TSL2561.h>
#include <Wire.h>



/******************** ETHERNET SETTINGS ********************/

byte mac[] = { 0x90, 0xA2, 0xDA, 0x0D, 0x85, 0xD9 };   //physical mac address
byte ip[] = { 192, 168, 0, 112 };                   // ip in lan
byte subnet[] = { 255, 255, 255, 0 };              //subnet mask
byte gateway[] = { 192, 168, 0, 1 };              // default gateway
EthernetServer server(80);                       //server port

/***********************************************************/

UTFT myGLCD(SSD1289,38,39,40,41); //display parameters
URTouch myTouch( 6, 5, 4, 3, 2);

// Define Variables
extern uint8_t SmallFont[];
extern uint8_t BigFont[];
extern uint8_t SevenSegNumFont[];

int count = 0;// needs to run off RTC, not an internal counter
#define Solenoid 11
#define Pressure A2

#define DHTPIN 8            // dht digital pin 
#define DHTTYPE DHT11       // DHT 11
DHT dht(DHTPIN, DHTTYPE); 

#define LightRelay 13        // Light relay digital pin 13
TSL2561 tsl(TSL2561_ADDR_FLOAT); 

#define SensorPin 0          //pH meter Analog output to Arduino Analog Input 0
int x, y;
char currentPage; 
char selectedUnit;
unsigned long int avgValue;  //Store the average value of the sensor feedback
float b;
int buf[10],temp;
//const int PHsense = 13; 


unsigned long  previousMillis = 0;      // stores last time solenoid was on
int RelayState = LOW;                   // RelayState used to update solenoid
#define CH3 9                           // Connect Digital Pin 9 on Arduino to CH3 on Relay Module

long SolenoidOnTime = 3000;             // Solenoid on for 3 seconds
long SolenoidOffTime = 5000;            // Off for 5 seconds //300000; // solenoid off for 5 minutes

void setup() {
  Ethernet.begin(mac,ip,gateway,subnet);
  server.begin();
  
  //Initial Setup
  myGLCD.InitLCD();
  myGLCD.clrScr();
  myTouch.InitTouch();
  myTouch.setPrecision(PREC_MEDIUM);
// pressure sensor
pinMode(Pressure,INPUT);
pinMode(Solenoid,OUTPUT);
Serial.begin(9600);
  
//////// lux
pinMode(LightRelay, OUTPUT);
  if (tsl.begin()) {
    Serial.println("Found sensor");
  } else {
    Serial.println("No sensor?");
    while (1);
  }
  ///////////
  dht.begin();
  /////////// Solenoid relay timer

 
   pinMode(CH3, OUTPUT);                // Set Solenoid relay outpt
   //Turn OFF any power to the Relay channels
   digitalWrite(CH3,LOW);
   delay(2000);                         //Wait 2 seconds before starting sequence
  
  
  pinMode(0,OUTPUT);  
  Serial.begin(9600);  
  Serial.println("Ready");              //Test the serial monitor  
 }

//drawHomeScreen - Custome Function
void drawHomeScreen(){
  //Title
  myGLCD.setBackColor(0,0,0); //Set background color to black
  myGLCD.setColor(255, 255, 255); //Sets color to white
  myGLCD.setFont(BigFont); // Sets font to big
  myGLCD.print("Aeroduino", CENTER, 10); // Prints the string on the screen
  
  myGLCD.setColor(255, 0, 0); // Sets color to red
  myGLCD.drawLine(0,32,319,32); // Draws the red line
 }
 
void loop() {
 phSensor();
 dhtsensor();
 Luxsensor();
 Solenoidtimer();
 pressuresensor();
 webserver();
}
void  Luxsensor(){
// Simple data read example. Just read the infrared, fullspecrtrum diode 
  // or 'visible' (difference between the two) channels.
  // This can take 13-402 milliseconds! Uncomment whichever of the following you want to read
  uint16_t x = tsl.getLuminosity(TSL2561_VISIBLE);     
  //uint16_t x = tsl.getLuminosity(TSL2561_FULLSPECTRUM);
  //uint16_t x = tsl.getLuminosity(TSL2561_INFRARED);

  Serial.println(x, DEC);

  // More advanced data read example. Read 32 bits with top 16 bits IR, bottom 16 bits full spectrum
  // That way you can do whatever math and comparisons you want!
  uint32_t lum = tsl.getFullLuminosity();
  uint16_t ir, full;
  ir = lum >> 16;
  full = lum & 0xFFFF;
  Serial.print("IR: "); Serial.print(ir);   Serial.print("\t\t");
  Serial.print("Full: "); Serial.print(full);   Serial.print("\t");
  Serial.print("Visible: "); Serial.print(full - ir);   Serial.print("\t");
  Serial.print("Lux: "); Serial.println(tsl.calculateLux(full, ir));
  if(full<50){digitalWrite(LightRelay, HIGH);}
  else{digitalWrite(LightRelay, LOW);}
  delay(100); 


  myGLCD.setFont(BigFont);
  myGLCD.setColor(0, 255, 0);
  myGLCD.setBackColor(0, 0, 0);
  myGLCD.print("Lux: ", 0, 120);
  myGLCD.printNumI(tsl.calculateLux(full, ir),50, 120, 3,'0');
}

void  dhtsensor(){

 // Wait a few seconds between measurements.
  delay(200);

  // Reading temperature or humidity takes about 250 milliseconds!
  // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
  float h = dht.readHumidity();
  // Read temperature as Celsius (the default)
  float t = dht.readTemperature();
  // Read temperature as Fahrenheit (isFahrenheit = true)
  float f = dht.readTemperature(true);

  // Check if any reads failed and exit early (to try again).
  if (isnan(h) || isnan(t) || isnan(f)) {
    Serial.println("Failed to read from DHT sensor!");
    return;
  }

  // Compute heat index in Fahrenheit (the default)
  float hif = dht.computeHeatIndex(f, h);
  // Compute heat index in Celsius (isFahreheit = false)
  float hic = dht.computeHeatIndex(t, h, false);

  Serial.print("Humidity: ");
  Serial.print(h);
  Serial.print(" %\t");
  Serial.print("Temperature: ");
  Serial.print(t);
  Serial.print(" *C ");
  Serial.print(f);
  Serial.print(" *F\t");
  Serial.print("Heat index: ");
  Serial.print(hic);
  Serial.print(" *C ");
  Serial.print(hif);
  Serial.println(" *F");
  
// LCD printouts

  myGLCD.setFont(BigFont);
  myGLCD.setColor(0, 255, 0);
  myGLCD.setBackColor(0, 0, 0);
  myGLCD.print("Hum: ", 0, 40);
  myGLCD.printNumI(h,60, 40, 2,'0');
  myGLCD.print("% ", 90, 40);
  myGLCD.print("Temp: ", 0, 70);
  myGLCD.printNumI(f,75, 70, 2,'0');
  myGLCD.print(" F ", 110, 70);
}

void phSensor(){
  
  for(int i=0;i<10;i++)       //Get 10 sample value from the sensor for smooth the value
  { 
    buf[i]=analogRead(SensorPin);
    delay(10);
  }
  for(int i=0;i<9;i++)        //sort the analog from small to large
  {
    for(int j=i+1;j<10;j++)
    {
      if(buf[i]>buf[j])
      {
        temp=buf[i];
        buf[i]=buf[j];
        buf[j]=temp;
      }
    }
  }
  avgValue=0;
  for(int i=2;i<8;i++)                      //take the average value of 6 center sample
    avgValue+=buf[i];
  float phValue=(float)avgValue*5.0/1024/6; //convert the analog into millivolt
  phValue=3.5*phValue;                      //convert the millivolt into pH value
 Serial.print("    pH:");  
 Serial.print(phValue,2);
 Serial.println(" ");
 
  myGLCD.setFont( BigFont);
  myGLCD.setColor(0, 255, 0);
  myGLCD.setBackColor(0, 0, 0);
  myGLCD.print("pH: ", 0, 90);
  myGLCD.printNumI(phValue,50, 90, 2,'0');

 
  digitalWrite(0, HIGH);       
  delay(800);
  digitalWrite(0, LOW); 
}

void pressuresensor(){
float sensorVoltage = analogRead(2);//Sensor Analog 2
Serial.println(sensorVoltage);
int psi = ((sensorVoltage-99)/204)*50; //Voltage to psi calculation
Serial.print("PSI = ");
Serial.println(psi);
count = count+1;
Serial.println(count);
delay(1000);

if(count >= 180)
{digitalWrite(Solenoid,HIGH);
count = 0;
delay(5000);}
else
{digitalWrite(Solenoid,LOW);}

// LCD Screen
 myGLCD.setFont(BigFont);
  myGLCD.setColor(0, 255, 0);
  myGLCD.setBackColor(0, 0, 0);
  myGLCD.print("PSI: ", 0, 150);
  myGLCD.printNumI(psi,50, 150, 3,'0');
  }

void Solenoidtimer(){

unsigned long currentMillis = millis();
 
 if((RelayState== LOW)&&(currentMillis - previousMillis >=SolenoidOnTime))
  {
   
   digitalWrite(CH3, HIGH);  // Solenoid off 
   previousMillis= currentMillis; // Remember the time
   RelayState= HIGH;
   
  }
   else if((RelayState== HIGH)&&(currentMillis - previousMillis >= SolenoidOffTime))
  {
   digitalWrite(CH3,LOW);    //Solenoid on
   previousMillis= currentMillis; // Remember the time
   RelayState= LOW;
  }

}
void webserver(){
//Ethernet Start

EthernetClient client = server.available();    // look for the client

// send a standard http response header

client.println("HTTP/1.1 200 OK");
client.println("Content-Type: text/html");
client.println("Connnection: close");
client.println();

/* 
This portion is the webpage which will be
sent to client web browser one can use html , javascript
and another web markup language to make particular layout 
*/

client.println("<!DOCTYPE html>");      //web page is made using html
client.println("<html>");
client.println("<head>");
client.println("<title>Ethernet Arduino</title>");
client.println("<meta http-equiv=\"refresh\" content=\"1\">");

/*
The above line is used to refresh the page in every 1 second
This will be sent to the browser as the following HTML code:
<meta http-equiv="refresh" content="1">
content = 1 sec i.e assign time for refresh 
*/

client.println("</head>");
client.println("<body>");
client.println("<h2>Airduino</h2>");
client.println("<h2>Status:</h2>");
String PH = "PH ";              //phValue
String Pressure = "Pressure ";  //psi
String Lux = "Lux ";            //
String Temp = "Temperature ";   //temp
String Hum = "Humidity ";       //hum
String pHRead = " <div pH Value is: style='color:black;'>";
String TempRead = " <div Temperature is:  style='color:black;'>";
String HumidRead = " <div Humidityis:  style='color:black;'>";
String PSIRead = " <div Pressure is:  style='color:black;'>";

                  
client.print("<h2>pH Value is: ");
client.print (phValue);
client.print("</h2>"); 

client.print("<h2>Temperature in Celsius: ");
client.print(t);
client.println("</h2>"); 

client.print("<h2>Humidity: ");
client.print(h);
client.println("</h2>"); 

client.print("<h2>Pressure in PSI: ");
client.print(psi);
client.println("</h2>"); 

client.println("</body>");
client.println("</html>");

delay(1);         // giving time to receive the data
client.stop();

}



