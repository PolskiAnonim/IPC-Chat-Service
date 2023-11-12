#include "klient.h"

long Encoding(char Name[9]) {
    long l;
    memcpy(&l, Name, sizeof(long));
    return l;
}

void MyFlush() {
    int flags = fcntl(1, F_GETFL);
    fcntl(1, F_SETFL, flags | O_NONBLOCK);
    while (getchar() != EOF); // flush stdin
    fcntl(1, F_SETFL, flags);
}

void Mwait() {
    MyFlush();
    printf("Press key to continue\n");
    getchar(); // Works as wait
    MyFlush();
}

void Registration(int server) {
    // Get username
    char userName[9];
    MyFlush();
    printf("\nPodaj nazwę użytkownika -> ");
    int n = -1;
    for (int i = 0; i < 9; ++i)
        if ((userName[i] = getchar()) == '\n') {
            n = i;
            break;
        }
    if (n != -1)
    {
        for (int i = n; i < 9; i++)
            userName[i] = '\0';
    }
    userName[8] = '\0';
    MyFlush();
    //Get password
    char password[17];
    printf("\nPodaj hasło -> ");
    n = -1;
    for (int i = 0; i < 16; ++i)
        if ((password[i] = getchar()) == '\n') {
            n = i;
            break;
        }
    if (n != -1)
    {
        for (int i = n; i < 15; i++)
            password[i] = '\0';
        
    }
    password[16] = '\0';
    //send request for register
    msgText msg;
    long userEncoding = Encoding(userName);
    msg.type = 1;
    msg.sender = userEncoding;
    strcpy(msg.text, password);
    msgsnd(server, &msg, sizeof(msg) - sizeof(long), 0);
    //receive reply
    msgInfo feedback;
    msgrcv(server, &feedback, sizeof(feedback)-sizeof(long), userEncoding, 0);
    if (feedback.flags == 10) {
        printf("\nZAREJESTROWANO POMYŚLNIE\n");
    }
    if (feedback.flags == 11) {
        printf("\nUŻYTKOWNIK O TAKIEJ NAZWIE JUŻ ISTNIEJE\n");
    }
    else if (feedback.flags == 12) {
        printf("\nBRAK WOLNYCH KANAŁÓW");
    }
}

int Login(int server,char userName[9]) {
    // Get username
    MyFlush();
    printf("\nPodaj nazwę użytkownika (max 8 znaków) -> ");
    int n = -1;
    for (int i = 0; i < 9; ++i)
        if ((userName[i] = getchar()) == '\n') {
            n = i;
            break;
        }
    if (n != -1)
    {
        for (int i = n; i < 9; i++)
            userName[i] = '\0';
    }
    userName[8] = '\0';
    MyFlush();
    char password[17];
    printf("\nPodaj hasło -> ");
    n = -1;
    for (int i = 0; i < 16; ++i)
        if ((password[i] = getchar()) == '\n') {
            n = i;
            break;
        }
    if (n != -1)
    {
        for (int i = n; i < 16; i++)
            password[i] = '\0';
    }
    password[16] = '\0';
    //send request
    msgText msg;
    long userEncoding = Encoding(userName);
    msg.type = 2;
    msg.sender = userEncoding;
    strcpy(msg.text,password);
    msgsnd(server, &msg, sizeof(msg) - sizeof(long), 0);
    //receive reply
    msgInfo feedback;
    msgrcv(server, &feedback, sizeof(feedback)-sizeof(long), userEncoding, 0);
    if (feedback.flags == 20) {
        printf("LOGOWANIE POPRAWNE - Dzien dobry %s\n", userName);
        return feedback.additional;
    }
    else if (feedback.flags == 21) {
        printf("UŻYTKOWNIK O TAKIEJ NAZWIE NIE ISTNIEJE\n");
        return -1;
    }
    else if (feedback.flags == 22)
    {
        printf("UŻYTKOWNIK JUŻ ZALOGOWANY\n");
        return -1;
    }
    if (feedback.flags == 23)
    {
        printf("NIEPOPRAWNE HASŁO! POZOSTAŁE PRÓBY: %li\n", feedback.additional);
        if (!feedback.additional)
            printf("Za dużo nieudanych prób. Konto usunięte");
        return -1;
    }
    return -1;
}

