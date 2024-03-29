/*
	author: Macro Gu
	email: macrogu@qq.com
	QQ: 877188891
*/

#include <iostream>
#include <stdint.h>
#include <nlohmann/json.hpp>
#include "AcceptServer.h"
#include "protocol.h"
#include "ServerConf.h"

WebSocketWatcher::WebSocketWatcher()
{

}

WebSocketWatcher::~WebSocketWatcher()
{

}

int WebSocketWatcher::OnEpollAcceptEvent(stSocketContext &socket_context)
{
	int conn_sock = socket_context.fd;
	std::string clientIP = socket_context.client_ip;
	LOG_DEBUG("accept client IP: %s, connected fd: %d" , clientIP.c_str() , conn_sock);

	return 0;
}

int WebSocketWatcher::OnEpollReadableEvent(int &epollfd, epoll_event &event)
{
	if (event.data.ptr == nullptr)
	{
		LOG_ERROR("event.data.ptr is nullptr , epollfd: %d" , epollfd);
		return false;
	}

	stSocketContext *socket_context = (stSocketContext *)event.data.ptr;
	int fd = socket_context->fd;

	char read_buffer[SS_READ_BUFFER_SIZE];
	memset(read_buffer, 0, SS_READ_BUFFER_SIZE);

	int read_size = recv(fd, read_buffer, SS_READ_BUFFER_SIZE, 0);
	if (read_size == -1 && errno == EINTR) {
		return READ_CONTINUE;
	}
	if (read_size == -1 /* io err*/ || read_size == 0 /* close */)
	{
		return READ_CLOSE;
	}
	//DEBUG_MSG("read success which read size: " << read_size);


	// read_buffer can be parsed data here
	this->HandleClientWebSocketData(socket_context, read_buffer, read_size);


	return READ_OVER;
}

int WebSocketWatcher::OnEpollWriteableEvent(stSocketContext &socket_context)
{
	int client_fd = socket_context.fd;
	std::cout << "send data to client " << std::endl;
	int ret = send(socket_context.fd, socket_context.stToClient.c_str(), socket_context.stToClient.length(), 0);
	if (ret < 0)
	{
		LOG_ERROR("send data to client failed ! client IP: %s", socket_context.client_ip.c_str());
	}

	return 0;
}

int WebSocketWatcher::OnEpollCloseEvent(stSocketContext &socket_context)
{
	LOG_DEBUG("close client client ip: %s client fd: %d" , socket_context.client_ip.c_str() , socket_context.fd);

	return 0;
}

bool WebSocketWatcher::HandleClientWebSocketData(stSocketContext *socket_context, char clientData[], int dataLength)
{
	if (socket_context == nullptr)
	{
		LOG_ERROR("epoll_context is nullptr , dataLength: %d" , dataLength);
		return false;
	}

	LOG_INFO("get client message ID: %d ,size: %d" , socket_context->uiMessageID , dataLength);

	std::string tempData = clientData;
	bool isWS = webSocketStyle.isWSHandShake(tempData);
	if (isWS)
	{
		std::cout << " client: " << std::endl << tempData << std::endl;
		webSocketStyle.wsHandshake(tempData, socket_context->stToClient);
	}
	else
	{
		std::string sendData = "";
		std::string recvData = "";
		webSocketStyle.wsDecodeFrame(tempData, recvData);
		webSocketStyle.wsEncodeFrame(recvData, socket_context->stToClient, WebSocketOpCode::TextFrame);
	}

	return true;
}


NormalSocketWatcher::NormalSocketWatcher()
{

}

NormalSocketWatcher::~NormalSocketWatcher()
{

}

int NormalSocketWatcher::OnEpollAcceptEvent(stSocketContext &socket_context)
{
	int conn_sock = socket_context.fd;
	std::string clientIP = socket_context.client_ip;
	LOG_DEBUG("accept client IP: %s, connected fd: %d", clientIP.c_str(), conn_sock);

	return 0;
}

int NormalSocketWatcher::OnEpollReadableEvent(int &epollfd, epoll_event &event)
{
	if (event.data.ptr == nullptr)
	{
		LOG_ERROR("event.data.ptr is nullptr , epollfd: %d", epollfd);
		return false;
	}

	stSocketContext *socket_context = (stSocketContext *)event.data.ptr;
	int fd = socket_context->fd;

	char read_buffer[SS_READ_BUFFER_SIZE];
	memset(read_buffer, 0, SS_READ_BUFFER_SIZE);

	int read_size = recv(fd, read_buffer, SS_READ_BUFFER_SIZE, 0);
	if (read_size == -1 && errno == EINTR) {
		return READ_CONTINUE;
	}
	if (read_size == -1 /* io err*/ || read_size == 0 /* close */)
	{
		return READ_CLOSE;
	}
	//DEBUG_MSG("read success which read size: " << read_size);


	// read_buffer can be parsed data here
	this->HandleClientNormalSocketData(socket_context, read_buffer, read_size);


	return READ_OVER;
}

int NormalSocketWatcher::OnEpollWriteableEvent(stSocketContext &socket_context)
{
	int client_fd = socket_context.fd;
	std::cout << "send data to client " << std::endl;
	int ret = send(socket_context.fd, socket_context.stToClient.c_str(), socket_context.stToClient.length(), 0);
	if (ret < 0)
	{
		LOG_ERROR("send data to client failed ! client IP: %s", socket_context.client_ip.c_str());
	}

	return 0;
}

int NormalSocketWatcher::OnEpollCloseEvent(stSocketContext &socket_context)
{
	LOG_DEBUG("close client client ip: %s client fd: %d", socket_context.client_ip.c_str(), socket_context.fd);

	return 0;
}

bool NormalSocketWatcher::HandleClientNormalSocketData(stSocketContext *socket_context, char clientData[], int dataLength)
{
	socket_context->stToClient = clientData;
	return true;
}

CAcceptServer::CAcceptServer()
{
}

CAcceptServer::~CAcceptServer()
{
	WebSocketPool.StopEpoll();
}

bool CAcceptServer::StartServer()
{
	if (!ServerConf::GetInstance().loadConf(WEBSOCKET_ADDRESS_INFO))
	{
		LOG_ERROR("websocket address info configure file read failed!");
		return false;
	}

	if (!ServerConf::GetInstance().loadConf(NORMALSOCKET_ADDRESS_INFO))
	{
		LOG_ERROR("normal socket address info configure file read failed!");
		return false;
	}

// 	WebSocketPool.SetAddressInfo(WS_ADDRESS_INFO_CONFIGURE);
// 	WebSocketPool.SetSocketWatcher(new WebSocketWatcher());
// 
// 	if (!WebSocketPool.StartEpoll()) return false;

	NormalSocketPool.SetAddressInfo(NS_ADDRESS_INFO_CONFIGURE);
	NormalSocketPool.SetSocketWatcher(new NormalSocketWatcher());

	if (!NormalSocketPool.StartEpoll()) return false;

	return true;
}