// Copyright (c) 2026 Microsoft GmbH.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_BROWSER_NET_NETWORK_SERVICE_RESTART_OBSERVER_H_
#define ELECTRON_SHELL_BROWSER_NET_NETWORK_SERVICE_RESTART_OBSERVER_H_

#include "base/observer_list_types.h"

// Observer interface for Network Service restart notifications.
class NetworkServiceRestartObserver : public base::CheckedObserver {
 public:
  // Called after the Network Service has been recreated following a crash.
  // Observers should use this to refresh any cached URLLoaderFactory instances.
  virtual void OnNetworkServiceRestarted() = 0;
};

#endif  // ELECTRON_SHELL_BROWSER_NET_NETWORK_SERVICE_RESTART_OBSERVER_H_
