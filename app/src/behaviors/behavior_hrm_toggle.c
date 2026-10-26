#define DT_DRV_COMPAT zmk_behavior_hrm_toggle

#include <zephyr/device.h>
#include <zephyr/settings/settings.h>
#include <zephyr/kernel.h>
#include <drivers/behavior.h>
#include <zephyr/logging/log.h>
#include <zmk/behavior.h>
#include <zmk/hrm_off.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

static bool hrm_off_state;
static int hrm_off_save_errors;
static struct k_work_delayable hrm_off_save_work;

bool zmk_hrm_off(void) { return hrm_off_state; }

#if IS_ENABLED(CONFIG_SETTINGS)
static void hrm_off_save(struct k_work *work) {
    int rc = settings_save_one("hrm/off", &hrm_off_state, sizeof(hrm_off_state));
    if (rc < 0) {
        hrm_off_save_errors++;
        LOG_ERR("hrm/off save FAILED rc=%d (state %d is RAM only until a later save works)", rc,
                hrm_off_state);
    }
}

static int hrm_off_settings_set(const char *name, size_t len, settings_read_cb read_cb,
                                void *cb_arg) {
    if (strcmp(name, "off") != 0) {
        return -ENOENT;
    }
    bool v;
    if (len != sizeof(v)) {
        LOG_ERR("hrm/off stored with bad size %d, ignoring (HRM stays ON)", (int)len);
        return -EINVAL;
    }
    int rc = read_cb(cb_arg, &v, sizeof(v));
    if (rc < 0) {
        LOG_ERR("hrm/off read failed rc=%d (HRM stays ON)", rc);
        return rc;
    }
    hrm_off_state = v;
    return 0;
}

SETTINGS_STATIC_HANDLER_DEFINE(hrm_off, "hrm", NULL, hrm_off_settings_set, NULL, NULL);
#endif

int zmk_hrm_off_set(bool off) {
    hrm_off_state = off;
#if IS_ENABLED(CONFIG_SETTINGS)
    int rc = k_work_reschedule(&hrm_off_save_work, K_MSEC(CONFIG_ZMK_SETTINGS_SAVE_DEBOUNCE));
    return rc < 0 ? rc : 0;
#else
    return 0;
#endif
}

static int hrm_off_init(void) {
#if IS_ENABLED(CONFIG_SETTINGS)
    k_work_init_delayable(&hrm_off_save_work, hrm_off_save);
#endif
    return 0;
}
SYS_INIT(hrm_off_init, APPLICATION, 40);

#if DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT)
static int on_pressed(struct zmk_behavior_binding *binding,
                      struct zmk_behavior_binding_event event) {
    int rc = zmk_hrm_off_set(!hrm_off_state);
    if (rc < 0) {
        LOG_ERR("hrm toggle: could not schedule save rc=%d", rc);
    }
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
