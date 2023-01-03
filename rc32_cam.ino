#include "esp_camera.h"
#include <WiFi.h>
#include "soc/soc.h"             //disable brownout problems
#include "soc/rtc_cntl_reg.h"    //disable brownout problems
#include "SPIFFS.h"
#include <AsyncTCP.h>
#include <Servo.h>
#include <ESPAsyncWebServer.h>

#define PIN_LED 33
#define PIN_LED_FLASH 4 

#define PIN_PWM1 14
#define PIN_PWM2 15
#define PWM_RESOLUTION 8
#define PWM_FREQ 10000
#define PIN_SERVO 12

#define CAMERA_MODEL_AI_THINKER

#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27

#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22

// Current time
unsigned long currentTime = millis();
unsigned long tm_cam = 0;

// AsyncWebServer on port 80
AsyncWebServer server(80);

// Websocket
AsyncWebSocket ws("/ws");

int16_t motor_ctl;
int16_t motor_act;
uint8_t motor_out1;
uint8_t motor_out2;
unsigned long motor_tm;

const int pwmChannel1 = 14;
const int pwmChannel2 = 15;

Servo myservo; 
int16_t servo_ctl;
int16_t servo_act;
unsigned long servo_tm;

uint8_t led_state;

void setup() {
   //disable brownout detector
   WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);
 
   // variables
   led_state = 0;
   
   motor_ctl = 0;
   motor_act = 0;
   motor_out1 = 255;
   motor_out2 = 255;
   motor_tm = 0;

   servo_ctl = 0;
   servo_act = 0;
   servo_tm = 0;

   // servo
   myservo.attach(PIN_SERVO);
   myservo.writeMicroseconds(1500);

   // configure LED PWM functionalitites
   ledcSetup(pwmChannel1, PWM_FREQ, PWM_RESOLUTION);
   ledcSetup(pwmChannel2, PWM_FREQ, PWM_RESOLUTION);

   // attach the channel to the GPIO to be controlled
   ledcAttachPin(PIN_PWM2, pwmChannel1);
   ledcAttachPin(PIN_PWM1, pwmChannel2);

   ledcWrite(pwmChannel1, motor_out1);
   ledcWrite(pwmChannel2, motor_out2);   

   // prenastaveni pwm pro motor
   setOutputMotor();

   // pin mode
   pinMode(PIN_LED, OUTPUT);
   pinMode(PIN_LED_FLASH, OUTPUT);

   // mala cervena sviti pri low
   digitalWrite(PIN_LED, LOW);

   // velka bila sviti pri high, otazka jestli muze svitit dele, dost hreje
   setFlashOff();

   // seriak
   Serial.begin(115200);
   Serial.setDebugOutput(false);
  
   // msg
   Serial.println("- RC32-CAM v0.3 --------------------------------");

   // SPIFFS
   if(!SPIFFS.begin(true)){
      Serial.println("SPIFFS - Error has occurred while mounting");
   } else {
      Serial.println("SPIFFS OK");
   }

   // camera config
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
   config.pixel_format = PIXFORMAT_JPEG; 
  
   if(psramFound()){
      //config.frame_size = FRAMESIZE_SVGA;
      //config.frame_size = FRAMESIZE_VGA;
      config.frame_size = FRAMESIZE_HVGA;
      config.jpeg_quality = 20;
      config.fb_count = 2;
   } else {
      config.frame_size = FRAMESIZE_SVGA;
      config.jpeg_quality = 12;
      config.fb_count = 1;
   }
  
   // Camera init
   esp_err_t err = esp_camera_init(&config);
   if (err != ESP_OK) {
      Serial.printf("Camera init failed with error 0x%x", err);
      Serial.println();
   } else {
      Serial.println("Camera init OK");
   }

   // wifi AP
   WiFi.softAP("Mikul");
   IPAddress IP = WiFi.softAPIP();
   Serial.print("AP IP address: ");
   Serial.println(IP);

   // Wifi STA
   /*
   WiFi.begin("", "");
   Serial.print("Connecting to WiFi");
   while (WiFi.status() != WL_CONNECTED) {
      Serial.print('.');
      delay(1000);
   }
   Serial.println(".");
   */

   // Print ESP Local IP Address
   Serial.println(WiFi.localIP());

   // WS
   initWebSocket();

   // http server
   server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
      sendFSFile(request, "/index.html", "text/html");
   });
   server.on("/script.js", HTTP_GET, [](AsyncWebServerRequest *request){
      sendFSFile(request, "/script.js", "text/javascript");
   });
   server.on("/style.css", HTTP_GET, [](AsyncWebServerRequest *request){
      sendFSFile(request, "/style.css", "text/css");
   });

   // Start server
   server.begin();
}

/**
 * Main loop 
 */
