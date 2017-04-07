//Time Declerations
#include <FlexiTimer2.h>
float seconds = 0;
float startTime = 0;
float currentTime;
float deltaT;
float steptime = 1.0/5000;

//Motor Declerations
#define outputA A0
#define outputB A1
float counter = 0;
float counterValue = 0;
float oldcounterValue = 0;
float oldCounter = 0;
int aState;
int aLastState;
int set1 = 3;
int set2 = 4;
int enable = 5;

//Angle Declerations
float realAngle = 0;
float counterAngle = 0;
float oldcounterAngle = 0;
float newAngle = 0;
const float maxAngle = 360;
const float scaleFactor = 13.8461538; 
float countarray[3]={0,0,0};
float deltacount=0;

//Angular Velocity Declerations
long double oldTime = 0;
long double newTime = 0;
double oldThetaM = 0;
double newThetaM = 0;
float angVelMotor;
long int revs;
float rpm;

int sizing=30;
float countvalues[30];
int i = sizing-1;


void flash(){
  setMotor(0);
  getEncoder();
  getAngVelMotor();
  
}

void setup(){

  Serial.begin(250000);

// Timer Setup
  FlexiTimer2::set(1, steptime, flash); // call every 500 1ms "ticks"
  //FlexiTimer2::set(500, flash); // MsTimer2 style is also supported
  FlexiTimer2::start();
  
// Motor/Encoder Setup
  pinMode(outputA,INPUT);
  pinMode(outputB,INPUT);
  pinMode(set1, OUTPUT);
  pinMode(set2, OUTPUT);
  pinMode(enable, OUTPUT);
  aLastState = digitalRead(outputA);
}

void loop(){
  float seconds = micros()*0.000001;
  Serial.print(seconds);
  Serial.print("\t");
  Serial.print(counter);
  Serial.print("\t");
  Serial.print(countvalues[sizing-1]);
  Serial.print("\t");
  Serial.print(countvalues[0]);
  Serial.print("\t");
  Serial.println(angVelMotor);
}

void setMotor(int val){
  digitalWrite(set1, LOW);
  digitalWrite(set2, HIGH);
  analogWrite(enable, val);
}

void getEncoder(){
  aState = digitalRead(outputA); // Reads the "current" state of the outputA
  // If the previous and the current state of the outputA are different, that means a Pulse has occured
  if (aState != aLastState){     
    // If the outputB state is different to the outputA state, that means the encoder is rotating clockwise
    if (digitalRead(outputB) != aState) {
      oldCounter = counter;
      counter ++;
    } else {
      oldCounter = counter;
      counter --;
    }
  }
  
  aLastState = aState; // Updates the previous state of the outputA with the current state
}

void getAngMotor(){ 
  counterAngle = abs(scaleFactor*counter);
  realAngle = scaleFactor*counter;
  newAngle = realAngle;
  revs = realAngle/maxAngle;
  if(counterAngle >= maxAngle || counterAngle <= -maxAngle){
    long int revsOver = (long int) counterAngle/maxAngle;
    long int revsOverDeg = maxAngle*revsOver;
    counterAngle = counterAngle - revsOverDeg; 
  }
  
}

void getAngVelMotor(){

  if(i != -1){
    countvalues[i] = counter;
    i--;
  } else{
    for(int j=sizing-1; j>=0; j--){
      if(j!=0){
        countvalues[j] = countvalues[j-1];
      } else if(j == 0){
        countvalues[j] = counter;
      }
    }
    i = -1;
    deltacount = abs(countvalues[sizing-1]-countvalues[0]);
    angVelMotor = deltacount/(steptime*sizing);
  }





  
//  deltacount = counter- oldcounterValue;
//  oldcounterValue= counter;
//  deltacount = abs(deltacount);
//  angVelMotor = deltacount/(steptime);
  
}
