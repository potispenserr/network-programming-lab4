#undef UNICODE

#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <string>
#include "struct.h"
#include <future>



// Need to link with Ws2_32.lib
#pragma comment (lib, "Ws2_32.lib")
// #pragma comment (lib, "Mswsock.lib")

#define DEFAULT_BUFLEN 512
#define DEFAULT_PORT "49152"


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
        return std::string("JoinMsg");
        break;
    case Change: {
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
    case Event: {
        std::cout << "This message was revealed to be a Event" << "\n";
        EventMsg* eventmsg = (EventMsg*)recvbuf;
        std::cout << "After further investigation this turned out to be a " << eventmsg->type << " aka move message" << "\n";

        MoveEvent* moveevent = (MoveEvent*)eventmsg;
        std::cout << "MoveEvent X: " << moveevent->pos.x << "\n";
        std::cout << "MoveEvent Y: " << moveevent->pos.y << "\n";
        return std::string("MoveEvent");


        break;
    }
    default:
        std::cout << "Type number " << msghead->type << " doesn't exist" << "\n";

    }
    return std::string("Beats me");
}

void sendInvalidMoveMessage(int ID, int &seq_num, SOCKET ClientSocket, std::vector<int> playerpos, int iResult) {

    NewPlayerPositionMsg newPlayerPosition;
    newPlayerPosition.msg.type = NewPlayerPosition;
    newPlayerPosition.msg.head.id = ID;
    newPlayerPosition.msg.head.seq_no = seq_num;
    newPlayerPosition.msg.head.type = Change;
    newPlayerPosition.pos.x = playerpos[0];
    newPlayerPosition.pos.y = playerpos[1];
    newPlayerPosition.msg.head.length = sizeof(newPlayerPosition);
    char sendbuffer[sizeof(newPlayerPosition)];


    std::cout << "size of movemsg was " << sizeof(newPlayerPosition) << "\n";

    memcpy((void*)sendbuffer, (void*)&newPlayerPosition, sizeof(newPlayerPosition));

    std::cout << "size of sendbuffer was " << sizeof(sendbuffer) << "\n";


    std::cout << "Sending back the same movemsg aka invalid move" << "\n";
    // Send NewPlayer message
    int iSendResult = send(ClientSocket, sendbuffer, iResult, 0);
    if (iSendResult == SOCKET_ERROR) {
        printf("send failed with error: %d\n", WSAGetLastError());
        closesocket(ClientSocket);
        WSACleanup();
        return;
    }
    printf("Bytes sent: %d\n", iSendResult);
    seq_num++;

}

void serverThread() {

}

