#include <Wire.h>
#include "I2Cdev.h"
#include "MPU6050_6Axis_MotionApps20.h"
#define INTERRUPT_PIN 2
#define OUTPUT_READABLE_YAWPITCHROLL
MPU6050 mpu;


//Acceleromter delcarations
bool dmpReady = false;
uint8_t mpuIntStatus;
uint8_t devStatus;
uint16_t packetSize;
uint16_t fifoCount;
uint8_t fifoBuffer[64];
Quaternion q;
VectorInt16 aa;
VectorInt16 aaReal;
VectorInt16 aaWorld;
VectorFloat gravity;
float euler[3];
float ypr[3];

volatile bool mpuInterrupt = false;
void dmpDataReady() {
    mpuInterrupt = true;
}


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



void setup(){

// Accelerometer Setup
      #if I2CDEV_IMPLEMENTATION == I2CDEV_ARDUINO_WIRE
          Wire.begin();
          Wire.setClock(400000);
      #elif I2CDEV_IMPLEMENTATION == I2CDEV_BUILTIN_FASTWIRE
          Fastwire::setup(400, true);
      #endif
      Serial.begin(250000);
      while (!Serial);
      mpu.initialize();
      pinMode(INTERRUPT_PIN, INPUT);
      devStatus = mpu.dmpInitialize();

//Gyro Offsets
      mpu.setXGyroOffset(-180);
      mpu.setYGyroOffset(76);
      mpu.setZGyroOffset(-85);
      mpu.setZAccelOffset(1788);


      if (devStatus == 0) {
          mpu.setDMPEnabled(true);
          attachInterrupt(digitalPinToInterrupt(INTERRUPT_PIN), dmpDataReady, RISING);
          mpuIntStatus = mpu.getIntStatus();
          dmpReady = true;
          packetSize = mpu.dmpGetFIFOPacketSize();
      }
      else {
          Serial.print(F("DMP Initialization failed (code "));
          Serial.print(devStatus);
          Serial.println(F(")"));
      }
}




void loop(){
  get_omega();
  if (!dmpReady) return;
    while (!mpuInterrupt && fifoCount < packetSize) {}
    mpuInterrupt = false;
    mpuIntStatus = mpu.getIntStatus();
    fifoCount = mpu.getFIFOCount();
    if ((mpuIntStatus & 0x10) || fifoCount == 1024) {
        mpu.resetFIFO();
        Serial.println(F("FIFO overflow!"));
    }
    else if (mpuIntStatus & 0x02) {
        while (fifoCount < packetSize) fifoCount = mpu.getFIFOCount();
      mpu.getFIFOBytes(fifoBuffer, packetSize);
      fifoCount -= packetSize;

      #ifdef OUTPUT_READABLE_YAWPITCHROLL
           mpu.dmpGetQuaternion(&q, fifoBuffer);
           mpu.dmpGetGravity(&gravity, &q);
           mpu.dmpGetYawPitchRoll(ypr, &q, &gravity);
           Serial.print("roll: \t");
           Serial.print(ypr[2] * 180/M_PI);
           Serial.print("\t");
           Serial.print("\tOmega: ");
           Serial.println(omega_final_speed);
       #endif
    }
}

