#pragma once

#include "GeneralSettings.h"

#ifdef BLE_HID_REMOTE_ENABLED

class StateManager;

using BleHidRemoteTofRangeAdjustCallback = void (*)(int deltaMm);

void bleHidRemoteSetStateManager(StateManager* stateManager);
void bleHidRemoteSetTofRangeAdjustCallback(BleHidRemoteTofRangeAdjustCallback callback);
/** Same action as the BLE remote A button (next slot in the remote effect ring). */
void bleHidRemoteNextEffect(void);
/** Same action as the BLE remote Y button (previous slot in the remote effect ring). */
void bleHidRemotePrevEffect(void);
/** Re-align the remote effect ring index after external mode/effect changes (e.g. idle screensaver). */
void bleHidRemoteSyncEffectRingFromState(void);
void bleHidRemoteTask(void* parameter);

#endif
