#pragma once

#include "GeneralSettings.h"

#ifdef BLE_HID_REMOTE_ENABLED

class StateManager;

void bleHidRemoteSetStateManager(StateManager* stateManager);
void bleHidRemoteTask(void* parameter);

#endif
