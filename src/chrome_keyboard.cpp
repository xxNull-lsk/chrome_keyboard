#include <errno.h>
#include <zmq.h>
#include <fcntl.h>
#include <linux/input.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "ck_types.h"

static const char *kbd_event_file = "/dev/input/event2";
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

static void *g_socket_pub = nullptr;

CK_KEYBOARD_TYPE load_setting()
{
    FILE *f = fopen("/etc/chrome_keyboard", "r");
    if (!f)
    {
        return CK_KEYBOARD_PC;
    }
    char buf[1024] = {0};
    while (fgets(buf, sizeof(buf), f))
    {
        auto mid = strchr(buf, '=');
        *mid = 0;
        if (strcmp(buf, "keyboard_type") == 0)
        {
            fclose(f);
            return (CK_KEYBOARD_TYPE)atoi(mid + 1);
        }
    }
    fclose(f);
    return CK_KEYBOARD_PC;
}

void save_setting(CK_KEYBOARD_TYPE type)
{
    FILE *f = fopen("/etc/chrome_keyboard", "w+");
    if (!f)
    {
        return;
    }
    char buf[1024] = {0};
    sprintf(buf, "keyboard_type=%d", type);
    fwrite(buf, 1, strlen(buf), f);
    fclose(f);
}

bool publish_msg(int id, int value)
{
    if (!g_socket_pub)
    {
        return false;
    }

    zmq_msg_t msg;
    zmq_msg_init_size(&msg, sizeof(CK_REP));
    CK_REP *data_rep = (CK_REP *)zmq_msg_data(&msg);
    data_rep->magic = CK_MAGIC;
    data_rep->id = id;
    data_rep->value = value;
    zmq_sendmsg(g_socket_pub, &msg, 0);
    return true;
}

