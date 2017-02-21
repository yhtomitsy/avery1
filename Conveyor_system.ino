// debug messages
// set as 1 so as to receive debug messages
// set as 0 to not receive debug messages
#define DEBUG 1

// maximum number of palletes in queue
#define QUEUE_MAX 10

// estimated pallete weight threshold
// least weight of pallete set to 0 kg
#define WEIGHT_THRESH 0

// pallete detected when sensor analog reading is 400
// can be adjusted when calibrating system
#define DETECTION_THRESHOLD 300

//IR sensor pins
// IR1 & IR2 are on first weighing platform
// IR3, IR4 & IR5 are on the second weighing platform
#define IR1_PIN 0
#define IR2_PIN 1
#define IR3_PIN 2
#define IR4_PIN 3
#define IR5_PIN 4

// Relay pins
#define RELAY1 6
#define RELAY2 7

//LED pins
#define LED1_PIN 8
#define LED2_PIN 9
#define LED3_PIN 10

// ENQ character
// should be followed by a <CR> character
#define ENQ 5

// decimal places
// how many decimal places to read
#define DP 1

//String for holding the incoming data
String incomingData = "";

// weight measurement data
float emptyPallete[10] = {0};
float fullPallete[10] = {0};
float weightDifference[10] = {0};

//flags
boolean emptyReadingTaken = false;
boolean fullReadingTaken = false;
boolean jam = false;

// measurement index
int emptyIndex = 0;
int fullIndex = 0;

void setup() {
  // initialize serial communication.
  Serial.begin(9600);
  Serial1.begin(9600);
  Serial2.begin(9600);
  Serial3.begin(9600);

  // configure pins
  pinMode(RELAY1, OUTPUT);
  pinMode(RELAY2, OUTPUT);
  pinMode(LED1_PIN, OUTPUT);
  pinMode(LED2_PIN, OUTPUT);
  pinMode(LED3_PIN, OUTPUT);

  // intialize pin states
  digitalWrite(RELAY1, LOW);
  digitalWrite(RELAY2, LOW);
  digitalWrite(LED1_PIN, LOW);
  digitalWrite(LED2_PIN, LOW);
  digitalWrite(LED3_PIN, LOW);
}

void loop() {
  if(palleteOnPlatform(IR1_PIN, IR2_PIN) && !jam){
    digitalWrite(LED1_PIN, HIGH); // glow LED 1
    if(DEBUG)Serial.println("Platform 1 sensors triggered");
    if(!emptyReadingTaken)requestWeight(1); // ask for weight data from indicator 1
    if(Serial1.available()&& !emptyReadingTaken){ // measurement available and not previously taken
      incomingData = Serial1.readString(); //  read incoming data
      
      float x = stringToFloat(incomingData); 
      if(x >= WEIGHT_THRESH){ // positive weight
        emptyPallete[emptyIndex] = x;
        fullPallete[emptyIndex] = 0;
        // keep track of weights of empty palletes
        if(emptyIndex >= (QUEUE_MAX - 1))emptyIndex = 0;
        else emptyIndex++; 
        if(emptyIndex == fullIndex){
          digitalWrite(LED3_PIN, HIGH);
          jam = true; // items have jammed the conveyor line
        }
        emptyReadingTaken = true; // prevent same reading from being saved twice
      }

      if(DEBUG){
        Serial.println("Reading Indicator 1");
        Serial.println(incomingData);
        displayMeasurements(); // display the measurements
      }
      delay(1000);
    }
  }
  else if(palleteOnPlatform(IR3_PIN, IR4_PIN)){ // pallete on platform 2
    digitalWrite(LED2_PIN, HIGH); // glow LED 1
    if(DEBUG)Serial.println("platform 2 sensors triggered");
    if(analogRead(IR5_PIN) > DETECTION_THRESHOLD){ // pallete loaded
      if(!fullReadingTaken)requestWeight(2); // ask for weight data from indicator 2
      if(Serial2.available() && !fullReadingTaken){ // data available on serial 2
        incomingData = Serial2.readString();
        
        fullPallete[fullIndex] = stringToFloat(incomingData); // get weight of loaded pallete
        weightDifference[fullIndex] = fullPallete[fullIndex] - emptyPallete[fullIndex]; // get the weight difference
        printerOutput(fullIndex); //  print weight difference
        
        // keep track of weights of loaded palletes
        if(fullIndex >= (QUEUE_MAX - 1))fullIndex = 0;
        else fullIndex++; 
        fullReadingTaken = true; // prevent same reading from being saved twice
                      
        if(DEBUG){
          Serial.println("reading Indicator 2");
          Serial.println(incomingData);
          displayMeasurements(); // display the measurements
        }
        delay(1000); 
      }
    }
    else{ // pallete was not loaded
      if (!fullReadingTaken){
        emptyPallete[fullIndex] = 0;
        if(fullIndex >= (QUEUE_MAX - 1))fullIndex = 0;
        else fullIndex++; // keep track of weights of loaded palletes
        displayMeasurements(); // display the measurements
        fullReadingTaken = true; // prevent same reading from being saved twice
        delay(1000);
      }
    }
    jam = false;
  }
  clearFlags(); // clear the flags when pallete leaves weighing scale
  delay(100);
}

