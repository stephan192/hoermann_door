#ifndef Hoermann_h
#define Hoermann_h

#include "Arduino.h"

typedef enum
{
  cover_stopped = 0,
  cover_open,
  cover_closed,
  cover_opening,
  cover_closing
} cover_state_t;

typedef struct
{
  cover_state_t cover;
  bool venting;
  bool error;
  bool prewarn;
  bool light;
  bool option_relay;
  bool data_valid;
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
    void trigger_action(hoermann_action_t action);
  private:
    hoermann_state_t actual_state;
    hoermann_action_t actual_action;
    uint8_t rx_buffer[19];
    uint8_t output_buffer[19];
    bool read_rs232();
    void parse_input();
    void send_command();
    uint8_t calc_checksum(uint8_t *p_data, uint8_t length);
};

#endif
