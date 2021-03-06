//Time Declerations
#include <FlexiTimer2.h>
#include "eceRoverMath.h"
float seconds = 0;

//Motor Declerations
#define outputA A0
#define outputB A1
double counter = 0; 
int aState;
int aLastState;
int set1 = 3;
int set2 = 4;
int enable = 5;

// Accelerometer Declerations 
// "OUTPUT_READABLE_YAWPITCHROLL" if you want to see the yaw/
// pitch/roll angles (in degrees) calculated from the quaternions coming
// from the FIFO. Note this also requires gravity vector calculations.
// Also note that yaw/pitch/roll angles suffer from gimbal lock (for
// more info, see: http://en.wikipedia.org/wiki/Gimbal_lock)
#include <Wire.h>
#include "I2Cdev.h"
#include "MPU6050_6Axis_MotionApps20.h"
#define INTERRUPT_PIN 2  // use pin 2 on Arduino Uno & most boards
MPU6050 mpu;
#define OUTPUT_READABLE_YAWPITCHROLL
// MPU control/status vars
bool dmpReady = false;  // set true if DMP init was successful
uint8_t mpuIntStatus;   // holds actual interrupt status byte from MPU
uint8_t devStatus;      // return status after each device operation (0 = success, !0 = error)
uint16_t packetSize;    // expected DMP packet size (default is 42 bytes)
uint16_t fifoCount;     // count of all bytes currently in FIFO
uint8_t fifoBuffer[64]; // FIFO storage buffer
// orientation/motion vars
Quaternion q;           // [w, x, y, z]         quaternion container
VectorInt16 aa;         // [x, y, z]            accel sensor measurements
VectorInt16 aaReal;     // [x, y, z]            gravity-free accel sensor measurements
VectorInt16 aaWorld;    // [x, y, z]            world-frame accel sensor measurements
VectorFloat gravity;    // [x, y, z]            gravity vector
float euler[3];         // [psi, theta, phi]    Euler angle container
float ypr[3];           // [yaw, pitch, roll]   yaw/pitch/roll container and gravity vector
// Interrupt Deteton Routine
volatile bool mpuInterrupt = false;     // indicates whether MPU interrupt pin has gone high
void dmpDataReady() {
    mpuInterrupt = true;
}
double yaw;
double pitch;
double roll;

//Angular Velocity Declerations
double oldTheta = 0;
double newTheta = 0;
double oldThetaM = 0;
double newThetaM = 0;
float oldTime = 0;
float newTime = 0;
float angVelMain;
float angVelMotor;
float omega_initial = 0;
float omega_final = 0;
float omega_final_unfiltered = 0;
float omega_inital_speed = 0;
float omega_final_speed = 0;
unsigned long time_prev = 0;
unsigned long time_now = 0;
double omega;
#define omega_Filter 0.7
#define omega_Speed_Filter 0.7
#define angle_Rounding_Value 1000.

void get_omega() {
  time_prev = time_now;
  time_now = micros();

  omega_initial = omega_final;
  omega_final_unfiltered = round((-ypr[2] * 180 / M_PI) * angle_Rounding_Value) / angle_Rounding_Value;
  omega_final = (1 - omega_Filter) * (omega_final_unfiltered) + omega_Filter * (omega_initial);
  omega_inital_speed = omega_final_speed;
  omega_final_speed = ((1 - omega_Speed_Filter) * (omega_final - omega_initial) / (time_now - time_prev) * 1000000.) + omega_Speed_Filter * omega_inital_speed;
}


void flash(){
  setMotor(0);
  getEncoder();
  getAngVelMain();
  getAngVelMotor();
}