/*
 * save weight from incoming data from indicator 1
 */
void saveWeight1(){
  
}

/* 
 *  clear flags when pallete leaves scale
 */
void clearFlags(){
  if(analogRead(IR1_PIN) < DETECTION_THRESHOLD && analogRead(IR2_PIN) < DETECTION_THRESHOLD && emptyReadingTaken){
    emptyReadingTaken = false;
    digitalWrite(LED1_PIN, LOW); // turn off LED 1
  }
  if(analogRead(IR3_PIN) < DETECTION_THRESHOLD && analogRead(IR4_PIN) < DETECTION_THRESHOLD && analogRead(IR5_PIN) < DETECTION_THRESHOLD && fullReadingTaken){
    fullReadingTaken = false;
    digitalWrite(LED2_PIN, LOW); // turn off LED 2
  }
  if(!jam) digitalWrite(LED3_PIN, LOW); // glow LED 1
}

/*
 * check status of IR sensors to determine if there is a pallete on the scale
 */
boolean palleteOnPlatform(int IR1, int IR2){
  if(analogRead(IR1) > DETECTION_THRESHOLD && analogRead(IR2) > DETECTION_THRESHOLD)return true;
  else return false;
}

/*
 * convert string to float
 */
float stringToFloat(String s){
  // prepare string by trimming off the unwanted characters
  s = s.substring(s.indexOf("Net") + 3, s.indexOf('.', s.indexOf("Net"))+(DP+1));
  s.trim(); // remove spaces
  
  String whole = ""; // holds whole part
  String decimal = ""; // holds decimal part
  float f = 0; // will hold the number
  
  if(s.indexOf('.')!= -1){
    whole = s.substring(0,s.indexOf('.')); // get whole number
    decimal = s.substring(s.indexOf('.')+ 1,s.indexOf('\n')); // get decimal
    f = stringtoInt(whole); // assign the whole part of the number to the float variable
    f += float(stringtoInt(decimal)) / float(pow(10,decimal.length())); // get decimal part
  }
  else{
    f = stringtoInt(s); // assign the whole part of the number to the float variable
  }
  return f;
}

/*
 * convert from string to int
 */
int stringtoInt(String buff){
  int r = 0;
  for (int i = 0; i < buff.length(); i++){
   r = (r*10) + (buff[i] - '0');
  }
  return r;
}
 /*
  * display measurements
  */
 void displayMeasurements(){
  Serial.print("Empty");
  Serial.print("\t\t");
  Serial.print("Loaded");
  Serial.print("\t\t");
  Serial.print("Difference");
  Serial.println();
  for(uint8_t i = 0; i < QUEUE_MAX; i++){
    Serial.print(emptyPallete[i]);
    Serial.print("\t\t");
    Serial.print(fullPallete[i]);
    Serial.print("\t\t");
    Serial.print(weightDifference[i]);
    Serial.println();
  }
 }

 /*
  * trigger the indicator to send weight data
  */
void requestWeight(int platform){
  switch (platform){
    case 1: // read trigger indicator 1
      Serial1.write(0x05);
      Serial1.write(0x0D);
      break;
    case 2: // trigger indicator 2
      Serial2.write(0x05);
      Serial2.write(0x0D);
      break;
  }
  delay(10); // short delay
}

/*
 * send difference to printer
 */
void printerOutput(int i){
  Serial.print(weightDifference[i]);
}

