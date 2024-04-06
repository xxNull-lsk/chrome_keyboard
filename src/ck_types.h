#ifndef CK_TYPE_H
#define CK_TYPE_H

#pragma pack(1)

#define CK_FUNC_GET_KEYBOARD_TYPE      1
#define CK_FUNC_SWITCH_TO_PC_MODE      2
#define CK_FUNC_SWITCH_TO_CHROME_MODE  3

#define CK_MAGIC  0x1a2c3e4f

typedef struct{
    int32_t magic; // CK_MAGIC
    int32_t id;
}CK_REQ;


typedef struct{
    int32_t magic; // CK_MAGIC
    int32_t id;
    int32_t value;
}CK_REP;

typedef enum
{
     CK_KEYBOARD_UNKOWN = 0,
     CK_KEYBOARD_PC = 1,
     CK_KEYBOARD_CHROME = 2,
} CK_KEYBOARD_TYPE;

#define CK_ADDR_PUB "7555"
#define CK_ADDR_REP "7556"
#pragma pack()
#endif //CK_TYPE_H