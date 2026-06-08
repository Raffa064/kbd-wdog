#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <dirent.h>
#include <fcntl.h>
#include <poll.h>
#include <sys/ioctl.h>
#include <linux/input.h>
#include <limits.h>

#define DEV_INPUT_DIR  "/dev/input"
#define DEV_NAME       "TPPS/2 Elan TrackPoint"
#define POLL_MS        50
#define HOLD_MS        400
#define COOLDOWN_MS    1000

static volatile int running = 1;

static void msg(const char *s)
{
    fprintf(stdout, "%s\n", s);
    fflush(stdout);
}

static void handle_signal(int sig)
{
    (void)sig;
    running = 0;
}

static long long now_ms(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (long long)ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
}

struct dev {
    char path[PATH_MAX];
    int fd;
};

static int find_device(const char *target, struct dev *d)
{
    DIR *dir = opendir(DEV_INPUT_DIR);
    if (!dir) return -1;

    struct dirent *entry;
    int found = -1;

    while ((entry = readdir(dir)) != NULL)
    {
        if (strncmp(entry->d_name, "event", 5) != 0)
            continue;

        char fullpath[PATH_MAX];
        snprintf(fullpath, sizeof(fullpath), "%s/%s", DEV_INPUT_DIR, entry->d_name);

        int fd = open(fullpath, O_RDONLY);
        if (fd < 0) continue;

        char devname[256];
        memset(devname, 0, sizeof(devname));
        if (ioctl(fd, EVIOCGNAME(sizeof(devname)), devname) >= 0)
        {
            if (strcmp(devname, target) == 0)
            {
                strncpy(d->path, fullpath, sizeof(d->path));
                d->path[sizeof(d->path)-1] = '\0';
                d->fd = -1;
                found = 0;
            }
        }
        close(fd);
        if (found == 0) break;
    }
    closedir(dir);
    return found;
}

static int open_dev(struct dev *d)
{
    d->fd = open(d->path, O_RDONLY);
    if (d->fd < 0) return -1;
    return 0;
}

static void close_dev(struct dev *d)
{
    if (d->fd >= 0)
    {
        close(d->fd);
        d->fd = -1;
    }
}

static void run_fix(void)
{
    msg("watchdog: executing fix script");
    int ret = system("/usr/local/lib/kbd-wdog/fix");
    if (ret == -1)
        msg("watchdog: system() failed");
    else if (WIFEXITED(ret))
    {
        char buf[64];
        snprintf(buf, sizeof(buf), "watchdog: ./fix done (exit %d)", WEXITSTATUS(ret));
        msg(buf);
    }
}

int main(void)
{
    signal(SIGINT, handle_signal);
    signal(SIGTERM, handle_signal);

    struct dev dev;
    memset(&dev, 0, sizeof(dev));

    if (find_device(DEV_NAME, &dev) != 0)
    {
        msg("watchdog: device not found");
        return 1;
    }

    msg("watchdog: ready");

    while (running)
    {
        if (dev.fd < 0)
        {
            if (open_dev(&dev) != 0)
            {
                msg("watchdog: failed to open, retry in 3s");
                struct timespec ts = { .tv_sec = 3 };
                while (running && nanosleep(&ts, &ts) != 0);
                continue;
            }
        }

        int left_down = 0, right_down = 0;
        long long both_pressed_at = 0;
        long long last_fix_at = 0;
        int was_triggered = 0;

        while (running)
        {
            struct pollfd pfd = { .fd = dev.fd, .events = POLLIN };
            int pret = poll(&pfd, 1, POLL_MS);

            if (pret < 0)
            {
                if (errno == EINTR) continue;
                msg("watchdog: poll error");
                break;
            }

            if (pret == 0)
            {
                if (left_down && right_down && !was_triggered)
                {
                    if (both_pressed_at == 0)
                        both_pressed_at = now_ms();

                    long long elapsed = now_ms() - both_pressed_at;

                    if (elapsed >= HOLD_MS)
                    {
                        long long since_last = now_ms() - last_fix_at;

                        if (since_last >= COOLDOWN_MS)
                        {
                            char buf[128];
                            snprintf(buf, sizeof(buf), "watchdog: both held %lldms", elapsed);
                            msg(buf);
                            run_fix();
                            last_fix_at = now_ms();
                        }
                        was_triggered = 1;
                    }
                }
                continue;
            }

            struct input_event ev;
            ssize_t n = read(dev.fd, &ev, sizeof(ev));

            if (n < 0)
            {
                msg("watchdog: read error");
                break;
            }

            if (n != sizeof(ev)) continue;

            if (ev.type == EV_KEY)
            {
                if (ev.code == BTN_LEFT)
                {
                    left_down = ev.value;
                    char buf[128];
                    snprintf(buf, sizeof(buf), "watchdog: BTN_LEFT=%s", ev.value ? "DOWN" : "UP");
                    msg(buf);
                }
                else if (ev.code == BTN_RIGHT)
                {
                    right_down = ev.value;
                    char buf[128];
                    snprintf(buf, sizeof(buf), "watchdog: BTN_RIGHT=%s", ev.value ? "DOWN" : "UP");
                    msg(buf);
                }
            }

            if (left_down && right_down)
            {
                if (!was_triggered)
                {
                    if (both_pressed_at == 0)
                        both_pressed_at = now_ms();

                    long long elapsed = now_ms() - both_pressed_at;

                    if (elapsed >= HOLD_MS)
                    {
                        long long since_last = now_ms() - last_fix_at;

                        if (since_last >= COOLDOWN_MS)
                        {
                            char buf[128];
                            snprintf(buf, sizeof(buf), "watchdog: both held %lldms", elapsed);
                            msg(buf);
                            run_fix();
                            last_fix_at = now_ms();
                        }
                        was_triggered = 1;
                    }
                }
            }
            else
            {
                both_pressed_at = 0;
                was_triggered = 0;
            }
        }

        close_dev(&dev);
    }

    msg("watchdog: exiting");
    return 0;
}