void loop() {
   ws.cleanupClients();

   currentTime = millis();

   // servo
   if(currentTime % 2 == 0 && currentTime != servo_tm){
      if(servo_act > servo_ctl){
         servo_act--;
      }
      if(servo_act < servo_ctl){
         servo_act++;
      }
      servo_tm = currentTime;

      // prenastaveni serva
      setOutputServo();  
   }

   // motor
   if(currentTime % 4 == 0 && currentTime != motor_tm){
      if(motor_act > motor_ctl){
         motor_act--;
         // preskakovani oblasti, kde motor na nizke napeti nevrci
         if(motor_act == -1) motor_act = -400;
         if(motor_act == 399) motor_act = 0;
      }
      if(motor_act < motor_ctl){
         motor_act++;
         // preskakovani oblasti, kde motor na nizke napeti nevrci
         if(motor_act == 1) motor_act = 400;
         if(motor_act == -399) motor_act = 0;
      }
      motor_tm = currentTime;

      // prenastaveni pwm pro motor
      setOutputMotor();
   }

   // kamera
   if(currentTime % 100 == 0 && currentTime != tm_cam){
      liveCam();
      tm_cam = currentTime;
   }

   delay(1);
}

/**
 * Posle po HTTP file z spiffs
 */
uint8_t sendFSFile(AsyncWebServerRequest *request, char *filename, char *content_type){
    char buff[3000];
    int i;

    buff[0] = '\0';

    File file = SPIFFS.open(filename);
    if(file){
        i = 0;
        while(file.available()){
            buff[i] = file.read();
            i++;
        }
        buff[i] = '\0';
        file.close();

        request->send(200, content_type, buff);
        return 0;
    } else {
        request->send(404);
        return 0;
    }
}

/**
 * Posle zpravu vsem klientum
 */
void notifyClients() {
   char buff[100];
   sprintf(buff, "%d|%d|%u|%d|%u|%u|%d", motor_ctl, servo_ctl, led_state, motor_act, motor_out1, motor_out2, servo_act);
   ws.textAll(buff);
}

/**
 * Prichozi WS message
 */
void handleWebSocketMessage(void *arg, uint8_t *data, size_t len) {
   AwsFrameInfo *info = (AwsFrameInfo*)arg;
   if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
      data[len] = 0;
      
      Serial.print("WS msg: ");
      Serial.println((char *)data);

      if(data[0] == 'M'){
         motor_ctl = atoi((char*)(data + 1));
      }

      if(data[0] == 'S'){
         servo_ctl = atoi((char*)(data + 1));
      }

      if(data[0] == 'L'){
         if(data[1] == '1'){
            led_state = 1;
            setFlashOn();
         }
         if(data[1] == '0'){
            led_state = 0;
            setFlashOff();
         }
      }

      notifyClients();
   }
}

/**
 * Websocket eventy 
 */
void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type,
             void *arg, uint8_t *data, size_t len) {
  switch (type) {
    case WS_EVT_CONNECT:
      Serial.printf("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
      break;
    case WS_EVT_DISCONNECT:
      Serial.printf("WebSocket client #%u disconnected\n", client->id());
      break;
    case WS_EVT_DATA:
      handleWebSocketMessage(arg, data, len);
      break;
    case WS_EVT_PONG:
    case WS_EVT_ERROR:
      break;
  }
}

/**
 * Init websocketu 
 */
void initWebSocket() {
  ws.onEvent(onEvent);
  server.addHandler(&ws);
}

/**
 * Rozne blesk
 */
void setFlashOn(){
    digitalWrite(PIN_LED_FLASH, HIGH);
}

/**
 * Zhasne blesk
 */
void setFlashOff(){
    digitalWrite(PIN_LED_FLASH, LOW);
}

/**
 * live cam
 */
void liveCam(){
   char buff[100];

  //capture a frame
  camera_fb_t *fb = esp_camera_fb_get();
  if (!fb) {
      Serial.println("Frame buffer could not be acquired");
      return;
  } else {
   //sprintf(buff, "Frame OK / %u", fb->len);
   //Serial.println(buff);
  }

  // poslani
  ws.binaryAll(fb->buf, fb->len);

  // return the frame buffer back to be reused
  esp_camera_fb_return(fb);
}

/**
 * Nastaveni PWM na motoru
 */
void setOutputMotor(){
   uint8_t out1;
   uint8_t out2;

   if(motor_act == 0){
      out1 = 255;
      out2 = 255;
   } else {
      if(motor_act > 0){
         out1 = 255;
         out2 = (uint8_t)round((1000.0 - motor_act) * 0.255);
      } else {
         out1 = (uint8_t)round((1000.0 + motor_act) * 0.255);
         out2 = 255;
      }
   }

   if(out1 != motor_out1 || out2 != motor_out2){
      motor_out1 = out1;
      motor_out2 = out2;      
      ledcWrite(pwmChannel1, motor_out1);
      ledcWrite(pwmChannel2, motor_out2);
   }
}

/**
 * Nastaveni serva tam kde ma byt
 */
void setOutputServo(){
   int servo_neutral_ms = 1565;
   myservo.writeMicroseconds(servo_neutral_ms + round(servo_act / 3.0));
}