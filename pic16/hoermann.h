typedef enum
{
  hoermann_action_stop = 0,
  hoermann_action_open = 1,
  hoermann_action_close = 2,
  hoermann_action_venting = 3,
  hoermann_action_toggle_light = 4
} hoermann_action_t;

extern void hoermann_init(void);
extern void hoermann_run(void);
extern uint16_t hoermann_get_broadcast(void);
extern void hoermann_trigger_action(hoermann_action_t action);
extern void hoermann_rx_isr(void);
extern void hoermann_tx_isr(void);


