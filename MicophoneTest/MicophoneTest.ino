#include <M5StickCPlus.h>
#include <driver/i2s.h>

#define PIN_CLK  0
#define PIN_DATA 34
#define READ_LEN (2 * 256)
#define GAIN_FACTOR 6
#define DISPLAY_WIDTH 240
#define DISPLAY_HEIGHT 135

uint8_t BUFFER1[READ_LEN] = { 0 };
uint8_t BUFFER2[READ_LEN] = { 0 };

uint16_t oldy[DISPLAY_WIDTH];
int16_t *adcBuffer = NULL;

uint8_t readyForUpdate = 0;
const TickType_t delay10 = 10 / portTICK_PERIOD_MS;
const TickType_t delay100 = 100 / portTICK_PERIOD_MS;

TaskHandle_t micTaskHandle;
TaskHandle_t drawTaskHandle;

void showSignal(){
  int y;
  for (int n = 0; n < DISPLAY_WIDTH; n++){
    y = adcBuffer[n] * GAIN_FACTOR;
    y = map(y, INT16_MIN, INT16_MAX, 10, 125);
    M5.Lcd.drawPixel(n, oldy[n], BLACK);
    M5.Lcd.drawPixel(n, y, RED);
    oldy[n] = y;
  }
}

void show_signal_task(void* arg) {
  while(1) {
    if(readyForUpdate) {
      showSignal();
      readyForUpdate = 0;
    }
    vTaskDelay(delay10);
  }
}

void mic_record_task (void* arg)
{ 
  i2sInit();
  size_t bytesread;
  uint8_t * currentBuffer = 0;
  while(1) {
    currentBuffer = currentBuffer == BUFFER1 ? BUFFER2 : BUFFER1;
    i2s_read(I2S_NUM_0, currentBuffer, READ_LEN, &bytesread, (100 / portTICK_RATE_MS));
    adcBuffer = (int16_t *)currentBuffer;
    readyForUpdate = 1;
    vTaskDelay(delay10);
  }
}

void i2sInit()
{
   i2s_config_t i2s_config = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX | I2S_MODE_PDM),
    .sample_rate =  44100,
    .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT, // is fixed at 12bit, stereo, MSB
    .channel_format = I2S_CHANNEL_FMT_ALL_RIGHT,
    .communication_format = I2S_COMM_FORMAT_I2S,
    .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
    .dma_buf_count = 2,
    .dma_buf_len = 128,
   };

   i2s_pin_config_t pin_config;
   pin_config.bck_io_num   = I2S_PIN_NO_CHANGE;
   pin_config.ws_io_num    = PIN_CLK;
   pin_config.data_out_num = I2S_PIN_NO_CHANGE;
   pin_config.data_in_num  = PIN_DATA;
	
   
   i2s_driver_install(I2S_NUM_0, &i2s_config, 0, NULL);
   i2s_set_pin(I2S_NUM_0, &pin_config);
   i2s_set_clk(I2S_NUM_0, 44100, I2S_BITS_PER_SAMPLE_16BIT, I2S_CHANNEL_MONO);
}


void setup() {
  M5.begin();
  M5.Lcd.setRotation(3);
  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.setTextColor(WHITE, BLACK);
  M5.Lcd.println("mic test");
  delay(1000);
  M5.Lcd.fillScreen(BLUE);

  
  
   xTaskCreatePinnedToCore(
      show_signal_task, // Function to implement the task
      "show_signal_task", // Name of the task
      2048,  // Stack size in words
      NULL,  // Task input parameter
      0,  // Priority of the task
      &drawTaskHandle,  // Task handle.
      0); // Core where the task should run

   
   xTaskCreatePinnedToCore(
      mic_record_task, // Function to implement the task 
      "mic_record_task", // Name of the task 
      2048,  // Stack size in words 
      NULL,  // Task input parameter 
      0,  // Priority of the task 
      &micTaskHandle,  // Task handle.
      1); // Core where the task should run 


}




void loop() {
  printf("loop cycling\n");
  vTaskDelay(1000 / portTICK_RATE_MS); // otherwise the main task wastes half of the cpu cycles
}
