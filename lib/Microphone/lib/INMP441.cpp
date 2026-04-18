#include "INMP441.h"

INMP441::INMP441()
    : rx_handle(nullptr), aux_buffer(nullptr), aux_buffer_size(0) {}
// Destruct.
INMP441::~INMP441() {
  if (aux_buffer) {
    delete[] aux_buffer;
    aux_buffer = nullptr;
  }
}
// INMP441::begin -> Init I2S STD Master Mode.
// Pin config, sample_rate, buf_count*(buf_len-1)*word_size is the DMA buffer
// size.
bool INMP441::begin(int bck, int ws, int din, int sample_rate, int buf_count,
                    int buf_len, bool channelLeft) {
  Serial.println("Iniciando INMP441");
  this->channelLeft = channelLeft; // Guardar la opción

  // Liberar canal I2S previo si existe
  if (rx_handle) {
    i2s_channel_disable(rx_handle);
    i2s_del_channel(rx_handle);
    rx_handle = nullptr;
  }

  aux_buffer_size = (buf_count - 1) * buf_len * word_size;
  aux_buffer = new uint8_t[aux_buffer_size];
  if (!aux_buffer)
    return false;

  i2s_chan_config_t chan_cfg =
      I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_0, I2S_ROLE_MASTER);
  chan_cfg.dma_desc_num = buf_count;
  chan_cfg.dma_frame_num = buf_len;

  if (i2s_new_channel(&chan_cfg, nullptr, &rx_handle) != ESP_OK)
    return false;

  i2s_std_clk_config_t clk_cfg = {
      .sample_rate_hz = sample_rate,
      .clk_src = I2S_CLK_SRC_DEFAULT,
      .mclk_multiple = I2S_MCLK_MULTIPLE_384,
  };

  i2s_std_config_t std_cfg = {
      .clk_cfg = clk_cfg,
      .slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_32BIT,
                                                      I2S_SLOT_MODE_STEREO),
      .gpio_cfg =
          {
              .mclk = I2S_GPIO_UNUSED,
              .bclk = static_cast<gpio_num_t>(bck),
              .ws = static_cast<gpio_num_t>(ws),
              .din = static_cast<gpio_num_t>(din),
              .invert_flags = {false, false, false},
          },
  };

  if (i2s_channel_init_std_mode(rx_handle, &std_cfg) != ESP_OK)
    return false;
  Serial.println("INMP441 OK");
  return i2s_channel_enable(rx_handle) == ESP_OK;
}
bool INMP441::read(uint8_t *destination, size_t bytes_to_read, size_t *read) {
  size_t bytes_read = 0;
  if (i2s_channel_read(rx_handle, aux_buffer, aux_buffer_size, &bytes_read,
                       1000) != ESP_OK)
    return false;

  uint32_t *in = (uint32_t *)aux_buffer;
  int32_t *out = (int32_t *)destination;
  size_t samples = bytes_read / word_size;

  size_t first_sample = channelLeft ? 0 : 1; // ← usa el valor guardado
  size_t count = 0;
  for (size_t i = first_sample; i < samples; i += 2) {
    uint32_t pcm24 = (in[i] >> 8) & 0x00FFFFFF;
    int32_t sample = (int32_t)(pcm24 << 8) >> 8;
    out[count++] = sample;
  }
  *read = count * sizeof(int32_t);
  return true;
}