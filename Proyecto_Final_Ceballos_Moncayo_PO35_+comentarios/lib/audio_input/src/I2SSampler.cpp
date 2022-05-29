
#include "I2SSampler.h"
#include "driver/i2s.h"

I2SSampler::I2SSampler(i2s_port_t i2sPort, const i2s_config_t &i2s_config) : m_i2sPort(i2sPort), m_i2s_config(i2s_config)
{
}

void I2SSampler::start()
{
    //install and start i2s driver
    i2s_driver_install(m_i2sPort, &m_i2s_config, 0, NULL);
    // set up the I2S configuration from the subclass
    configureI2S();
}

void I2SSampler::stop()
{
    // clear any I2S configuration
    unConfigureI2S();
    // stop the i2S driver
    i2s_driver_uninstall(m_i2sPort);
}

/* I2SSampler actua como clase base tanto para I2SMEMSSampler como para ADCSampler. Aqui simplemente definimos
la función start() y stop(). En la primera, pasandole como valores de entrada nuestro puerto y configuración,
instalamos e inicializamos el driver I2S a la vez que llamamos a
configureI2S() para establecer la configuración I2S que contiene cada subclase (I2SMEMS/ADC). Al contrario, con
stop() hacemos un clear de la configuracion y i2s_driver_uninstall(m_i2sPort) nos desinstala los drivers I2S
del puerto especificado.

*/