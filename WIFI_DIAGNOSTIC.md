# WiFi Connection Diagnostic Guide

## Issue: AUTH_EXPIRE (Reason: 2)

Your logs show repeated `AUTH_EXPIRE` errors, which almost always means:

### **Most Likely Cause: Wrong Password**
- Double-check the password for "Tardigrade"
- WiFi passwords are **case-sensitive**
- Check for extra spaces at the beginning or end
- Make sure there are no hidden/special characters

### Quick Verification Steps:

1. **Verify the password on another device:**
   - Try connecting a phone/laptop to "Tardigrade" 
   - Use the EXACT same password: `chocolate-milk`
   - If it fails, the password in the code is wrong

2. **Check router settings:**
   - Log into your router at 192.168.1.1 (or your gateway IP)
   - Find "Tardigrade" in wireless settings
   - Verify:
     - **Band**: Must be 2.4GHz (NOT 5GHz only)
     - **Security**: WPA2-PSK or WPA2/WPA3 mixed (NOT WPA3-only)
     - **Password**: Confirm it matches what's in the code

3. **Test with different credentials:**
   - Try one of your other WiFi networks first
   - Comment out the Tardigrade line and use another:

```cpp
// In WebServerManager.h:
// #define WIFI_SSID "Tardigrade"
// #define WIFI_PASSWORD "chocolate-milk"
#define WIFI_SSID "Pixel_9363"
#define WIFI_PASSWORD "yellow22"
```

## New Diagnostic Features Added

The updated code now includes:

### 1. **Network Scanner**
Before connecting, it will scan and show all nearby networks:
```
[WiFi] Found X networks
[WiFi] 0: NetworkName (RSSI: -45, Encryption: 3, Ch: 6)
[WiFi] ✓ Target SSID 'Tardigrade' found! RSSI: -XX dBm
```

If you don't see "Target SSID 'Tardigrade' found!", the network isn't visible because:
- Router is off
- Router is 5GHz only
- SSID name is wrong
- Network is hidden

### 2. **Password Character Verification**
Shows first and last character of password to catch hidden characters:
```
[WiFi] Password verification (first/last chars): 'c'...'k'
```

For "chocolate-milk", you should see: `'c'...'k'`

### 3. **Encryption Type Check**
Warns if network uses WPA3-only (incompatible with ESP32)

## What The New Logs Will Show:

**Good connection:**
```
[WiFi] Scanning for networks...
[WiFi] Found 8 networks
[WiFi] 3: Tardigrade (RSSI: -52, Encryption: 3, Ch: 6)
[WiFi] ✓ Target SSID 'Tardigrade' found!
[WiFi] Password verification (first/last chars): 'c'...'k'
[WiFi] Connected successfully on attempt 1
```

**Network not found:**
```
[WiFi] Scanning for networks...
[WiFi] Found 5 networks
[WiFi] 0: OtherNetwork (RSSI: -45, Encryption: 3, Ch: 1)
[WiFi] ✗ Target SSID 'Tardigrade' NOT FOUND in scan!
[WiFi] Possible causes:
[WiFi]   - Router is on 5GHz only (ESP32 only supports 2.4GHz)
```

**Wrong password:**
```
[WiFi] ✓ Target SSID 'Tardigrade' found! RSSI: -52 dBm
[WiFi] Password verification (first/last chars): 'c'...'k'
[WiFi] Waiting... 0/60, Status: 6
[STA.cpp] Reason: 2 - AUTH_EXPIRE  ← This means wrong password!
```

## Common Encryption Type Values:

- `0` = OPEN (no password)
- `2` = WPA_PSK
- `3` = WPA2_PSK ✓ (Best for ESP32)
- `4` = WPA_WPA2_PSK ✓ (Mixed mode, works)
- `5` = WPA2_ENTERPRISE (requires certificates)
- `6` = WPA3_PSK (May not work with ESP32)
- `7` = WPA2_WPA3_PSK ✓ (Mixed mode, works)

## Quick Fix Options:

### Option 1: Update Password in Code (if wrong)
In `WebServerManager.h`, line 17-18:
```cpp
#define WIFI_SSID "Tardigrade"
#define WIFI_PASSWORD "your-actual-password-here"  // Update this!
```

### Option 2: Use Different Network
Try one of your other networks that you KNOW works:
```cpp
#define WIFI_SSID "Pixel_9363"
#define WIFI_PASSWORD "yellow22"
```

### Option 3: Router Settings (if 5GHz only)
In your router settings:
1. Find "Tardigrade" network
2. Enable 2.4GHz band (disable "5GHz only" mode)
3. Or create a separate 2.4GHz-only network

### Option 4: Create Test Network
Create a simple open network temporarily to test:
1. Router settings → Create new network
2. Name: "ESP32-Test"
3. Security: WPA2-PSK
4. Password: "test1234"
5. Band: 2.4GHz only
6. Update code to use this network

## Upload and Test

1. Save your changes to `WebServerManager.h`
2. Upload the new firmware
3. Monitor serial output
4. Copy the scan results and post them

The scan will tell us exactly what's wrong!

## Expected AUTH_EXPIRE Causes:

| Symptom | Cause | Fix |
|---------|-------|-----|
| AUTH_EXPIRE immediately | Wrong password | Update password in code |
| AUTH_EXPIRE + Status 1 | Network found but wrong password | Verify password on router |
| NO_AP_FOUND (201) | 5GHz only or SSID wrong | Check router is 2.4GHz |
| AUTH_EXPIRE + WPA3 warning | WPA3-only security | Change router to WPA2 or WPA2/WPA3 mixed |

## Still Having Issues?

After uploading the new firmware with diagnostics, send me:
1. The network scan output (list of all networks found)
2. Whether "Tardigrade" appears in the scan
3. The RSSI and encryption type if it's found
4. The password character verification output
