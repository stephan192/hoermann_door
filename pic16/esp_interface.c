#include <xc.h>
#include <stdint.h>
#include <stdbool.h>
#include "sysconfig.h"
#include "hoermann.h"


#define RS232_BRGVAL          (uint16_t)(((float)FCY/(4.0 * (float)RS232_BAUDRATE))-0.5)

#define SYNC_BYTE             0x55
#define STATUS_SEND_INTERVAL  5000


static uint8_t rx_buffer[15+3] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
static bool rx_message_ready = false;

static uint8_t tx_buffer[15+3] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
static uint8_t tx_counter = 0;
static uint8_t tx_length = 0;


static uint8_t calc_checksum(uint8_t *p_data, uint8_t length)
{
  uint8_t i;
  uint8_t crc = SYNC_BYTE;
  
  for(i = 0; i < length; i++)
  {
    crc += *p_data;
    p_data++;
  }
  
  return crc;
}


static uint8_t calc_checksum_isr(uint8_t *p_data, uint8_t length)
{
  uint8_t i;
  uint8_t crc = SYNC_BYTE;
  
  for(i = 0; i < length; i++)
  {
    crc += *p_data;
    p_data++;
  }
  
  return crc;
}


void esp_interface_init(void)
{
  /* UART2 - RS232 */
  
  /* Configure baudrate */
  SP2BRG = RS232_BRGVAL;
  
  BAUD2CONbits.BRG16 = 1;
  TX2STAbits.BRGH = 1;
  /* All other bits in BAUD2CON, RC2STA and TX2STA need not to be changed.
   * Their values at POR/BOR match the required configuration. */

  /* Enable UART module, receiver and transmitter */
  RC2STAbits.SPEN = 1;
  RC2STAbits.CREN = 1;
  TX2STAbits.TXEN = 1;
  
  /* Enable receive interrupt */
  RC2IE = 1;
}


static void parse_message(void)
{
  if(rx_buffer[0] == 0x01)
  {
    if(rx_buffer[1] == 0x01)
    {
      hoermann_trigger_action((hoermann_action_t)rx_buffer[2]);
    }
  }
}


static void send_status(void)
{
  uint16_t broadcast;
  broadcast = hoermann_get_broadcast();
  
  tx_buffer[0] = 0x00;
  tx_buffer[1] = 0x02;
  tx_buffer[2] = (uint8_t)broadcast;
  tx_buffer[3] = (uint8_t)(broadcast>>8);
  tx_buffer[4] = calc_checksum(tx_buffer, 4);
  tx_length = 5;
  
  /* Start with Syncbyte */
  tx_counter = 0;
  TX2REG = SYNC_BYTE;

  /* Activate transmit interrupt */
  TX2IE = 1;
}


void esp_interface_run(void)
{
  static uint16_t ms_counter = 0;
  
  if(rx_message_ready)
  {
    parse_message();
    rx_message_ready = false;
  }  

  ms_counter++;
  if(ms_counter == STATUS_SEND_INTERVAL)
  {
    ms_counter = 0;
    send_status();
  }
}


void esp_rx_isr(void)
{
  static int8_t counter = -1;
  static uint8_t length = 0;
  uint8_t data;

  while(RC2IF == 1)
  {
    if(rx_message_ready)
    {
      data = RC2REG;
    }
    else
    {
      data = RC2REG;
      if((data == SYNC_BYTE)&&(counter == -1))
      {
        counter = 0;
        length = 0;
      }
      else if(counter >= 0)
      {
        rx_buffer[counter] = data;
        counter++;
        if(counter == 2)
        {
          if(data < 16)
          {
            length = data + 3; /* 3 = CMD + LEN + CHK, limit to 15 data bytes */
          }
          else
          {
            counter = -1;
          }
        }
        else if(counter == length)
        {
          if(calc_checksum_isr(rx_buffer, length-1) == data)
          {
            rx_message_ready = true;
          }
          counter = -1;
        }
      }
    }
  }
}


void esp_tx_isr(void)
{
  while((TX2IF == 1) && (TX2IE == 1))
  {
    if(tx_counter < tx_length)
    {
      TX2REG = tx_buffer[tx_counter];
      tx_counter++;
    }
    else
    {
      TX2IE = 0;
    }
  }
}
