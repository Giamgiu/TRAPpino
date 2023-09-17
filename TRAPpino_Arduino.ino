#include <EloquentTinyML.h>
#include <eloquent_tinyml/tensorflow.h>
#include "novanta.h" //File of the tensorflow model, quantized for int8_t
#define N_INPUTS 4096  //Number of the input (for a 64x64 image)
#define N_OUTPUTS 6    //Number of output or # of classes
// Define the size of the array
const int arraySize = 4096;
boolean newdata = false;
boolean MLready = false;

// Initialize the array to store received integers
int8_t  x_test[4096] = {};

// Allocate tensor memory
#define TENSOR_ARENA_SIZE 80*1024

//Config tensorflow library
Eloquent::TinyML::TensorFlow::TensorFlow<N_INPUTS, N_OUTPUTS, TENSOR_ARENA_SIZE> tf;



void setup() {
  //Set serials to comunication to monitor and to receive array
  Serial.begin(115200);
  Serial1.begin(9600);
  delay(4000);
  pinMode(LED_BUILTIN, OUTPUT);

  //Initialize model for inference
  tf.begin(novanta);
  Serial.println("Ready");

  // check if model loaded fine
  if (!tf.isOk()) {
    Serial.print("ERROR: ");
    Serial.println(tf.getErrorMessage());

    while (true) delay(1000);
  }
}



void loop() {

  //Listening for the input array
  receive();

  //Printing the filled array in serial monitor
  writeit();

  //If array is filled starting the classifier
  if (MLready) {
    classify();
  }




}

void receive() {
  //Listening for the input array


  String rec;
  char comma = ',';



  while (Serial1.available() && newdata == false) {
    Serial.println("Receiving the image...");

    for (int i = 0; i < arraySize; i++) {
      rec = Serial1.readStringUntil(comma);
      x_test[i] = (rec.toInt()) - 128;   //The string is converted to an int and then the int8_t is set
    }
    newdata = true;




  }
}
void writeit() {
  //Printing the filled array in serial monitor

  if (newdata == true) {
    Serial.println();
    for (int i = 0; i < arraySize; i++) {
      Serial.print(x_test[i]);
      Serial.print(',');
      MLready = true;
    }
    Serial.println();
  }

}

void classify() {
  //Start the classifier

  int8_t  y_pred[6] = {0};
  int8_t reclass = 0;

  uint32_t start = micros();
  tf.predict(x_test, y_pred);
  uint32_t timeit = (micros() - start) / 1000;
  Serial.print("It took ");
  Serial.print(timeit);
  Serial.println(" ms to run inference");
  Serial.print("Predicted probabilities are: ");

  for (int i = 0; i < 6; i++) {
    Serial.print(y_pred[i]);
    Serial.print(i == 5 ? '\n' : ',');
  }

  Serial.print("Predicted class is: ");



  reclass = tf.predictClass(x_test);

  Serial.print(reclass + 1);

  switch (reclass) {
    case 0: Serial.print(" (Badger)");
      break;
    case 1: Serial.print(" (Boar)");
      break;
    case 2: Serial.print(" (Deer)");
      break;
    case 3: Serial.print(" (Fox)");
      break;
    case 4: Serial.print(" (Horse)");
      break;
    case 5: Serial.print(" (Woodpigeon)");
      break;
  }
  Serial.print('\n');
  memset(x_test, 0, 4096);//Reset memory allocated for x_test
  MLready = false;
  newdata = false;
  delay(1000);

}
