/**
 * @file main.cpp
 * @author Saulo Aislan (aislansaulo@gmail.com)
 * @brief Firmware para um monitor de pH com ESP32, módulo ADS1115
 *             e sensor pH-4502C.
 * @version 0.1
 * @date 2024-07-12
 *
 * @copyright Copyright (c) 2024
 *
 */
#include <Arduino.h>
#include <SPI.h>
#include <Adafruit_ADS1X15.h>
#include <Wire.h>

Adafruit_ADS1115 ads;

int buffer_arr[10], temp;
unsigned long int avgval;
float ph_act;
float calibration_value = 22.5;

void setup(void)
{
    Serial.begin(115200);
    Serial.println("::::: Monitor de pH ::::");

    // Inicializar o I2C com os pinos personalizados
    Wire.begin(22, 23); // SDA no GPIO 22 e SCL no GPIO 23

    // A faixa de entrada ADC (ou ganho) pode ser alterada através das seguintes
    // funções, mas tome cuidado para nunca exceder VDD +0,3V.
    // Definir esses valores incorretamente pode destruir seu ADC!
    //                                                                ADS1015  ADS1115
    //                                                                -------  -------
    // ads.setGain(GAIN_TWOTHIRDS);  // 2/3x gain +/- 6.144V  1 bit = 3mV      0.1875mV (default)
    // Seta o ganho do ADC
    ads.setGain(GAIN_ONE); // 1x gain   +/- 4.096V  1 bit = 2mV      0.125mV
    // ads.setGain(GAIN_TWO);        // 2x gain   +/- 2.048V  1 bit = 1mV      0.0625mV
    // ads.setGain(GAIN_FOUR);       // 4x gain   +/- 1.024V  1 bit = 0.5mV    0.03125mV
    // ads.setGain(GAIN_EIGHT);      // 8x gain   +/- 0.512V  1 bit = 0.25mV   0.015625mV
    // ads.setGain(GAIN_SIXTEEN);    // 16x gain  +/- 0.256V  1 bit = 0.125mV  0.0078125mV

    if(!ads.begin())
    {
        Serial.println("Falha ao iniciar o ADS.");
        while(1)
            ;
    }
}

void loop(void)
{
    avgval = 0;
    float voltage = 0.0;
    
    // Ler o ADC do canal especificado
    int16_t adc0 = ads.readADC_SingleEnded(0);
    
    // Calcular a tensão
    float volts0 = ads.computeVolts(adc0);

    // Armazenar os valores de tensão em um buffer
    for(int i = 0; i < 10; i++)
    {
        buffer_arr[i] = volts0;
        delay(30);
    }

    // Ordenar os valores do buffer
    for(int i = 0; i < 9; i++)
    {
        for(int j = i + 1; j < 10; j++)
        {
            if(buffer_arr[i] > buffer_arr[j])
            {
                temp = buffer_arr[i];
                buffer_arr[i] = buffer_arr[j];
                buffer_arr[j] = temp;
            }
        }
    }

    // Calcular a média dos valores do buffer
    for(int i = 2; i < 8; i++)
        avgval += buffer_arr[i];

    float volt = (float)avgval * 3.3 / 4096 / 6;

    // Exibir a tensão no monitor serial
    Serial.print("Tensão: ");
    Serial.println(volts0);
    
    // Calcular o valor de pH com base na tensão lida e no valor de calibração
    ph_act = -5.70 * volts0 + calibration_value;

    // Exibir o valor de pH no monitor serial
    Serial.print("Valor de pH: ");
    Serial.println(ph_act);
    
    delay(1000);  // Atraso de 1 segundo antes da próxima leitura
}
