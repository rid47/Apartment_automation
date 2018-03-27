//------------------------PIR CREDENTIALS--------------------------------//

int pirValue1=0;
int calibrationTime = 15;
long unsigned int lowIn1;         

//the amount of milliseconds the sensor has to be low 
//before we assume all motion has stopped

long unsigned int pause =15000;  

boolean lockLow1 = true;
boolean takeLowTime1;  

//-----------------Defining required pins---------------------------//
             
#define pirPin1 2              
//#define pirPin2 3
#define pirIndicator1 4
//define pirIndicator2 5
//---------------------------------------------setup-------------------//


void setup() {
  // put your setup code here, to run once:
  

Serial.begin(9600);
pinMode(pirPin1,INPUT);
pinMode(pirIndicator1,OUTPUT);
//give the sensor some time to calibrate
Serial.print("calibrating sensor ");
for(int i = 0; i < calibrationTime; i++){
      Serial.print(".");
      delay(1000);
      }
    Serial.println(" done");
    Serial.println("SENSORs are ACTIVE");
    delay(50);
  

}

//-------------------------------------------loop-------------------//

void loop() 
{

  
 //---------------------Reading PIR data---------------------------------------------//

    
  pirValue1=digitalRead(pirPin1);
  delay(100);
  Serial.print("pirValue1:");
  Serial.println(pirValue1);

      
  if(pirValue1== HIGH)
  {
        lockLow1 = true;
        
        //digitalWrite(lightPin2,LOW);

    if(lockLow1){  
         //makes sure we wait for a transition to LOW before any further output is made:
         lockLow1 = false;            
         Serial.println("---");
         Serial.print("motion detected at ");
         Serial.print(millis()/1000);
         Serial.println(" sec");

         // Making Indicator pin HIGH

         digitalWrite(pirIndicator1,HIGH);
          
         delay(50);
         }         
         takeLowTime1 = true;
       }
  
  
  
  if(pirValue1==LOW)

  {

       
        if(takeLowTime1){
        lowIn1 = millis();          //save the time of the transition from high to LOW
        takeLowTime1 = false;       //make sure this is only done at the start of a LOW phase
        }
       //if the sensor is low for more than the given pause, 
       //we assume that no more motion is going to happen
       if(!lockLow1 && millis() - lowIn1 > pause){  
           //makes sure this block of code is only executed again after 
           //a new motion sequence has been detected
           lockLow1 = true;                        


        
        //digitalWrite(lightPin,HIGH);
        
           
        Serial.print("motion ended at ");
        Serial.print(millis()/1000);
        Serial.println(" sec");
        // Making Indicator pin HIGH
        digitalWrite(pirIndicator1,LOW);
        delay(50);
           }
       } 
       }
  
  
  
