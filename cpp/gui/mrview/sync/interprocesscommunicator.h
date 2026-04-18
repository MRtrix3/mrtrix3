/* Copyright (c) 2008-2026 the MRtrix3 contributors.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Covered Software is provided under this License on an "as is"
 * basis, without warranty of any kind, either expressed, implied, or
 * statutory, including, without limitation, warranties that the
 * Covered Software is free of defects, merchantable, fit for a
 * particular purpose or non-infringing.
 * See the Mozilla Public License v. 2.0 for more details.
 *
 * For more details, see http://www.mrtrix.org/.
 */
#pragma once

#include <qlocalsocket.h>

#include "mrview/sync/client.h"
#include "mrview/sync/localsocketreader.h"

namespace MR::GUI::MRView::Sync {

// maximum number of inter process syncers that are allowed. This can be
// raised, but may reduce performance when new IPS are created.
constexpr ssize_t maximum_instances = 32;

/**
 * Sends and receives information from other MRView processes
 */
class InterprocessCommunicator : public QObject {
  Q_OBJECT

public:
  InterprocessCommunicator();
  bool SendData(QByteArray data); // sends data to be synced

private slots:
  void OnNewIncomingConnection();
  void OnDataReceived(std::vector<std::shared_ptr<QByteArray>> dat);

signals:
  // Fires when data is received which is for syncing.
  // It is up to listeners to validate and store this value
  void SyncDataReceived(std::vector<std::shared_ptr<QByteArray>> data);

private:
  int id;                                                          // id which is unique between mrview processes
  std::vector<std::shared_ptr<GUI::MRView::Sync::Client>> senders; // send information
  QLocalServer *receiver;                                          // listens for information
  void TryConnectTo(int connectToId); // tries to connect with another interprocesscommunicator
};

} // namespace MR::GUI::MRView::Sync
