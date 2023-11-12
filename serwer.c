#include "serwer.h"

Channel ChannelConstructor(key_t channelNumber, long ownerName,char password[17]) {
    Channel newChannel;
    newChannel.active = true;
    newChannel.channelNumber = channelNumber;
    newChannel.channelOwnerName = ownerName;
    newChannel.channelId = msgget(channelNumber, IPC_CREAT | 0600);
    newChannel.logged = false;
    newChannel.failed = 0;
    strcpy(newChannel.password, password);
    for (int i = 0; i < GROUPARRAYSIZE; ++i)
        newChannel.blockedGroup[i] = 0;
    for (int i = 0; i < CHANNELARRAYSIZE; ++i)
        newChannel.blockedUsers[i] = 0;
    return newChannel;
}

Group GroupConstructor(long name)
{
    Group newGroup;
    newGroup.active = true;
    newGroup.groupName = name;
    for (int i = 0; i < GROUPSIZE; ++i)
        newGroup.members[i] = 0;
    return newGroup;
}

int FindGroup(Group* groupArray,long groupName)
{
    for (int i = 0; i != CHANNELARRAYSIZE; i++) {
        if (groupArray[i].active == false) continue;
        if (groupArray[i].groupName == groupName) return i;
    }
    return -1;
}

int FindUser(Channel* channelArray, long userName) {
    for (int i = 0; i != CHANNELARRAYSIZE; i++) {
        if (channelArray[i].active == false) continue;
        if (channelArray[i].channelOwnerName == userName) return i;
    }
    return -1;
}
//Register user
int FirstAvailableChannel(Channel* channelArray, long ownerName) {
    int ix=-1;
    for (int i = CHANNELARRAYSIZE-1; i >=0; --i) {
        if (channelArray[i].active == false) ix=i;
        if (channelArray[i].channelOwnerName == ownerName) return -2; // User already exists
    }
    return ix;
}

void RegisterUser(int server, Channel* channelArray, long userName,char password[17]) {
    int channelToOpen = FirstAvailableChannel(channelArray, userName);

    msgInfo feedback;

    switch (channelToOpen) {
    case -1:
        feedback.flags = 12;
        break;
    case -2:
        feedback.flags = 11;
        break;
    default:
        //create IPC
        channelArray[channelToOpen] = ChannelConstructor((key_t)(channelToOpen + 1001), userName,password);

        feedback.additional = 0; 
        feedback.flags = 10;
        break;
    }
    feedback.type = userName;
    // Send back register feedback
    msgsnd(server, &feedback, sizeof(msgInfo) - sizeof(long),0);
}
//login + logout
void LoginUser(int server, Channel* channelArray, long userName,char password[17]) {
    int userArrayPosition = FindUser(channelArray, userName);

    msgInfo feedback;
    feedback.type = userName;
    feedback.additional = 0;

    switch (userArrayPosition)
    {
    case -1:
        feedback.flags = 21;
        break;
    default:
        if (channelArray[userArrayPosition].logged == true)
            feedback.flags = 22;
        else { 
            if (strcmp(password, channelArray[userArrayPosition].password) == 0) //if password is the same
            {
                feedback.flags = 20;
                channelArray[userArrayPosition].failed = 0;
                channelArray[userArrayPosition].logged = true;
                feedback.additional = (long)channelArray[userArrayPosition].channelNumber;
            }
            else //if passwords are different
            {
                channelArray[userArrayPosition].failed++;
                if (channelArray[userArrayPosition].failed == 4)
                {
                    channelArray[userArrayPosition].active = false;
                    msgctl(channelArray[userArrayPosition].channelId, IPC_RMID, NULL);
                }
                feedback.flags = 23;
                feedback.additional = 4 - channelArray[userArrayPosition].failed;
            }
        }
        break;
    }
    // Send back login feedback
    msgsnd(server, &feedback, sizeof(msgInfo) - sizeof(long), 0);
}