void Logout(int server,long userCode)
{
    //send request
    msgInfo msg;
    msg.additional = 0;
    msg.type = 3;
    msg.flags = userCode;
    msgsnd(server, &msg, sizeof(msgInfo) - sizeof(long),0);
    
    //receive feedback
    msgrcv(server, &msg, sizeof(msgInfo) - sizeof(long), userCode,0);
    if (msg.flags == 30)
        printf("Wylogowano!\n");
}

void SendMessage(int server, long userCode, bool t)
{
    msgText msg;
    msg.sender = userCode;
    if (t)
    {
        printf("Grupa: ");
        msg.type = 12;
    }
    else
    {
        printf("\nAdresat: ");
        msg.type = 11;
    }
    //to whom
    char receiver[9];
    
    MyFlush();
    int n = -1;
    for (int i = 0; i < 9; ++i)
        if ((receiver[i] = getchar()) == '\n') {
            n = i;
            break;
        }
    if (n != -1)
    {
        for (int i = n; i < 9; i++)
            receiver[i] = '\0';
    }
    receiver[8] = '\0';
    msg.receiver = Encoding(receiver);
    MyFlush();
    //text
    printf("Treść:\n");
    for (int i = 0; i < MESSAGESIZE; ++i){
        if ((msg.text[i] = getchar()) == '\n')
            break;
    }

    MyFlush();
    msgsnd(server, &msg, sizeof(msgText) - sizeof(long), 0);
    //feedback
    if (t)
    {
        msgList feedback;
        msgrcv(server, &feedback, sizeof(msgList) - sizeof(long), userCode, 0);
        if (feedback.flags == 120)
            printf("Dostarczono wiadomość dla wszystkich członków grupy\n");
        else if (feedback.flags == 121)
            printf("\nNie istnieje grupa o podanej nazwie.\n");
        else if (feedback.flags == 122)
            printf("\nNie możesz wysłać wiadomości do grupy do której nie należysz.\n");
        else if (feedback.flags == 123)
        {
            printf("\nNie dostarczono dla wszystkich. Lista osób do których nie dotarła wiadomość:\n");
            for (int i=0;i<GROUPSIZE;++i)
                if (feedback.list[i]!=0)
                {
                    char name[9];
                    long h = feedback.list[i];
                    memcpy(name,&h , sizeof(char) * 8);
                    name[8] = '\0';
                    printf("%s\n", name);
                }
        }
    }
    else
    {
        msgInfo feedback;
        msgrcv(server, &feedback, sizeof(msgInfo) - sizeof(long), userCode, 0);
        if (feedback.flags == 110)
            printf("Wiadomość dostarczona\n");
        else if (feedback.flags == 111)
            printf("Nie istnieje użytkownik o podanej nazwie/loginie/nicku?\n");
        else if (feedback.flags == 112)
            printf("Przepełniona kolejka użytkownika, wiadomość nie dostarczona\n");
        else if (feedback.flags == 113)
            printf("Jesteś zablokowany/na przez tego użytkownika, wiadomość nie dostarczona\n");
    }
}

void GetList(int local,int server, long userCode, int t)
{
    msgInfo msg;
    msg.flags = userCode;
    msg.type = t + 5;

    char groupName[9];
    if (t == 3) //for group members
    {
        MyFlush();
        printf("\nPodaj nazwę grupy (max 8 znaków) -> ");
        int n = -1;
        for (int i = 0; i < 9; ++i)
            if ((groupName[i] = getchar()) == '\n') {
                n = i;
                break;
            }
        if (n != -1)
        {
            for (int i = n; i < 8; i++)
                groupName[i] = '\0';
            
        }
        groupName[8] = '\0';
        MyFlush();
        msg.additional = Encoding(groupName);
    }
    else //for logged in and group list
        msg.additional = 0;
    //send request
    msgsnd(server, &msg, sizeof(msgInfo) - sizeof(long), 0);
    //receive feedback
    msgrcv(server, &msg, sizeof(msgInfo) - sizeof(long), userCode, 0);
    if (msg.flags % 10 == 1)
        printf("\nKanał przepełniony! Najpierw odbierz inne wiadomości!\n");
    else if (msg.flags == 82)
        printf("\nGrupa o podanej nazwie nie istnieje.\n");
    else   //reply
    {
        msgList list;
        msgrcv(local,&list , sizeof(msgList) - sizeof(long), 31, 0);
        printf("\nLista ");
        if (t == 1)
            printf("zalogowanych:\n");
        else if (t == 2)
            printf("grup:\n");
        else
            printf("członków grupy %s\n", groupName);
        for (int i = 0; i < sizeof(list.list) / sizeof(list.list[0]); ++i)
        {
            if (list.list[i] != 0)
            {
                char name[9];
                long h = list.list[i];
                memcpy(&name, &h, sizeof(char) * 8);
                name[8] = '\0';
                printf("%s\n", name);
            }
        }
    }
}

