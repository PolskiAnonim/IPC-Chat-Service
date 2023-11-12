#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <stdlib.h>
#define GROUPARRAYSIZE 10
#define CHANNELARRAYSIZE 10 
#define GROUPSIZE 10
#define MESSAGESIZE 128 
#define LISTSIZE (CHANNELARRAYSIZE > GROUPARRAYSIZE ? CHANNELARRAYSIZE : GROUPARRAYSIZE)

struct Channel { 
    bool active;
    bool logged;
    char password[17];
    int failed;
    key_t channelNumber;
    long channelOwnerName;
    int channelId;
    bool blockedGroup[GROUPARRAYSIZE];
    bool blockedUsers[CHANNELARRAYSIZE];
};
typedef struct Channel Channel;

struct Group {
    bool active;
    long members[GROUPSIZE];
    long groupName;
};
typedef struct Group Group;

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

Channel ChannelConstructor(key_t, long,char[17]);
Group GroupConstructor(long);
int FindGroup(Group*, long);
int FindUser(Channel*, long);
int FirstAvailableChannel(Channel*, long);
void RegisterUser(int, Channel*, long,char[17]);
void LoginUser(int, Channel*, long,char[17]);
void LogoutUser(int, Channel*, long);
void ChatHandlerUsers(int, msgText, Channel*);
void ChatHandlerGroup(int, msgText, Channel*, Group*);
void GetList(int, int, long, long, Channel*, Group*);
void RemoveGroup(Group*, int, long, long);
void AddGroup(Group*, int , long, long);
void Remove(Group* , int , long , long);
void Add(Group* , int , long , long);
void Block(int, long, long, long, Channel*, Group*);
void BlockedList(int, long, Channel*, Group*);
void MyFlush();
void CloseAll(Channel*);
void SaveCfg(Channel*, Group*);
void LoadCfg(Channel*, Group*);