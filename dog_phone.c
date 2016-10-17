#include <LCheckSIM.h>
#include <LGSM.h>
#include <LGPS.h>
#include <string.h>
#include <stdio.h>
#include <LBattery.h>

#define YOUR_NUMBER ""
#define YOUR_SECOND_NUMBER ""
#define SOUND_SENSOR A3
#define MAX_NOISE 1500
#define NOISE_LIMIT 350
#define MESSAGE_COUNTER_RESET 300000

char num[20] = {0};
char buf_contex[100];
int noiseLevel = 0;
int batteryCount = MESSAGE_COUNTER_RESET;
int noiseCounter =  MESSAGE_COUNTER_RESET;

//Set up
void setup()
{
    Serial.begin(115200);
    Serial.print("\r\n");
    Serial.print("Init\r\n");
}

void answerCall() {
  char buffer[100] = {0,};
  LVoiceCall.retrieveCallingNumber(num,20);
  sprintf(buffer, "Call come in, number is %s \r\n", num);
  Serial.println(buffer);
  LVoiceCall.answerCall();                                      // pick up the phone call
}

boolean isRinging() {
  return LVoiceCall.getVoiceCallStatus() == RECEIVINGCALL;
}

boolean hasSMS() {
  return LSMS.available();
}

void sendSMS() {
  unsigned KeyValue = 0;
  unsigned char *utc_date_time = 0;
  char buffer[100] = {0,};
  LSMS.remoteNumber(num, 20);
  LSMS.remoteContent(buf_contex, 50);

  sprintf(buffer, "Get new sms, content: %s, number: %s \r\n", buf_contex, num);
  Serial.println(buffer);

  if(strcmp("GPS",(char*)buf_contex) == 0)                       // if the SMS content is 'GPS', then send the GPS information of RePhone back to the sender
  {
      if(LGPS.check_online())
      {
          utc_date_time = LGPS.get_utc_date_time();
          sprintf(buffer, "GPS list:\r\nUTC:%d-%d-%d  %d:%d:%d,\r\nlatitude: %c:%f,\r\nlongitude: %c:%f,\r\naltitude: %f,\r\nspeed: %f,\r\ncourse: %f.",
                           utc_date_time[0], utc_date_time[1], utc_date_time[2], utc_date_time[3], utc_date_time[4],utc_date_time[5],
                           LGPS.get_ns(), LGPS.get_latitude(), LGPS.get_ew(), LGPS.get_longitude(), LGPS.get_altitude(), LGPS.get_speed(), LGPS.get_course());
          Serial.println(buffer);

          if(LSMS.ready())
          {
              LSMS.beginSMS(num);
              LSMS.print(buffer);

              if(LSMS.endSMS()) Serial.println("SMS sent ok!");
              else Serial.println("SMS send fail!");
          }
          else Serial.println("SMS no ready!");
      }
  }
  LSMS.flush();
}

void sendNoiseAlert(int noiseLevel, const char* number) {
  char buffer[100] = {0,};
  if(LSMS.ready())
  {
      sprintf(buffer, "NOISY %d", noiseLevel);

      LSMS.beginSMS(number);
      LSMS.print(buffer);

      if(LSMS.endSMS()) Serial.println("SMS sent ok!");
      else Serial.println("SMS send fail!");
  }
}

void sendBatteryAlert(const char* number) {
  char buffer[100] = {0,};
  if(LSMS.ready())
  {
      sprintf(buffer, "Battery %d", LBattery.level());

      LSMS.beginSMS(number);
      LSMS.print(buffer);

      if(LSMS.endSMS()) Serial.println("SMS sent ok!");
      else Serial.println("SMS send fail!");
  }
}

boolean isBatteryLow() {
  return LBattery.level() < 5;
}

int calculateNoiseLevel() {
  long sum = analogRead(SOUND_SENSOR);

  if(sum > NOISE_LIMIT) {
    return  10;
  } else {
    return -1;
  }
}

void loop()
{
    if(isRinging()) {
      answerCall();
    }

    if(hasSMS()) {
      sendSMS();
    }

    noiseLevel += calculateNoiseLevel();

    if (noiseLevel < 0) {
      noiseLevel = 0;
    }

    if(noiseLevel > MAX_NOISE && noiseCounter == 0) {
      sendNoiseAlert(noiseLevel, YOUR_NUMBER);
      sendNoiseAlert(noiseLevel, YOUR_SECOND_NUMBER);
      noiseLevel = 0;
      noiseCounter = MESSAGE_COUNTER_RESET
    }

    if (noiseCounter > 0) {
      noiseCounter--;
    }

    if (isBatteryLow()) {
      if (batteryCount == 0) {
        sendBatteryAlert(YOUR_NUMBER);
        batteryCount = MESSAGE_COUNTER_RESET;
      } else {
        batteryCount--;
      }
    }

    Serial.print("Noise");
    Serial.println(noiseLevel);
    delay(100);
}
