#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include<Ticker.h>
#include <DNSServer.h>.
#include <ESP8266WebServer.h>
#include <WiFiManager.h>        // https://github.com/tzapu/WiFiManager
#include "DHT.h" 
#include <EEPROM.h>
#define EEPROM_Address_initial_flag 54
#define EEPROM_Address_Motion_State 6
#define EEPROM_allocate_Size 50
#define Threshold_sensorReadingTime 1000
DHT dht;


unsigned int prev_sensorReadingTime=0;
int Version=1;
int pirValue1,pirValue2,sonarValue,prev_pirValue1=0,prev_pirValue2=0;
int motionflag=0;
_Bool flag=true;
volatile float data1_temp = 0;
volatile float data2_hum = 0;
volatile float prev_Temp = 0;
volatile float prev_Hum = 0;


//--------------------ISR for implementing watchdog-------------------//


Ticker secondTick;
volatile int watchdogCount=0;
void ISRwatchdog(){


  watchdogCount++;
  if(watchdogCount==200){


    Serial.println();
    Serial.println("The watch dog bites......");
    //ESP.reset();
    ESP.restart();
  }
}


//-----------------Defining required pins---------------------------//

#define pirPin1 14             // NODE MCU PIN D5
#define pirPin2 12             // NODE MCU PIN D6
#define wifi 13             // NODE MCU PIN D7
#define roomlight 15             // NODE MCU PIN D8
#define dht_dpin 4                //Node MCU PIN D2
#define DHTTYPE DHT22

//-------------------------------WiFi Credentials--------------------//

const char* mqtt_server = "182.163.112.207";

WiFiClient espClient;
PubSubClient client(espClient);


//---------------------------------------------setup-------------------//
void setup() 
{

  Serial.begin(115200);
  secondTick.attach(1,ISRwatchdog);
  
  //-------------------------------------Declare Pin Mode-------------------//
  pinMode(pirPin1,INPUT);
  pinMode(pirPin2,INPUT);
  pinMode(roomlight,OUTPUT);
  pinMode(wifi,OUTPUT);
  dht.setup(dht_dpin);
  
   //-------------------------------------Initial Condition-------------------//
  digitalWrite(wifi,LOW);
  digitalWrite(roomlight,LOW);

  //....................EEPROM.........................//
  EEPROM.begin(EEPROM_allocate_Size);
  if(EEPROM.read(EEPROM_Address_initial_flag)!='w')
    {
      EEPROM.write(EEPROM_Address_initial_flag,'w');
      EEPROM.write(EEPROM_Address_Motion_State,0);
      EEPROM.commit();
    }
  motionflag=EEPROM.read(EEPROM_Address_Motion_State);

  //....................WiFi Manager.........................//
  WiFiManager wifiManager;
  //wifiManager.resetSettings();
  wifiManager.autoConnect("Smart Home", "admin1234");
  if (WiFi.status() == WL_CONNECTED)
  {
    Serial.println("Connected.");
    digitalWrite(wifi,HIGH);
  }

  else
  {
    Serial.println("Not Connected.");
    digitalWrite(wifi,LOW);  
  }
  //....................MQTT Connect.........................//
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
  

}

void loop() 
{
  // put your main code here, to run repeatedly:
  watchdogCount=0;

  //....................MQTT Connection Check.........................//
  if (!client.connected()) {
    reconnect();
  }
  client.loop();


  //---------------------Updating device status---------------------------------------------//

    if(Version==1)
    {
      Serial.println("Firmware 18.0");
      Version++;
    }

   //---------------------Reading DHT22 Data---------------------------------------------//
   
    
  if(((millis()-prev_sensorReadingTime)>Threshold_sensorReadingTime) && readTempHum() )
{
  if(prev_Temp!=data1_temp)
  {
    prev_sensorReadingTime=millis();
    prev_Temp=data1_temp;
    
    // making string for temp
    String temp=""; 
    temp= temp+ data1_temp;
    
    char temperature[6];
    temp.toCharArray(temperature,6);
  
    // Making string for hum
    String hum=""; 
    hum= hum+data2_hum;
    
    char humidity[6];
    hum.toCharArray(humidity,6);
  
   
    client.publish("dssmart/temp/1",temperature);
    client.publish("dssmart/hum/1",humidity);
    Serial.println("TEMP and Hum published");
  }
    
}

//---------------------Reading PIR Value---------------------------------------------//

pirValue1=digitalRead(pirPin1);
pirValue2=digitalRead(pirPin2);

if(pirValue1!=prev_pirValue1 || pirValue2!=prev_pirValue2)
{
    prev_pirValue1=pirValue1;
    prev_pirValue2=pirValue2;
    Serial.println("pirValue1: "+ String (pirValue1));  
    Serial.println("pirValue2: "+ String (pirValue2));
}

if (motionflag==1)
  {    
    if((pirValue1== 1 || pirValue2== 1)&& flag==true)
  {
    client.publish("shs247/relay/0/set/1","1");
    Serial.println("Published data to turn on light");
    digitalWrite(roomlight,HIGH);
    flag=false;
    Serial.println("Light on");
  } 
    
if((pirValue1==0 && pirValue2==0)&& flag==false)
  {
     client.publish("shs247/relay/0/set/1","0");
     Serial.println("Published data to turn off light");
     digitalWrite(roomlight,LOW);
     flag=true;
     Serial.println("light off");        
  }
  
}
}


