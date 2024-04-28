

// #include<spiConfig.h>

// // Activate SPI hardware with correct speed and mode.

// void MySpiClass::activate()
// {
//     SPI.beginTransaction(m_spiSettings);
// }
// // Initialize the SPI bus.
// void MySpiClass::begin(SdSpiConfig config)
// {
//     (void)config;
//     SPI.begin(PIN_NUM_CLK, PIN_NUM_MISO, PIN_NUM_MOSI, -1);
// }
// // Deactivate SPI hardware.
// void MySpiClass::deactivate() { SPI.endTransaction(); }
// // Receive a byte.
// uint8_t MySpiClass::receive() { return SPI.transfer(0XFF); }
// // Receive multiple bytes.
// // Replace this function if your board has multiple byte receive.
// uint8_t MySpiClass::receive(uint8_t *buf, size_t count)
// {
//     for (size_t i = 0; i < count; i++)
//     {
//         buf[i] = SPI.transfer(0XFF);
//     }
//     return 0;
// }
// // Send a byte.
// void MySpiClass::send(uint8_t data) { SPI.transfer(data); }
// // Send multiple bytes.
// // Replace this function if your board has multiple byte send.
// void MySpiClass::send(const uint8_t *buf, size_t count)
// {
//     for (size_t i = 0; i < count; i++)
//     {
//         SPI.transfer(buf[i]);
//     }
// }
// // Save SPISettings for new max SCK frequency
// void MySpiClass::setSckSpeed(uint32_t maxSck)
// {
//     m_spiSettings = SPISettings(maxSck, MSBFIRST, SPI_MODE0);
// }



