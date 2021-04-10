#define DEBUG false

//OUTPUT
const int ledPins[] = {5,6,7,8,9,10,11,12,13};
const int ledsNo = 9;
bool ledStates[] = {false, false, false, false, false, false, false, false, false};

////testing
//const int ledPins[] = {10,11,12,13}
//const boolean ledStates[] = {false, false, false, false};
//const int ledsNo = 4;

int phase = 0; //0-idle, 
               //1-turning on from bottom to top, 
               //2-turning off from bottom to top, 
               //3-turning on from top to bottom, 
               //4-turning off from top to bottom
int switchingTime = 0; //current step time when switching lights on/off
const int switchingInterval = 500; //interval between lights
int nextLedToSwitch = 0;

//INPUT
//int lightTimePin = A0 //potentiometer
const int pirBottomPin = 2;
const int pirTopPin = 3;
const int darknessSensorPin = 4;

//Light time
int lightTime = 0;
int lightTimeToler = 2; //tolerance of reading light time potentiometer
int potentiometerValCurr = 0;
int potentiometerValPrev = 0;

//PIR sensors
int calibrationTime = 3; //the time we give the sensor to calibrate (10-60 secs according to the datasheet)[s]

int cycleTime = 5; //time of one loop iteration [ms]
int workTime = 0; //current time of working of light [ms]
bool duringWork = false; //flag if the light is shining

int evalLightTime(int potentiometerValue) {
  float potentiometerValueFloat = potentiometerValue;
  potentiometerValueFloat = (potentiometerValueFloat / 1023) * 300;
  int lightTime = potentiometerValueFloat;
  return lightTime;
}

void setup() {
  //init serial port
  Serial.begin(9600);

  if (DEBUG) Serial.println("Trwa inicjowanie...");

  //Led reset
  for (int i = 0; i < ledsNo; i++) {
    pinMode(ledPins[i], OUTPUT);
    digitalWrite(ledPins[i], LOW);
  }
  if (DEBUG) Serial.println("Reset LED");

  //Read light time
  for (int i=0; i<100; i++) {
    potentiometerValCurr = analogRead(A0);
    delay(1);
  }
  potentiometerValPrev = potentiometerValCurr;
  lightTime = evalLightTime(potentiometerValCurr);
  printLightTime();

  if (DEBUG) Serial.print("Czas cyklu: ");
  if (DEBUG) Serial.print(cycleTime);
  if (DEBUG) Serial.println("ms");

  //Init PIR sensors
  pinMode(pirBottomPin, INPUT_PULLUP);
  pinMode(pirTopPin, INPUT_PULLUP);
  pinMode(darknessSensorPin, INPUT);
  
  if (DEBUG) Serial.print("kalibrowanie czujników ");
  for(int i = 0; i < calibrationTime; i++){
    if (DEBUG) Serial.print(".");
    delay(1000);
  }
  if (DEBUG) Serial.print(" ukończono w czasie ");
  if (DEBUG) Serial.print(calibrationTime);
  if (DEBUG) Serial.println("s");
  digitalWrite(pirBottomPin, LOW);
  digitalWrite(pirTopPin, LOW);
  digitalWrite(darknessSensorPin, LOW);
  if (DEBUG) Serial.println("PIR DÓŁ AKTYWNY");
  if (DEBUG) Serial.println("PIR GÓRA AKTYWNY");
  if (DEBUG) Serial.println("CZUJNIK ZMIERZCHU AKTYWNY");
  delay(50);
}

void loop() {
  handleLightTimeChanged();

  //PIR sensors
  int pirBottomState = digitalRead(pirBottomPin); //read only once per cycle
  int pirTopState = digitalRead(pirTopPin);
  int darknessSensorState = digitalRead(darknessSensorPin);

  printSensorsStates(pirBottomState, pirTopState, darknessSensorState);

  if (darknessSensorState == HIGH) {
    phase = 0; //idle
  }
  else {
    if ( (pirBottomState == HIGH) || (pirTopState == HIGH) ) {
      workTime = lightTime * 1000;
      duringWork = true;
      if (pirBottomState == HIGH) {
        if (phase == 0) {
          switchingTime = 0;
          nextLedToSwitch = 0;
          phase = 1; //turning on from bottom to top
          if (DEBUG) Serial.println("Faza=1 zapalanie z dołu na górę");
        }
      }
      if (pirTopState == HIGH) {
        if (phase == 0) {
          switchingTime = 0;
          nextLedToSwitch = ledsNo - 1;
          phase = 3; //turning on from top to bottom
          if (DEBUG) Serial.println("Faza=3 zapalanie z góry na dół");
        }
      }
      if (phase == 2) {
        switchingTime = 0;
        nextLedToSwitch = 0;
        phase = 1;
        for (int i = 0; i < ledsNo; i++) ledStates[i] = true;
      }
      if (phase == 4) {
        switchingTime = 0;
        nextLedToSwitch = ledsNo - 1;
        phase = 3;
        for (int i = 0; i < ledsNo; i++) ledStates[i] = true;
      }
    }
    if( (pirBottomState == LOW) && (pirTopState == LOW) && duringWork ){
      workTime -= cycleTime;
      if (workTime < 0) workTime = 0;
      if (workTime % 1000 == 0 && workTime != 0) if (DEBUG) Serial.println("Pozostało: " + String(workTime/1000) + "s");
    }
    if ( (pirBottomState == LOW) && (pirTopState == LOW) && (workTime <= 50) && duringWork ) {
      if (phase == 1) {
        if (DEBUG) Serial.println("Faza=2 gaszenie z dołu na górę");
        switchingTime = 0;
        nextLedToSwitch = 0;
        phase = 2;
      }
      if (phase == 3) {
        if (DEBUG) Serial.println("Faza=4 gaszenie z góry na dół");
        switchingTime = 0;
        nextLedToSwitch = ledsNo - 1;
        phase = 4;
      }
    }
  }

  handlePhase();
  handleLights();

  delay(cycleTime);
}

