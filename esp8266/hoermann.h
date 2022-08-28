#ifndef Hoermann_h
#define Hoermann_h

#include "Arduino.h"

typedef enum
{
  hoermann_state_stopped = 0,
  hoermann_state_open,
  hoermann_state_closed,
  hoermann_state_venting,
  hoermann_state_opening,
  hoermann_state_closing,
  hoermann_state_error,
  hoermann_state_unkown
} hoermann_state_t;

typedef enum
{
  hoermann_action_stop = 0,
  hoermann_action_open,
  hoermann_action_close,
  hoermann_action_venting,
  hoermann_action_toggle_light,
  hoermann_action_none
} hoermann_action_t;

class Hoermann
{
  public:
    Hoermann();
    void loop();
    hoermann_state_t get_state();
    String get_state_string();
    void trigger_action(hoermann_action_t action);
  private:
    hoermann_state_t actual_state;
    String actual_state_string;
    hoermann_action_t actual_action;
    uint8_t rx_buffer[19];
    uint8_t output_buffer[19];
    bool read_rs232();
    void parse_input();
    void send_command();
    uint8_t calc_checksum(uint8_t *p_data, uint8_t length);
};

#endif
