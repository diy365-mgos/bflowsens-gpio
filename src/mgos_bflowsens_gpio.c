#include "mgos.h"
#include "mg_bthing_sdk.h"
#include "mgos_bflowsens_gpio.h"
#include "mgos_bflowsens.h"

#ifdef MGOS_HAVE_MJS
#include "mjs.h"
#endif

struct mg_bflowsens_gpio_cfg {
  mgos_bflowsens_t sensor;
  int pin;
  enum mgos_gpio_pull_type pull_type;
  bool high_pulse; 
  float flow_ratio;
  int64_t last_sample_time; // microseconds
  int64_t pulse_count;      // pulse counter
  float frequency;          // Hz
  float flow_rate;          // L/min
  float total_flow;         // L
  int timer_id;
};

void mg_bflowsens_gpio_timer_cb(void *arg) {
  struct mg_bflowsens_gpio_cfg *cfg = (struct mg_bflowsens_gpio_cfg *)arg;
  float sample_duration = (mg_bthing_duration_micro(cfg->last_sample_time, mgos_uptime_micros()) / 1000); //ms

  // Frequency (Hz) = (1sec / (sample duration / pulse count))
  cfg->frequency = cfg->pulse_count == 0 ? 0 : (1000 / (sample_duration / cfg->pulse_count)); // Hz

  // Because frequency (Hz) = flow ratio (e.g.: 7.5) * flow rate (L/min)
  // so flow rate (L/min) = frequency (Hz) / flow ratio
  cfg->flow_rate = cfg->frequency / cfg->flow_ratio; // L/min

  // Total_flow (L) += flow rate (L/min) * sample duration (min)
  cfg->total_flow += (cfg->flow_rate * (sample_duration / 60000)); // L

  // Update sensor state
  mgos_bthing_update_state(MGOS_BFLOWSENS_THINGCAST(cfg->sensor), false);
  //mg_bthing_update_state(MGOS_BFLOWSENS_THINGCAST(cfg->sensor));
  //mg_bthing_update_state(MGOS_BFLOWSENS_THINGCAST(cfg->sensor), false);

  if (cfg->pulse_count > 0) {
    // Restart counting pulses
    cfg->pulse_count = 0;
    cfg->last_sample_time = mgos_uptime_micros();
  } else if (sample_duration >= 2000) {
    // No pulses in the last 2 seconds, so I stop timer
    mgos_clear_timer(cfg->timer_id);
    cfg->timer_id = MGOS_INVALID_TIMER_ID;
    cfg->last_sample_time = 0;
  }
}

void mg_bflowsens_gpio_int_handler(int pin, void *arg) {
  struct mg_bflowsens_gpio_cfg *cfg = (struct mg_bflowsens_gpio_cfg *)arg;
  ++cfg->pulse_count;

  if (cfg->timer_id == MGOS_INVALID_TIMER_ID) {
    // start timer on first interrupt
    cfg->last_sample_time = mgos_uptime_micros();
    cfg->timer_id = mgos_set_timer(1000, MGOS_TIMER_REPEAT, mg_bflowsens_gpio_timer_cb, cfg);
  }
}

bool mg_bflowsens_gpio_get_state_cb(mgos_bthing_t thing, mgos_bvar_t state, void *userdata) {
  struct mg_bflowsens_gpio_cfg *cfg = (struct mg_bflowsens_gpio_cfg *)userdata;
  if (thing && state && cfg) {
    mgos_bvar_set_key_decimal(state, "flowRate", cfg->flow_rate);
    mgos_bvar_set_key_decimal(state, "totalFlow", cfg->total_flow);
    return true;
  }
  return false;
}

bool mgos_bflowsens_gpio_attach(mgos_bflowsens_t sensor, 
                                  int pin, enum mgos_gpio_pull_type pull_type,
                                  bool high_pulse, float flow_ratio) {
  if (mgos_gpio_setup_input(pin, pull_type)) {
    if (mgos_gpio_enable_int(pin)) {
      struct mg_bflowsens_gpio_cfg *cfg = calloc(1, sizeof(struct mg_bflowsens_gpio_cfg));
      if (mgos_gpio_set_int_handler(pin, (high_pulse ? MGOS_GPIO_INT_EDGE_POS : MGOS_GPIO_INT_EDGE_NEG ), mg_bflowsens_gpio_int_handler, cfg)) {
        cfg->sensor = sensor;
        cfg->pin = pin;
        cfg->pull_type = pull_type;
        cfg->high_pulse = high_pulse;
        cfg->flow_ratio = flow_ratio;
        cfg->timer_id = MGOS_INVALID_TIMER_ID;

        if(mgos_bthing_on_get_state(MGOS_BFLOWSENS_THINGCAST(sensor), mg_bflowsens_gpio_get_state_cb, cfg)) {
          LOG(LL_INFO, ("bFlowSensor '%s' successfully attached to pin %d",
            mgos_bthing_get_uid(MGOS_BFLOWSENS_THINGCAST(sensor)), pin));
          return true;
        } else {
          LOG(LL_ERROR, ("Error setting the get-state handler of bFlowSensor '%s'",
            mgos_bthing_get_uid(MGOS_BFLOWSENS_THINGCAST(sensor))));
        }
      } else {
        LOG(LL_ERROR, ("Error setting interrupt's handler on pin %d for bFlowSensor '%s'",
          pin, mgos_bthing_get_uid(MGOS_BFLOWSENS_THINGCAST(sensor))));
      }
      free(cfg);
    } else {
      LOG(LL_ERROR, ("Error enabling interrupt on pin %d for bFlowSensor '%s'",
        pin, mgos_bthing_get_uid(MGOS_BFLOWSENS_THINGCAST(sensor))));
    }
  } else {
    LOG(LL_ERROR, ("Error initializing the GPIO pin %d as input for bFlowSensor '%s'",
        pin, mgos_bthing_get_uid(MGOS_BFLOWSENS_THINGCAST(sensor))));
  }
  return false;
}

bool mgos_bflowsens_gpio_init() {
  return true;
}