void handlePhase() {
  if (phase == 0) {
    for (int i = 0; i < ledsNo; i++ ) ledStates[i] = false;
    duringWork = false;
  }
  else if (phase == 1) {
    duringWork = true;
    switchingTime += cycleTime;
    if ( (switchingTime > (nextLedToSwitch * switchingInterval)) && (nextLedToSwitch < ledsNo ) ) {
      if (DEBUG) Serial.println("Zapalanie " + String(nextLedToSwitch));
      ledStates[nextLedToSwitch++] = true;
    }
  }
  else if (phase == 2) {
    duringWork = true;
    switchingTime += cycleTime;
    if ( (switchingTime > (nextLedToSwitch * switchingInterval)) && (nextLedToSwitch < ledsNo ) ) {
      if (DEBUG) Serial.println("Gaszenie " + String(nextLedToSwitch));
      ledStates[nextLedToSwitch++] = false;
    }
    if (nextLedToSwitch == ledsNo) {
      duringWork = false;
      phase = 0;
      if (DEBUG) Serial.println("Faza=0 czuwanie");
    }
  }
  else if (phase == 3) {
    duringWork = true;
    switchingTime += cycleTime;
    if ( (switchingTime > ((ledsNo - nextLedToSwitch - 1) * switchingInterval)) && (nextLedToSwitch > -1) ) {
      if (DEBUG) Serial.println("Zapalanie " + String(nextLedToSwitch));
      ledStates[nextLedToSwitch--] = true;
    }
  }
  else if (phase == 4) {
    duringWork = true;
    switchingTime += cycleTime;
    if ( (switchingTime > ((ledsNo - nextLedToSwitch - 1) * switchingInterval)) && (nextLedToSwitch > -1) ) {
      if (DEBUG) Serial.println("Gaszenie " + String(nextLedToSwitch));
      ledStates[nextLedToSwitch--] = false;
    }
    if (nextLedToSwitch == -1) {
      duringWork = false;
      phase = 0;
      if (DEBUG) Serial.println("Faza=0 czwanie");
    }
  }
}

void handleLightTimeChanged() {
  potentiometerValCurr = analogRead(A0);
  if ( (potentiometerValCurr < (potentiometerValPrev-lightTimeToler) ) || 
    (potentiometerValCurr > (potentiometerValPrev+lightTimeToler) ) ) {
    potentiometerValPrev = potentiometerValCurr;
    lightTime = evalLightTime(potentiometerValCurr);
    printLightTime();
  }
}

void printLightTime() {
    if (DEBUG) Serial.print("                                              Czas świecenia: ");
    if (DEBUG) Serial.print(lightTime);
    if (DEBUG) Serial.println("s");
}

//switch light on/off according to its state
void handleLights() {
  for (int i = 0; i < ledsNo; i++) {
    if (ledStates[i]) {
      digitalWrite(ledPins[i], LOW); //przekazniki wyzwalane stanem niskim, a nie wysokim
    }
    else {
      digitalWrite(ledPins[i], HIGH);
    } 
  }
}

void printSensorsStates(int pirBottomState, int pirTopState, int darknessSensorState) {
  if (DEBUG) {
    const int toler = 5;
    const int interval = 1000;
    short matchingInterval = millis() % interval;
    if (matchingInterval < toler && matchingInterval > -toler){
      Serial.println("            PIR DÓŁ [ "+String(pirBottomState)+" ] PIR GÓRA [ "+String(pirTopState)+" ] ZMIERZCH [ "+String(darknessSensorState)+" ]");
    }
  }
}

void printWorkTime(int workTime) {
  if (DEBUG) {
    const int toler = 5;
    const int interval = 1000;
    short matchingInterval = millis() % interval;
    if (matchingInterval < toler && matchingInterval > -toler){
      Serial.println("Pozostało: " + String(workTime/1000) + "s");    
    }
  }
}
