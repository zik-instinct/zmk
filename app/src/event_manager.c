/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#include <zmk/event_manager.h>
#include <zmk/events/position_state_changed.h>

// Gactyl recorder ingress hook: weak no-op, overridden by the gactyl module.
// Called exactly once per original position event (local or split-peripheral),
// before any listener (including hold-tap capture). Replayed events re-raised
// via raise_at/raise_after do NOT pass through here.
__weak void zmk_gactyl_on_position_event(uint8_t source, uint32_t position, bool state,
                                         int64_t timestamp) {}

extern struct zmk_event_type *__event_type_start[];
extern struct zmk_event_type *__event_type_end[];

extern struct zmk_event_subscription __event_subscriptions_start[];
extern struct zmk_event_subscription __event_subscriptions_end[];

int zmk_event_manager_handle_from(zmk_event_t *event, uint8_t start_index) {
    int ret = 0;
    uint8_t len = __event_subscriptions_end - __event_subscriptions_start;
    for (int i = start_index; i < len; i++) {
        struct zmk_event_subscription *ev_sub = __event_subscriptions_start + i;
        if (ev_sub->event_type != event->event) {
            continue;
        }
        event->last_listener_index = i;
        ret = ev_sub->listener->callback(event);
        switch (ret) {
        case ZMK_EV_EVENT_BUBBLE:
            continue;
        case ZMK_EV_EVENT_HANDLED:
            LOG_DBG("Listener handled the event");
            return 0;
        case ZMK_EV_EVENT_CAPTURED:
            LOG_DBG("Listener captured the event");
            return 0;
        default:
            LOG_DBG("Listener returned an error: %d", ret);
            return ret;
        }
    }

    return 0;
}

int zmk_event_manager_raise(zmk_event_t *event) {
    if (event->event == &zmk_event_zmk_position_state_changed) {
        struct zmk_position_state_changed *ev = as_zmk_position_state_changed(event);
        if (ev != NULL) {
            zmk_gactyl_on_position_event(ev->source, ev->position, ev->state, ev->timestamp);
        }
    }
    return zmk_event_manager_handle_from(event, 0);
}

int zmk_event_manager_raise_after(zmk_event_t *event, const struct zmk_listener *listener) {
    uint8_t len = __event_subscriptions_end - __event_subscriptions_start;
    for (int i = 0; i < len; i++) {
        struct zmk_event_subscription *ev_sub = __event_subscriptions_start + i;

        if (ev_sub->event_type == event->event && ev_sub->listener == listener) {
            return zmk_event_manager_handle_from(event, i + 1);
        }
    }

    LOG_WRN("Unable to find where to raise this after event");

    return -EINVAL;
}

int zmk_event_manager_raise_at(zmk_event_t *event, const struct zmk_listener *listener) {
    uint8_t len = __event_subscriptions_end - __event_subscriptions_start;
    for (int i = 0; i < len; i++) {
        struct zmk_event_subscription *ev_sub = __event_subscriptions_start + i;

        if (ev_sub->event_type == event->event && ev_sub->listener == listener) {
            return zmk_event_manager_handle_from(event, i);
        }
    }

    LOG_WRN("Unable to find where to raise this event");

    return -EINVAL;
}

int zmk_event_manager_release(zmk_event_t *event) {
    return zmk_event_manager_handle_from(event, event->last_listener_index + 1);
}