void LogoutUser(int server,Channel* channelArray,long userName) {
    int userArrayPosition = FindUser(channelArray, userName);

    msgInfo feedback;
    feedback.type = userName;
    feedback.flags = 30;
    feedback.additional = 0;
    channelArray[userArrayPosition].logged = false;
    // Send back logout feedback
    msgsnd(server, &feedback, sizeof(msgInfo) - sizeof(long), 0);
}
//users chat
void ChatHandlerUsers(int server,msgText msg,Channel* channelArray)
{
    msg.type = 11;
    msgInfo feedback;
    feedback.additional = 0;
    feedback.type = msg.sender;
    int index = FindUser(channelArray, msg.receiver);
    int snd = FindUser(channelArray, msg.sender);
    if (index == -1)
    {
        feedback.flags = 111;
    }
    else
    {
        feedback.flags = 110;
        if (channelArray[index].blockedUsers[snd] == true)
            feedback.flags = 113;
        else if (msgsnd(channelArray[index].channelId, &msg, sizeof(msgText) - sizeof(long), IPC_NOWAIT) == -1)
            feedback.flags = 112;
    }
    msgsnd(server, &feedback, sizeof(msgInfo) - sizeof(long), 0);
}
//group chat
void ChatHandlerGroup(int server, msgText msg, Channel* channelArray,Group* groupArray)
{
    int count=0, usr = -1;
    msg.type = 12;
    msgList feedback;
    feedback.type = msg.sender;
    int index = FindGroup(groupArray, msg.receiver);
    if (index == -1)
        feedback.flags = 121;
    else
    {
        feedback.flags = 120;
        for (int i = 0; i < GROUPSIZE; ++i)
            if (groupArray[index].members[i] != 0)
                if (groupArray[index].members[i] == msg.sender)
                    usr = i;
        if (usr != -1) {
            for (int i = 0; i < GROUPSIZE; ++i)
                if (groupArray[index].members[i] != 0)
                {
                    int u = FindUser(channelArray, groupArray[index].members[i]);
                    if (channelArray[u].blockedGroup[index]==true || 
                    msgsnd(channelArray[u].channelId, &msg, sizeof(msgList) - sizeof(long), IPC_NOWAIT) == -1)
                    {
                        feedback.list[count++]= groupArray[index].members[i];
                        feedback.flags = 123;
                    }
                }
            for (int i = count; i < GROUPSIZE; ++i)
                feedback.list[i] = 0;
        }
        else
            feedback.flags = 122;
    }

    msgsnd(server, &feedback, sizeof(msgList) - sizeof(long), 0);
}
//lists
void GetList(int server, int t, long userName,long groupName, Channel* channelArray, Group* groupArray)
{
    msgInfo feedback;
    feedback.additional = 0;
    feedback.type = userName;

    msgList msg;
    int count = 0;
    msg.type = 31;
    int groupid = -1;

    switch (t)
    {
    case 6: //users list
        for (int i = 0; i < CHANNELARRAYSIZE; ++i)
            if (channelArray[i].logged == true)
                msg.list[count++] = channelArray[i].channelOwnerName;
        break;
    case 7://groups list
        for (int i = 0; i < GROUPARRAYSIZE; ++i)
            if (groupArray[i].active == true)
                msg.list[count++] = groupArray[i].groupName;
        break;
    case 8://group members list
        groupid = FindGroup(groupArray, groupName);
        if (groupid == -1)
            break;
        for (int i = 0; i < GROUPSIZE; ++i)
            if (groupArray[groupid].members[i] != 0)
                msg.list[count++] = groupArray[groupid].members[i];
    }
    //fill rest of the list with 0
    for (int i = count; i < sizeof(msg.list) / sizeof(msg.list[0]); ++i)
        msg.list[i] = 0;

    feedback.flags = 10 * t;
    int sender = FindUser(channelArray, userName);
    if (groupid == -1 && t==8)
    {
        feedback.flags = 82;
    }
    else if (msgsnd(channelArray[sender].channelId, &msg, sizeof(msgList) - sizeof(long), IPC_NOWAIT) == -1)
        feedback.flags = t * 10 + 1;
    msgsnd(server, &feedback, sizeof(msgInfo) - sizeof(long), 0);
}
//add user as group member
void Add(Group* groupArray,int server,long user,long group)
{
    int groupid = FindGroup(groupArray, group);
    int usr = -1;
    int fstfr = -1;
    msgInfo feedback;
    feedback.type = user;
    feedback.additional = 0;
    feedback.flags = 130;
    if (groupid == -1)
        feedback.flags = 131;
    else{
        for (int i = GROUPSIZE - 1; i >=0; --i)
        {
            if (groupArray[groupid].members[i] == 0) fstfr = i;
            if (groupArray[groupid].members[i] == user) usr = i;
        }
        if (fstfr == -1)
            feedback.flags = 133;
        else
        {
            if (usr != -1)
                feedback.flags = 132;
            else
                groupArray[groupid].members[fstfr] = user;
        }
    }

    msgsnd(server, &feedback, sizeof(msgInfo) - sizeof(long), 0);
}
//remove user from group
void Remove(Group* groupArray, int server, long user, long group)
{
    int groupid = FindGroup(groupArray, group);
    int usr = -1;
    int count0 = 0;
    msgInfo feedback;
    feedback.type = user;
    feedback.additional = 0;
    feedback.flags = 140;
    if (groupid == -1)
        feedback.flags = 141;
    else {
        for (int i = 0; i < GROUPSIZE; ++i)
        {
            if (groupArray[groupid].members[i] == 0) count0++;
            if (groupArray[groupid].members[i] == user) usr = i;
        }
        if (usr == -1)
            feedback.flags = 142;
        else
        {
            groupArray[groupid].members[usr] = 0;
            count0++;
        }
    }
    if (count0 == GROUPSIZE)
        feedback.additional = 145;
    msgsnd(server, &feedback, sizeof(msgInfo) - sizeof(long), 0);
}

