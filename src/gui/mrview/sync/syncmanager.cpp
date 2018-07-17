#include "gui/mrview/sync/syncmanager.h"
#include "gui/mrview/window.h"
#include <iostream>
#include <vector>
#include <memory> //shared_ptr
#include "gui/mrview/sync/enums.h"

namespace MR
{
  namespace GUI
  {
	namespace MRView
	{
		namespace Sync
		{



		SyncManager::SyncManager() : QObject(0)
		{
			ips = new InterprocessSyncer();	
			connect(ips,SIGNAL(SyncDataReceived(std::vector<std::shared_ptr<QByteArray>>)),this,SLOT(OnIPSDataReceived(std::vector<std::shared_ptr<QByteArray>>)));
		}
	
		void SyncManager::SetWindow(MR::GUI::MRView::Window* wind)
		{
			/*if(wind)
			{
				std::cout<<"ok";
			}
			if(win)
			{
				//detaching from the existing window is not implemented
				std::cout<<"boo";//throw std::runtime_error("Window can only be set once");
				wind-->show();	
			}
			else
				*/
			
				win=wind;
				connect(win,SIGNAL(focusChanged()),this,SLOT(OnWindowFocusChanged()));
				
		}

		void SyncManager::OnWindowFocusChanged()
		{
			if(win->sync_focus_on ())
			{
				Eigen::Vector3f foc = win->focus();
				SendData(DataKey::WindowFocus,ToQByteArray(foc));
			}
		}
		
		bool SyncManager::SendData(DataKey code, QByteArray dat)
		{		
			QByteArray data;
			char codeAsChar[4];
			InterprocessSyncer::Int32ToChar(codeAsChar,(int)code);
			data.insert(0,codeAsChar,4);
			data.insert(4,dat,dat.size());
			
			return ips->SendData(data);
		}
		void SyncManager::OnIPSDataReceived(std::vector<std::shared_ptr<QByteArray>> all_messages)
		{
			//WARNING This code assumes that the order of syncing operations does not matter
			
			//We have a list of messages found
			//Categorise these. Only keep the last value sent for each message type, or we will change to an old value and then update other processes to this old value
			std::shared_ptr<QByteArray> winFocus=0;
			
			for(size_t i=0; i < all_messages.size();i++)
			{
				std::shared_ptr<QByteArray> data=all_messages[i];
				
				if(data->size() < 4)
				{
					DEBUG("Bad data received to syncmanager: too short");
					continue;
				}

				int idOfDataEntry=InterprocessSyncer::CharTo32bitNum(data->data());
					
				if(idOfDataEntry>1)
				{
					return;
				}
				
				switch(idOfDataEntry)
				{
					case (int)DataKey::WindowFocus:
					{
						winFocus = data;
						break;
					}
					default:
					{
						DEBUG("Unknown data key received: " + idOfDataEntry);
						break;
					}
				}
			}
			
			
			if(winFocus && win->sync_focus_on())
			{
				unsigned int offset=4;//we have already read 4 bytes, above
				//Read three single point floats 
				if(winFocus->size() != (int)(offset + 12)) //cast to int to avoid compiler warning
				{
					DEBUG("Bad data received to sync manager: wrong length (window focus)");
				}
				else
				{				
					Eigen::Vector3f vec= FromQByteArray(*winFocus,offset);
					
					//Check if already set to this value - Basic OOP: don't trust window to check things are changed before emitting a signal that the value has changed!
					Eigen::Vector3f win_vec =win->focus();
					if(win_vec[0] != vec[0] || win_vec[1] != vec[1] || win_vec[2] != vec[2])
					{
						//Send to window
						win->set_focus(vec); 
					}
				}
			}
			

			//Redraw the window. 
			win->updateGL();		
		}
		QByteArray SyncManager::ToQByteArray(Eigen::Vector3f data)
		{
			char a[12];
			memcpy(a, &data, 12);
			QByteArray q;
			q.insert(0,a,12); //don't use the constructor; it ignores any data after hitting a \0
			return q;
		}
		Eigen::Vector3f SyncManager::FromQByteArray(QByteArray data, unsigned int offset)
		{
			Eigen::Vector3f read;
			memcpy(&read,data.data()+offset, 12);
			return read;
		}

}}}}