void setup(){

// Timer Setup
  FlexiTimer2::set(1, 1.0/10000, flash); // call every 500 1ms "ticks"
  //FlexiTimer2::set(500, flash); // MsTimer2 style is also supported
  FlexiTimer2::start();
  
// Motor/Encoder Setup
  pinMode(outputA,INPUT);
  pinMode(outputB,INPUT);
  pinMode(set1, OUTPUT);
  pinMode(set2, OUTPUT);
  pinMode(enable, OUTPUT);
  aLastState = digitalRead(outputA);

// Acceleromotor Setup
  // join I2C bus (I2Cdev library doesn't do this automatically)
    #if I2CDEV_IMPLEMENTATION == I2CDEV_ARDUINO_WIRE
        Wire.begin();
        Wire.setClock(400000); // 400kHz I2C clock. Comment this line if having compilation difficulties
    #elif I2CDEV_IMPLEMENTATION == I2CDEV_BUILTIN_FASTWIRE
        Fastwire::setup(400, true);
    #endif
    // initialize serial communication
    // (115200 chosen because it is required for Teapot Demo output, but it's
    // really up to you depending on your project)
    Serial.begin(115200);
    while (!Serial); // wait for Leonardo enumeration, others continue immediately
    // initialize device
    Serial.println(F("Initializing I2C devices..."));
    mpu.initialize();
    pinMode(INTERRUPT_PIN, INPUT);
    // verify connection
    Serial.println(F("Testing device connections..."));
    Serial.println(mpu.testConnection() ? F("MPU6050 connection successful") : F("MPU6050 connection failed"));
    // wait for ready
   
    // load and configure the DMP
    Serial.println(F("Initializing DMP..."));
    devStatus = mpu.dmpInitialize();
    // supply your own gyro offsets here, scaled for min sensitivity
    mpu.setXGyroOffset(220);
    mpu.setYGyroOffset(76);
    mpu.setZGyroOffset(-85);
    mpu.setZAccelOffset(1788); // 1688 factory default for my test chip
    // make sure it worked (returns 0 if so)
    if (devStatus == 0) {
        // turn on the DMP, now that it's ready
        Serial.println(F("Enabling DMP..."));
        mpu.setDMPEnabled(true);
        // enable Arduino interrupt detection
        Serial.println(F("Enabling interrupt detection (Arduino external interrupt 0)..."));
        attachInterrupt(digitalPinToInterrupt(INTERRUPT_PIN), dmpDataReady, RISING);
        mpuIntStatus = mpu.getIntStatus();
        // set our DMP Ready flag so the main loop() function knows it's okay to use it
        Serial.println(F("DMP ready! Waiting for first interrupt..."));
        dmpReady = true;
        // get expected DMP packet size for later comparison
        packetSize = mpu.dmpGetFIFOPacketSize();
    } else {
        // ERROR!
        // 1 = initial memory load failed
        // 2 = DMP configuration updates failed
        // (if it's going to break, usually the code will be 1)
        Serial.print(F("DMP Initialization failed (code "));
        Serial.print(devStatus);
        Serial.println(F(")"));
    }
}

void loop(){
  get_omega();
  getAngles();
  float seconds = micros()*0.000001;
  Serial.print("time: ");
  Serial.print(seconds);
  Serial.print("\t");
  Serial.print("roll: ");
  Serial.print(roll);
  Serial.print("\t");
  Serial.print("counter: ");
  Serial.print(counter);
  Serial.print("\t");
  Serial.print("omega: ");
  Serial.print(omega_final_speed);
  Serial.print("\t");
  Serial.print("angVelMotor: ");
  Serial.println(angVelMotor);
   

}


void getAngles(){
  // if programming failed, don't try to do anything
    if (!dmpReady) return;
    // wait for MPU interrupt or extra packet(s) available
    while (!mpuInterrupt && fifoCount < packetSize) {
        // other program behavior stuff here
        // .
        // .
        // .
        // if you are really paranoid you can frequently test in between other
        // stuff to see if mpuInterrupt is true, and if so, "break;" from the
        // while() loop to immediately process the MPU data
        // .
        // .
        // .
    }
    // reset interrupt flag and get INT_STATUS byte
    mpuInterrupt = false;
    mpuIntStatus = mpu.getIntStatus();
    // get current FIFO count
    fifoCount = mpu.getFIFOCount();
    // check for overflow (this should never happen unless our code is too inefficient)
    if ((mpuIntStatus & 0x10) || fifoCount == 1024) {
        // reset so we can continue cleanly
        mpu.resetFIFO();
        Serial.println(F("FIFO overflow!"));
    // otherwise, check for DMP data ready interrupt (this should happen frequently)
    } else if (mpuIntStatus & 0x02) {
        // wait for correct available data length, should be a VERY short wait
        while (fifoCount < packetSize) fifoCount = mpu.getFIFOCount();
        // read a packet from FIFO
        mpu.getFIFOBytes(fifoBuffer, packetSize);       
        // track FIFO count here in case there is > 1 packet available
        // (this lets us immediately read more without waiting for an interrupt)
        fifoCount -= packetSize;
        #ifdef OUTPUT_READABLE_YAWPITCHROLL
            // display Euler angles in degrees
            mpu.dmpGetQuaternion(&q, fifoBuffer);
            mpu.dmpGetGravity(&gravity, &q);
            mpu.dmpGetYawPitchRoll(ypr, &q, &gravity);
            yaw = ypr[0] * 180/M_PI;
            pitch = ypr[1] * 180/M_PI;
            roll = ypr[2] * 180/M_PI;
//            Serial.print("ypr\t");
//            Serial.print(yaw);
//            Serial.print("\t");
//            Serial.print(pitch);
//            Serial.print("\t");
//            Serial.println(roll);
        #endif
    }
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
      counter ++;
    } else {
      counter --;
    }
  } 
  aLastState = aState; // Updates the previous state of the outputA with the current state
}

void getAngVelMain(){
  newTime = seconds;
  newTheta = roll;
  if (newTime != oldTime){
    angVelMain = (newTheta - oldTheta)/(newTime - oldTime);
  }
  oldTime = newTime;
  oldTheta = newTheta;
}

void getAngVelMotor(){
  newTime = seconds;
  newThetaM = roll;
  if (newTime != oldTime){
    angVelMotor = (newThetaM - oldThetaM)/(newTime - oldTime);
  }
  oldTime = newTime;
  oldThetaM = newThetaM;
}

