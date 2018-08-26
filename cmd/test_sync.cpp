/*
 * Copyright (c) 2008-2018 the MRtrix3 contributors.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, you can obtain one at http://mozilla.org/MPL/2.0/
 *
 * MRtrix3 is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * For more details, see http://www.mrtrix.org/
 */


#include "debug.h"
#include "command.h"
#include "file/mmap.h"

#include <atomic>
#include <chrono>
#include <thread>
#include <fcntl.h>
#include <unistd.h>

using namespace MR;
using namespace App;


void usage ()
{

  AUTHOR = "J-Donald Tournier (d.tournier@brain.org.au)";

  SYNOPSIS = "tesing purposes";

  ARGUMENTS
    + Argument ("image", "the input image(s).").type_various();

}













constexpr int num_el = 100;

class SyncData {
  public:
    float data [num_el];
};


class Sync
{
  public:
    class SHM {
      public:
        std::atomic<int> state;
        std::atomic<int> pid_lock;
        SyncData data;
    };


    Sync ();
    Sync (const Sync&) = delete;
    Sync& operator=(const Sync&) = delete;
    Sync (Sync&&) = delete; // could use default, but would need to implement
    Sync& operator=(Sync&&) = delete; // File::MMap move operators...

    bool need_update () const
    {
      return previous_state != int(shm.state);
    }

    void write (const SyncData& data)
    {
      lock();
      memcpy (&shm.data, &data, sizeof(SyncData));
      previous_state = ++shm.state;
      release();
    }

    void read (SyncData& data)
    {
      lock();
      previous_state = shm.state;
      memcpy (&data, &shm.data, sizeof(SyncData));
      release();
    }

  private:
    File::MMap mmap;
    SHM& shm;
    int previous_state;

    void lock () {
      int expected = 0, attempts = 0;
      while (!shm.pid_lock.compare_exchange_weak (expected, getpid())) {
        if (++attempts > 50) { // try for at least 50ms
          WARN ("lock has not been released! Trying to grab lock");
          if (shm.pid_lock.compare_exchange_weak (expected, getpid()))
            return;
          // PID stored in lock has changed - assuming another process grabbed
          // the lock in the meantime. Go back to trying to acquire lock:
          WARN ("lock grabbed by another process - trying again");
          attempts = 0;
        }
        expected = 0;
        std::this_thread::sleep_for (std::chrono::milliseconds(1));
      }
    }

    void release () {
      int pid = getpid();
      if (!shm.pid_lock.compare_exchange_weak (pid, 0))
        WARN ("error releasing lock! PID in lock is not ours");
    }
};




// to go into cpp file:

constexpr const char* sync_file = ".mrview.sync";

namespace {

  inline File::Entry __get_entry ()
  {
    const std::string sync_file_name = Path::join (getenv ("HOME"), sync_file);

    int fid = open (sync_file_name.c_str(), O_CREAT | O_RDWR
#ifdef _WIN32 // needed on windows...
        | O_BINARY
#endif
        , 0600);
    if (fid < 0)
      throw Exception ("error opening sync file \"" + sync_file_name + "\": " + std::strerror (errno));

    size_t size = ftruncate (fid, sizeof(Sync::SHM));
    close (fid);
    if (size)
      throw Exception ("cannot resize file \"" + sync_file_name + "\": " + strerror (errno));

    return { sync_file_name };
  }

}



Sync::Sync () :
  mmap (__get_entry(), true, false, sizeof(SHM)),
  shm (*reinterpret_cast<SHM*>(mmap.address())),
  previous_state (shm.state)
{
  if (ATOMIC_INT_LOCK_FREE != 2)
    WARN("atomic<int> is not guaranteed lock-free");
}


// end of Sync class








void prepare_sync_data (SyncData& data)
{
  float sum = 0.0;
  for (int n = 0; n < num_el-1; ++n)
    sum += data.data[n] = float (std::rand()) / RAND_MAX;
  // last value is sum of the rest - used as checksum in verify
  data.data[num_el-1] = sum;
}



int verify_sync_data (const SyncData& data)
{
  float sum = 0.0;
  for (int n = 0; n < num_el-1; ++n)
    sum += data.data[n];
  // lasst value should be sum of the rest:
  return data.data[num_el-1] != sum;
}



void run ()
{
  std::srand (getpid());
  Sync sync;
  SyncData data;


  size_t total = 0, failures = 0;
  while (true) {
    for (int n = 0; n < 100000; ++n) {
      prepare_sync_data (data);
      sync.write (data);
      sync.read (data);
      failures += verify_sync_data (data);
      total++;
    }
    std::cerr << failures << " / " << total << "\n";
  }


}

