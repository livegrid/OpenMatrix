#pragma once

#include "GeneralSettings.h"

#ifdef BLE_HID_REMOTE_ENABLED

class StateManager;

using BleHidRemoteTofRangeAdjustCallback = void (*)(int deltaMm);

void bleHidRemoteSetStateManager(StateManager* stateManager);
void bleHidRemoteSetTofRangeAdjustCallback(BleHidRemoteTofRangeAdjustCallback callback);
void bleHidRemoteTask(void* parameter);

#endif
