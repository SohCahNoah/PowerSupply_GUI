/*
Program Title:        LM75A Temperature Sensor w/ GUI
Program Description:  This code is meant to be used with a custom python file that creates a tkinter window which
                      displays the data sent from this module to the user. The GUI program uses the serial bus to
                      communicate back and forth with this program, and the Arduino uses I2C communication to get
                      data from and configure the LM75A sensor. This code also communicates with an LCD in order to
                      display system information. Any bugs found should be sent to the author below.
Author(s):            Noah Roberts, SID: 932-989-402, robertno@oregonstate.edu
                      Shuyi Zheng, zhengshu@oregonstate.edu
Date:                 05/09/2024
Version:              Version 2.1
*/

#include <Wire.h>
#include <LiquidCrystal.h>

//-----LCD Display Constants-----//
const int rs = 12, en = 11, d4 = 5, d5 = 4, d6 = 3, d7 = 2;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);

//-----LM75A Constants-----//
byte temp_c;  //Value of ambient temperature in Celcius
byte temp_f;  //Value of ambient temperature in Fahrenheit 
byte temp_OS; //Value of the T_OS value
int tempSensor_addr = 0x4c; //Alternative IC address; used for testing purposes
//int tempSensor_addr = 0x48; //I2C Address for IC
int t_os_pointer = 0x03;  //Pointer Address for T_OS register
int temp_pointer = 0x00;  //Pointer Address for Temp register

int ledPin = 13;  //Used for debugging purposes

void setup() {
  Serial.begin(9600); //Initialize serial communication at 9600 baud
  Wire.begin(); //Initialize I2C communication with device
  setupLCD(); //Initialize LCD display
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
    temp_c = data;
    lcd_setAmbTemp(temp_c);
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
  lcd_setOSTemp(custom_T_os);

  if (exitTicket == 0) {
    temp_OS = custom_T_os;
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

void setupLCD() {
  //Setup the number of columns and rows on the LCD and initialize with blank values
  lcd.begin(16, 2);
  lcd.setCursor(0,0);
  lcd.print("Amb Temp: ---C");
  lcd.setCursor(0,1);
  lcd.print("Temp_OS: ---C");
}

void lcd_setAmbTemp(byte temp_c) {
  if(temp_c > 99 || temp_c < 10) {
    lcd.setCursor(10, 0);
    lcd.print("---");
  }
  lcd.setCursor(10, 0);
  lcd.print(temp_c);
}

void lcd_setOSTemp(byte temp_OS) {
  lcd.setCursor(9, 1);
  lcd.print("---");
  lcd.setCursor(9, 1);
  lcd.print(temp_OS);
}