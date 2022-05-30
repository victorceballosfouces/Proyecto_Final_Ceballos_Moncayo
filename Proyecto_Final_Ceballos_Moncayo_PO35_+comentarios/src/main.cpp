#include <Arduino.h>
#include <stdio.h>
#include <FreeRTOS.h>
#include <I2SMEMSSampler.h>
#include <ADCSampler.h>
#include <I2SOutput.h>
#include <DACOutput.h>
#include <SDCard.h>
#include "SPIFFS.h"
#include <WAVFileReader.h>
#include <WAVFileWriter.h>
#include "config.h"

#include <SPI.h>
#include <Wire.h>
#include <Adafruit_I2CDevice.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#include <arduinoFFT.h>

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);


/*
RECORD:
Función void() donde iniciamos la grabación y la guardamos en un archivo .wav dentro de la SD. Los
parámetros de entrada són: I2SSampler *input (señal entrada I2S del micrófono) y const char *fname (nombre del fichero).
Primero incializamos el objeto input con start() instalando los drivers I2S y configurando los pines. Seguidamente
abrimos un archivo dentro de la SD y creamos un WAVFILEWriter con el que empezar a escribir sobre el, pasandole
el nombre del mismo y el sample rate del input del microfono (m_i2s_config). Después establecemos el bucle
para solo hacer la lectura/escritura mientras se mantenga pulsado el boton. Vamos leyendo la señal I2S
en el contenedor samples a la vez que lo escribimos con el writer y contando tambien el tamaño total del
archivo escrito en la SD. Tambien se escribe por pantalla el numero total de samples escritos y el tiempo
que se ha tardado. Finalmente para acabar la función paramos el input del I2SSampler, paramos la escritura del
writer, cerramos el archivo de la SD y liberamos memoria borrando objetos y limpiando el contenedor de samples.
 */

void record(I2SSampler *input, const char *fname)
{
  int16_t *samples = (int16_t *)malloc(sizeof(int16_t) * 1024);
  ESP_LOGI(TAG, "Start recording");
  input->start();
  // open the file on the sdcard
  FILE *fp = fopen(fname, "wb");
  // create a new wave file writer
  WAVFileWriter *writer = new WAVFileWriter(fp, input->sample_rate());
  // keep writing until the user releases the button
  while (gpio_get_level(GPIO_BUTTON) == 1)
  {
    int samples_read = input->read(samples, 1024);
    int64_t start = esp_timer_get_time();
    writer->write(samples, samples_read);
    int64_t end = esp_timer_get_time();
    ESP_LOGI(TAG, "Wrote %d samples in %lld microseconds", samples_read, end - start);
  }
  // stop the input
  input->stop();
  // and finish the writing
  writer->finish();
  fclose(fp);
  delete writer;
  free(samples);
  ESP_LOGI(TAG, "Finished recording");
}

/*
PLAY:
Funcion void() donde leemos el .wav escrito anteriormente en la SD y lo reproducimos por el speaker. Los
parámetros de entrada són: Output *output (señal salida I2S del speaker) y const char *fname (nombre del fichero).
Para empezar abrimos el archivo dentro de la targeta y creamos un objeto tipo WAVFileReader que apunte a este
para empezar a leerlo. Para esto solo necesitamos pasarle el nombre del archivo. Seguimos inicializando output
con start() instalando los drivers I2S y configurando los pines, ademas de pasarle el sample_rate asociado al
reader. Con el .wav abierto y todo preparado para empezar, declaramos el bucle que leerá el archivo
hasta que no haya mas muestras. Dentro del while, con samples_read almacenamos las muestras totales usando el
reader (función read()). Si no encontramos samples paramos el bucle e indicamos que no hay audio. Después
indicamos el numero total de muestras leidas y finalmente con la función write del output preparamos los datos
a la vez que los vamos escribiendo en el periférico I2S, en nuestro caso el speaker, reproduciendo asi el
audio del archivo .wav hasta que no haya mas muestras. Para acabar simplemente paramos el output, cerramos
el archivo, borramos el reader y liberamos memoria de samples.
 */

void play(Output *output, const char *fname)
{
  int16_t *samples = (int16_t *)malloc(sizeof(int16_t) * 1024);
  // open the file on the sdcard
  FILE *fp = fopen(fname, "rb");
  // create a new wave file writer
  WAVFileReader *reader = new WAVFileReader(fp);
  ESP_LOGI(TAG, "Start playing");
  output->start(reader->sample_rate());
  ESP_LOGI(TAG, "Opened wav file");
  // read until theres no more samples
  while (true)
  {
    int samples_read = reader->read(samples, 1024);
    if (samples_read == 0)
    {
      break; Serial.print("No hay audio");
    }
    ESP_LOGI(TAG, "Read %d samples", samples_read);
    output->write(samples, samples_read);
    ESP_LOGI(TAG, "Played samples");
  }
  
  // stop the input
  output->stop();
  fclose(fp);
  delete reader;
  free(samples);
  ESP_LOGI(TAG, "Finished playing");
}

int selected = 0;
int entered = -1;

