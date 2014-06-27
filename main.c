#include <stdlib.h>
#include <fcntl.h>
#define GNOME_DESKTOP_USE_UNSTABLE_API 1
#include <gnome-idle-monitor.h>

#define SECOND (1000)
#define DELAY_TIME 10 * SECOND
#define FADE_INTERVAL (50000)
#define FADE_STEPS (15)
#define BACKLIGHT_SYS_FILE "/sys/class/leds/smc::kbd_backlight/brightness"
#define BACKLIGHT_SET_COMMAND "kbdlight set %d\n"

static int saved_backlight_value = 255;
GnomeIdleMonitor *gim; // idle monitor instance

#define MAX_LINE 255
static char line[MAX_LINE];

void set_backlight_value(int value) {
    sprintf(line, BACKLIGHT_SET_COMMAND, value);
    system(line);
}

int get_backlight_value(void) {
    // read the SYS_FILE
    int n;
    char line[MAX_LINE];

    // open the backlight control file
    int backlight_fp = open(BACKLIGHT_SYS_FILE, O_RDONLY);
    n = read(backlight_fp, line, MAX_LINE);
    if (n == -1) {
        g_print("File read error\n");
        return saved_backlight_value;
    }
    n = atoi(line);
    close(backlight_fp);
    return n;
}

void fade_off_backlight(void) {
    // fade the backlight down
    int value = get_backlight_value();
    int fade_step = value / FADE_STEPS;
    while (value > 0) {
        value -= fade_step;
        value = MAX(0, value);
        set_backlight_value(value);
        usleep(FADE_INTERVAL);
    }
}

void fade_up_backlight(int target) {
    // fade the backlight up (we assume from 0)
    int value = 0;
    int fade_step = target / FADE_STEPS;
    while (value < target) {
        value += fade_step;
        value = MIN(target, value);
        set_backlight_value(value);
        usleep(FADE_INTERVAL);
    }
}

void active_watch_func(GnomeIdleMonitor *monitor,
        guint id,
        gpointer user_data) {
    // restore the backlight
    fade_up_backlight(saved_backlight_value);
//    g_print("user active - restored %d\n", saved_backlight_value);
}

void idle_watch_func(GnomeIdleMonitor *monitor,
        guint id,
        gpointer user_data) {
//    g_print("user idle\n");
    // we want to save the existing backlight value
    saved_backlight_value = get_backlight_value();
//    g_print("saved backlight value %d\n", saved_backlight_value);
    // and turn it down to zero
    fade_off_backlight();
    gnome_idle_monitor_add_user_active_watch(gim,
            &active_watch_func,
            NULL,
            NULL);
}

int main(void) {
    g_print("Starting idle time detect\n");

    GMainLoop *loop;

    gim = gnome_idle_monitor_new();

    gnome_idle_monitor_add_idle_watch(gim,
            DELAY_TIME,
            &idle_watch_func,
            NULL,
            NULL);

    loop = g_main_loop_new(NULL, FALSE);
    g_main_loop_run(loop);
    g_main_loop_unref(loop);

    return 0;
}