int AddGroup(int server,long userCode,long groupCode,msgInfo* msg)
{
    //send request
    msg->type = 15;
    msg->flags = userCode;
    msg->additional = groupCode;
    msgsnd(server, msg, sizeof(msgInfo) - sizeof(long), 0);
    //receive feedback
    msgrcv(server, msg, sizeof(msgInfo) - sizeof(long), userCode, 0);

    if (msg->flags == 150) {
        printf("Utworzono grupę\n");
        return 0;
    }
    else if (msg->flags == 151) {
        printf("Istnieje maksymalna ilość grup\n");
        return -1;
    }
    return -1;
}

void Add(int server,long userCode)
{
    //which group
    char groupName[9];
    printf("Podaj nazwę grupy do której chcesz dołączyć: ");
    MyFlush();
    int n = -1;
    for (int i = 0; i < 9; ++i)
        if ((groupName[i] = getchar()) == '\n') {
            n = i;
            break;
        }
    if (n != -1)
    {
        for (int i = n; i < 8; i++)
            groupName[i] = '\0';
        
    }
    groupName[8] = '\0';
    MyFlush();
    //send request
send:

    long groupCode = Encoding(groupName);
    msgInfo msg;
    msg.type = 13;
    msg.flags = userCode;
    msg.additional = groupCode;
    msgsnd(server, &msg, sizeof(msgInfo) - sizeof(long), 0);
    //reply - add to group
    msgrcv(server, &msg, sizeof(msgInfo) - sizeof(long), userCode, 0);
    if (msg.flags == 130)
        printf("Dodano do grupy: %s\n", groupName);
    else if (msg.flags == 132)
        printf("Już jesteś członkiem tej grupy.\n");
    else if (msg.flags == 133)
        printf("Grupa ma już maksymalną ilość członków.\n");
    //new group
    else if (msg.flags == 131)
    {
        printf("Grupa o podanej nazwie nie istnieje. Czy chcesz ją utworzyć? y/n\n");
        while (1)
        {
            MyFlush();
            char a = getchar();
            switch (a)
            {
            case 'n':
                return;
            case 'y':
                if (AddGroup(server, userCode, groupCode, &msg) == 0)
                    goto send;
                return;
            default:
                printf("Zły wybór\n");
            }
        }
        
    }
}

void RemoveGroup(int server, long userCode, long groupCode, msgInfo* msg)
{
    //send request
    msg->type = 16;
    msg->flags = userCode;
    msg->additional = groupCode;
    msgsnd(server, msg, sizeof(msgInfo) - sizeof(long), 0);
    //receive feedback
    msgrcv(server, msg, sizeof(msgInfo) - sizeof(long), userCode, 0);

    if (msg->flags == 160) {
        printf("Usunięto grupę\n");
    }
}

void Remove(int server, long userCode)
{
    //which group
    char groupName[9];
    printf("Podaj nazwę grupy którą chcesz opuścić: ");
    MyFlush();
    int n = -1;
    for (int i = 0; i < 9; ++i)
        if ((groupName[i] = getchar()) == '\n') {
            n = i;
            break;
        }
    if (n != -1)
    {
        for (int i = n; i < 8; i++)
            groupName[i] = '\0';
        
    }
    groupName[8] = '\0';
    MyFlush();
    //send request
    long groupCode = Encoding(groupName);
    msgInfo msg;
    msg.type = 14;
    msg.flags = userCode;
    msg.additional = groupCode;
    msgsnd(server, &msg, sizeof(msgInfo) - sizeof(long), 0);
    //reply - remove from group
    msgrcv(server, &msg, sizeof(msgInfo) - sizeof(long), userCode, 0);

    if (msg.flags == 141)
        printf("Grupa nie istnieje\n");
    else if (msg.flags == 142)
        printf("Nie jesteś członkiem tej grupy.\n");
    //remove group
    else if (msg.flags == 140)
    {
        printf("Usunięto z grupy: %s\n", groupName);
        if (msg.additional == 145)
        {
            printf("Grupa nie ma teraz członków. Czy chcesz ją usunąć? y/n\n");
            while (1)
            {
                MyFlush();
                char a = getchar();
                switch (a)
                {
                case 'n':
                    return;
                case 'y':
                    RemoveGroup(server, userCode, groupCode, &msg);
                    return;
                default:
                    printf("Zły wybór\n");
                }
            }
        }
    }
}

