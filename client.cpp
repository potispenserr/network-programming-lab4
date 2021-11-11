#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include "struct.h"
#include <conio.h>
#include <iostream>
#include <thread>
#include <future>

// Need to link with Ws2_32.lib, Mswsock.lib, and Advapi32.lib
#pragma comment (lib, "Ws2_32.lib")
#pragma comment (lib, "Mswsock.lib")
#pragma comment (lib, "AdvApi32.lib")

#define DEFAULT_BUFLEN 512
#define DEFAULT_PORT "49152"
#define SERVERIP "130.240.40.25"
#define KEY_UP 72
#define KEY_DOWN 80
#define KEY_LEFT 75
#define KEY_RIGHT 77

int recieveID(SOCKET ConnectSocket) {
    MsgHead* msghead;
    for (int i = 0; i < 2; i++) {
        std::cout << "starting thread" << "\n";
        char recvbuf[DEFAULT_BUFLEN];
        int iResult;
        int recvbuflen = DEFAULT_BUFLEN;
        iResult = recv(ConnectSocket, recvbuf, recvbuflen, 0);
        std::cout << "Sizeof recvbuffer: " << sizeof(recvbuf) << "\n";
        msghead = (MsgHead*)recvbuf;
        std::cout << "message head type is: " << msghead->type << "\n";
        std::cout << "message head id is: " << msghead->id << "\n";
        std::cout << "message head length is: " << msghead->length << "\n";
        std::cout << "message head sequence number is: " << msghead->seq_no << "\n";

    }
    return msghead->id;
}
auto recieveOnce(SOCKET ConnectSocket) {
    std::cout << "starting thread RecieveOnce" << "\n";
    char recvbuf[DEFAULT_BUFLEN];
    int iResult;
    int recvbuflen = DEFAULT_BUFLEN;
    iResult = recv(ConnectSocket, recvbuf, recvbuflen, 0);
    std::cout << "Sizeof recvbuffer: " << sizeof(recvbuf) << "\n";
    MsgHead* msghead = (MsgHead*)recvbuf;
    std::cout << "message head type is: " << msghead->type << "\n";
    std::cout << "message head id is: " << msghead->id << "\n";
    std::cout << "message head length is: " << msghead->length << "\n";
    std::cout << "message head sequence number is: " << msghead->seq_no << "\n";
    return recvbuf;
}

std::string deSerialize(char* recvbuf) {
    MsgHead* msghead = (MsgHead*)recvbuf;
    std::cout << "message head type is: " << msghead->type << "\n";
    std::cout << "message head id is: " << msghead->id << "\n";
    std::cout << "message head length is: " << msghead->length << "\n";
    std::cout << "message head sequence number is: " << msghead->seq_no << "\n";
    switch (msghead->type) {
    case Join:
        std::cout << "MsgType was Join" << "\n";
        break;
    case Change:
        std::cout << "MsgType was Change" << "\n";
        ChangeMsg* changemsg = (ChangeMsg*)recvbuf;
        std::cout << changemsg->type << "\n";
        switch (changemsg->type) {
        case NewPlayer: {

            NewPlayerMsg* newplayer;
            std::cout << "Deeper into the rabbit hole reveals that this message was actually NewPlayer" << "\n";
            newplayer = (NewPlayerMsg*)changemsg;
            std::cout << "player name: " << newplayer->name << "\n";
            return std::string("NewPlayerMsg");
        }
        break;

        case PlayerLeave: {

            PlayerLeaveMsg* playerleft;
            std::cout << "After consooming the red pill this message was revealed to be PlayerLeave" << "\n";
            playerleft = (PlayerLeaveMsg*)changemsg;
            std::cout << "ID: " << playerleft->msg.head.id << "left" << "\n";
            return std::string("PlayerLeft");
        }
        break;

        case NewPlayerPosition: {

            NewPlayerPositionMsg* playerpos;
            std::cout << "It was I NewPlayerPosition" << "\n";
            playerpos = (NewPlayerPositionMsg*)changemsg;
            std::cout << "playerID: " << playerpos->msg.head.id << " newX: " << playerpos->pos.x << " newY: " << playerpos->pos.y << "\n";
            return std::string("NewPlayerPositionMsg");
        }
        break;

        }

    }
    return std::string("Beats me");
}

