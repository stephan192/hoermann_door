#include "Arduino.h"
#include "hoermann.h"

#define SYNC_BYTE   0x55

Hoermann::Hoermann(void)
{
  actual_state.data_valid = false;
  actual_action = hoermann_action_none;
}

void Hoermann::loop(void)
{
  if (read_rs232() == true)
  {
    parse_input();
  }

  if (actual_action != hoermann_action_none)
  {
    send_command();
    actual_action = hoermann_action_none;
  }
}

hoermann_state_t Hoermann::get_state(void)
{
  return actual_state;
}

void Hoermann::trigger_action(hoermann_action_t action)
{
  actual_action = action;
}

bool Hoermann::read_rs232(void)
{
  static uint8_t counter = 0;
  static uint8_t len = 0;
  uint8_t data;

  while (Serial.available() > 0)
  {
    // read the incoming byte:
    data = (uint8_t)Serial.read();

    if ((data == SYNC_BYTE) && (counter == 0))
    {
      rx_buffer[counter] = data;
      counter++;
      len = 0;
    }
    else if (counter > 0)
    {
      rx_buffer[counter] = data;
      counter++;
      if (counter == 3)
      {
        if (data < 16)
        {
          len = data + 4; //4 = SYNC + CMD + LEN + CHK, limit to 15 data bytes
        }
        else
        {
          counter = 0;
        }
      }
      else if (counter == len)
      {
        if (calc_checksum(rx_buffer, len - 1) == data)
        {
          counter = 0;
          return true;
        }
        counter = 0;
      }
    }
  }

  return false;
}

void Hoermann::parse_input(void)
{
  if (rx_buffer[1] == 0x00)
  {
    if (rx_buffer[2] == 0x02)
    {
      /* Determine cover state */
      if ((rx_buffer[3] & 0x01) == 0x01)
      {
        actual_state.cover = cover_open;
      }
      else if ((rx_buffer[3] & 0x02) == 0x02)
      {
        actual_state.cover = cover_closed;
      }
      else if ((rx_buffer[3] & 0x60) == 0x40)
      {
        actual_state.cover = cover_opening;
      }
      else if ((rx_buffer[3] & 0x60) == 0x60)
      {
        actual_state.cover = cover_closing;
      }
      else
      {
        actual_state.cover = cover_stopped;
      }

      /* Determine option relay state */
      if ((rx_buffer[3] & 0x04) == 0x04)
      {
        actual_state.option_relay = true;
      }
      else
      {
        actual_state.option_relay = false;
      }

      /* Determine light state */
      if ((rx_buffer[3] & 0x08) == 0x08)
      {
        actual_state.light = true;
      }
      else
      {
        actual_state.light = false;
      }

      /* Determine error state */
      if ((rx_buffer[3] & 0x10) == 0x10)
      {
        actual_state.error = true;
      }
      else
      {
        actual_state.error = false;
      }

      /* Determine venting state */
      if ((rx_buffer[3] & 0x80) == 0x80)
      {
        actual_state.venting = true;
      }
      else
      {
        actual_state.venting = false;
      }

      /* Determine prewarn state */
      if ((rx_buffer[4] & 0x01) == 0x01)
      {
        actual_state.prewarn = true;
      }
      else
      {
        actual_state.prewarn = false;
      }

      /* Finally mark data as valid */
      actual_state.data_valid = true;
    }
  }
}

void Hoermann::send_command(void)
{
  output_buffer[0] = 0x55;
  output_buffer[1] = 0x01;
  output_buffer[2] = 0x01;
  output_buffer[3] = (uint8_t)actual_action;
  output_buffer[4] = output_buffer[0] + output_buffer[1] + output_buffer[2] + output_buffer[3];
  Serial.write(&output_buffer[0], 5);
}

uint8_t Hoermann::calc_checksum(uint8_t *p_data, uint8_t length)
{
  uint8_t i;
  uint8_t crc = 0;

  for (i = 0; i < length; i++)
  {
    crc += *p_data;
    p_data++;
  }

  return crc;
}