void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) 
  {
    Serial.println("Attempting MQTT connection...");
    // Create a random client ID
    String clientId = "ESP8266-";
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    //if your MQTT broker has clientID,username and password
    //please change following line to    if (client.connect(clientId,userName,passWord))
    if (client.connect(clientId.c_str()))
    {
      Serial.println("connected");
     //once connected to MQTT broker, subscribe command if any


//---------------------------------------Subscribing to required topics-----------------------//

     
      client.subscribe("dssmart/reset/1");
      Serial.println("Subsribed to topic: dssmart/reset/1");
      client.subscribe("dssmart/coo.motionsense/1");
      Serial.println("Subsribed to topic: dssmart/coo.motionsense/1");
      client.subscribe("dssmart/coo.motionstatus/1");
      Serial.println("Subsribed to topic: dssmart/coo.motionstatus/1");
      client.subscribe("shs247/relay/0/set/1");
      Serial.println("Subsribed to topic: shs247/relay/0/set/1");
      
    } 
    else 
    {
      Serial.println("failed, rc=");
      Serial.println(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 6 seconds before retrying
      delay(5000);
    }
  }
}



//---------------------------Callback funtion-------------------------------------//

void callback(char* topic, byte* payload, unsigned int length) {
 
  Serial.println("Message arrived in topic: ");
  Serial.println(topic);

//----------------------Restarting the board from Engineer's end----------------------------//



  if(strcmp(topic, "dssmart/reset/1") == 0){
  Serial.println("Message:");
  for (int i = 0; i < length; i++) {
    Serial.println((char)payload[i]);
    char data3=payload[i];
    
  
    if (data3=='1')
    {
      
      Serial.println("Resetting Device.........");
       ESP.restart();
      }}}

   if(strcmp(topic, "dssmart/coo.motionsense/1") == 0){
  Serial.println("Message:");
  for (int i = 0; i < length; i++) {
     motionflag=(int)payload[i]-48;
    Serial.println("Motion Sensing state: "+String(motionflag));
    EEPROM.write(EEPROM_Address_Motion_State,motionflag);
      EEPROM.commit();
    }}


    if(strcmp(topic, "dssmart/coo.motionstatus/1") == 0){
  Serial.println("Message:");
  for (int i = 0; i < length; i++) {
    Serial.println((char)payload[i]);
    char data3=payload[i];
    
  
    if (data3=='1')
    {
      String msg= "";
      msg=msg+ motionflag;
      char m[2];
      msg.toCharArray(m,2);
      client.publish("dssmart/coo.motionsense/1", m );  
    }
      
    }
    }
    if(strcmp(topic, "shs247/relay/0/set/1") == 0){
    Serial.println("Message:");
    for (int i = 0; i < length; i++) {
    Serial.println((char)payload[i]);
    char data3=payload[i];
    
  
    if (data3=='1')
      {
        digitalWrite(roomlight,HIGH);
      }
      
    if (data3=='0')
      {
        digitalWrite(roomlight,LOW);
      }
      }
    }
    
}

unsigned char readTempHum()
{
  volatile unsigned char flag = 0, error_Counter = 0;
  //...................
  while (!flag)
  {
    delay(dht.getMinimumSamplingPeriod());
    data2_hum = dht.getHumidity();
    data1_temp =dht.getTemperature();
    if (data1_temp<5)
        {
        delay(2000);
        client.publish("dssmart/reset/1","1");
    
        }
    if (isnan(data1_temp) || isnan(data1_temp))
    {
      Serial.println("Temp and Humidity reading NaN type");
      flag = 0;
    }
    else
      flag = 1;
    if (++error_Counter > 10)
      break;
  }
  Serial.println("Temp :"+String(data1_temp)+"\n\rHum : "+String(data2_hum));
  return flag;
}