void displaymenu(void) {
  
  gpio_set_direction(GPIO_BUTTON, GPIO_MODE_INPUT);
  gpio_set_pull_mode(GPIO_BUTTON, GPIO_PULLDOWN_ONLY);


  int down = digitalRead(25);
  int up = digitalRead(33);
  int enter = digitalRead(19);
  int back = digitalRead(5);

  if (up == LOW && down == LOW) {};
  if (up == LOW) {
    if(selected < 2){
      selected = selected + 1;
      delay(200);
    }
    else if (selected >=2){
      selected=0;
      delay(200);
    }
  };
  if (down == LOW) {
    if(selected >0){
      selected = selected - 1;
      delay(200);
    }
    else if (selected <=0){
      selected=2;
      delay(200);
    }
  };
  if (enter == LOW) {
    entered = selected;
  };
  if (back == LOW) {
    entered = -1;
  };
  const char *options[3] = {
    " 1.- Grabar",
    " 2.- Reproducir",
    " 3.- Creditos"
  };

  if (entered == -1) {
    display.clearDisplay();
    display.setTextSize(2);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 0);
    display.println(F("PO-35 MENU"));
    display.setTextSize(1);
    display.println("");
    for (int i = 0; i < 3; i++) {
      if (i == selected) {
        display.setTextColor(SSD1306_BLACK, SSD1306_WHITE);
        display.println(options[i]);
      } else if (i != selected) {
        display.setTextColor(SSD1306_WHITE);
        display.println(options[i]);
      }
    }

  } 
  else if (entered == 0) {
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 0);
    display.println(F("PO-35 Opcion:"));
    display.setTextSize(2);
    display.println("Grabar");
    display.setTextColor(SSD1306_WHITE);
    display.setTextSize(1);
    display.println(F("Pulse el boton X para grabar"));
    
    /*ESP_LOGI(TAG, "Creating microphone");*/
    #ifdef USE_I2S_MIC_INPUT
      I2SSampler *input = new I2SMEMSSampler(I2S_NUM_0, i2s_mic_pins, i2s_mic_Config);
    #else
      I2SSampler *input = new ADCSampler(ADC_UNIT_1, ADC1_CHANNEL_7, i2s_adc_config);
    #endif
    while (gpio_get_level(GPIO_BUTTON) == 1){
        /*wait_for_button_push();*/
          record(input, "/sdcard/test.wav");
    }
  }

  else if (entered == 1) {
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 0);
    display.println(F("PO-35 Opcion:"));
    display.setTextSize(2);
    display.println("Reproducir");
    display.setTextColor(SSD1306_WHITE);
    display.setTextSize(1);
    display.println(F("Pulse el boton X para Reproducir"));
    
    #ifdef USE_I2S_SPEAKER_OUTPUT
      Output *output = new I2SOutput(I2S_NUM_0, i2s_speaker_pins);
    #else
      Output *output = new DACOutput(I2S_NUM_0);
    #endif

    while (gpio_get_level(GPIO_BUTTON) == 1)
      { 
        //spectrum_analyzer();
        
        play(output, "/sdcard/test.wav");
        
      }
      
  }

  else if (entered == 2) {
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 0);
    display.println(F("PO-35 Opcion:"));
    display.setTextSize(2);
    display.println("Creditos:");
    display.setTextColor(SSD1306_BLACK, SSD1306_WHITE);
    display.setTextSize(1);
    display.println("   -Victor Ceballos");
    display.println("   -Rafael Moncayo");
    display.setTextColor(SSD1306_WHITE);
    display.println(("Estamos trabajando\n en nuevas\n actualizaciones."));
  }
  display.display();
}

void setup()
{
  Serial.begin(115200);
  
  
  ESP_LOGI(TAG, "Starting up");

#ifdef USE_SPIFFS
  ESP_LOGI(TAG, "Mounting SPIFFS on /sdcard");
  SPIFFS.begin(true, "/sdcard");
#else
  ESP_LOGI(TAG, "Mounting SDCard on /sdcard");
  new SDCard("/sdcard", PIN_NUM_MISO, PIN_NUM_MOSI, PIN_NUM_CLK, PIN_NUM_CS);
#endif


  pinMode(25, INPUT_PULLUP);
  pinMode(33, INPUT_PULLUP);
  pinMode(19, INPUT_PULLUP);
  pinMode(5, INPUT_PULLUP);

  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    for (;;); // Don't proceed, loop forever
  }
  display.clearDisplay();
  // Draw a single pixel in white
  display.drawPixel(10, 10, SSD1306_WHITE);
  display.display();
  delay(2000); // Pause for 2 seconds

  //sampling_period_us = round(1000000 * (1.0 / SAMPLING_FREQUENCY)); Para FFT
  
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.setTextSize(1);
  display.println("   Projecte final:\n");
  display.setTextSize(2);
  display.setTextColor(SSD1306_BLACK, SSD1306_WHITE);
  display.println("   PO-35  \n");
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.println("   -Victor Ceballos");
  display.println("   -Rafael Moncayo");
  display.display();
  delay(4000);
}

void loop()
{
  displaymenu();
}
