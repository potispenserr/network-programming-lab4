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
#define DEFAULT_PORT 49152
#define MAX_CLIENTS 50

//char recvbuf[DEFAULT_BUFLEN] = "";
int len, receiveres, clients_connected = 0;
bool active = TRUE;

SOCKET server_socket = INVALID_SOCKET;
SOCKET client_fd;
sockaddr_in server;

//function declarations

void start_server();

//table for clients sockets



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

    case Leave:
        std::cout << "MsgType was Leave" << "\n";
        return std::string("LeaveMsg");
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
    int iSendResult = send(ClientSocket, sendbuffer, sizeof(sendbuffer), 0);
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



struct _clients_b {

    bool connected;
    SOCKET ss;
    int clientID = 0;
    std::vector<int> playerpos = { -99,-100 };

};

bool checkCollision(std::vector<_clients_b> clients, MoveEvent* moveevent, int clients_connected, int socketID) {
    for (int i = 0; i < clients_connected; i++) {
        if (clients[i].ss == socketID) {
            continue;
        }
        std::cout << "This players newplayerpositionmsg X: " << moveevent->pos.x << " Y: " << moveevent->pos.y << "\n";
        std::cout << "Checking against socket id " << clients[i].ss << " pos X: " << clients[i].playerpos[0] << " pos Y: " << clients[i].playerpos[1] << "\n";
        if (moveevent->pos.x == clients[i].playerpos[0] && moveevent->pos.y == clients[i].playerpos[1]) {
            std::cout << "player was about to collide with another player" << "\n";
            return true;
        }
    }

    return false;

}

