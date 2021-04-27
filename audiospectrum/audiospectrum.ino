#include <arduinoFFT.h>
#include <M5StickCPlus.h>
#include <driver/i2s.h>

#define PIN_CLK  0
#define PIN_DATA 34
#define SAMPLES 1024
#define READ_LEN (2 * SAMPLES)
#define GAIN_FACTOR 10
#define DISPLAY_WIDTH 240
#define DISPLAY_HEIGHT 135

#define BANDS 8
#define ATTENUATION 2.5
#define MAGNIFY 23
#define NOISE_FLOOR 316.227766017

uint8_t buffer1[READ_LEN] = { 0 };
uint8_t buffer2[READ_LEN] = { 0 };
double fftBufferReal[ SAMPLES ];
double fftBufferImag[ SAMPLES ];

uint16_t bgColor = TFT_BLACK;
uint16_t meterColor = GREEN;
uint16_t traceColor = TFT_DARKGREEN;

uint16_t oldy[DISPLAY_WIDTH];
int16_t *adcBuffer = NULL;

volatile uint8_t readyForUpdate = 0;
volatile uint8_t redrawBackground = 0;
const TickType_t delay10 = 10 / portTICK_PERIOD_MS;
const TickType_t delay100 = 100 / portTICK_PERIOD_MS;

TaskHandle_t micTaskHandle;
TaskHandle_t drawTaskHandle;
TaskHandle_t buttonTaskHandle;

SemaphoreHandle_t bufferMutex;

TFT_eSprite img = TFT_eSprite(&M5.Lcd);

arduinoFFT FFT = arduinoFFT();

byte getBand(int i) {
  if (i >= 2   && i < 4  ) return 0;  // 125Hz
  if (i >= 4   && i < 8  ) return 1;  // 250Hz
  if (i >= 8   && i < 16 ) return 2;  // 500Hz
  if (i >= 16  && i < 32 ) return 3;  // 1000Hz
  if (i >= 32  && i < 64 ) return 4;  // 2000Hz
  if (i >= 64  && i < 128) return 5;  // 4000Hz
  if (i >= 128 && i < 256) return 6;  // 8000Hz
  if (i >= 256 && i < 512) return 7;  // 16000Hz
  return 8;
}

void showSignal() {
  int y;
  //if (redrawBackground) {
    img.fillSprite(bgColor);
    //redrawBackground = false;
  //}

  xSemaphoreTake(bufferMutex, portMAX_DELAY);
  FFT.Windowing(fftBufferReal, SAMPLES, FFT_WIN_TYP_HAMMING, FFT_FORWARD);
  FFT.Compute(fftBufferReal, fftBufferImag, SAMPLES, FFT_FORWARD);
  FFT.ComplexToMagnitude(fftBufferReal, fftBufferImag, SAMPLES);
  double values[BANDS] = {};
  for (int i = 2; i < (SAMPLES/2); i++){ 
    // Don't use sample 0 and only first SAMPLES/2 are usable. 
    // Each array element represents a frequency and its value the amplitude.
    if (fftBufferReal[i] > NOISE_FLOOR) {
      byte bandNum = getBand(i);
      if(bandNum != 8 && fftBufferReal[i] > values[bandNum]) {
        values[bandNum] = fftBufferReal[i];
      }
    }
  }

  xSemaphoreGive(bufferMutex);

  for (int n = 0; n < DISPLAY_WIDTH; n++) {
    y = adcBuffer[n] * GAIN_FACTOR;
    y = map(y, INT16_MIN, INT16_MAX, 10, 125);
    img.drawPixel(n, oldy[n], traceColor);
    img.drawPixel(n, y, meterColor);
    oldy[n] = y;
  }

  int imgHeight = img.height();
  for(uint32_t i; i < BANDS; i++) {
    uint32_t x = i * 12 + 60;
    int32_t height = (log10(values[i]) - ATTENUATION) * MAGNIFY;
    if(height < 1)
      height = 1; 
    img.drawFastVLine(x, imgHeight - height, imgHeight, TFT_RED);
    img.drawFastVLine(x + 1, imgHeight - height, imgHeight, TFT_RED);
    img.drawFastVLine(x + 2, imgHeight - height, imgHeight, TFT_RED);
  }

  img.pushSprite(0, 0);
}

void show_signal_task(void* arg) {
  while (1) {
    if (readyForUpdate) {
      showSignal();
      readyForUpdate = 0;
    }
    vTaskDelay(delay10);
  }
}

void button_task(void* arg) {
  while (1) {
    M5.update();
    if (M5.BtnA.wasReleased()) {
      redrawBackground = 1;
    }
    vTaskDelay(delay100);
  }
}

void mic_record_task (void* arg)
{
  i2sInit();
  size_t bytesread;
  uint8_t * currentBuffer = 0;
  while (1) {
    currentBuffer = currentBuffer == buffer1 ? buffer2 : buffer1;
    i2s_read(I2S_NUM_0, currentBuffer, READ_LEN, &bytesread, (100 / portTICK_RATE_MS));
    adcBuffer = (int16_t *)currentBuffer;

    xSemaphoreTake(bufferMutex, portMAX_DELAY);
    for (int i = 0; i < SAMPLES; ++i) {
      fftBufferReal[i] = (int)adcBuffer[i];
      fftBufferImag[i] = 0.0;
    }
    xSemaphoreGive(bufferMutex);

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

  img.createSprite(DISPLAY_WIDTH, DISPLAY_HEIGHT);
  img.fillSprite(TFT_BLACK);
  
  delay(1000);
  redrawBackground = 1;

  bufferMutex = xSemaphoreCreateMutex();

  xTaskCreatePinnedToCore(
    show_signal_task, // Function to implement the task
    "show_signal_task", // Name of the task
    2048,  // Stack size in words
    NULL,  // Task input parameter
    0,  // Priority of the task
    &drawTaskHandle,  // Task handle.
    1); // Core where the task should run

  xTaskCreatePinnedToCore(
    button_task, // Function to implement the task
    "button_task", // Name of the task
    2048,  // Stack size in words
    NULL,  // Task input parameter
    0,  // Priority of the task
    &buttonTaskHandle,  // Task handle.
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
