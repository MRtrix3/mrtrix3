#ifndef __sync_syncmanager_h__
#define __sync_syncmanager_h__

//#include <QtNetwork>
#include "gui/mrview/sync/enums.h"
#include "gui/mrview/sync/interprocesssyncer.h"
#include "gui/mrview/window.h"

//#include "gui/gui.h"
//#include "command.h"
#include "progressbar.h"
#include "memory.h"
#include "gui/mrview/icons.h"
#include "gui/mrview/window.h"
#include "gui/mrview/mode/list.h"
#include "gui/mrview/tool/list.h"
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
* Syncs values in mrview to the interprocess syncer
*/

class SyncManager:public QObject
{
Q_OBJECT

public:
	SyncManager();
	void SetWindow(MR::GUI::MRView::Window* wind);
	
	
	
	
private slots:
    void OnWindowFocusChanged();
	void OnIPSDataReceived(std::vector<std::shared_ptr<QByteArray>> all_messages);
	
private:
	MR::GUI::MRView::Window* win;
    InterprocessSyncer* ips;
	QByteArray ToQByteArray(Eigen::Vector3f data);
	Eigen::Vector3f FromQByteArray(QByteArray vec, unsigned int offset);
	bool SendData(DataKey code, QByteArray data);
};	

}}}}
#endif