int __cdecl main(int argc, char** argv)
{
    

    WSADATA wsaData;
    SOCKET ConnectSocket = INVALID_SOCKET;
    struct addrinfo* result = NULL,
        * ptr = NULL,
        hints;
    //const char* sendbuf = "this is a test";
    int ID;

    JoinMsg join;
    int seq_num = 1;
    join.head.id = 0;
    join.head.length = sizeof(join);
    join.head.type = Join;
    join.head.seq_no = seq_num;
    int playerX = -100;
    int playerY = -100;
    char sendbuf[sizeof(join)];
    memcpy((void*)sendbuf, (void*)&join, sizeof(join));

    char recvbuf[DEFAULT_BUFLEN];
    int iResult;
    int recvbuflen = DEFAULT_BUFLEN;


    // Initialize Winsock
    iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult != 0) {
        printf("WSAStartup failed with error: %d\n", iResult);
        return 1;
    }

    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    // Resolve the server address and port
    iResult = getaddrinfo(SERVERIP, DEFAULT_PORT, &hints, &result);
    if (iResult != 0) {
        printf("getaddrinfo failed with error: %d\n", iResult);
        WSACleanup();
        return 1;
    }

    // Attempt to connect to an address until one succeeds
    for (ptr = result; ptr != NULL; ptr = ptr->ai_next) {

        // Create a SOCKET for connecting to server
        ConnectSocket = socket(ptr->ai_family, ptr->ai_socktype,
            ptr->ai_protocol);
        if (ConnectSocket == INVALID_SOCKET) {
            printf("socket failed with error: %ld\n", WSAGetLastError());
            WSACleanup();
            return 1;
        }

        // Connect to server.
        iResult = connect(ConnectSocket, ptr->ai_addr, (int)ptr->ai_addrlen);
        if (iResult == SOCKET_ERROR) {
            closesocket(ConnectSocket);
            ConnectSocket = INVALID_SOCKET;
            continue;
        }
        break;
    }

    freeaddrinfo(result);

    if (ConnectSocket == INVALID_SOCKET) {
        printf("Unable to connect to server!\n");
        WSACleanup();
        return 1;
    }

    // Send an initial buffer
    iResult = send(ConnectSocket, sendbuf, sizeof(sendbuf), 0);
    std::cout << "Message sent was: " << sizeof(sendbuf) << "\n";
    if (iResult == SOCKET_ERROR) {
        printf("send failed with error: %d\n", WSAGetLastError());
        closesocket(ConnectSocket);
        WSACleanup();
        return 1;
    }

    printf("Bytes Sent: %ld\n", iResult);
    seq_num++;

    std::future<int> ret = std::async(std::launch::async, recieveID, ConnectSocket);
    ID = ret.get();
    std::cout << "ID got from thread is " << ID << "\n";

    // shutdown the connection since no more data will be sent
    /*iResult = shutdown(ConnectSocket, SD_SEND);
    if (iResult == SOCKET_ERROR) {
        printf("shutdown failed with error: %d\n", WSAGetLastError());
        closesocket(ConnectSocket);
        WSACleanup();
        return 1;
    }*/

    //std::cout << "receiving" << "\n";
    // Receive until the peer closes the connection
    MsgHead* msghead;

    int key = 0;
    bool run = true;
    while (run)
    {
        key = _getch();

        switch (key) {
        case KEY_UP: {
            std::cout << "Up" << std::endl;//key up
            /*EventMsg eventmsg;
            eventmsg.head.id = 0;
            eventmsg.head.length = sizeof(eventmsg);
            eventmsg.head.type = Event;
            eventmsg.head.seq_no = seq_num;
            seq_num++;*/
            
            MoveEvent movemsg;
            playerY++;
            movemsg.pos.x = playerX;
            movemsg.pos.y = playerY;
            movemsg.event.type = Move;
            movemsg.event.head.id = ID;
            movemsg.event.head.seq_no = seq_num;
            movemsg.event.head.type = Event;
            movemsg.event.head.length = sizeof(movemsg);
            char sendbuffer[sizeof(movemsg)];

            std::cout << "size of movemsg was " << sizeof(movemsg) << "\n";

            memcpy((void*)sendbuffer, (void*)&movemsg, sizeof(movemsg));

            iResult = send(ConnectSocket, sendbuffer, sizeof(sendbuffer), 0);
            std::cout << "Message sent was: " << sizeof(sendbuffer) << "\n";
            if (iResult == SOCKET_ERROR) {
                printf("send failed with error: %d\n", WSAGetLastError());
                closesocket(ConnectSocket);
                WSACleanup();
                return 1;
            }
            printf("Bytes Sent: %ld\n", iResult);
            //seq_num++;
            std::future<char*> ret = std::async(std::launch::async, recieveOnce, ConnectSocket);
            char* recievebuffer = ret.get();
            //std::async(std::launch::async, recieveOnce, ConnectSocket);
            deSerialize(recievebuffer);

        }
        break;
        case KEY_DOWN: {

            std::cout << "Down" << std::endl;   // key down
        }
            break;
        case KEY_LEFT: {
            std::cout << "Left" << std::endl;  // key left

        }
            break;
        case KEY_RIGHT: {
            std::cout << "Right" << std::endl;  // key right
            LeaveMsg leave;
            leave.head.id = ID;
            leave.head.type = Leave;
            leave.head.seq_no = seq_num;
            leave.head.length = sizeof(leave);
            seq_num++;
            memcpy((void*)sendbuf, (void*)&leave, sizeof(leave));

            iResult = send(ConnectSocket, sendbuf, sizeof(sendbuf), 0);
            if (iResult == SOCKET_ERROR) {
                printf("send failed with error: %d\n", WSAGetLastError());
                closesocket(ConnectSocket);
                WSACleanup();
                return 1;
            }
            run = false;
        }
        break;

        }
    }
    
    do {

        iResult = recv(ConnectSocket, recvbuf, recvbuflen, 0);
        std::cout << "Sizeof recvbuffer: " << sizeof(recvbuf) << "\n";
        msghead = (MsgHead*)recvbuf;
        std::cout << "message head type is: " << msghead->type << "\n";
        std::cout << "message head id is: " << msghead->id << "\n";
        std::cout << "message head length is: " << msghead->length << "\n";
        std::cout << "message head sequence number is: " << msghead->seq_no << "\n";
        switch (msghead->type) {
        case Join:
            std::cout << "MsgType was Join" << "\n";
            break;
        case Change:
            std::cout << "MsgType was Change" << "\n";
            ChangeMsg* changemsg = (ChangeMsg*)recvbuf;
            std::cout << changemsg->type << "\n";
            NewPlayerMsg* shit = (NewPlayerMsg*)changemsg;
            std::cout << "player name: " << shit->name << "\n";
            break;
        }

        if (iResult > 0)
            printf("Bytes received: %d\n", iResult);
        else if (iResult == 0)
            printf("Connection closed\n");
        else
            printf("recv failed with error: %d\n", WSAGetLastError());
        
    } while (true);

   /* LeaveMsg leave;
    leave.head.id = 0;
    leave.head.length = sizeof(leave);
    leave.head.type = Leave;
    leave.head.seq_no = seq_num;
    seq_num++;
    memcpy((void*)sendbuf, (void*)&leave, sizeof(leave));

    iResult = send(ConnectSocket, sendbuf, sizeof(sendbuf), 0);
    std::cout << "Message sent was: " << sizeof(sendbuf) << "\n";*/
    if (iResult == SOCKET_ERROR) {
        printf("send failed with error: %d\n", WSAGetLastError());
        closesocket(ConnectSocket);
        WSACleanup();
        return 1;
    }
    // shutdown the send half of the connection since no more data will be sent
    iResult = shutdown(ConnectSocket, SD_SEND);
    if (iResult == SOCKET_ERROR) {
        printf("shutdown failed: %d\n", WSAGetLastError());
        closesocket(ConnectSocket);
        WSACleanup();
        return 1;
    }
}