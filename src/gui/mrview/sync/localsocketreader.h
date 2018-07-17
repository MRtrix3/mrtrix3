#ifndef __sync_localsocketreader_h__
#define __sync_localsocketreader_h__

#include <QtGui>
#include <QtNetwork>

#include <qlocalsocket.h>
#include <iostream> //temp
#include <vector>
#include <memory> //shared_ptr
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
	void DataReceived(std::vector<std::shared_ptr<QByteArray>> dat);//emits every message currently available
	
	
private slots:
    void OnDataReceived();
	
	
private:
    QLocalSocket *socket;
};	

}}}}
#endif