int get_keycode(int scancode)
{
    int fd, i;
    unsigned int buf[2];

    fd = open(kbd_event_file, O_RDWR);
    if (fd < 0)
    {
        fprintf(stderr, "open: %s (%s)\n", kbd_event_file, strerror(errno));
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

CK_KEYBOARD_TYPE get_keyboard_type()
{
    bool is_pc = true, is_chrome = true;
    for (int i = 0; i < sizeof(pc_keys) / sizeof(pc_keys[0]); i++)
    {
        auto keycode = get_keycode(pc_keys[i].scancode);
        if (keycode != pc_keys[i].keycode)
        {
            is_pc = false;
        }
        if (keycode != chrome_keys[i].keycode)
        {
            is_chrome = false;
        }
    }

    if (is_pc)
    {
        fprintf(stderr, "get_keyboard_type CK_KEYBOARD_PC\n");
        return CK_KEYBOARD_PC;
    }
    else if (is_chrome)
    {
        fprintf(stderr, "get_keyboard_type CK_KEYBOARD_CHROME\n");
        return CK_KEYBOARD_CHROME;
    }
    fprintf(stderr, "get_keyboard_type CK_KEYBOARD_UNKOWN\n");
    return CK_KEYBOARD_UNKOWN;
}

int switch_to_pc_mode()
{
    fprintf(stderr, "switch_to_pc_mode\n");
    int fd, i;
    unsigned int buf[2];
    const int nkeys = sizeof(pc_keys) / sizeof(pc_keys[0]);
    fd = open(kbd_event_file, O_RDWR);
    if (fd < 0)
    {
        fprintf(stderr, "open: %s (%s)\n", kbd_event_file, strerror(errno));
        return 1;
    }
    // 通过ioctl更改键盘映射
    for (i = 0; i < nkeys; i++)
    {
        buf[0] = pc_keys[i].scancode;
        buf[1] = pc_keys[i].keycode;
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
    save_setting(CK_KEYBOARD_PC);
    publish_msg(CK_FUNC_GET_KEYBOARD_TYPE, CK_KEYBOARD_PC);
    return 0;
}

int switch_to_chromebook_mode()
{
    fprintf(stderr, "switch_to_chromebook_mode\n");
    int fd, i;
    unsigned int buf[2];
    const int nkeys = sizeof(chrome_keys) / sizeof(chrome_keys[0]);
    fd = open(kbd_event_file, O_RDWR);
    if (fd < 0)
    {
        fprintf(stderr, "open: %s (%s)\n", kbd_event_file, strerror(errno));
        return 1;
    }
    // 通过ioctl更改键盘映射
    for (i = 0; i < nkeys; i++)
    {
        buf[0] = chrome_keys[i].scancode;
        buf[1] = chrome_keys[i].keycode;
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
    save_setting(CK_KEYBOARD_CHROME);
    publish_msg(CK_FUNC_GET_KEYBOARD_TYPE, CK_KEYBOARD_CHROME);
    return 0;
}

bool start_publisher(void *context)
{
    g_socket_pub = zmq_socket(context, ZMQ_PUB);
    int rc = zmq_bind(g_socket_pub, "tcp://*:" CK_ADDR_PUB);
    if (rc != 0)
    {
        fprintf(stderr, "start publisher failed!err:%s\n", zmq_strerror(zmq_errno()));
        zmq_close(g_socket_pub);
        g_socket_pub = nullptr;
        return false;
    }
    fprintf(stderr, "start publisher succeed!port=%s\n", CK_ADDR_PUB);
    return true;
}

bool start_listener(void *context)
{
    void *socket_rep = zmq_socket(context, ZMQ_REP);
    int rc = zmq_bind(socket_rep, "tcp://*:" CK_ADDR_REP);
    if (rc != 0)
    {
        fprintf(stderr, "start listener failed!errno=%d, %s\n", zmq_errno(), zmq_strerror(zmq_errno()));
        zmq_close(socket_rep);
        socket_rep = nullptr;
        return false;
    }
    fprintf(stderr, "start listener succeed!port=%s\n", CK_ADDR_REP);
    zmq_msg_t msg_rep;
    zmq_msg_t msg_req;
    zmq_msg_init(&msg_req);
    while (true)
    {
        // fprintf(stderr, "recv msg_req.....\n");
        int ret = zmq_recvmsg(socket_rep, &msg_req, 0);
        if (ret < 0)
        {
            fprintf(stderr, "recv msg_req failed!err:%d,%s\n", zmq_errno(), zmq_strerror(zmq_errno()));
            break;
        }
        // fprintf(stderr, "recv msg_req succeed\n");

        CK_REQ *req = (CK_REQ *)zmq_msg_data(&msg_req);
        int len = zmq_msg_size(&msg_req);
        ret = -1;
        if (len == sizeof(CK_REQ) && req->magic == CK_MAGIC)
        {
            // fprintf(stderr, "recv func: %d\n", req->id);
            switch (req->id)
            {
            case CK_FUNC_GET_KEYBOARD_TYPE:
                ret = get_keyboard_type();
                break;
            case CK_FUNC_SWITCH_TO_PC_MODE:
                ret = switch_to_pc_mode();
                break;
            case CK_FUNC_SWITCH_TO_CHROME_MODE:
                ret = switch_to_chromebook_mode();
                break;

            default:
                break;
            }
        }

        zmq_msg_init_size(&msg_rep, sizeof(CK_REP));
        CK_REP *data_rep = (CK_REP *)zmq_msg_data(&msg_rep);
        data_rep->magic = CK_MAGIC;
        data_rep->id = req->id;
        data_rep->value = ret;
        ret = zmq_sendmsg(socket_rep, &msg_rep, 0);
        if (ret < 0)
        {
            fprintf(stderr, "send msg_rep failed!err:%s\n", zmq_strerror(zmq_errno()));
            break;
        }
    }
    zmq_msg_close(&msg_rep);
    zmq_msg_close(&msg_req);
    zmq_close(socket_rep);
    socket_rep = nullptr;
    return true;
}

int main()
{
    if (get_keyboard_type() == CK_KEYBOARD_UNKOWN)
    {
        fprintf(stderr, "ERROR: Your laptop type does not match. Unable to use this program.\n");
        return 1;
    }
    void *context = zmq_ctx_new();
    switch (load_setting())
    {
    case CK_KEYBOARD_PC:
        switch_to_pc_mode();
        break;
    case CK_KEYBOARD_CHROME:
        switch_to_chromebook_mode();
        break;
    default:
        break;
    }
    start_publisher(context);
    while (1)
    {
        start_listener(context);
        sleep(1);
    }

    zmq_ctx_destroy(context);
    return 0;
}