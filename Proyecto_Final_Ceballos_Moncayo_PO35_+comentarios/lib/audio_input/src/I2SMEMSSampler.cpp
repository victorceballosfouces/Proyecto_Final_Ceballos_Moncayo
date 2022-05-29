#include "I2SMEMSSampler.h"
#include "soc/i2s_reg.h"

I2SMEMSSampler::I2SMEMSSampler(
    i2s_port_t i2s_port,
    i2s_pin_config_t &i2s_pins,
    i2s_config_t i2s_config,
    bool fixSPH0645) : I2SSampler(i2s_port, i2s_config)
{
    m_i2sPins = i2s_pins;
    m_fixSPH0645 = fixSPH0645;
}

void I2SMEMSSampler::configureI2S()
{
    if (m_fixSPH0645)
    {
        // FIXES for SPH0645
        REG_SET_BIT(I2S_TIMING_REG(m_i2sPort), BIT(9));
        REG_SET_BIT(I2S_CONF_REG(m_i2sPort), I2S_RX_MSB_SHIFT);
    }

    i2s_set_pin(m_i2sPort, &m_i2sPins);
}

int I2SMEMSSampler::read(int16_t *samples, int count)
{
    // read from i2s
    int32_t *raw_samples = (int32_t *)malloc(sizeof(int32_t) * count);
    size_t bytes_read = 0;
    i2s_read(m_i2sPort, raw_samples, sizeof(int32_t) * count, &bytes_read, portMAX_DELAY);
    int samples_read = bytes_read / sizeof(int32_t);
    for (int i = 0; i < samples_read; i++)
    {
        samples[i] = (raw_samples[i] & 0xFFFFFFF0) >> 11;
    }
    free(raw_samples);
    return samples_read;
}

/* I2S MEMS Sampler es una clase derivada de I2S Sampler preparada para establecer
una conexión I2S con micrófonos MEMS digitales. En el constructor definimos las variables
del puerto, los pines y la configuración necesarias para la creación del objeto. Después, con
la función configureI2S() establecemos solamente el numero de pines de la configuración I2S
estandar que contienen las variables m_i2sPort(I2S_NUM_0 o I2S_NUM_1) y m_i2sPins (I2S estructura pin).
Read es una función tipo int que lee la señal I2S del microfono, almacenando el numero de muestras
totales leidas en la varibale samples_read y haciendo el return de la misma al final de la función.

*/