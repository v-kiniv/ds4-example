#include "mgos.h"
#include "mgos_app.h"
#include "mgos_ds4.h"

static void ds4_connection_cb(int ev, void *evd, void *userdata)
{
  struct mgos_ds4_connection_arg *event = (struct mgos_ds4_connection_arg *) evd;

  LOG(LL_INFO, ("Controller #%u(%s) %s",
                event->index,
                event->device_address.p,
                event->connected ? "connected" : "disconnected"));

}

static void ds4_input_cb(int ev, void *evd, void *userdata)
{
  bool res = false;
  struct mgos_ds4_input_arg *event  = (struct mgos_ds4_input_arg *) evd;
  struct mgos_ds4_3axis     *s      = NULL;
  
  // Share button: print battery status
  if(event->button_down.share) {
    LOG(LL_INFO, ("Controller #%u battery: %d%%(%s)", 
                  event->index,
                  event->state.battery.capacity,
                  event->state.battery.status == DS4_PS_CHARGING ? "charging" : "discharging"
    ));
  }

  // PS button: disconnect controller
  if(event->button_down.ps) {
    mgos_ds4_led_set(event->index, 0, 0, 0);
    res = mgos_ds4_disconnect(event->index);
    LOG(LL_INFO, ("Disconnect controller #%u: %s", 
                  event->index, res ? "success" : "fail"));
  }
  
  // Cross button: button_down event
  if(event->button_down.cross) {
    LOG(LL_INFO, ("X DOWN"));
  }
  if(event->button_up.cross) {
    LOG(LL_INFO, ("X UP"));
  }

  // Sticks: stick_move event update LED color and brightness
  if(event->stick_move[0] || event->stick_move[1])
  {
    uint8_t l = 255 - (127 - event->state.stick[1].y);
    uint8_t r =       (127 - event->state.stick[0].y) * l / 255;
    uint8_t g =       (127 - event->state.stick[0].x) * l / 255;
    uint8_t b =       (127 - event->state.stick[1].x) * l / 255;

    mgos_ds4_led_set(event->index, r, g, b);
  }

  // Triggers: trigger_move event set rumble and LED blink
  if(event->trigger_move[0] || event->trigger_move[1]) {
    mgos_ds4_rumble_set(event->index, event->state.trigger[1],
                        event->state.trigger[0]);
  }

  // Hold Circle: print gyroscope data
  if(event->state.button.circle && event->gyro_move) {
    s = &event->state.gyro;
    LOG(LL_INFO, ("Gyroscope PITCH: %05d, YAW: %05d, ROLL: %05d", 
                  s->x, s->y, s->z));
  }
  // Hold Triangle: print accelerometer data
  if(event->state.button.triangle && event->accel_move) {
    s = &event->state.accel;
    LOG(LL_INFO, ("Accelerometer X: %05d, Y: %05d, Z: %05d", s->x, s->y, s->z));
  }

  // Touchpad: print coords for each finger
  for(int i = 0; i < 2; i++) {
    s = &event->state.touchpad[i];
    if(s->z) {
      LOG(LL_INFO, ("Finger #%d X:%04d, Y: %04d", i + 1, s->x, s->y));
    }
  }
}

static void ds4_paired_cb(int ev, void *evd, void *userdata)
{
  struct mgos_ds4_paired_arg *arg = (struct mgos_ds4_paired_arg *) evd;

  mgos_ds4_led_set(0, 0, 255, 0); // indicate success pair with a green LED
  LOG(LL_INFO, ("Controller paired(%s)", arg->device_address.p));
}

static void ds4_discovery_cb(int ev, void *evd, void *userdata)
{
  if(ev == MGOS_DS4_DISCOVERY_STARTED) {
    LOG(LL_INFO, ("Device discovery started..."));
  }
  if(ev == MGOS_DS4_DISCOVERY_STOPPED) {
    LOG(LL_INFO, ("Device discovery stopped."));
  }
}

static void scan_button_cb(int pin, void *arg)
{
  if(mgos_ds4_is_discovering()) {
    LOG(LL_INFO, ("Button: cancel scan."));
    mgos_ds4_cancel_discovery();
  } else {
    LOG(LL_INFO, ("Button: start scan."));
    mgos_ds4_discover_and_pair();
  }
  
}

enum mgos_app_init_result mgos_app_init(void)
{
  // Setup button for pairing
  int  btn_pin = mgos_sys_config_get_board_btn1_pin();
  bool pull_up = mgos_sys_config_get_board_btn1_pull_up();
  mgos_gpio_set_button_handler(btn_pin,
                               pull_up ?
                                  MGOS_GPIO_PULL_UP : MGOS_GPIO_PULL_DOWN,
                               pull_up ?
                                  MGOS_GPIO_INT_EDGE_NEG : MGOS_GPIO_INT_EDGE_POS,
                               20, scan_button_cb, NULL);

  // Register ds4 event handlers
  mgos_event_add_handler(MGOS_DS4_CONNECTION, ds4_connection_cb, NULL);
  mgos_event_add_handler(MGOS_DS4_INPUT, ds4_input_cb, NULL);
  mgos_event_add_handler(MGOS_DS4_PAIRED, ds4_paired_cb, NULL);
  mgos_event_add_handler(MGOS_DS4_DISCOVERY_STARTED, ds4_discovery_cb, NULL);
  mgos_event_add_handler(MGOS_DS4_DISCOVERY_STOPPED, ds4_discovery_cb, NULL);

  return MGOS_APP_INIT_SUCCESS;
}