int __cdecl main()
{
    system("title Master server for Twitch plays pixel art");

    std::cout << "Server starting...";

    //start the server and do basic tcp setup ------------------
    start_server();

    /*WSADATA wsaData;
    int iResult;

    SOCKET ListenSocket = INVALID_SOCKET;
    SOCKET ClientSocket = INVALID_SOCKET;

    struct addrinfo* result = NULL;
    struct addrinfo hints;*/

    int nextID = 4;
    int iResult = 0;

    /*MsgHead joinedMsg;
    joinedMsg.id = nextID;
    joinedMsg.length = sizeof(joinedMsg);
    joinedMsg.type = Join;
    char sendbuf[sizeof(joinedMsg)];
    joinedMsg.seq_no = seq_num;
    memcpy((void*)sendbuf, (void*)&joinedMsg, sizeof(joinedMsg));*/
    //std::vector<int> playerpos = { -99,-100 };
    int seq_num = 1;

    std::vector<_clients_b> clients;

    int iSendResult = 0;
    char recvbuf[DEFAULT_BUFLEN];
    int recvbuflen = DEFAULT_BUFLEN;


    //// Initialize Winsock
    //iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    //if (iResult != 0) {
    //    printf("WSAStartup failed with error: %d\n", iResult);
    //    return 1;
    //}

    //ZeroMemory(&hints, sizeof(hints));
    //hints.ai_family = AF_INET;
    //hints.ai_socktype = SOCK_STREAM;
    //hints.ai_protocol = IPPROTO_TCP;
    //hints.ai_flags = AI_PASSIVE;

    //// Resolve the server address and port
    //iResult = getaddrinfo(NULL, DEFAULT_PORT, &hints, &result);
    //if (iResult != 0) {
    //    printf("getaddrinfo failed with error: %d\n", iResult);
    //    WSACleanup();
    //    return 1;
    //}

    //// Create a SOCKET for connecting to server
    //ListenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    //if (ListenSocket == INVALID_SOCKET) {
    //    printf("socket failed with error: %ld\n", WSAGetLastError());
    //    freeaddrinfo(result);
    //    WSACleanup();
    //    return 1;
    //}

    //// Setup the TCP listening socket
    //iResult = bind(ListenSocket, result->ai_addr, (int)result->ai_addrlen);
    //if (iResult == SOCKET_ERROR) {
    //    printf("bind failed with error: %d\n", WSAGetLastError());
    //    freeaddrinfo(result);
    //    closesocket(ListenSocket);
    //    WSACleanup();
    //    return 1;
    //}

    //freeaddrinfo(result);

    //iResult = listen(ListenSocket, SOMAXCONN);
    //if (iResult == SOCKET_ERROR) {
    //    printf("listen failed with error: %d\n", WSAGetLastError());
    //    closesocket(ListenSocket);
    //    WSACleanup();
    //    return 1;
    //}
    //std::cout << "Starting server" << "\n";

    //// Accept a client socket
    //ClientSocket = accept(ListenSocket, NULL, NULL);
    //if (ClientSocket == INVALID_SOCKET) {
    //    printf("accept failed with error: %d\n", WSAGetLastError());
    //    closesocket(ListenSocket);
    //    WSACleanup();
    //    return 1;
    //}

    //// No longer need server socket
    //closesocket(ListenSocket);
    std::cout << "Started" << "\n";

    while (active) {

        //start accepting clients ------------------------------
        len = sizeof(server);
        client_fd = accept(server_socket, (struct sockaddr*)&server, &len);

        //our client is a real thing?
        if (client_fd != INVALID_SOCKET) {


            _clients_b client;

            //save client socket into our struct table
            client.ss = client_fd;
            client.connected = TRUE;
            client.clientID = nextID;

            clients.push_back(client);

            //and of course we need a calculator too
            clients_connected++;

            std::cout << "New client: " << client_fd << std::endl;
        }

        //we might need to add some delays cause our code might be too fast
        //commenting this function will eat your cpu like the hungriest dog ever
        //plus we don't need to loop that fast anyways
        Sleep(1);

        //receiving and sending data ---------------------------

        //we have clients or no?
        if (clients_connected > 0) {

            //lets go through all our clients
            for (int cc = 0; cc < clients.size(); cc++) {

                memset(&recvbuf, 0, sizeof(recvbuf));

                if (clients[cc].connected) {

                    //receive data
                    receiveres = recv(clients[cc].ss, recvbuf, recvbuflen, 0);
                    //printf("Bytes received: %d\n", receiveres);


                    //and do stuff with it if we receive any
                    if (receiveres > 0) {

                        Sleep(10);

                        /*std::cout << "Client data received: " << recvbuf << "\n";
                        send(client_fd, recvbuf, strlen(recvbuf), 0);*/

                        std::string messageType = deSerialize(recvbuf);
                        std::cout << "MessageType was" << messageType << "\n";
                        if (messageType == "JoinMsg") {
                            std::cout << "Sending JoinMsg back" << "\n";

                            MsgHead joinedMsg;
                            joinedMsg.id = nextID;
                            joinedMsg.length = sizeof(joinedMsg);
                            joinedMsg.type = Join;
                            joinedMsg.seq_no = seq_num;
                            char sendbuf[sizeof(joinedMsg)];
                            memcpy((void*)sendbuf, (void*)&joinedMsg, sizeof(joinedMsg));
                            nextID++;

                            // Send Join message back
                            std::cout << "Sizeof of sendbuf: " << sizeof(sendbuf) << "\n";
                            iSendResult = send(clients[cc].ss, sendbuf, sizeof(sendbuf), 0);
                            //send(client_fd, recvbuf, strlen(recvbuf), 0);
                            if (iSendResult == SOCKET_ERROR) {
                                printf("send failed with error: %d\n", WSAGetLastError());
                                closesocket(clients[cc].ss);
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
                                sendInvalidMoveMessage(moveevent->event.head.id, seq_num, clients[cc].ss, clients[cc].playerpos, iResult);
                                continue;
                            }
                            else if (moveevent->pos.y > 100 || moveevent->pos.y < -100) {
                                std::cout << "player is going over the edge on Y" << "\n";
                                sendInvalidMoveMessage(moveevent->event.head.id, seq_num, clients[cc].ss, clients[cc].playerpos, iResult);
                                //sendInvalidMoveMessage(ID, seq_num, ClientSocket, playerpos);
                                //NewPlayerPositionMsg newPlayerPosition;
                                //newPlayerPosition.msg.type = NewPlayerPosition;
                                //newPlayerPosition.msg.head.id = moveevent->event.head.id;
                                //newPlayerPosition.msg.head.seq_no = seq_num;
                                //newPlayerPosition.msg.head.type = Change;
                                //newPlayerPosition.pos.x = playerpos[0];
                                //newPlayerPosition.pos.y = playerpos[1];
                                //newPlayerPosition.msg.head.length = sizeof(newPlayerPosition);
                                //char sendbuffer[sizeof(newPlayerPosition)];


                                //std::cout << "size of movemsg was " << sizeof(newPlayerPosition) << "\n";

                                //memcpy((void*)sendbuffer, (void*)&newPlayerPosition, sizeof(newPlayerPosition));

                                //std::cout << "size of sendbuffer was " << sizeof(sendbuffer) << "\n";

                                //std::cout << "Sending back the same movemsg aka invalid move" << "\n";
                                //// Send NewPlayer message
                                //int iSendResult = send(ClientSocket, sendbuffer, iResult, 0);
                                //if (iSendResult == SOCKET_ERROR) {
                                //    printf("send failed with error: %d\n", WSAGetLastError());
                                //    closesocket(ClientSocket);
                                //    WSACleanup();
                                //    return 1;
                                //}
                                //printf("Bytes sent: %d\n", iSendResult);
                                //seq_num++;
                                continue;
                                    
                            }
                            else if (abs(moveevent->pos.x - clients[cc].playerpos[0]) > 1) {
                                std::cout << "too big jump on X" << "\n";
                                sendInvalidMoveMessage(moveevent->event.head.id, seq_num, clients[cc].ss, clients[cc].playerpos, iResult);
                                continue;
                            }
                            else if (abs(moveevent->pos.y - clients[cc].playerpos[1]) > 1) {
                                std::cout << "too big jump on X" << "\n";
                                sendInvalidMoveMessage(moveevent->event.head.id, seq_num, clients[cc].ss, clients[cc].playerpos, iResult);
                                continue;
                            }

                            else if (checkCollision(clients, moveevent, clients_connected, clients[cc].ss) == true) {
                                sendInvalidMoveMessage(moveevent->event.head.id, seq_num, clients[cc].ss, clients[cc].playerpos, iResult);
                                continue;

                            }
                            std::cout << "This movemsg was valid" << "\n";

                            NewPlayerPositionMsg newPlayerPosition;
                            newPlayerPosition.msg.type = NewPlayerPosition;
                            newPlayerPosition.msg.head.id = moveevent->event.head.id;
                            newPlayerPosition.msg.head.seq_no = seq_num;
                            newPlayerPosition.msg.head.type = Change;
                            newPlayerPosition.pos.x = moveevent->pos.x;
                            newPlayerPosition.pos.y = moveevent->pos.y;
                            newPlayerPosition.msg.head.length = sizeof(newPlayerPosition);
                            char sendbuffer[sizeof(newPlayerPosition)];

                            clients[cc].playerpos[0] = moveevent->pos.x;
                            clients[cc].playerpos[1] = moveevent->pos.y;

                            std::cout << "Updated player " << moveevent->event.head.id << " position to X:" << clients[cc].playerpos[0] << " Y: " << clients[cc].playerpos[1] << "\n";

                            std::cout << "size of movemsg was " << sizeof(newPlayerPosition) << "\n";

                            memcpy((void*)sendbuffer, (void*)&newPlayerPosition, sizeof(newPlayerPosition));


                            std::cout << "Sending the new movemsg to client_fd " << clients[cc].ss << "\n";
                            // Send MoveMsg message to all connected clients
                            for (int i = 0; i < clients_connected; i++) {
                                int iSendResult = send(clients[i].ss, sendbuffer, sizeof(sendbuffer), 0);
                                //iSendResult = send(client_fd, sendbuf, sizeof(sendbuf), 0);
                                if (iSendResult == SOCKET_ERROR) {
                                    printf("send failed with error: %d\n", WSAGetLastError());
                                    closesocket(client_fd);
                                    WSACleanup();
                                    return 0;
                                }
                                printf("Bytes sent: %d\n", iSendResult);
                                seq_num++;
                            }
                            



                        }
                        else if (messageType == "LeaveMsg") {
                            nextID--;
                            std::cout << "ClientID: " << clients[cc].clientID << " or sockID " << clients[cc].ss  << " disconnected." << std::endl;
                            clients[cc].connected = FALSE;
                            clients_connected--;
                            std::cout << "size of clients before removal and shrinking " << clients.size() << "\n";
                            clients.erase(clients.begin() + cc);
                            clients.shrink_to_fit();
                            std::cout << "size of clients after removal and shrinking " << clients.size() << "\n";
                            std::cout << "Clients left: ";
                            for (int i = 0; i < clients.size(); i++) {
                                std::cout << clients[i].ss << " ";
                            }
                            std::cout << "\n";

                            

                            for (int i = 0; i < clients_connected; i++) {
                                PlayerLeaveMsg leave;
                                leave.msg.head.id = clients[i].clientID;
                                leave.msg.head.type = Change;
                                leave.msg.head.seq_no = seq_num;
                                leave.msg.head.length = sizeof(leave);
                                leave.msg.type = PlayerLeave;
                                seq_num++;
                                char sendbuffer[sizeof(leave)];
                                memcpy((void*)sendbuffer, (void*)&leave, sizeof(leave));

                                int iSendResult = send(clients[i].ss, sendbuffer, sizeof(sendbuffer), 0);
                                //iSendResult = send(client_fd, sendbuf, sizeof(sendbuf), 0);
                                if (iSendResult == SOCKET_ERROR) {
                                    printf("send failed with error: %d\n", WSAGetLastError());
                                    closesocket(client_fd);
                                    WSACleanup();
                                    return 0;
                                }
                                printf("Bytes sent: %d\n", iSendResult);
                                seq_num++;
                            }

                        }
                    }
                    //how to close connection
                    //this just for quick example
                    //so you are just getting rid off client's socket data
                    else if (receiveres == 0 || strcmp(recvbuf, "disconnect") == 0) {

                        std::cout << "Client disconnected." << std::endl;
                        clients[cc].connected = FALSE;
                        clients_connected--;
                        //delete [cc] clients;
                    }
                }
            }
        }
    }

    bool run = true;
    //// Receive until the peer shuts down the connection
    //do {

    //    iResult = recv(ClientSocket, recvbuf, recvbuflen, 0);
    //    //std::future<char*> ret = std::async(std::launch::async, recieveOnce, ListenSocket);
    //    //char* recievebuffer = ret.get();
    //    std::string messageType = deSerialize(recvbuf);
    //    std::cout << "MessageType was" << messageType << "\n";
    //    if (iResult > 0) {
    //        printf("Bytes received: %d\n", iResult);

    //        if (messageType == "JoinMsg") {

    //            std::cout << "Sending JoinMsg back" << "\n";
    //            // Send Join message back
    //            iSendResult = send(ClientSocket, sendbuf, iResult, 0);
    //            if (iSendResult == SOCKET_ERROR) {
    //                printf("send failed with error: %d\n", WSAGetLastError());
    //                closesocket(ClientSocket);
    //                WSACleanup();
    //                return 1;
    //            }
    //            printf("Bytes sent: %d\n", iSendResult);
    //            seq_num++;
    //            JoinMsg* joinedmsg;
    //            joinedmsg = (JoinMsg*)recvbuf;

    //            //NewPlayerMsg newplayermsg;
    //            //newplayermsg.msg.head.id = ID;
    //            //newplayermsg.msg.head.seq_no = seq_num;
    //            //newplayermsg.msg.head.type = Change;
    //            //newplayermsg.msg.head.length = sizeof(newplayermsg);

    //            //newplayermsg.msg.type = NewPlayer;
    //            //newplayermsg.name = joinedmsg->name;
    //            //char sendbuffer[sizeof(newplayermsg)];
    //            //memcpy((void*)sendbuffer, (void*)&newplayermsg, sizeof(newplayermsg));

    //            //std::cout << "Sending NewPlayerMsg" << "\n";
    //            //// Send NewPlayer message
    //            //iSendResult = send(ClientSocket, sendbuffer, iResult, 0);
    //            //if (iSendResult == SOCKET_ERROR) {
    //            //    printf("send failed with error: %d\n", WSAGetLastError());
    //            //    closesocket(ClientSocket);
    //            //    WSACleanup();
    //            //    return 1;
    //            //}
    //            //printf("Bytes sent: %d\n", iSendResult);
    //            //seq_num++;

    //        }
    //        else if (messageType == "MoveEvent") {
    //            MoveEvent* moveevent = (MoveEvent*)recvbuf;
    //            if (moveevent->pos.x > 100 || moveevent->pos.x < -100) {
    //                std::cout << "player is going over the edge on X" << "\n";
    //                sendInvalidMoveMessage(ID, seq_num, ClientSocket, playerpos, iResult);
    //                continue;
    //            }
    //            else if (moveevent->pos.y > 100 || moveevent->pos.y < -100) {
    //                std::cout << "player is going over the edge on Y" << "\n";
    //                //sendInvalidMoveMessage(ID, seq_num, ClientSocket, playerpos);
    //                NewPlayerPositionMsg newPlayerPosition;
    //                newPlayerPosition.msg.type = NewPlayerPosition;
    //                newPlayerPosition.msg.head.id = ID;
    //                newPlayerPosition.msg.head.seq_no = seq_num;
    //                newPlayerPosition.msg.head.type = Change;
    //                newPlayerPosition.pos.x = playerpos[0];
    //                newPlayerPosition.pos.y = playerpos[1];
    //                newPlayerPosition.msg.head.length = sizeof(newPlayerPosition);
    //                char sendbuffer[sizeof(newPlayerPosition)];


    //                std::cout << "size of movemsg was " << sizeof(newPlayerPosition) << "\n";

    //                memcpy((void*)sendbuffer, (void*)&newPlayerPosition, sizeof(newPlayerPosition));

    //                std::cout << "size of sendbuffer was " << sizeof(sendbuffer) << "\n";

    //                std::cout << "Sending back the same movemsg aka invalid move" << "\n";
    //                // Send NewPlayer message
    //                int iSendResult = send(ClientSocket, sendbuffer, iResult, 0);
    //                if (iSendResult == SOCKET_ERROR) {
    //                    printf("send failed with error: %d\n", WSAGetLastError());
    //                    closesocket(ClientSocket);
    //                    WSACleanup();
    //                    return 1;
    //                }
    //                printf("Bytes sent: %d\n", iSendResult);
    //                seq_num++;
    //                continue;
    //            
    //            }
    //            else if (abs(moveevent->pos.x - playerpos[0]) > 1) {
    //                std::cout << "too big jump on X" << "\n";
    //                sendInvalidMoveMessage(ID, seq_num, ClientSocket, playerpos, iResult);
    //                continue;
    //            }
    //            else if (abs(moveevent->pos.y - playerpos[1]) > 1) {
    //                std::cout << "too big jump on X" << "\n";
    //                sendInvalidMoveMessage(ID, seq_num, ClientSocket, playerpos, iResult);
    //                continue;
    //            }
    //            std::cout << "This movemsg was valid" << "\n";

    //            NewPlayerPositionMsg newPlayerPosition;
    //            newPlayerPosition.msg.type = NewPlayerPosition;
    //            newPlayerPosition.msg.head.id = ID;
    //            newPlayerPosition.msg.head.seq_no = seq_num;
    //            newPlayerPosition.msg.head.type = Change;
    //            newPlayerPosition.pos.x = moveevent->pos.x;
    //            newPlayerPosition.pos.y = moveevent->pos.y;
    //            newPlayerPosition.msg.head.length = sizeof(newPlayerPosition);
    //            char sendbuffer[sizeof(newPlayerPosition)];

    //            playerpos[0] = moveevent->pos.x;
    //            playerpos[1] = moveevent->pos.y;

    //            std::cout << "size of movemsg was " << sizeof(newPlayerPosition) << "\n";

    //            memcpy((void*)sendbuffer, (void*)&newPlayerPosition, sizeof(newPlayerPosition));


    //            std::cout << "Sending the new movemsg" << "\n";
    //            // Send MoveMsg message
    //            int iSendResult = send(ClientSocket, sendbuffer, iResult, 0);
    //            if (iSendResult == SOCKET_ERROR) {
    //                printf("send failed with error: %d\n", WSAGetLastError());
    //                closesocket(ClientSocket);
    //                WSACleanup();
    //                return 0;
    //            }
    //            printf("Bytes sent: %d\n", iSendResult);
    //            seq_num++;



    //        }

    //        
    //    }
    //    else {
    //        printf("recv failed with error: %d\n", WSAGetLastError());
    //        closesocket(ClientSocket);
    //        WSACleanup();
    //        return 1;
    //    }

    //} while (run);

    // shutdown the connection since we're done
    iResult = shutdown(server_socket, SD_SEND);
    if (iResult == SOCKET_ERROR) {
        printf("shutdown failed with error: %d\n", WSAGetLastError());
        closesocket(server_socket);
        WSACleanup();
        return 1;
    }

    // cleanup
    closesocket(server_socket);
    WSACleanup();

    return 0;
}

void start_server() {
    int wsaresult, i = 1;

    WSADATA wsaData;

    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(DEFAULT_PORT);

    // Initialize Winsock
    wsaresult = WSAStartup(MAKEWORD(2, 2), &wsaData);

    //if error
    if (wsaresult != 0) {
        printf("WSAStartup failed with error: %d\n", wsaresult);
    }


    // Create a SOCKET for connecting to server
    server_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    if (server_socket == INVALID_SOCKET) {
        printf("socket failed with error: %ld\n", WSAGetLastError());
        WSACleanup();
    }

    setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, (char*)&i, sizeof(i));

    //Binding part
    wsaresult = bind(server_socket, (sockaddr*)&server, sizeof(server));

    if (wsaresult == SOCKET_ERROR) {
        printf("bind failed with error: %d\n", WSAGetLastError());
        closesocket(server_socket);
        WSACleanup();
    }

    // Setup the TCP listening socket
    wsaresult = listen(server_socket, 5);

    unsigned long b = 1;

    //make it non blocking
    ioctlsocket(server_socket, FIONBIO, &b);

    if (wsaresult == SOCKET_ERROR) {
        printf("listen failed with error: %d\n", WSAGetLastError());
        closesocket(server_socket);
        WSACleanup();
    }

}