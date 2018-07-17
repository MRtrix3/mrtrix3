#ifndef __sync_client_h__
#define __sync_client_h__

#ifdef Q_WS_MAEMO_5
#include <QWidget>
#else
#include <QDialog>
#endif

#include <qlocalsocket.h>

class QDialogButtonBox;
class QLabel;
class QLineEdit;
class QPushButton;
class QLocalSocket;


namespace MR
{
  namespace GUI
  {
	namespace MRView
	{
		namespace Sync
		{

class Client
{

public:
    //Client(QWidget *parent = 0);
	Client();
	Client(const Client&) = delete;
	bool TryConnect();
	void SetServerName(QString connectTo);//sets the name of the server to connect to
	QString GetServerName();//gets the name of the server this is connected to
	void SendData(QByteArray dat);
	
private slots:
    void sendUpdate();

private:
	QString connectToServerName;//the name of the server to connect to
    QLocalSocket *socket;
};	

}}}}
#endif