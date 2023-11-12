#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include <stdio_ext.h>
#include <stdlib.h>
#include <stdbool.h>
#include <fcntl.h>
#define GROUPARRAYSIZE 10
#define CHANNELARRAYSIZE 10
#define GROUPSIZE 10
#define MESSAGESIZE 128     
#define LISTSIZE (CHANNELARRAYSIZE > GROUPARRAYSIZE ? CHANNELARRAYSIZE : GROUPARRAYSIZE)

struct msgInfo {
    long type;
    long flags;
    long additional;
};
typedef struct msgInfo msgInfo;

struct msgText {
    long type;
    long sender;
    long receiver;
    char text[MESSAGESIZE];
};
typedef struct msgText msgText;

struct msgList {
    long type;
    long flags;
    long list[LISTSIZE];
};
typedef struct msgList msgList;

long Encoding(char[9]);
void MyFlush();
void Mwait();
void Registration(int);
int Login(int, char[9]);
void Logout(int, long);
void SendMessage(int, long, bool);
void GetList(int, int, long, int);
int AddGroup(int, long, long, msgInfo*);
void Add(int, long);
void RemoveGroup(int, long, long, msgInfo*);
void Remove(int, long);
void HandleOneMessage(int, long, bool);
void HandleMessages(int, long);