#include <Wire.h>

byte temp_c;  //Value of ambient temperature in Celcius
byte temp_f;  //Value of ambient temperature in Fahrenheit 
// int tempSensor_addr = 0x4c; //Alternative IC address; used for testing purposes
int tempSensor_addr = 0x48; //I2C Address for IC
int t_os_pointer = 0x03;  //Pointer Address for T_OS register
int temp_pointer = 0x00;  //Pointer Address for Temp register

int ledPin = 13;  //Used for debugging purposes

void setup() {
  Serial.begin(9600); //Initialize serial communication at 9600 baud
  Wire.begin(); //Initialize I2C communication with device
}

void loop() {
  String command = "";
  //Checks if the serial bus is available, and reads the incoming command (on average this will be the GET_TEMP command)
  if (Serial.available() > 0) {
    command = Serial.readStringUntil('\n');
    executeCommand(command);
  }
  delay(100);
}

//Function checks is the string is made up of digits, returns True if so
bool isDigit(String str) {
  for (size_t i = 0; i < str.length(); i++) {
    if (!isdigit(str.charAt(i))) {
      return false;
    }
  }
  return true;
}

//Function to communication with the LM75A; grabs the ambient temperature and sends via Serial bus to GUI
void GET_TEMP() {
  Wire.beginTransmission(tempSensor_addr);
  Wire.write(temp_pointer);
  Wire.endTransmission();

  int bytesReceived = Wire.requestFrom(tempSensor_addr, 2);
  if (bytesReceived == 2) {
    int data = Wire.read();
    Serial.println(data);
  } else {
    Serial.println("Error: ByteMismatch");
  }
}

//Function to communicate with the LM75A; reads incoming temperature to set and configures LM75A accordingly
void SET_TEMP() {
  bool ACK_condition = "False";
  String value = Serial.readStringUntil('\n');

  if (isDigit(value)) {
    ACK_condition = change_OS_temp(value.toInt());
  }

  //Verify if temperature was successfully sent and send ACK/NACK to GUI
  if (ACK_condition) {
    Serial.println("ACK");
  } else {
    Serial.println("NACK");
  }
}

//Helper function; Add additional functions the GUI will be calling here as needed
void executeCommand(String command) {
  command.trim();

  if (command == "GET_TEMP") {
    GET_TEMP();
  } else if (command = "SET_TEMP") {
    SET_TEMP();
  }
}

//Configures the LM75A OS temp to new value, returns True if successful
bool change_OS_temp(int custom_T_os) {
  if (custom_T_os < 0) {
    custom_T_os = 0;
  } else if (custom_T_os > 127) {
    custom_T_os = 127;
  }

  Wire.beginTransmission(tempSensor_addr);
  Wire.write(0x03);
  Wire.write(custom_T_os);
  Wire.write(0x00);
  int exitTicket = Wire.endTransmission();
  change_to_temp_reg(); //Switch back to temperature register

  if (exitTicket == 0) {
    return 'True';
  } else if (exitTicket > 0) {
    return 'False';
  }

}

//Configures the LM75A register to the default temperature register (used as a helper function)
void change_to_temp_reg() {
  Wire.beginTransmission(tempSensor_addr);
  Wire.write(temp_pointer);
  Wire.endTransmission();
}