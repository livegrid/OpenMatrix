#pragma once

#include "GeneralSettings.h"

#ifdef BLE_HID_REMOTE_ENABLED

class StateManager;

using BleHidRemoteTofRangeAdjustCallback = void (*)(int deltaMm);
/** direction: +1 = next performance mode, -1 = previous. */
using BleHidRemotePerformanceModeCallback = void (*)(int direction);
/** Manual brightness step (typically +/-10). */
using BleHidRemoteBrightnessAdjustCallback = void (*)(int delta);
/** Toggle aquarium demo start/stop (MOCUTE report suffix 00 00 09 02 00). */
using BleHidRemoteDemoToggleCallback = void (*)();
/**
 * TOP button (… 00 FF 09 …): if not in Aquarium, switch to it; if already there,
 * toggle fake high CO2 for video demos.
 */
using BleHidRemoteAquariumButtonCallback = void (*)();

void bleHidRemoteSetStateManager(StateManager* stateManager);
void bleHidRemoteSetTofRangeAdjustCallback(BleHidRemoteTofRangeAdjustCallback callback);
void bleHidRemoteSetPerformanceModeCallback(BleHidRemotePerformanceModeCallback callback);
void bleHidRemoteSetBrightnessAdjustCallback(BleHidRemoteBrightnessAdjustCallback callback);
void bleHidRemoteSetDemoToggleCallback(BleHidRemoteDemoToggleCallback callback);
void bleHidRemoteSetAquariumButtonCallback(BleHidRemoteAquariumButtonCallback callback);
/** Same action as the BLE remote A button (next slot in the remote effect ring). */
void bleHidRemoteNextEffect(void);
/** Same action as the BLE remote Y button (previous slot in the remote effect ring). */
void bleHidRemotePrevEffect(void);
/** Re-align the remote effect ring index after external mode/effect changes (e.g. idle screensaver). */
void bleHidRemoteSyncEffectRingFromState(void);
void bleHidRemoteTask(void* parameter);

#endif
