#define PI 3.14159
int i = 0;
int startTime;
float omega1 = PI;
float omega2 = 4*PI;
float alpha1 = 5.0;
float alpha2 = 1.25;

String startChar = "~";
String endChar = "~";

void setup(void) {
  Serial.begin(115200);
  startTime = millis();
}

void loop(void) {
  float e = (millis() - startTime) / 1000.0;
  if(Serial.available()){
    Serial.read();
    i = 0;
    e = 0.0;
  }
  
  // generate example data
  float data1 = alpha1 * sin(omega1 * e);
  float data2 = data1 + alpha2 * sin(omega2 * e);
  float data3 = random(10);

  // create data string by sending data in the format "Time:Val,Name1:Val1,..." 
  // Be sure to bookend your data with startChar and endChar
  // remember double quotes "" creates a string, don't use single quotes ''
  String data = ""; // empty string

  // concatenate strings together using +
  data += startChar; // start with startChar

  // you can concatenate your"Name:Val," separately like this...
  data += "Time(ms):"; // always send your time first!
  data += String(millis()); // use String() to convert numbers to a string
  data += ",";

  // or all at once, like this
  data += "Data1:" + String(data1) + ",";
  data += "Data2:" + String(data2); //no comma for last data value

  // not all data has to be sent every single time
  if(i%10==0){
    data += ",Data3:" + String(data3); //no comma for last data value
  }

 //new data can be sent later
  if (e > 5.0)
  {
    data += ",Data4:" + String(random(25));
  }

  data += endChar; // end with endChar

  // send serial messages including data
  Serial.println("This line will not be plotted, only logged");
  Serial.print("This line will contain data, preceded by the startChar and ended with the endChar: ");
  Serial.println(data);
  
  i++;
  delay(25);
  
}
