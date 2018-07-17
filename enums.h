#ifndef __sync_enums_h__
#define __sync_enums_h__

namespace MR
{
  namespace GUI
  {
	namespace MRView
	{
		namespace Sync
		{
			/**
			* Message keys used in interprocess syncer
			*/
			enum class MessageKey {
				ConnectedID=1,
				SyncData=2 //Data to be synced
			};
			/**
			* The type of data being sent
			*/
			enum class DataKey {
				WindowFocus=1
			};

		}
	}
  }
}
#endif