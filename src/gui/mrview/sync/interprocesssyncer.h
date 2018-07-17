#ifndef __sync_interprocesssyncer_h__
#define __sync_interprocesssyncer_h__

#ifdef Q_WS_MAEMO_5
#include <QWidget>
#else
#include <QDialog>
#endif

#include "gui/mrview/sync/localsocketreader.h"
#include <qlocalsocket.h>
#include "gui/mrview/sync/client.h"
#include <vector>
#include <memory> //shared_ptr


class QDialogButtonBox;
class QLabel;
class QLineEdit;
class QPushButton;
class QLocalSocket;

#define MAX_NO_ALLOWED 32 //maximum number of inter process syncers that are allowed

namespace MR
{
  namespace GUI
  {
	namespace MRView
	{
		namespace Sync
		{


class InterprocessSyncer : public QObject
{
    Q_OBJECT

public:
    InterprocessSyncer();
	static void Int32ToChar(char a[], int n);
	static int CharTo32bitNum(char a[]);
	bool SendData(QByteArray data);//sends data to be synced
	
private slots:
	void OnNewIncomingConnection();
	void OnDataReceived(std::vector<std::shared_ptr<QByteArray>> dat);
	
signals:
	void SyncDataReceived(std::vector<std::shared_ptr<QByteArray>> data); //fires when data is received which is for syncing. It is up to listeners to validate and store this value
	
 private:
	int id;
	std::vector<std::shared_ptr<GUI::MRView::Sync::Client>> senders;//send information
	QLocalServer *receiver;//listens for information
	void TryConnectTo(int connectToId);//tries to connect with another interprocesssyncer
	

};	

}}}}
#endif