#include <errno.h>
#include <fcntl.h>
#include <linux/input.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static char g_kbd_event_file[256] = "/dev/input/event2";
unsigned int scancode2keycode(int fd, unsigned int scancode)
{
    unsigned int buf[2] = {scancode, 0};
    ioctl(fd, EVIOCGKEYCODE, buf);
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
            return;
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

int main()
{
    select_device();
    int fd = open(g_kbd_event_file, O_RDWR);
    if (fd < 0)
    {
        fprintf(stderr, "open: %s (%s)\n", g_kbd_event_file, strerror(errno));
        return 1;
    }
    // 通过ioctl读取键盘映射
    while (1)
    {
        struct input_event ev[64];

        size_t rb = read(fd, ev, sizeof(struct input_event) * 64);
        if (rb < (int)sizeof(struct input_event))
        {
            perror("evtest: short read");
            exit(1);
        }
        for (int i = 0; i < (int)(rb / sizeof(struct input_event)); i++)
        {
            if (ev[i].type == EV_SYN)
            {
                printf("\r-------------EV_SYN------------- \n");
                continue;
            }
            printf("\r%ld.%06ld ",
                   ev[i].time.tv_sec,
                   ev[i].time.tv_usec);
            if (EV_KEY == ev[i].type)
            {
                printf("EV_KEY ");
            }
            else if (EV_MSC == ev[i].type)
            {
                printf("EV_MSC ");
            }
            printf("code %2x(%2d) value %2x(%2d)\n",
                   ev[i].code, ev[i].code,
                   ev[i].value, ev[i].value);
        }
    }
    return 0;
}