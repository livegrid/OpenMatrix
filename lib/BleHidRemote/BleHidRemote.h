#pragma once

#include "GeneralSettings.h"

#ifdef BLE_HID_REMOTE_ENABLED

class StateManager;

using BleHidRemoteTofRangeAdjustCallback = void (*)(int deltaMm);

void bleHidRemoteSetStateManager(StateManager* stateManager);
void bleHidRemoteSetTofRangeAdjustCallback(BleHidRemoteTofRangeAdjustCallback callback);
/** Same action as the BLE remote Y button (previous slot in the remote effect ring). */
void bleHidRemotePrevEffect(void);
void bleHidRemoteTask(void* parameter);

#endif
