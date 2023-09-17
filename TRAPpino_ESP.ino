/**********************************************************************
  Use onboard button to shoot the photo, downsize to grayscale 64x64 and send it over UART on pin #20.

**********************************************************************/
#include "esp_camera.h"
#define CAMERA_MODEL_ESP32S3_EYE
#include "camera_pins.h"
#include "ws2812.h"
#include "sd_read_write.h"
#include "math.h"
#include <HardwareSerial.h>

#define BUTTON_PIN  0



//Configure serial UART2 communication
HardwareSerial channel(2); // use UART2

//Configure target image size (desired input for the CNN model)
const int targetSize = 64;
uint8_t downsized [targetSize * targetSize] = {};

void setup() {

  //Configure serial monitor and serial for communicate to arduino nano connect
  Serial.begin(115200);
  channel.begin(9600, SERIAL_8N1, 19, 20);




  //Initialize setting for button and led color
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  ws2812Init();

  //Initialize SD card, to save images at full resolution
  sdmmcInit();
  createDir(SD_MMC, "/camera");
  listDir(SD_MMC, "/camera", 0);

  if (cameraSetup() == 1) {
    ws2812SetColor(2);
  }
  else {
    ws2812SetColor(1);
    return;
  }
}

void loop() {
  //Listening for pressed button, entering the loop if is pressed for >20 ms
  if (digitalRead(BUTTON_PIN) == LOW) {
    delay(20);
    if (digitalRead(BUTTON_PIN) == LOW) {
      ws2812SetColor(3);
      while (digitalRead(BUTTON_PIN) == LOW);
      //Emptying camera buffers
      camera_fb_t * fb = NULL;
      //Store the image buffers in fb
      fb = esp_camera_fb_get();


      if (fb != NULL) {
        int photo_index = readFileNum(SD_MMC, "/camera");
        if (photo_index != -1)
        {
          String path = "/camera/" + String(photo_index) + ".jpg";
          writejpg(SD_MMC, path.c_str(), fb->buf, fb->len);
        }
        // Print on serial monitor the width, height, the original dimension and the target dimension of image

        int origWidth = fb->width;
        int origHeight = fb->height;
        Serial.println ("Original dimensions of image: ");
        Serial.print (origWidth);
        Serial.write (',');
        Serial.print (origHeight);
        Serial.println ("Original dimensions of array: ");
        Serial.println (fb->len);
        Serial.println ("Target dimensions of image: ");
        Serial.print (targetSize);

        int blockWidth = origWidth / targetSize; //Block width and height to calculate gaussian mean
        int blockHeight = origHeight / targetSize;
        if (((origWidth % targetSize ) != 0) || ((origHeight % targetSize) != 0)) {
          Serial.println("The original frame of image is not a multiple of targetSize image"); //This error will lead to a semi-empty array
        }
        Serial.print(blockWidth);
        Serial.write(',');
        Serial.println(blockHeight);
        int kernel[5][5] = {
          {1, 4, 7, 4, 1},
          {4, 20, 33, 20, 4},
          {7, 33, 55, 33, 7},
          {4, 20, 33, 20, 4},
          {1, 4, 7, 4, 1}
        };
        for (int y = 0; y < targetSize; y++) { //For every row of the final image...
          for (int x = 0; x < targetSize; x++) {//For every column of the final image...
            int sum = 0;
            int meanValue = 0;

            for (int blockY = 0; blockY < blockHeight; blockY++) {
              for (int blockX = 0; blockX < blockWidth; blockX++) {
                int px = x * blockWidth + blockX;
                int py = y * blockHeight + blockY;

                // Calculate the index in the original matrix
                int originalIndex = py * origWidth + px;

                // Apply the Gaussian kernel to the pixel
                int kx = (5 * (blockX / blockWidth)); //Nearest value of Gaussian filter for X
                int ky = (5 * (blockY / blockHeight));
                sum += fb->buf[originalIndex] * kernel[ky][kx];

              }
            }

            // Calculate the mean value of the block, divided by the sum of the kernel filter
            int gaussum = 331;
            meanValue = round(sum / gaussum);
            //Serial.print((sum / 331));


            // Calculate the index in the downsized matrix
            int targetIndex = y * targetSize + x;

            // Set the mean value in the downsized matrix
            downsized[targetIndex] = meanValue;

            //Serial.print(downsized[targetIndex]);
            //Serial.print(',');



          }
        }

        esp_camera_fb_return(fb); //Emptying the fb camera buffers

        sendArray();              //Call fnuction to send downsized array to arduino


      }
      else {
        Serial.println("Camera capture failed.");
      }
      ws2812SetColor(2);
    }
  }
}
//Default setting to set the camera pins for ESP32S3
int cameraSetup(void) {
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.frame_size = FRAMESIZE_SXGA;   //1280x1024
  config.pixel_format =  PIXFORMAT_GRAYSCALE;
  config.grab_mode = CAMERA_GRAB_WHEN_EMPTY;
  config.fb_location = CAMERA_FB_IN_PSRAM;
  config.jpeg_quality = 12;
  config.fb_count = 1;

  // if PSRAM IC present, init with QVGA resolution and higher JPEG quality
  // for larger pre-allocated frame buffer.
  if (psramFound()) {
    config.jpeg_quality = 10;
    config.fb_count = 2;
    config.grab_mode = CAMERA_GRAB_LATEST;
  } else {
    // Limit the frame size when PSRAM is not available
    config.frame_size = FRAMESIZE_QVGA;
    config.fb_location = CAMERA_FB_IN_DRAM;
  }

  // camera init
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    return 0;
  }

  sensor_t * s = esp_camera_sensor_get();
  // initial sensors are flipped vertically and colors are a bit saturated
  s->set_vflip(s, 1); // flip it back
  s->set_brightness(s, 1); // up the brightness just a bit
  s->set_saturation(s, 0); // lower the saturation

  Serial.println("Camera configuration complete!");
  return 1;
}

void sendArray () {

  //Serial.print(single);

  for (int i = 0; i < (targetSize * targetSize); i++) {
    channel.print(String(downsized[i]));
    channel.print(',');
  }



  delay(1000);



}
