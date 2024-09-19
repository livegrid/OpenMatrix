#include "UI.h"
#include "interface.h"

UI::UI(WebServer* server, StateManager* stateManager) {
  _server = server;
  _state_manager = stateManager;
}

void UI::begin() {
  // For STA clients
  _server
      ->on("/", HTTP_GET,
           [&]() {
             _server->sendHeader("Content-Encoding", "gzip");
             _server->send_P(200, "text/html", (const char*)OPEN_MATRIX_HTML,
                             sizeof(OPEN_MATRIX_HTML));
             log_i("Sent Open Matrix HTML for STA clients.");
           })
      .setFilter(_onSTAFilter);

  // For AP clients
  _server
      ->on("/openmatrix", HTTP_GET,
           [&]() {
             _server->sendHeader("Content-Encoding", "gzip");
             _server->send_P(200, "text/html", (const char*)OPEN_MATRIX_HTML,
                             sizeof(OPEN_MATRIX_HTML));
             log_i("Sent Open Matrix HTML for AP clients.");
           })
      .setFilter(_onAPFilter);

  // OpenMatrix State
  _server->on("/openmatrix/state", HTTP_GET, [&]() {
    String state;
    _state_manager->serialize(state);
    log_i("Serialized OpenMatrix state.");
    return _server->send(200, "application/json", state);
  });

  // OpenMatrix Switch Modes
  _server->on("/openmatrix/mode", HTTP_POST, [&]() {
    JsonDocument json;
    DeserializationError err = deserializeJson(json, _server->arg("plain"));
    if (err == DeserializationError::Ok) {
      if (_on_mode_cb) {
        _on_mode_cb(json["mode"].as<OpenMatrixMode>());
      }
      return _server->send(200, "application/json", ok_response);
    } else {
      log_e("Failed to deserialize mode change request.");
      return _server->send(400, "application/json", invalid_response);
    }
  });

  // on Power Change
  _server->on("/openmatrix/power", HTTP_POST, [&]() {
    JsonDocument json;
    DeserializationError err = deserializeJson(json, _server->arg("plain"));

    if (err == DeserializationError::Ok) {
      if (_on_power_cb) {
        _on_power_cb(json["power"].as<bool>());
        log_i("Power changed to: %s", json["power"].as<bool>() ? "ON" : "OFF");
      }
      return _server->send(200, "application/json", ok_response);
    } else {
      log_e("Failed to deserialize power change request.");
      return _server->send(400, "application/json", invalid_response);
    }
  });

  // on Power Change
  _server->on("/openmatrix/autobrightness", HTTP_POST, [&]() {
    JsonDocument json;
    DeserializationError err = deserializeJson(json, _server->arg("plain"));

    if (err == DeserializationError::Ok) {
      if (_on_autobrightness_cb) {
        _on_autobrightness_cb(json["autobrightness"].as<bool>());
        log_i("Auto brightness set to: %s", json["autobrightness"].as<bool>() ? "ON" : "OFF");
      }
      return _server->send(200, "application/json", ok_response);
    } else {
      log_e("Failed to deserialize auto brightness request.");
      return _server->send(400, "application/json", invalid_response);
    }
  });

  // on Brightness Change
  _server->on("/openmatrix/brightness", HTTP_POST, [&]() {
    JsonDocument json;
    DeserializationError err = deserializeJson(json, _server->arg("plain"));

    if (err == DeserializationError::Ok) {
      uint8_t brightness = json["brightness"].as<uint8_t>();
      if (_on_brightness_cb) {
        _on_brightness_cb(brightness);
        log_i("Brightness changed to: %d", brightness);
      }

      return _server->send(200, "application/json", ok_response);
    } else {
      log_e("Failed to deserialize brightness change request.");
      return _server->send(400, "application/json", invalid_response);
    }
  });

  // on effect selection
  _server->on("/openmatrix/effect", HTTP_POST, [&]() {
    JsonDocument json;
    DeserializationError err = deserializeJson(json, _server->arg("plain"));
    if (err == DeserializationError::Ok) {
      if (_on_effect_cb) {
        _on_effect_cb(json["effect"].as<Effects>());
        log_i("Effect changed to: %d", json["effect"].as<Effects>());
      }
      return _server->send(200, "application/json", ok_response);
    } else {
      log_e("Failed to deserialize effect change request.");
      return _server->send(400, "application/json", invalid_response);
    }
  });

  _server->on("/openmatrix/effect/settings", HTTP_POST, [&]() {
  JsonDocument json;
  DeserializationError err = deserializeJson(json, _server->arg("plain"));
  if (err == DeserializationError::Ok) {
    if (_on_effect_settings_cb) {
      Effects effect = json["effectId"].as<Effects>();
      JsonObject settings = json["settings"].as<JsonObject>();
      
      // Log which effect and settings are being updated
      log_i("Updating settings for effect: %d", effect);
      for (JsonPair kv : settings) {
        log_i("  Setting %s to %s", kv.key().c_str(), kv.value().as<String>().c_str());
      }
      
      _on_effect_settings_cb(effect, settings);
      log_i("Effect settings updated for effect: %d", effect);
    }
    return _server->send(200, "application/json", ok_response);
  } else {
    log_e("Failed to deserialize effect settings request.");
    return _server->send(400, "application/json", invalid_response);
  }
});
  
  _server->on("/openmatrix/image", HTTP_GET, [&]() {
    JsonDocument doc;
    JsonArray json = doc.to<JsonArray>();
    // Go through LitteFS and scan all files in 'img' directory
    File root = LittleFS.open("/img", "r");

    if (!root) {
      log_e("Failed to open '/img' directory.");
      return _server->send(200, "application/json", "[]");
    }

    // Loop over all files, adding them to the root array without (file
    // extension)
    File file = root.openNextFile();
    while (file) {
      // Skip directories
      if (file.isDirectory()) {
        continue;
      }

      String name = file.name();
      // Remove special characters
      name.replace("(", "");
      name.replace(")", "");
      name.replace("[", "");
      name.replace("]", "");
      name.replace("{", "");
      name.replace("}", "");
      name.replace("|", "");
      name.replace(":", "");
      name.replace("?", "");
      name.replace("\"", "");
      name.replace("<", "");
      name.replace(">", "");
      name.replace("/", "");
      name.replace("\\", "");
      name.replace("*", "");
      name.replace("\r", "");
      name.replace("\n", "");

      JsonDocument image;
      image["name"] = name;
      image["size"] = file.size();
      json.add(image);
      // Get next file
      file.close();
      file = root.openNextFile();
    }

    root.close();

    // If overflowed, delete elements from JsonArray until it doesn't overflow
    if (doc.overflowed()) {
      while (doc.overflowed() && json.size() > 0) {
        json.remove(json.size() - 1);
      }
      log_w("JSON document overflowed, removed excess images.");
    }

    // Serialize JSON
    String payload;
    serializeJson(doc, payload);
    doc.clear();

    return _server->send(200, "application/json", payload);
  });

  // on image selection
  _server->on("/openmatrix/image", HTTP_POST, [&]() {
    JsonDocument json;
    DeserializationError err = deserializeJson(json, _server->arg("plain"));
    if (err == DeserializationError::Ok) {
      if (imageExists(json["name"].as<const char*>())) {
        if (_on_image_cb) {
          _on_image_cb(json["name"].as<const char*>());
          log_i("Image selected: %s", json["name"].as<const char*>());
        }
      }
      return _server->send(200, "application/json", ok_response);
    } else {
      log_e("Failed to deserialize image selection request.");
      return _server->send(400, "application/json", invalid_response);
    }
  });

  // on image upload
  _server->on(
      "/openmatrix/imageupload", HTTP_POST,
      [this]() {
        _server->send(200, "text/plain", "File uploaded successfully");
        log_i("Image upload request received.");
      },
      [this]() { handleImageUpload(); });
  
  // on image delete
  _server->on("/openmatrix/image/delete", HTTP_POST, [&]() {
    JsonDocument json;
    DeserializationError err = deserializeJson(json, _server->arg("plain"));
    if (err == DeserializationError::Ok) {
      const char* imageName = json["name"].as<const char*>();
      if (imageExists(imageName)) {
        LittleFS.remove("/img/" + String(imageName)); // Delete the image file
        log_i("Image deleted: %s", imageName);
        return _server->send(200, "application/json", ok_response);
      } else {
        log_e("Image does not exist: %s", imageName);
        return _server->send(404, "application/json", invalid_response);
      }
    } else {
      log_e("Failed to deserialize image delete request.");
      return _server->send(400, "application/json", invalid_response);
    }
  });



  // on text change
  _server->on("/openmatrix/text", HTTP_POST, [&]() {
    JsonDocument json;
    DeserializationError err = deserializeJson(json, _server->arg("plain"));
    if (err == DeserializationError::Ok) {
      if (_on_text_cb) {
        _on_text_cb(json["payload"].as<const char*>(),
                    json["size"].as<TextSize>());
        log_i("Text changed: %s", json["payload"].as<const char*>());
      }
      return _server->send(200, "application/json", ok_response);
    } else {
      log_e("Failed to deserialize text change request.");
      return _server->send(400, "application/json", invalid_response);
    }
  });

  // on mqtt settings
  _server->on("/openmatrix/settings/mqtt", HTTP_POST, [&]() {
    JsonDocument json;
    DeserializationError err = deserializeJson(json, _server->arg("plain"));
    if (err == DeserializationError::Ok) {
      if (_on_mqtt_settings_cb) {
        _on_mqtt_settings_cb(json["host"].as<const char*>(),
                             json["port"].as<uint16_t>(),
                             json["client_id"].as<const char*>(),
                             json["username"].as<const char*>(),
                             json["password"].as<const char*>(),
                             json["co2_topic"].as<const char*>(),
                             json["matrix_text_topic"].as<const char*>(),
                             json["show_text"].as<bool>());
        log_i("MQTT settings updated.");
      }
      return _server->send(200, "application/json", ok_response);
    } else {
      log_e("Failed to deserialize MQTT settings request.");
      return _server->send(400, "application/json", invalid_response);
    }
  });

  // on dmx settings
  _server->on("/openmatrix/settings/edmx", HTTP_POST, [&]() {
    JsonDocument json;
    DeserializationError err = deserializeJson(json, _server->arg("plain"));
    if (err == DeserializationError::Ok) {
      if (_on_dmx_settings_cb) {
        _on_dmx_settings_cb(
            json["protocol"].as<eDmxProtocol>(), json["mode"].as<eDmxMode>(),
            json["multicast"].as<bool>(), json["start_universe"].as<bool>(),
            json["start_address"].as<uint16_t>(),
            json["timeout"].as<uint16_t>());
        log_i("DMX settings updated.");
      }
      return _server->send(200, "application/json", ok_response);
    } else {
      log_e("Failed to deserialize DMX settings request.");
      return _server->send(400, "application/json", invalid_response);
    }
  });

  // on network reset
  _server->on("/openmatrix/settings/network/reset", HTTP_POST, [&]() {
    if (_on_network_reset_cb) {
      _on_network_reset_cb();
      log_i("Network reset requested.");
    }
    return _server->send(200, "application/json", ok_response);
  });

  // on factory reset
  _server->on("/openmatrix/settings/factory/reset", HTTP_POST, [&]() {
    if (_on_factory_reset_cb) {
      _on_factory_reset_cb();
      log_i("Factory reset requested.");
    }
    return _server->send(200, "application/json", ok_response);
  });
}

