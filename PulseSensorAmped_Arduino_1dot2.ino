#include <EEPROM.h>
#include <Time.h>

//  VARIABLES
int pulsePin = 0;                 // Pulse Sensor purple wire connected to analog pin 0
int blinkPin = 13;                // pin to blink led at each beat
int fadePin = 5;                  // pin to do fancy classy fading blink at each beat
int fadeRate = 0;                 // used to fade LED on with PWM on fadePin

int countQS = 0;
int countTime = 0;

int sum = 0;
int average = 0; //Trung binh nhip tim
int average_err = 0; //Trung binh do loi
const int beat_start_calc = 5; //Nhip bat dau tinh (bo qua nhung nhip dau de tranh nhieu)

//Real time on arduino:
int yy, mm, dd, hh, mi, ss;

int a[100];
int b[100];
int current_add_arr = 0;

const int ADD_COUNT_SET = 511; //Dia chi luu vung nho thoi gian max
int maxCount = 15;

char command[50];
int pos_command = 0;

int iEEPROM = 2;

// these variables are volatile because they are used during the interrupt service routine!
volatile int BPM;                   // used to hold the pulse rate
volatile int Signal;                // holds the incoming raw data
volatile int IBI = 600;             // holds the time between beats, must be seeded! 
volatile boolean Pulse = false;     // true when pulse wave is high, false when it's low
volatile boolean QS = false;        // becomes true when Arduoino finds a beat.


void setup(){
  pinMode(blinkPin,OUTPUT);         // pin that will blink to your heartbeat!
  pinMode(fadePin,OUTPUT);          // pin that will fade to your heartbeat!
  Serial.begin(9600);             // we agree to talk fast!
  interruptSetup();                 // sets up to read Pulse Sensor signal every 2mS 

  maxCount = EEPROM.read(0);
  iEEPROM = EEPROM.read(1);

  EEPROM.write(1,0);
}

void loop(){

  char x;

  if(Serial.available()){
    x = Serial.read();

    if(x == '\n'){      
      if(strstr(command, "sync") != NULL ){

        char strTemp[3];
        strTemp[0] = command[5];
        strTemp[1] = command[6];
        int hour = atoi(strTemp);

        strTemp[0] = command[8];
        strTemp[1] = command[9];
        int minutes = atoi(strTemp);

        strTemp[0] = command[11];
        strTemp[1] = command[12];
        int sec = atoi(strTemp);

        strTemp[0] = command[14];
        strTemp[1] = command[15];
        int day = atoi(strTemp);

        strTemp[0] = command[17];
        strTemp[1] = command[18];
        int month = atoi(strTemp);        

        strTemp[0] = command[20];
        strTemp[1] = command[21];
        int year = atoi(strTemp);

        setTime(hour, minutes, sec, day, month, year);

        Serial.print("sync");
        for(int i=2; i<iEEPROM; i++){
          Serial.print(EEPROM.read(i));
          Serial.print(" ");
        }

        pos_command = 0;
        memset(&command[0], 0, sizeof(command)); 
      }
      else if(strcmp(command, "clear") == 0){
        iEEPROM = 2;
        EEPROM.write(1, iEEPROM);

        Serial.print("clear");
        pos_command = 0;
        memset(&command[0], 0, sizeof(command));
      }
      else if(strstr(command, "times") != NULL){

        char strTimes[20];
        for(int i=6; i<pos_command; i++)
          strTimes[i-6] = command[i];

        maxCount = atoi(strTimes);

        if(maxCount > 5)
          EEPROM.write(0, maxCount);
        else
          EEPROM.write(0, 15);

        Serial.print("times ");
        Serial.print(maxCount);

        pos_command = 0;
        memset(&command[0], 0, sizeof(command));
      }
      else if(strstr(command, "settime") != NULL){
        char strTemp[3];
        strTemp[0] = command[8];
        strTemp[1] = command[9];
        int hour = atoi(strTemp);

        strTemp[0] = command[11];
        strTemp[1] = command[12];
        int minutes = atoi(strTemp);

        strTemp[0] = command[14];
        strTemp[1] = command[15];
        int sec = atoi(strTemp);

        strTemp[0] = command[17];
        strTemp[1] = command[18];
        int day = atoi(strTemp);

        strTemp[0] = command[20];
        strTemp[1] = command[21];
        int month = atoi(strTemp);        

        strTemp[0] = command[23];
        strTemp[1] = command[24];
        int year = atoi(strTemp);

        setTime(hour, minutes, sec, day, month, year);
        pos_command = 0;
        memset(&command[0], 0, sizeof(command));
      }
      else{
        Serial.println("command error");
        Serial.print(hour());
        Serial.print(" ");
        Serial.print(minute());
        Serial.print(" ");
        Serial.print(second());
        Serial.print(" ");
        Serial.print(day());
        Serial.print(" ");
        Serial.print(month());
        Serial.print(" ");
        Serial.print(year()-2000);
        Serial.print(" ");
        pos_command = 0;
        memset(&command[0], 0, sizeof(command));
      }
    }

    if(x != '\n'){
      command[pos_command] = x;
      pos_command++;
      //Serial.print(x);
    }
  }

  //Dem so nhip phan hoi tu sensor, neu QS = 0 > 70 lan thi ko co tin hieu
  if(QS == false)
    countQS++;
  else 
    countQS = 0;

  if(countQS > 50){ //Neu ko co phan hoi tu sensor trong 50 nhip thi reset do lai tu dau
    countTime = 0;
    sum = 0;
    current_add_arr = 0;
  }

  if (QS == true){                       // Quantified Self flag is true when arduino finds a heartbeat
    countTime++;
    a[countTime] = BPM;

    Serial.println(countTime);
    Serial.println(BPM);

    if(countTime >= maxCount){
      average = 0;
      average_err = 100;
      while(average_err > 3){

        int i = 0;

        average = 0;
        int count_temp = 0;
        for(i = beat_start_calc; i <= countTime; i++){
          if(a[i] != -1){
            average += a[i];
            count_temp++;
          }
        }

        average /= count_temp;

        average_err = 0;
        int count_err = 0;
        for(i = beat_start_calc; i <= countTime; i++){
          if(a[i] != -1){
            b[i] = abs(a[i] - average);
            average_err += b[i];
            count_err++;
          }
        }

        average_err = average_err / count_err;

        //update new array
        for(i = beat_start_calc; i <= countTime; i++)
          if(b[i] > average_err)
            a[i] = -1;


        if(count_err == 2)
          break;

      }

      //store result average
      Serial.print("average");
      Serial.println(average);

      if(year() >= 2014){
        EEPROM.write(iEEPROM, average);
        iEEPROM++;
        EEPROM.write(1,iEEPROM);

        //EEPROM time, date:
        EEPROM.write(iEEPROM, hour());
        iEEPROM++;
        EEPROM.write(iEEPROM, minute());
        iEEPROM++;
        EEPROM.write(iEEPROM, second());
        iEEPROM++;
        EEPROM.write(iEEPROM, day());
        iEEPROM++;
        EEPROM.write(iEEPROM, month());
        iEEPROM++;
        EEPROM.write(iEEPROM, year()-2000);
        iEEPROM++;

        EEPROM.write(1,iEEPROM);
      }
      countTime = 0;
    }

    QS = false;                      // reset the Quantified Self flag for next time    
  }

  delay(20);                             //  take a break
}