void HandleOneMessage(int local, long userCode, bool t)
{
    int type = t + 11;
    msgText msg;
    receive:
    if (msgrcv(local, &msg, sizeof(msgText) - sizeof(long), type, IPC_NOWAIT) == -1)
    {
        printf("Brak nowych wiadomości\n");
        return;
    }
    else
    {
        long h = msg.sender;
        char name[9];
        memcpy(name, &h, sizeof(char) * 8);
        name[8] = '\0';
        printf("Nowa wiadomość!");
        if (type == 11)
            printf("Nadawca: %s\n", name);
        if (type == 12)
            printf("Grupa: %s\n", name);
        printf("Treść wiadomości:\n%s", msg.text);
        printf("\nCzy chcesz pobrać kolejną wiadomość? y/n\n");
    }
    while (1)
    {
        MyFlush();
        char c = getchar();
        switch (c)
        {
        case 'y':
            goto receive;
            break;
        case 'n':
            return;
        default:
            printf("Zła opcja");
        }
    }

}

void HandleMessages(int local, long userCode)
{
    MyFlush();
    char messagesmenu;

    while (1)
    {
        printf("Podaj rodzaj wiadomości:\n1. Wiadomości grupowe\n2. Wiadomości od użytkowników\n3.Powrót\nPodaj opcję: ");
        messagesmenu = getchar();
        switch (messagesmenu)
        {
        case '1':
            HandleOneMessage(local, userCode, 1);
            break;
        case '2':
            HandleOneMessage(local, userCode, 0);
            break;
        case '3':
            return;
        default:
            printf("\nBrak opcji\n");
        }
        Mwait();
    }


}

void BlockMsg(int server, long userCode, bool t)
{
    char name[9];
    MyFlush();
    if (t)
        printf("\nPodaj nazwę grupy -> ");
    else
        printf("\nPodaj nazwę użytkownika -> ");
    int n = -1;
    for (int i = 0; i < 9; ++i)
        if ((name[i] = getchar()) == '\n') {
            n = i;
            break;
        }
    if (n != -1)
    {
        for (int i = n; i < 8; i++)
            name[i] = '\0';
        
    }
    name[8] = '\0';
    MyFlush();

    msgInfo msg;
    msg.flags = userCode;
    msg.additional = Encoding(name);
    if (t)
        msg.type = 18;
    else
        msg.type = 17;
    msgsnd(server, &msg, sizeof(msgInfo) - sizeof(long), 0);

    msgrcv(server, &msg, sizeof(msgInfo) - sizeof(long), userCode, 0);
    if (msg.flags == 182)
        printf("Nie jesteś członkiem tej grupy. Zablokowanie niemożliwe.\n");
    if (msg.flags == 181)
        printf("Grupa o takiej nazwie nie istnieje\n");
    if (msg.flags == 171)
        printf("Użytkownik o takiej nazwie nie istnieje\n");
    if (msg.flags % 10 == 0)
    {
        if (msg.additional % 10 == 5)
            printf("Zablokowano ");
        else
            printf("Odblokowano ");
        if (msg.flags == 170)
            printf("użytkownika %s\n", name);
        else
            printf("grupę %s\n", name);
    }

}

