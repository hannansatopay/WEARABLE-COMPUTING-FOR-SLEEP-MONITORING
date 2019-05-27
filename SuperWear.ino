#include "I2Cdev.h"
#include "MPU6050.h"
#include "Wire.h"
#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h>

// Auth Token for Blynk
char auth[] = "44681a462bf54c2c90fbbbde2fa6ff17";

// WiFi credentials.
char ssid[] = "superadmin";
char pass[] = "helloworld";

// specific I2C addresses may be passed as a parameter here
// AD0 low = 0x68 (default)
MPU6050 accelgyro(0x68);

int16_t ax, ay, az;
int16_t gx, gy, gz;
float acc_vector;
float tempMPU;

int workMode = 0;

int vibrationPin = D3;

int slouchingCountLight=0;
int slouchingCountModerate=0;
int slouchingCountExtreme=0;

long highPeakStep;
long lowPeakStep;
long highPeakStepVal;
int highPeakStepDetected=0;
int steps=0;

int weight;
int height;
  
unsigned long previousMillis = 0;        
const long interval = 10000;     

BlynkTimer timer;

// Code for Sleep Monitoring
void sleepMonitoring()
{
  if(abs(ay-az)<4000)
  {
    Blynk.virtualWrite(V15, 1);
  }
  else
  {
    Blynk.virtualWrite(V15, 0);
  } 
}

// Code for Custom Alarm
BLYNK_WRITE(V1) {
  // Receive 1.0 when time starts so turn the vibration motor ON and receive 0.0 when time stops so turn the vibration motor OFF
  if (param.asInt() == 1.0)
  {
    digitalWrite(vibrationPin, HIGH);
  }
  else
  {
    digitalWrite(vibrationPin, LOW);
  }
}

BLYNK_WRITE(V10) {
 weight=param.asInt(); // Weight in kg
}

BLYNK_WRITE(V11) {
 height=param.asInt(); // Height in cm
}

// Code to check Work Mode Status
BLYNK_WRITE(V8) {
  workMode = param.asInt();
  if(workMode==0)
  {
    slouchingCountLight=0;
    slouchingCountModerate=0;
    slouchingCountExtreme=0;
    Blynk.virtualWrite(V6,"");
    digitalWrite(vibrationPin, LOW);
  }
}

void setup()
{
  pinMode(vibrationPin, OUTPUT);
  Wire.begin(); // join I2C bus
  accelgyro.initialize();

//  while(!accelgyro.testConnection())
//  {
//    Serial.println("Could not find a valid MPU6050 sensor, check wiring!");
//    delay(500);
//  }

// Add offsets
  accelgyro.setXAccelOffset(-1188);
  accelgyro.setYAccelOffset(1345);
  accelgyro.setZAccelOffset(1386);
  accelgyro.setXGyroOffset(135);
  accelgyro.setYGyroOffset(138);
  accelgyro.setZGyroOffset(-20);

  Blynk.begin(auth, ssid, pass);
  timer.setInterval(10000L, sleepMonitoring); // Check every one minute if sleeping
}

BLYNK_CONNECTED() {
Blynk.syncVirtual(V3, V8, V10, V11);
}

void loop()
{
  Blynk.run();
  timer.run();
  accelgyro.getMotion6(&ax, &ay, &az, &gx, &gy, &gz);
  acc_vector=sqrt(sq(ax)+sq(ay)+sq(az));
  if (workMode == 1)
  { 
    slouchingDetection();
  }
  pedometerActivity();
  temperatureMonitoring();
}

void slouchingDetection() {
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;
    slouchingCountLight=0;
    slouchingCountModerate=0;
    slouchingCountExtreme=0;
  }
 if(((az+ay) >= 100)&&((az+ay) < 3000)){
  slouchingCountLight=slouchingCountLight+1;
  if(slouchingCountLight>10)
  {
      Blynk.virtualWrite(V6,"Light Slouching"); 
      slouchingCountLight=0;
  }
 }
 else if(((az+ay) >= 3000)&&((az+ay) < 6000)){
  slouchingCountModerate=slouchingCountModerate+1;
    if(slouchingCountModerate>10)
  {
      Blynk.virtualWrite(V6,"Moderate Slouching");
      slouchingCountModerate=0;
      digitalWrite(vibrationPin, HIGH);
  }
 }
 else if(((az+ay) >= 6000)){
  slouchingCountExtreme=slouchingCountExtreme+1;
    if(slouchingCountExtreme>10)
  {
      Blynk.virtualWrite(V6,"Extreme Slouching");
      slouchingCountExtreme=0;
      digitalWrite(vibrationPin, HIGH);
  }
 }
 else
 {
  Blynk.virtualWrite(V6,"Good Posture");
  digitalWrite(vibrationPin, LOW);
 }
}

void pedometerActivity() {
 if(highPeakStepDetected==0)
{
  highPeakStep=0;
  lowPeakStep=1000;
}
if(acc_vector>16000)
{
 highPeakStep = millis();
 highPeakStepVal=acc_vector;
 highPeakStepDetected=1;
}
else if(acc_vector<(highPeakStepVal*0.95))
{
  lowPeakStep = millis();
}
if(abs(lowPeakStep-highPeakStep)<70)
{
  steps=steps+1;
  highPeakStepDetected=0;
  float caloriesBurnedPerMile = 0.57 * (weight * 2.2);
  float strip = height * 0.415;
  float stepCountMile = 160934.4 / strip;
  float conversationFactor = caloriesBurnedPerMile / stepCountMile;
  float caloriesBurned = steps * conversationFactor; // In cal
  float distance = (steps * strip) / 100000; // In km
  Blynk.virtualWrite(V12,steps);
  Blynk.virtualWrite(V13,caloriesBurned);
  Blynk.virtualWrite(V14,distance);
  delay(100);
}
}

void temperatureMonitoring() {
  tempMPU = accelgyro.getTemperature();
  Blynk.virtualWrite(V4, (tempMPU/340.00+36.53)); // Temperature in degree celcius
}