void AddGroup(Group* groupArray, int server, long user, long group)
{
    int fstfr = -1;
    msgInfo feedback;
    feedback.type = user;
    feedback.additional = 0;
    feedback.flags = 150;
    for (int i = GROUPSIZE - 1; i >= 0; --i)
        if (groupArray[i].active == false) fstfr = i;

    if (fstfr == -1) feedback.flags = 151;
    else groupArray[fstfr] = GroupConstructor(group);

    msgsnd(server, &feedback, sizeof(msgInfo) - sizeof(long), 0);
}

void RemoveGroup(Group* groupArray, int server, long user, long group)
{
    int groupid = FindGroup(groupArray, group);

    msgInfo feedback;
    feedback.type = user;
    feedback.additional = 0;
    feedback.flags = 160;
    if (groupid != -1)
        groupArray[groupid].active = false;
    else
        feedback.flags = 161;
    msgsnd(server, &feedback, sizeof(msgInfo) - sizeof(long), 0);
}

void Block(int server,long user,long toblock,long type,Channel* channelArray, Group* groupArray)
{
    int arrayIndex = FindUser(channelArray,user);
    int arrayBlock=-1;
    int usr=-1;

    msgInfo feedback;
    feedback.type = user;
    feedback.additional = 0;

    if (type == 17)//block user
    {
        arrayBlock = FindUser(channelArray, toblock);
        if (arrayBlock != -1)
        {
            feedback.flags = 170;
            channelArray[arrayIndex].blockedUsers[arrayBlock] = !channelArray[arrayIndex].blockedUsers[arrayBlock];
            if (channelArray[arrayIndex].blockedUsers[arrayBlock] == true)
                feedback.additional = 175;
            else
                feedback.additional = 176;
        }
        else
            feedback.flags = 171;
    }
    if (type == 18)//block group
    {
        arrayBlock = FindGroup(groupArray, toblock);
        if (arrayBlock != -1)
        {
            for (int i = 0; i < GROUPSIZE; ++i)
            {
                if (groupArray[arrayBlock].members[i] == user) usr = i;
            }
            if (usr == -1)
                feedback.flags = 182;
            else
            {
                feedback.flags = 180;
                channelArray[arrayIndex].blockedGroup[arrayBlock] = !channelArray[arrayIndex].blockedGroup[arrayBlock];
                if (channelArray[arrayIndex].blockedGroup[arrayBlock]==true)
                    feedback.additional = 185;
                else
                    feedback.additional = 186;
            }
        }
        else
            feedback.flags = 181;
    }

    msgsnd(server, &feedback, sizeof(msgInfo) - sizeof(long), 0);
}

void BlockedList(int server, long user, Channel* channelArray, Group* groupArray)
{
    msgInfo feedback;
    msgList groups;
    msgList users;
    feedback.flags = 190;
    feedback.type = user;
    feedback.additional = 0;
    int count = 0;
    int index = FindUser(channelArray, user);
    //groups
    for (int i = 0; i < GROUPARRAYSIZE; ++i)
        if (channelArray[index].blockedGroup[i] == true)
            groups.list[count++] = groupArray[i].groupName;

    for (int i = count; i < sizeof(groups.list) / sizeof(groups.list[0]); ++i)
        groups.list[i] = 0;
    //users
    count = 0;
    for (int i = 0; i < CHANNELARRAYSIZE; ++i)
        if (channelArray[index].blockedUsers[i] == true)
            users.list[count++] = channelArray[i].channelOwnerName;

    for (int i = count; i < sizeof(groups.list) / sizeof(groups.list[0]); ++i)
        users.list[i] = 0;

    groups.type = 33;
    groups.flags = 0;
    users.type = 32;
    users.flags = 0;
    if (msgsnd(channelArray[index].channelId, &users, sizeof(msgList) - sizeof(long), IPC_NOWAIT) == -1)
    {
        feedback.flags = 191;
        feedback.additional = 195;
    }
    if (msgsnd(channelArray[index].channelId, &groups, sizeof(msgList) - sizeof(long), IPC_NOWAIT) == -1)
    {
        feedback.flags = 191;
        if (feedback.additional==0)
            feedback.additional = 196;
        else
            feedback.additional = 197;
    }
    msgsnd(server, &feedback, sizeof(msgInfo) - sizeof(long), 0);
}