void BlockList(int local,int server,long userCode)
{
    //send request
    msgInfo msg;
    msg.type = 19;
    msg.flags = userCode;
    msg.additional = 0;
    msgsnd(server, &msg, sizeof(msgInfo) - sizeof(long), 0);
    //receive feedback
    msgrcv(server, &msg, sizeof(msgInfo) - sizeof(long), userCode, 0);
    if (msg.flags==191 && msg.additional==197)
    {
        printf("\nKanał przepełniony! Najpierw odbierz inne wiadomości!\n");
        return;
    }
    msgList list;
    if (msg.flags == 190 || (msg.flags=191 && msg.additional == 196))
    {
        msgrcv(local, &list, sizeof(msgList) - sizeof(long), 32, 0);
        printf("\nLista zablokowanych użytkowników:\n");
        for (int i = 0; i < sizeof(list.list) / sizeof(list.list[0]); ++i)
        {
            if (list.list[i] != 0)
            {
                char name[9];
                long h = list.list[i];
                memcpy(&name, &h, sizeof(char) * 8);
                name[8] = '\0';
                printf("%s\n", name);
            }
        }
    }
    if (msg.flags == 190 || (msg.flags = 191 && msg.additional == 195))
    {
        msgrcv(local, &list, sizeof(msgList) - sizeof(long), 33, 0);
        printf("\nLista zablokowanych grup:\n");
        for (int i = 0; i < sizeof(list.list) / sizeof(list.list[0]); ++i)
        {
            if (list.list[i] != 0)
            {
                char name[9];
                long h = list.list[i];
                memcpy(&name, &h, sizeof(char) * 8);
                name[8] = '\0';
                printf("%s\n", name);
            }
        }
    }

}

void Block(int local,int server, long userCode)
{
    MyFlush();
    while (1) {

        printf("\n1. Blokowanie/Odblokowanie użytkownika\n2. Blokowanie/Odblokowanie grupy");
        printf("\n3.Lista zablokowanych\n4.Powrót\nWybór opcji : ");
        char c = getchar();
        switch (c)
        {
        case '1':
            BlockMsg(server, userCode, 0);
            break;
        case '2':
            BlockMsg(server, userCode, 1);
            break;
        case '3':
            BlockList(local,server, userCode);
            break;
        case '4':
            return;
        default:
            printf("Brak opcji\n");
        }
        Mwait();
    }
}

int main() {
    int server;
    char userName[9];
    bool loggedin = false;
    if ((server = msgget(999, 0)) == -1)
    {
        printf("BRAK POŁĄCZENIA Z SERWEREM\n");
        return 0;
    }
    int localChatAddress;
loggedout:
    while (loggedin == false) {
        printf("1. Rejestracja\n2. Logowanie\n3. Wyjście\n");
        char menu = getc(stdin);
        switch (menu) {
        case('1'):
            Registration(server);
            break;
        case('2'):
            if ((localChatAddress = Login(server, userName)) != -1)
            {
                loggedin = true;
            }
            break;
        case('3'):
            return 0;
        default:
            printf("Brak opcji\n");
        }
            Mwait();
    }
    long encodedName = Encoding(userName);
    //logged in menu
    int localChat = msgget((key_t)localChatAddress, 0);
    printf("heh i'm in\n");
    while (1) {
        printf("1. Wyloguj\n2. Zalogowani\n3. Lista grup\n4. Zapisani do danej grupy tematycznej\n");
        printf("5. Zapis do grupy tematycznej\n6. Wypis z grupy tematycznej\n7. Wysłanie wiadomości do grupy");
        printf("\n8. Wysłanie wiadomości do użytkownika\n9. Odebranie wiadomości\n0. Blokowanie wiadomości\n");
        printf("\nPodaj opcję: ");
        char menu = getc(stdin);
        switch (menu)
        {
        case ('1'):
            Logout(server, encodedName);
            loggedin = false;
            MyFlush();
            goto loggedout;
        case ('2'):
            GetList(localChat, server, encodedName, 1);
            break;
        case ('3'):
            GetList(localChat, server, encodedName, 2);
            break;
        case ('4'):
            GetList(localChat, server, encodedName, 3);
            break;
        case('5'):
            Add(server, encodedName);
            break;
        case('6'):
            Remove(server, encodedName);
            break;
        case('7'):
            SendMessage(server, encodedName, 1);
            break;
        case('8'):
            SendMessage(server, encodedName, 0);
            break;
        case('9'):
            HandleMessages(localChat, encodedName);
            break;
        case('0'):
            Block(localChat,server, encodedName);
            break;
        default:
            printf("Brak opcji\n");
        }
        Mwait();
    }
}


