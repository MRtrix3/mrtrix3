#include <QtGui>
#include <QtNetwork>

#include "gui/mrview/sync/localsocketreader.h"
#include "gui/mrview/sync/interprocesssyncer.h"
#include "gui/mrview/sync/enums.h"
#include <iostream> //temp
#include <memory> //shared_ptr
#include "exception.h" 
#define MAX_NO_ALLOWED 32 //maximum number of inter process syncers that are allowed
namespace MR
{
  namespace GUI
  {
	namespace MRView
	{
		namespace Sync
		{
			InterprocessSyncer::InterprocessSyncer(): QObject(0)
			{
				bool connected=false;
				id=-1;
				int freeEntry;
				for(int i_attempt=0;i_attempt<100;i_attempt++)
				{
					//Search for a free id
					QLocalSocket *socket = new QLocalSocket(this);
					freeEntry=-1;
					for(int i = 0; i < MAX_NO_ALLOWED; i++)
					{
						QString serverName="mrtrix_interprocesssyncer_" + QString::number(i);
						socket->connectToServer(serverName);
						socket->waitForConnected();
						QLocalSocket::LocalSocketState state = socket->state();
						socket->abort();
						if(state != QLocalSocket::ConnectedState)
						{
							//we found a free name
							freeEntry=i;
							QLocalServer::removeServer(serverName);//this can be needed when previous processes used this but were forcibly killed, leaving a temporary file. If that file exists, then we will get an exception ("address in use") when trying to use this Id
							break;
						}
					}
					if(freeEntry==-1)
					{
						std::string errMsg="No free ids available";
						throw std::runtime_error(errMsg);
					}
					
					
					receiver = new QLocalServer(this);
					connected=receiver->listen("mrtrix_interprocesssyncer_" + QString::number(freeEntry));
					if (connected) 
					{
						id = freeEntry;
						connect(receiver, SIGNAL(newConnection()), this, SLOT(OnNewIncomingConnection()));
						
						break;
					}
					//else we didn't connect OK. This can occur when two processes start simultaneously and both try for the same number. Loop back and re-try
				}
				
				if(!connected)
				{
					throw std::runtime_error("Could not connect to any free id");
				}
				
				//Find all servers to connect to
				for(int i = 0; i < MAX_NO_ALLOWED; i++)
				{
					TryConnectTo(i);
				}
				
			}

			void InterprocessSyncer::OnNewIncomingConnection()
			{
				LocalSocketReader* lsr = new LocalSocketReader(receiver->nextPendingConnection());
				connect(lsr, SIGNAL(DataReceived(std::vector<std::shared_ptr<QByteArray>>)), this, SLOT(OnDataReceived(std::vector<std::shared_ptr<QByteArray>>)));
			}
			
			void InterprocessSyncer::TryConnectTo(int connectToId)
			{
				if(connectToId!=id)//don't connect to ourself!
				{
					QString serverName="mrtrix_interprocesssyncer_" + QString::number(connectToId);
				
					//check we are not already connected
					for(unsigned int i =0; i < senders.size(); i++)
					{
						if(senders[i]->GetServerName() == serverName)
						{
							//we have already connected to this
							return;
						}
					}
					
					std::shared_ptr<GUI::MRView::Sync::Client> curCl = std::shared_ptr<GUI::MRView::Sync::Client>(new GUI::MRView::Sync::Client());

					curCl->SetServerName(serverName);
					if(curCl->TryConnect())
					{
						//Save this connection
						senders.emplace_back(curCl);
						
						//Send it our id so that it connects back to us (i.e. two-way syncing)
						char a[8];
						Int32ToChar(a,(int)MessageKey::ConnectedID);
						char* aOffset =a;
						aOffset+=4;
						Int32ToChar(aOffset,id);
						QByteArray dat;
						dat.insert(0,a,8);
						curCl->SendData(dat);
					}
					//else we couldn't connect - likely that there was nothing to connect to
				}
			}

			void InterprocessSyncer::OnDataReceived(std::vector<std::shared_ptr<QByteArray>> allMessages)
			{
				std::vector<std::shared_ptr<QByteArray>> toSync;
				
				for(size_t i=0; i < allMessages.size();i++)
				{
					std::shared_ptr<QByteArray> dat= allMessages[i];
					char *data = dat->data();
					int dataLength=dat->size();

					if(dataLength < 4)
					{
						DEBUG("Bad data received to interprocesssyncer: too short");
						continue;
					}
					int code = CharTo32bitNum(data);
					data+=4;
					dataLength-=4;
				
					switch(code)
					{
						case (int)MessageKey::ConnectedID:
						{	
							//The other process has told us to update it when we change our values
							int idOfOtherProcess=CharTo32bitNum(data);
							TryConnectTo(idOfOtherProcess);
							break;
						}
						case (int)MessageKey::SyncData:
						{	
							std::shared_ptr<QByteArray> trimmed=std::shared_ptr<QByteArray>(new QByteArray());
							trimmed->insert(0,data,dataLength);
							toSync.emplace_back(trimmed);
							break;
						}
						default:
						{
							DEBUG("Bad data received to interprocesssyncer: unknown code");
						}		
					}
				}
				
				//We notify all listeners now that we have received this signal.
				//It is up to those listeners to check its validity and let us know if they have changed their value
				emit SyncDataReceived(toSync); 
			}


			/**
			* Sends data for syncing with other processes
			*/
			bool InterprocessSyncer::SendData(QByteArray dat)
			{
				//We only send sync data if we are the active application of the OS. This is to avoid 
				//infinite loops, which are easy to induce on slow systems otherwise. There are other 
				//ways to avoid these loops (like locking files in linux), but they tend to be awkward,
				//not cross-platform, probably poor performance, and/or require much more programming 
				//than below.
					
				if (QApplication::activeWindow() != 0 && QApplication::focusWidget() != 0)
				{
					//make an array: the message key followed by the message
					//--Key
					char codeAsChar[4];
					InterprocessSyncer::Int32ToChar(codeAsChar,(int)MessageKey::SyncData);
					QByteArray data;
					data.insert(0,codeAsChar,4);
					//--Data
					data.insert(4,dat.data(),dat.size());
					
					//send to all senders
					bool allOk=true;
					for(int i=senders.size()-1; i >= 0; i--)
					{
						try
						{
							senders[i]->SendData(data);
						}
						catch(...)
						{
							DEBUG("Send Error");
							allOk=false;
							//sending error. 
							//TODO: send again or disconnect this item
						}
					}
					return allOk;
				}
				else
				{
					//We are not the active window
					return false;
				}
				
			}

			void InterprocessSyncer::Int32ToChar(char a[], int n) {
			  memcpy(a, &n, 4);
			}

			int InterprocessSyncer::CharTo32bitNum(char a[]) {
			  int n = 0;
			  memcpy(&n, a, 4);
			  return n;
			}


}}}}