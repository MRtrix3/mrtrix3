#include <QtGui>
#include <QtNetwork>

#include "gui/mrview/sync/localsocketreader.h"


#include "exception.h" 
#include <vector>
#include <memory> //shared_ptr
namespace MR
{
  namespace GUI
  {
	namespace MRView
	{
		namespace Sync
		{
			/**
			* Retrieves information about the current status to be synced with (i.e. the state of other windows) from server, which is running as an independent process
			*/
LocalSocketReader::LocalSocketReader(QLocalSocket* mySocket) : QObject(0)
{
	socket = mySocket;
	QObject::connect(socket, SIGNAL(readyRead()), this, SLOT(OnDataReceived()));
}

void LocalSocketReader::OnDataReceived()
{
	std::vector<std::shared_ptr<QByteArray>> messagesReceived;
	while(socket->bytesAvailable() > 0)
	{		
		//First byte must always by an unsigned int32 stating how much data to read
		//Wait until we've read all of that
		int loops=0;
		while(socket->bytesAvailable() < 4)
		{
			loops++;
			socket->waitForReadyRead(1000);//NB that because we are in a slot, readyRead will not be re-emitted
			if(loops > 10)
			{
				DEBUG("OnDataReceived timeout (reading size)");
				return;
			}
		}
		
		char fourByteHeader[4];
		socket->read(fourByteHeader,4);
		unsigned int sizeOfMessage;
		memcpy(&sizeOfMessage, fourByteHeader, 4);
		
		loops=0;
		while(socket->bytesAvailable() < sizeOfMessage)
		{
			loops++;
			socket->waitForReadyRead(1000);
			if(loops > 10)
			{
				DEBUG("OnDataReceived timeout (reading data)");
				return;
			}
		}
		
		char read[sizeOfMessage];
		socket->read(read,sizeOfMessage);
		std::shared_ptr<QByteArray> readData = std::shared_ptr<QByteArray>(new QByteArray()); 
		readData->insert(0,read,sizeOfMessage);
		
		messagesReceived.emplace_back(readData);
	}
	
	emit DataReceived(messagesReceived);
}

}}}}