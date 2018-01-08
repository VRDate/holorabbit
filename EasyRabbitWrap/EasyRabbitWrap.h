
#pragma once
#pragma comment(lib, "Ws2_32.lib")
#define EASYRABBITWRAP_API __declspec(dllexport) 

#include <string>
#include <amqp_tcp_socket.h>
#include <amqp.h>
#include <amqp_framing.h>

extern "C" {

	EASYRABBITWRAP_API amqp_connection_state_t * CreateConnection(const char * host, int port);
	EASYRABBITWRAP_API bool Login(amqp_connection_state_t * conn, const char * user, const char * pass);

	EASYRABBITWRAP_API amqp_bytes_t * ConnectToExchange(amqp_connection_state_t * conn, const char * exchange, const char * routingkey);
	EASYRABBITWRAP_API void SendString(amqp_connection_state_t * conn, const char * exchange, const char * routingkey, const char * messagebody);
	EASYRABBITWRAP_API bool ConsumeMessage(amqp_connection_state_t * conn, unsigned int * headersize, unsigned char *  header, unsigned int * datasize, unsigned char *  data);
	EASYRABBITWRAP_API bool SendRPC(amqp_connection_state_t * conn, const char * exchange, const char * routingkey, const char * messagebody, unsigned int * datasize, unsigned char *  data);
	bool isErrorInReply(amqp_rpc_reply_t x);
	bool isError(int x);
}