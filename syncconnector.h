#ifndef __sync_localsocketreader_h__
#define __sync_localsocketreader_h__

#include <QtGui>
#include <QtNetwork>


#ifdef Q_WS_MAEMO_5
#include <QWidget>
#else
#include <QDialog>
#endif

#include <qlocalsocket.h>
#include <iostream> //temp

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
/**
* Auto reads data from its local socket when data arrives, and fires an event with that data attached
*/

class LocalSocketReader:public QObject
{
Q_OBJECT

public:
	LocalSocketReader(QLocalSocket* mySocket);
	
	
signals:
	void DataReceived(QByteArray dat);
	
	
private slots:
    void OnDataReceived();
	
	
private:
    QLocalSocket *socket;
	QByteArray readData;
};	

}}}}
#endif