void MyFlush() {
    int flags = fcntl(1, F_GETFL);
    fcntl(1, F_SETFL, flags | O_NONBLOCK);
    while (getchar() != EOF); // flush stdin
    fcntl(1, F_SETFL, flags);
}

int main() {

    bool c = false;
    bool p = true;
    while (p)
    {
        printf("Wczytać konfigurację początkową? y/n: ");
        char x = getchar();
        switch (x)
        {
        case 'y':
            c = true;
            p = false;
            break;
        case 'n':
            p = false;
            break;
        default:
            printf("Brak opcji\n");
        }
        MyFlush();
    }

    int server = msgget(999, 0600 | IPC_CREAT);

    if (fork())
    {
        Channel* channelArray = malloc(sizeof(Channel) * CHANNELARRAYSIZE);
        Group* groupArray = malloc(sizeof(Group) * GROUPARRAYSIZE);
        for (int i = 0; i < CHANNELARRAYSIZE; i++)
        {
            channelArray[i].active = false;
            channelArray[i].channelOwnerName = 0;
        }
        for (int i = 0; i < GROUPARRAYSIZE; i++)
        {
            groupArray[i].active = false;
        }

        if (c)
            LoadCfg(channelArray,groupArray);

        msgInfo msgInfo;
        msgText msgText;
        while (1) {
            // 1 - REJESTRACJA | 2 - LOGOWANIE | 3 - WYLOGOWANIE
            if (msgrcv(server, &msgText, sizeof(msgText) - sizeof(long), 1, IPC_NOWAIT) != -1)
                RegisterUser(server, channelArray, msgText.sender,msgText.text);
            if (msgrcv(server, &msgText, sizeof(msgText) - sizeof(long), 2, IPC_NOWAIT) != -1)
                LoginUser(server, channelArray, msgText.sender,msgText.text);
            if (msgrcv(server, &msgInfo, sizeof(msgInfo) - sizeof(long), 3, IPC_NOWAIT) != -1)
                LogoutUser(server, channelArray, msgInfo.flags);
            //5 - WYŁĄCZENIE SERWERA
            if (msgrcv(server, &msgInfo, sizeof(msgInfo) - sizeof(long), 5, IPC_NOWAIT) != -1)
            {
                if (msgInfo.flags == 51)
                    SaveCfg(channelArray, groupArray);
                CloseAll(channelArray);
                free(channelArray);
                free(groupArray);
                msgctl(server, IPC_RMID, NULL);
                return 0;
            }
            //6,7,8 - LISTY (ZALOGOWANI, GRUPY, CZŁONKÓW)
            if (msgrcv(server, &msgInfo, sizeof(msgInfo) - sizeof(long), 6, IPC_NOWAIT) != -1)
                GetList(server, msgInfo.type, msgInfo.flags,0 ,channelArray,groupArray);
            if (msgrcv(server, &msgInfo, sizeof(msgInfo) - sizeof(long), 7, IPC_NOWAIT) != -1)
                GetList(server, msgInfo.type, msgInfo.flags,0,channelArray, groupArray);
            if (msgrcv(server, &msgInfo, sizeof(msgInfo) - sizeof(long), 8, IPC_NOWAIT) != -1)
                GetList(server, msgInfo.type, msgInfo.flags,msgInfo.additional, channelArray, groupArray);
            // 11 - WIADOMOŚCI MIĘDZY UŻYTKOWNIKAMI | 12 - WIADOMOŚCI GRUPOWE
            if (msgrcv(server, &msgText, sizeof(msgText) - sizeof(long), 11, IPC_NOWAIT) != -1)
                ChatHandlerUsers(server,msgText,channelArray);
            if (msgrcv(server, &msgText, sizeof(msgText) - sizeof(long), 12, IPC_NOWAIT) != -1)
                ChatHandlerGroup(server, msgText, channelArray,groupArray);           
            //13 - DOPISANIE DO GRUPY | 14 - WYPISANIE Z GRUPY 
            if (msgrcv(server, &msgInfo, sizeof(msgInfo) - sizeof(long), 13, IPC_NOWAIT) != -1)
                Add(groupArray,server, msgInfo.flags,msgInfo.additional);
            if (msgrcv(server, &msgInfo, sizeof(msgInfo) - sizeof(long), 14, IPC_NOWAIT) != -1)
                Remove(groupArray, server, msgInfo.flags, msgInfo.additional);
            //15 - STWORZENIE GRUPY | 16 - USUNIĘCIE GRUPY
            if (msgrcv(server, &msgInfo, sizeof(msgInfo) - sizeof(long), 15, IPC_NOWAIT) != -1)
                AddGroup(groupArray, server, msgInfo.flags, msgInfo.additional);
            if (msgrcv(server, &msgInfo, sizeof(msgInfo) - sizeof(long), 16, IPC_NOWAIT) != -1)
                RemoveGroup(groupArray, server, msgInfo.flags, msgInfo.additional);
            //17 - BLOKOWANIE/ODBLOKOWANIE WIADOMOŚCI UŻYTKOWNIKÓW | 18 - <- GRUP | 19 - LISTY ZABLOKOWANYCH
            if (msgrcv(server, &msgInfo, sizeof(msgInfo) - sizeof(long), 17, IPC_NOWAIT) != -1)
                Block(server, msgInfo.flags, msgInfo.additional, msgInfo.type,channelArray,groupArray);
            if (msgrcv(server, &msgInfo, sizeof(msgInfo) - sizeof(long), 18, IPC_NOWAIT) != -1)
                Block(server, msgInfo.flags, msgInfo.additional, msgInfo.type,channelArray,groupArray);
            if (msgrcv(server, &msgInfo, sizeof(msgInfo) - sizeof(long), 19, IPC_NOWAIT) != -1)
                BlockedList(server, msgInfo.flags, channelArray, groupArray);
        }
    }
    
    msgInfo destroy;
    destroy.type = 5;
    while (1)
    {
        MyFlush();
        printf("1. Wyłączenie serwera z zapisem konfiguracji\n2. Wyłączenie serwera bez zapisu\nPodaj opcje: ");
        char c = getchar();
        switch (c)
        {
        case '1':
            destroy.flags = 51;
            goto leave;
        case '2':
            destroy.flags = 52;
            goto leave;
        default:
            printf("Brak opcji");
            MyFlush();
        }
    }
    leave:

    msgsnd(server, &destroy, sizeof(msgInfo) - sizeof(long), 0);

    return 0;
}



