#include <errno.h>
#include <fcntl.h>
#include <linux/input.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static char g_kbd_event_file[1024] = "/dev/input/event1";
typedef struct
{
    unsigned int scancode;
    unsigned int keycode;
} KeyConfig;
static KeyConfig pc_keys[] = {
    {0xea, KEY_F1},     // 后退          ==> F1
    {0xe7, KEY_F2},     // 刷新          ==> F2
    {0x91, KEY_F3},     // 全屏          ==> F3
    {0x92, KEY_F4},     // 缩小          ==> F4
    {0x93, KEY_F5},     // 截屏          ==> F5
    {0x94, KEY_F6},     // 屏幕亮度降低   ==> F6
    {0x95, KEY_F7},     // 屏幕亮度提升   ==> F7
    {0x97, KEY_F8},     // 键盘亮度降低   ==> F8
    {0x98, KEY_F9},     // 键盘亮度提升   ==> F9
    {0x9a, KEY_F10},    // 播放          ==> F10
    {0xa0, KEY_F11},    // 静音          ==> F11
    {0xae, KEY_F12},    // 音量降低       ==> F12
    {0xb0, KEY_INSERT}, // 音量提升       ==> Insert
    {0x5d, KEY_DELETE}, // 休眠          ==> Delete
    {0xb8, KEY_HOME},   // 右Ctrl        ==> Home
    {0x9d, KEY_END},    // 右Alt         ==> End
};

static KeyConfig chrome_keys[] = {
    {0xea, KEY_BACK},           // 后退
    {0xe7, KEY_REFRESH},        // 刷新
    {0x91, KEY_ZOOM},           // 全屏
    {0x92, KEY_SCALE},          // 缩小
    {0x93, KEY_SYSRQ},          // 截屏
    {0x94, KEY_BRIGHTNESSDOWN}, // 屏幕亮度降低
    {0x95, KEY_BRIGHTNESSUP},   // 屏幕亮度提升
    {0x97, KEY_KBDILLUMDOWN},   // 键盘亮度降低
    {0x98, KEY_KBDILLUMUP},     // 键盘亮度提升
    {0x9a, KEY_PLAYPAUSE},      // 播放
    {0xa0, KEY_MUTE},           // 静音
    {0xae, KEY_VOLUMEDOWN},     // 音量降低
    {0xb0, KEY_VOLUMEUP},       // 音量提升
    {0x5d, KEY_SLEEP},          // 休眠
    {0xb8, KEY_RIGHTCTRL},      // 右Ctrl
    {0x9d, KEY_RIGHTALT},       // 右Alt
};

int get_keycode(int scancode)
{
    int fd, i;
    unsigned int buf[2];

    fd = open(g_kbd_event_file, O_RDWR);
    if (fd < 0)
    {
        fprintf(stderr, "open: %s (%s)\n", g_kbd_event_file, strerror(errno));
        return -1;
    }
    buf[0] = scancode;
    buf[1] = 0;
    if (ioctl(fd, EVIOCGKEYCODE, buf) < 0)
    {
        fprintf(stderr, "ioctl: %x -> %u (%s)\n", buf[0], buf[1],
                strerror(errno));
        close(fd);
        return -1;
    }
    close(fd);
    return buf[1];
}

void select_device()
{
    int count = 0;
    while (count < 9999)
    {
        char filename[256];
        sprintf(filename, "/dev/input/event%d", count++);
        if (access(filename, F_OK) < 0)
        {
            break;
        }
        int fd = open(filename, O_RDWR);
        if (fd < 0)
        {
            continue;
        }
        char device_name[128] = {0};
        if (ioctl(fd, EVIOCGNAME(128), device_name) < 0)
        {
            fprintf(stderr, "ioctl failed!%s\n", strerror(errno));
            close(fd);
            continue;
        }
        close(fd);
        printf("[%d]: %s %s\n", count - 1, filename, device_name);
    }
    printf("input device index[0~%d]:", count - 2);
    int index = 0;
    fscanf(stdin, "%d", &index);
    sprintf(g_kbd_event_file, "/dev/input/event%d", index);
}

int main(int argc, char *argv[])
{
    select_device();
    printf("%s\nscancode: keycode(pc, chrome)\n", g_kbd_event_file);
    for (int i = 0; i < sizeof(pc_keys) / sizeof(pc_keys[0]); i++)
    {
        auto k = pc_keys[i];
        auto keycode = get_keycode(k.scancode);
        printf("0x%x: %d(%d, %d)\n", k.scancode, keycode, k.keycode, chrome_keys[i].keycode);
    }
    return 0;
}