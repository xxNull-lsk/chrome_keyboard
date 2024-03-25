#include <errno.h>
#include <fcntl.h>
#include <linux/input.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static const char *kbd_event_file = "/dev/input/event2";

static struct
{
    unsigned int scancode;
    unsigned int keycode;
} keys[] = {
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
    {0xb0, KEY_DELETE}, // 音量提升       ==> Delete
};

int main()
{
    int fd, nkeys, i;
    unsigned int buf[2];

    fd = open(kbd_event_file, O_RDWR);
    if (fd < 0)
    {
        fprintf(stderr, "open: %s (%s)\n", kbd_event_file, strerror(errno));
        return 1;
    }
    // 通过ioctl更改键盘映射
    nkeys = sizeof(keys) / sizeof(keys[0]);
    for (i = 0; i < nkeys; i++)
    {
        buf[0] = keys[i].scancode;
        buf[1] = keys[i].keycode;
        if (ioctl(fd, EVIOCSKEYCODE, buf) < 0)
        {
            fprintf(stderr, "ioctl: %x -> %u (%s)\n", buf[0], buf[1],
                    strerror(errno));
        }
    }
    // 使用udevadm使键盘映射生效
    {
        char cmd[128];
        sprintf(cmd, "udevadm trigger %s", kbd_event_file);
        system(cmd);
    }
    return 0;
}