void UI::onPower(onPowerCallback cb) {
  _on_power_cb = cb;
}

void UI::onAutoBrightness(onAutoBrightnessCallback cb) {
  _on_autobrightness_cb = cb;
}

void UI::onBrightness(onBrightnessCallback cb) {
  _on_brightness_cb = cb;
}

void UI::onMode(onModeChangeCallback cb) {
  _on_mode_cb = cb;
}

void UI::onEffect(onEffectChangeCallback cb) {
  _on_effect_cb = cb;
}

void UI::onEffectSettings(onEffectSettingsCallback cb) {
  _on_effect_settings_cb = cb;
}

void UI::onImage(onImageChangeCallback cb) {
  _on_image_cb = cb;
}

void UI::handleImageUpload() {
  HTTPUpload& upload = _server->upload();
  if (upload.status == UPLOAD_FILE_START) {
    String filename = upload.filename;
    if (!filename.startsWith("/"))
      filename = "/" + filename;
    if (!filename.endsWith(".gif"))
      filename += ".gif";
    log_i("handleFileUpload Name: %s", filename.c_str());

    File file = LittleFS.open("/img" + filename, "w");
    if (!file) {
      log_e("Failed to open file for writing");
      return;
    }
    file.write(upload.buf, upload.currentSize);
  } else if (upload.status == UPLOAD_FILE_WRITE) {
    File file = LittleFS.open("/img/" + upload.filename, "a");
    if (file) {
      file.write(upload.buf, upload.currentSize);
    }
  } else if (upload.status == UPLOAD_FILE_END) {
    File file = LittleFS.open("/img/" + upload.filename, "a");
    if (file) {
      file.close();
    }
    
    log_i("handleFileUpload Size: %d", upload.totalSize);

    // Update the state with the new image
    if (_state_manager) {
      _state_manager->getState()->image.selected = "/img/" + upload.filename;
      _state_manager->save();
      log_i("Image updated in state: %s", _state_manager->getState()->image.selected.c_str());
    }
  }
}

void UI::onText(onTextChangeCallback cb) {
  _on_text_cb = cb;
}

void UI::onMqttSettings(onMqttSettingsCallback cb) {
  _on_mqtt_settings_cb = cb;
}

void UI::onDmxSettings(onDmxSettingsCallback cb) {
  _on_dmx_settings_cb = cb;
}

void UI::onHomeAssistantSettings(onHomeAssistantSettingsCallback cb) {
  _on_home_assistant_settings_cb = cb;
}

void UI::onNetworkReset(onResetCallback cb) {
  _on_network_reset_cb = cb;
}

void UI::onFactoryReset(onResetCallback cb) {
  _on_factory_reset_cb = cb;
}

bool UI::_onAPFilter(WebServer& server) {
  return WiFi.AP.hasIP() && WiFi.AP.localIP() == server.client().localIP();
}

bool UI::_onSTAFilter(WebServer& server) {
  return WiFi.STA.hasIP() && WiFi.STA.localIP() == server.client().localIP();
}

bool UI::imageExists(const char* name) {
  File file = LittleFS.open("/img/" + String(name), "r");
  if (!file) {
    log_e("Image does not exist: %s", name);
    return false;
  }
  file.close();
  return true;
}

UI::~UI() {}