void CloseAll(Channel* channelArray) {
    for (int i = 0; i != CHANNELARRAYSIZE; i++)
        msgctl(channelArray[i].channelId, IPC_RMID, NULL);
}

void SaveCfg(Channel* channelArray,Group* groupArray)
{
    int idch = open("channels.cfg", O_WRONLY | O_CREAT,0600);
    int idgr = open("groups.cfg", O_WRONLY | O_CREAT, 0600);
    for (int i = 0; i < CHANNELARRAYSIZE; i++)
    {
        char buf[sizeof(Channel)];
        memcpy(&buf, &channelArray[i], sizeof(Channel));
        write(idch, &buf, sizeof(Channel));
    }
    for (int i = 0; i < GROUPARRAYSIZE; i++)
    {
        char buf2[sizeof(Group)];
        memcpy(&buf2, &groupArray[i], sizeof(Group));
        write(idgr, &buf2, sizeof(Group));
    }
    close(idch);
    close(idgr);
}

void LoadCfg(Channel* channelArray,Group* groupArray)
{
    int idch = open("channels.cfg", O_RDONLY);
    int idgr = open("groups.cfg", O_RDONLY);
    for (int i = 0; i < CHANNELARRAYSIZE; i++)
    {
        char buf[sizeof(Channel)];
        read(idch, &buf, sizeof(Channel));
        memcpy(&channelArray[i],&buf , sizeof(Channel));
        if (channelArray[i].active == true)
            channelArray[i].channelId = msgget((key_t)(i + 1001), IPC_CREAT | 0600);
    }
    for (int i = 0; i < GROUPARRAYSIZE; i++)
    {
        char buf2[sizeof(Group)];
        read(idgr, &buf2, sizeof(Group));
        memcpy(&groupArray[i],&buf2, sizeof(Group));
    }
    close(idch);
    close(idgr);
}