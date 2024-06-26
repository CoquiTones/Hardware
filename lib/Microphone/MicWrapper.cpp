#include <MicWrapper.h>

Microphone::Microphone()
{
    this->setup();
}

void Microphone::setup()
{

    sd = new SDCARD();
    bool initialized;

    while(!(initialized = sd->setup())) {
        Serial.println("ERROR INITIALIZING sd card");

        delay(1000);
    }
    ESP_LOGI(TAG, "Creating microphone");
#ifdef USE_I2S_MIC_INPUT
    this->input = new I2SMEMSSampler(I2S_NUM_0, i2s_mic_pins, i2s_mic_Config);
#else
    this->input = new ADCSampler(ADC_UNIT_1, ADC1_CHANNEL_7, i2s_adc_config);
#endif
}

const char *Microphone::recordToFile(const char *fname)
{

    int16_t *samples = (int16_t *)malloc(sizeof(int16_t) * 1024);
    Serial.println("Start Talking, mf...");
    input->start();
    // open the file on the sdcard

    Serial.print("Writing to ");
    Serial.print(fname);
    File fp = this->sd->open(fname, 'w');
    // create a new wav file writer that writes headers and data appropiately 
    WAVFileWriter *writer = new WAVFileWriter(&fp, input->sample_rate());

    unsigned long start_time;
    unsigned long current_time;
    unsigned long elapsed_time;

    int recordTimeInMs = AUDIO_DURATION_IN_SECONDS * 1000; // this should be 1-2 minutes 
    start_time = millis();
    do
    {

        int samples_read = input->read(samples, 1024);
        int64_t start = esp_timer_get_time();
        writer->write(samples, samples_read);
        int64_t end = esp_timer_get_time();
        ESP_LOGI(TAG, "Wrote %d samples in %lld microseconds", samples_read, end - start);

        current_time = millis();
        elapsed_time = current_time - start_time;
    } while (elapsed_time < recordTimeInMs);

    // stop the input
    input->stop();
    // and finish the writing / add the correct tail 
    writer->finish();
    fp.close();
    delete writer;
    free(samples);

    Serial.print("Finished Recording, mf... ");
    return fname;
}

int Microphone::takeMeasurement()
{
    int16_t *samples = (int16_t *)malloc(sizeof(int16_t) * 1024);

    input->start();
    int samples_read = input->read(samples, 1024);
    int64_t start = esp_timer_get_time();
    int64_t end = esp_timer_get_time();
    ESP_LOGI(TAG, "Wrote %d samples in %lld microseconds", samples_read, end - start);

    return samples_read;
}

char* Microphone::readFile(const char *fname)
{
    return "fuck";
}