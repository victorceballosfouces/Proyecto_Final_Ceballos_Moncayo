#include <freertos/FreeRTOS.h>
#include <stdio.h>
#include <string.h>
#include <sys/unistd.h>
#include <sys/stat.h>
#include "esp_err.h"
#include "esp_log.h"
#include "esp_vfs_fat.h"
#include "driver/sdmmc_host.h"
#include "driver/sdspi_host.h"
#include "sdmmc_cmd.h"

#include "SDCard.h"

static const char *TAG = "SDC";

#define SPI_DMA_CHAN 1

/*
SDcard():
Aqui definimos el constructor de la clase SDCard para nuestra tarjeta, reemplazando la libreria SD original.
Primero se declara el mount_point donde montaremos la SD y una variable ret que se usará para detectar errores
durante el proceso. A continuación con mount_config retocamos algunas opciones para montar el sistema de
ficheros. format_if_mount_failed a true nos permitirá formatear la SD en caso de que falle en el mounting,
max_files limita el máximo a 5 y .allocation_unit_size define el tamaño del ubicación en nuestra unidad.
Empezamos con la inicialización de la tarjeta: con  sdmmc_host_t host = SDSPI_HOST_DEFAULT() establecemos
la configuración del host para el driver SPI en la SD y conseguimos no tener el dispositivo como "slave"
dentro del sistema sino como master. Esto nos permite enviar y recibir datos con un bus I2S bidirecional,
ademas de poderlos comprimir. Después tenemos que cambiar la velocidad por defecto del host que define
SDSPI_HOST_DEFAULT() porque es demasiado alta para nuestra breakout board donde tenemos la microSD instalada. 
Con  host.max_freq_khz = 4000 declaramos el valor recomendado por la librería SDFat con esp32/esp8266.
A continuación con sdspi_slot_config_t slot_config = SDSPI_SLOT_CONFIG_DEFAULT(); configuramos los pines para
el host SPI y usando slot._config.gpio_ los asignamos a los que pasamos como valores de entrada de la función
(miso,mosi,clk,cs). Con la función esp_vfs_fat_sdmmc_mount() montamos el filesystem en la SD de la siguiente
manera: pasamos el path donde se registrará la partición (e.g. "/sdcard") con m_mount_point, inicializamos
el driver SPI con la configuración en host, inicializamos los pines de la SD con la configuración en slot_config,
pasamos los parametros extra de montaje con mount_config y finalmente guardamos la información de la estructura
de la SD montada en m_card. Todo este proceso lo llamamos dentro de la variable esp_err_t ret para después en
un senzillo bucle indicar al usuario, en el caso que falle la función, si el error se a producido en el montage
del sistema de ficheros o directamente no se ha podido inicializar la tarjeta. Para acabar y si todo funciona
correctamente, escribimos por pantalla el mount_point donde se encuentra la SD y su información almacenada
previamente en la variable m_card.

*/

SDCard::SDCard(const char *mount_point, gpio_num_t miso, gpio_num_t mosi, gpio_num_t clk, gpio_num_t cs)
{
  m_mount_point = mount_point;
  esp_err_t ret;
  
  // Options for mounting the filesystem.
  // If format_if_mount_failed is set to true, SD card will be partitioned and
  // formatted in case when mounting fails.
  esp_vfs_fat_sdmmc_mount_config_t mount_config = {
      .format_if_mount_failed = true,
      .max_files = 5,
      .allocation_unit_size = 16 * 1024};

  ESP_LOGI(TAG, "Initializing SD card");

  sdmmc_host_t host = SDSPI_HOST_DEFAULT();
  host.max_freq_khz = 4000;
  sdspi_slot_config_t slot_config = SDSPI_SLOT_CONFIG_DEFAULT();
  slot_config.gpio_miso = miso;
  slot_config.gpio_mosi = mosi;
  slot_config.gpio_sck = clk;
  slot_config.gpio_cs = cs;

  ret = esp_vfs_fat_sdmmc_mount(m_mount_point.c_str(), &host, &slot_config, &mount_config, &m_card);

  if (ret != ESP_OK)
  {
    if (ret == ESP_FAIL)
    {
      ESP_LOGE(TAG, "Failed to mount filesystem. "
                    "If you want the card to be formatted, set the format_if_mount_failed");
    }
    else
    {
      ESP_LOGE(TAG, "Failed to initialize the card (%s). "
                    "Make sure SD card lines have pull-up resistors in place.",
               esp_err_to_name(ret));
    }
    return;
  }
  ESP_LOGI(TAG, "SDCard mounted at: %s", m_mount_point.c_str());

  // Card has been initialized, print its properties
  sdmmc_card_print_info(stdout, m_card);
}


SDCard::~SDCard()
{
  // All done, unmount partition and disable SDMMC or SPI peripheral
  esp_vfs_fat_sdmmc_unmount();
  ESP_LOGI(TAG, "Card unmounted");
}

