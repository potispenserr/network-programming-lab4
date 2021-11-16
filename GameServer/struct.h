#pragma once
#define MAXNAMELEN 32
enum MsgType {
	Join,// Client joining game at server  
	Leave,// Client leaving game  
	Change,// Information to clients  
	Event,// Information from clients to server  
	TextMessage// Send text messages to one or all
};
enum ObjectDesc { 
	Human, 
	NonHuman,
	Vehicle,
	StaticObject 
};

enum ObjectForm{ 
	Cube, 
	Sphere, 
	Pyramid, 
	Cone 
};

struct Coordinate
{
	int x;
	int y;
};

// Included first in all messages
struct MsgHead{ 
	unsigned int length;// Total length for whole message  
	unsigned int seq_no;  // Sequence number  
	unsigned int id;// Client ID or 0;  
	MsgType type;// Type of message 
};
struct JoinMsg{  
	MsgHead head;  
	ObjectDesc desc;
	ObjectForm form;
	char name[MAXNAMELEN] = "Steffe\0";
};
struct LeaveMsg {
	MsgHead head;
};

enum ChangeType {
	NewPlayer,
	PlayerLeave,
	NewPlayerPosition
};
//Included first in all Changemessages 
struct ChangeMsg {
	MsgHead head;
	ChangeType type;
};

struct NewPlayerMsg {
	ChangeMsg msg;//Change message header with new client id 
	ObjectDesc desc;
	ObjectForm form;
	char name[MAXNAMELEN] = "Steffe\0";; //null terminated!,or empty
};
struct PlayerLeaveMsg
{
	ChangeMsg msg;  //Change  message header with new client id 
};

struct NewPlayerPositionMsg
{
	ChangeMsg  msg;  //Change  message header 
	Coordinate  pos;  //New object position 
	Coordinate  dir;  //New object direction 
};

enum EventType
{
	Move
};
// Included  first in all Event messages 
struct EventMsg
{
	MsgHead  head;
	EventType  type;
};

struct MoveEvent
{
	EventMsg event; 
	Coordinate pos; //New object position 
	Coordinate dir; //New object direction 

};