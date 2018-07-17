#include <QtGui>
#include <QtNetwork>

#include "gui/mrview/sync/client.h"
#include <iostream>


namespace MR
{
  namespace GUI
  {
	namespace MRView
	{
		namespace Sync
		{
			/**
			* Sends information to other processes. See interprocesssyncer.h for use
			*/
Client::Client()
{
    socket = new QLocalSocket();
	SetServerName("mrview_syncer");
}

QString Client::GetServerName()
{
	return connectToServerName;
}

void Client::SetServerName(QString connectTo)
{
	connectToServerName=connectTo;
	socket->abort();
}

bool Client::TryConnect()
{
	socket->abort();
    socket->connectToServer(connectToServerName);
	socket->waitForConnected();
	QLocalSocket::LocalSocketState state = socket->state();
	
	return state == QLocalSocket::ConnectedState;
}

void Client::sendUpdate()
{
    socket->abort();
    socket->connectToServer(connectToServerName);
}

void Client::SendData(QByteArray dat)
{
	//Prefix data with how long the message is
	QByteArray prefixedData;
	char a[4];
	unsigned int size=(unsigned int)dat.size();
	memcpy(a, &size, 4);
	prefixedData.insert(0,a,4);
	prefixedData.insert(4,dat,dat.size());
	
	//send
	socket->write(prefixedData.data(),prefixedData.size());
}

}}}}