int __cdecl main()
{
    system("title Master server for Twitch plays pixel art");

    WSADATA wsaData;
    int iResult;

    SOCKET ListenSocket = INVALID_SOCKET;
    SOCKET ClientSocket = INVALID_SOCKET;

    struct addrinfo* result = NULL;
    struct addrinfo hints;

    int ID = 4;

    MsgHead joinedMsg;
    int seq_num = 1;
    joinedMsg.id = ID;
    joinedMsg.length = sizeof(joinedMsg);
    joinedMsg.type = Join;
    joinedMsg.seq_no = seq_num;
    char sendbuf[sizeof(joinedMsg)];
    memcpy((void*)sendbuf, (void*)&joinedMsg, sizeof(joinedMsg));
    std::vector<int> playerpos = { -99,-100 };

    int iSendResult;
    char recvbuf[DEFAULT_BUFLEN];
    int recvbuflen = DEFAULT_BUFLEN;


    // Initialize Winsock
    iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult != 0) {
        printf("WSAStartup failed with error: %d\n", iResult);
        return 1;
    }

    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = AI_PASSIVE;

    // Resolve the server address and port
    iResult = getaddrinfo(NULL, DEFAULT_PORT, &hints, &result);
    if (iResult != 0) {
        printf("getaddrinfo failed with error: %d\n", iResult);
        WSACleanup();
        return 1;
    }

    // Create a SOCKET for connecting to server
    ListenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    if (ListenSocket == INVALID_SOCKET) {
        printf("socket failed with error: %ld\n", WSAGetLastError());
        freeaddrinfo(result);
        WSACleanup();
        return 1;
    }

    // Setup the TCP listening socket
    iResult = bind(ListenSocket, result->ai_addr, (int)result->ai_addrlen);
    if (iResult == SOCKET_ERROR) {
        printf("bind failed with error: %d\n", WSAGetLastError());
        freeaddrinfo(result);
        closesocket(ListenSocket);
        WSACleanup();
        return 1;
    }

    freeaddrinfo(result);

    iResult = listen(ListenSocket, SOMAXCONN);
    if (iResult == SOCKET_ERROR) {
        printf("listen failed with error: %d\n", WSAGetLastError());
        closesocket(ListenSocket);
        WSACleanup();
        return 1;
    }
    std::cout << "Starting server" << "\n";

    // Accept a client socket
    ClientSocket = accept(ListenSocket, NULL, NULL);
    if (ClientSocket == INVALID_SOCKET) {
        printf("accept failed with error: %d\n", WSAGetLastError());
        closesocket(ListenSocket);
        WSACleanup();
        return 1;
    }

    // No longer need server socket
    closesocket(ListenSocket);

    bool run = true;
    // Receive until the peer shuts down the connection
    do {

        iResult = recv(ClientSocket, recvbuf, recvbuflen, 0);
        //std::future<char*> ret = std::async(std::launch::async, recieveOnce, ListenSocket);
        //char* recievebuffer = ret.get();
        std::string messageType = deSerialize(recvbuf);
        std::cout << "MessageType was" << messageType << "\n";
        if (iResult > 0) {
            printf("Bytes received: %d\n", iResult);

            if (messageType == "JoinMsg") {

                std::cout << "Sending JoinMsg back" << "\n";
                // Send Join message back
                iSendResult = send(ClientSocket, sendbuf, iResult, 0);
                if (iSendResult == SOCKET_ERROR) {
                    printf("send failed with error: %d\n", WSAGetLastError());
                    closesocket(ClientSocket);
                    WSACleanup();
                    return 1;
                }
                printf("Bytes sent: %d\n", iSendResult);
                seq_num++;
                JoinMsg* joinedmsg;
                joinedmsg = (JoinMsg*)recvbuf;

                //NewPlayerMsg newplayermsg;
                //newplayermsg.msg.head.id = ID;
                //newplayermsg.msg.head.seq_no = seq_num;
                //newplayermsg.msg.head.type = Change;
                //newplayermsg.msg.head.length = sizeof(newplayermsg);

                //newplayermsg.msg.type = NewPlayer;
                //newplayermsg.name = joinedmsg->name;
                //char sendbuffer[sizeof(newplayermsg)];
                //memcpy((void*)sendbuffer, (void*)&newplayermsg, sizeof(newplayermsg));

                //std::cout << "Sending NewPlayerMsg" << "\n";
                //// Send NewPlayer message
                //iSendResult = send(ClientSocket, sendbuffer, iResult, 0);
                //if (iSendResult == SOCKET_ERROR) {
                //    printf("send failed with error: %d\n", WSAGetLastError());
                //    closesocket(ClientSocket);
                //    WSACleanup();
                //    return 1;
                //}
                //printf("Bytes sent: %d\n", iSendResult);
                //seq_num++;

            }
            else if (messageType == "MoveEvent") {
                MoveEvent* moveevent = (MoveEvent*)recvbuf;
                if (moveevent->pos.x > 100 || moveevent->pos.x < -100) {
                    std::cout << "player is going over the edge on X" << "\n";
                    sendInvalidMoveMessage(ID, seq_num, ClientSocket, playerpos, iResult);
                    continue;
                }
                else if (moveevent->pos.y > 100 || moveevent->pos.y < -100) {
                    std::cout << "player is going over the edge on Y" << "\n";
                    //sendInvalidMoveMessage(ID, seq_num, ClientSocket, playerpos);
                    NewPlayerPositionMsg newPlayerPosition;
                    newPlayerPosition.msg.type = NewPlayerPosition;
                    newPlayerPosition.msg.head.id = ID;
                    newPlayerPosition.msg.head.seq_no = seq_num;
                    newPlayerPosition.msg.head.type = Change;
                    newPlayerPosition.pos.x = playerpos[0];
                    newPlayerPosition.pos.y = playerpos[1];
                    newPlayerPosition.msg.head.length = sizeof(newPlayerPosition);
                    char sendbuffer[sizeof(newPlayerPosition)];


                    std::cout << "size of movemsg was " << sizeof(newPlayerPosition) << "\n";

                    memcpy((void*)sendbuffer, (void*)&newPlayerPosition, sizeof(newPlayerPosition));

                    std::cout << "size of sendbuffer was " << sizeof(sendbuffer) << "\n";

                    std::cout << "Sending back the same movemsg aka invalid move" << "\n";
                    // Send NewPlayer message
                    int iSendResult = send(ClientSocket, sendbuffer, iResult, 0);
                    if (iSendResult == SOCKET_ERROR) {
                        printf("send failed with error: %d\n", WSAGetLastError());
                        closesocket(ClientSocket);
                        WSACleanup();
                        return 1;
                    }
                    printf("Bytes sent: %d\n", iSendResult);
                    seq_num++;
                    continue;
                
                }
                else if (abs(moveevent->pos.x - playerpos[0]) > 1) {
                    std::cout << "too big jump on X" << "\n";
                    sendInvalidMoveMessage(ID, seq_num, ClientSocket, playerpos, iResult);
                    continue;
                }
                else if (abs(moveevent->pos.y - playerpos[1]) > 1) {
                    std::cout << "too big jump on X" << "\n";
                    sendInvalidMoveMessage(ID, seq_num, ClientSocket, playerpos, iResult);
                    continue;
                }
                std::cout << "This movemsg was valid" << "\n";

                NewPlayerPositionMsg newPlayerPosition;
                newPlayerPosition.msg.type = NewPlayerPosition;
                newPlayerPosition.msg.head.id = ID;
                newPlayerPosition.msg.head.seq_no = seq_num;
                newPlayerPosition.msg.head.type = Change;
                newPlayerPosition.pos.x = moveevent->pos.x;
                newPlayerPosition.pos.y = moveevent->pos.y;
                newPlayerPosition.msg.head.length = sizeof(newPlayerPosition);
                char sendbuffer[sizeof(newPlayerPosition)];

                playerpos[0] = moveevent->pos.x;
                playerpos[1] = moveevent->pos.y;

                std::cout << "size of movemsg was " << sizeof(newPlayerPosition) << "\n";

                memcpy((void*)sendbuffer, (void*)&newPlayerPosition, sizeof(newPlayerPosition));


                std::cout << "Sending the new movemsg" << "\n";
                // Send MoveMsg message
                int iSendResult = send(ClientSocket, sendbuffer, iResult, 0);
                if (iSendResult == SOCKET_ERROR) {
                    printf("send failed with error: %d\n", WSAGetLastError());
                    closesocket(ClientSocket);
                    WSACleanup();
                    return 0;
                }
                printf("Bytes sent: %d\n", iSendResult);
                seq_num++;



            }

            
        }
        else {
            printf("recv failed with error: %d\n", WSAGetLastError());
            closesocket(ClientSocket);
            WSACleanup();
            return 1;
        }

    } while (run);

    // shutdown the connection since we're done
    iResult = shutdown(ClientSocket, SD_SEND);
    if (iResult == SOCKET_ERROR) {
        printf("shutdown failed with error: %d\n", WSAGetLastError());
        closesocket(ClientSocket);
        WSACleanup();
        return 1;
    }

    // cleanup
    closesocket(ClientSocket);
    WSACleanup();

    return 0;
}