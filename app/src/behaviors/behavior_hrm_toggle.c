#define DT_DRV_COMPAT zmk_behavior_hrm_toggle

#include <zephyr/device.h>
#include <drivers/behavior.h>
#include <zephyr/logging/log.h>
#include <zmk/behavior.h>
#include <zmk/hrm_off.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

/* Runtime-only flag (not saved): cleared by any firmware restart, including deep-sleep wake. */
static bool hrm_off_state;

bool zmk_hrm_off(void) { return hrm_off_state; }

void zmk_hrm_off_set(bool off) { hrm_off_state = off; }

#if DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT)
static int on_pressed(struct zmk_behavior_binding *binding,
                      struct zmk_behavior_binding_event event) {
    hrm_off_state = !hrm_off_state;
    LOG_INF("HRM %s", hrm_off_state ? "OFF" : "ON");
    return ZMK_BEHAVIOR_OPAQUE;
}
static int on_released(struct zmk_behavior_binding *binding,
                       struct zmk_behavior_binding_event event) {
    return ZMK_BEHAVIOR_OPAQUE;
}
static const struct behavior_driver_api hrm_toggle_api = {
    .binding_pressed = on_pressed,
    .binding_released = on_released,
#if IS_ENABLED(CONFIG_ZMK_BEHAVIOR_METADATA)
    .get_parameter_metadata = zmk_behavior_get_empty_param_metadata,
#endif
};
BEHAVIOR_DT_INST_DEFINE(0, NULL, NULL, NULL, NULL, POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
                        &hrm_toggle_api);
#endif
