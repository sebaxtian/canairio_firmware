#include <ConfigApp.hpp>

bool setup_mode = false;
int setup_time = 10000;
bool first_run = true;

void wcli_debug(String opts) {
  maschinendeck::Pair<String, String> operands = maschinendeck::SerialTerminal::ParseCommand(opts);
  String param = operands.first();
  param.toUpperCase();
  cfg.debugEnable(param.equals("ON") || param.equals("1"));
  cfg.reload();
  sensors.setDebugMode(cfg.devmode);
}

bool isValidKey(String key) {
  for (int i = 0; i < KCOUNT; i++) {
    if (key.equals(cfg.getKey((CONFKEYS)i))) return true;
  }
  return false;
}

String getValue(String key) {
  ConfKeyType type = cfg.getKeyType(key);
  if (type == ConfKeyType::BOOL) return cfg.getBool(key, false) ? "true" : "false";
  if (type == ConfKeyType::FLOAT) return String(cfg.getFloat(key, false));
  if (type == ConfKeyType::INT) return String(cfg.getInt(key, false));
  if (type == ConfKeyType::STRING) return cfg.getString(key, "");
  return "";
}

void wcli_klist(String opts) {
  maschinendeck::Pair<String, String> operands = maschinendeck::SerialTerminal::ParseCommand(opts);
  String opt = operands.first();
  int key_count = KCOUNT;                       // Show all keys to configure 
  if (opt.equals("basic")) key_count = KBASIC; // Only show the basic keys to configure
  Serial.printf("\n%11s \t%s \t%s \r\n", "KEYNAME", "DEFINED", "VALUE");
  Serial.printf("\n%11s \t%s \t%s \r\n", "=======", "=======", "=====");

  for (int i = 0; i < key_count; i++) {
    if(i==KBASIC) continue;
    String key = cfg.getKey((CONFKEYS)i);
    bool isDefined = cfg.isKey(key);
    String defined = isDefined ? "custom " : "default";
    String value = "";
    if (isDefined) value = getValue(key);
    Serial.printf("%11s \t%s \t%s \r\n", key, defined.c_str(), value.c_str());
  }
}

void saveInteger(String key, String v) {
  uint16_t value = v.toInt();
  cfg.saveInt(key, value);
  cfg.reload();
  Serial.printf("saved: %s:%i\r\n",key.c_str(),value);
}

void saveFloat(String key, String v) {
  float value = v.toFloat();
  cfg.saveFloat(key, value);
  cfg.reload();
  Serial.printf("saved: %s:%.5f\r\n",key.c_str(),value);
}

void saveBoolean(String key, String v) {
    cfg.saveBool(key,v.equals("on") || v.equals("1") || v.equals("enable") || v.equals("true"));
    cfg.reload();
    Serial.printf("saved: %s:%s\r\n",key.c_str(),cfg.getBool(key,false) ? "true" : "false");
}

void wcli_kset(String opts) {
  maschinendeck::Pair<String, String> operands = maschinendeck::SerialTerminal::ParseCommand(opts);
  String key = operands.first();
  String v = operands.second();
  v.toLowerCase();
  if(isValidKey(key)){
    if(cfg.getKeyType(key) == ConfKeyType::BOOL) saveBoolean(key,v);
    else if(cfg.getKeyType(key) == ConfKeyType::FLOAT) saveFloat(key,v);
    else if(cfg.getKeyType(key) == ConfKeyType::INT) saveInteger(key,v);
    else Serial.println("Invalid key action for: " + key);
  }
  else {
    Serial.printf("invalid key: %s\r\nPlease see the valid keys with klist command.\r\n",key.c_str());
  }
}

void wcli_uartpins(String opts) {
  maschinendeck::Pair<String, String> operands = maschinendeck::SerialTerminal::ParseCommand(opts);
  int sTX = operands.first().toInt();
  int sRX = operands.second().toInt();
  if (sTX >= 0 && sRX >= 0) {
    cfg.saveSensorPins(sTX, sRX);
    cfg.reload();
  }
  else
    Serial.println("invalid pins values");
}

void wcli_stime(String opts) {
  maschinendeck::Pair<String, String> operands = maschinendeck::SerialTerminal::ParseCommand(opts);
  int stime = operands.first().toInt();
  if (stime >= 5) {
    cfg.saveSampleTime(stime);
    cfg.reload();
    sensors.setSampleTime(stime);
  }
  else 
    Serial.println("invalid sample time");
}

void wcli_stype(String opts) {
  maschinendeck::Pair<String, String> operands = maschinendeck::SerialTerminal::ParseCommand(opts);
  int stype = operands.first().toInt();
  if (stype > 7 || stype < 0) Serial.println("invalid UART sensor type. Choose one into 0-7");
  else {
    cfg.saveSensorType(stype);
    cfg.reload();
    Serial.printf("\nselected UART sensor model\t: %s\r\n", sensors.getSensorName((SENSORS)cfg.stype));
  }
}

void wcli_sgeoh (String opts) {
  maschinendeck::Pair<String, String> operands = maschinendeck::SerialTerminal::ParseCommand(opts);
  String geoh = operands.first();
  if (geoh.length() > 5) {
    geoh.toLowerCase(); 
    cfg.saveGeo(geoh);
    cfg.ifxdbEnable(true);
    cfg.reload();
  } else {
    Serial.println("\nInvalid Geohash. (Precision should be > to 6).\r\n");
    Serial.println("Please visit: http://bit.ly/geohashe");
    Serial.println("\nand select one of your fixed station.");
  }
}

void wcli_info(String opts) {
  Serial.println();
  Serial.print(getDeviceInfo());
}

void wcli_exit(String opts) {
  setup_time = 0;
  setup_mode = false;
}

void wcli_setup(String opts) {
  setup_mode = true;
  Serial.println("\r\nSetup Mode. Main presets:\r\n");
  String canAirIOname = "Please set your geohash with \"sgeoh\" cmd";
  if(cfg.geo.length()>5)canAirIOname = cfg.getStationName();
  Serial.printf("CanAirIO device id\t: %s\r\n", canAirIOname.c_str());
  Serial.printf("Device factory id\t: %s\r\n", cfg.getAnaireDeviceId().c_str());
  Serial.printf("Sensor geohash id\t: %s\r\n", cfg.geo.length() == 0 ? "undefined" : cfg.geo.c_str());
  Serial.printf("WiFi current status\t: %s\r\n", WiFi.status() == WL_CONNECTED ? "connected" : "disconnected");
  Serial.printf("Sensor sample time \t: %d\r\n", cfg.stime);
  Serial.printf("UART sensor model \t: %s\r\n", sensors.getSensorName((SENSORS)cfg.stype));
  Serial.printf("UART sensor TX pin\t: %d\r\n", cfg.sTX == -1 ? PMS_TX : cfg.sTX);
  Serial.printf("UART sensor RX pin\t: %d\r\n", cfg.sRX == -1 ? PMS_RX : cfg.sRX);
  Serial.printf("Current debug mode\t: %s\r\n", cfg.devmode == true ? "enabled" : "disabled");

  wcli_klist("basic");

  Serial.printf("\r\nType \"klist\" for advanced settings\r\n");
  Serial.printf("Type \"help\" for available commands details\r\n");
}

void wcli_reboot(String opts) {
  wd.execute();
}

void wcli_clear(String opts) {
  maschinendeck::Pair<String, String> operands = maschinendeck::SerialTerminal::ParseCommand(opts);
  String deviceId = operands.first();
  if (deviceId.equals(cfg.getAnaireDeviceId())) {
    Serial.println("Clearing device to defaults..");
    wcli.clearSettings();
    cfg.clear();
  }
  else {
    Serial.println("\nPlease type clear and the factory device id to confirm.");
  }
}

class mESP32WifiCLICallbacks : public ESP32WifiCLICallbacks {
  void onWifiStatus(bool isConnected) {
    
  }

  void onHelpShow() {
    // Enter your custom help here:
    Serial.println("\r\nCanAirIO Commands:\r\n");
    Serial.println("reboot\t\t\tperform a soft ESP32 reboot");
    Serial.println("clear\t\t\tfactory settings reset. (needs confirmation)");
    Serial.println("debug\t<on/off>\tto enable debug mode");
    Serial.println("stime\t<time>\t\tset the sample time in seconds");
    Serial.println("spins\t<TX> <RX>\tset the UART pins");
    Serial.println("stype\t<sensor_type>\tset the UART sensor type. Integer.");
    Serial.println("sgeoh\t<GeohashId>\tset geohash id. Choose it here http://bit.ly/geohashe");
    Serial.println("kset\t<key> <value>\tset preference key value (e.g on/off or 1/0 or text)");
    Serial.println("klist\t\t\tlist valid preference keys");
    Serial.println("info\t\t\tget the device information");
    Serial.println("exit\t\t\texit of the initial setup mode");
    Serial.println("setup\t\t\ttype this to start the configuration");

    if(first_run) Serial.println("\n\nEnter the word: \"setup\" to configure the device");
    first_run = false;
  }

  void onNewWifi(String ssid, String passw){
    cfg.saveWifi(ssid,passw);
    cfg.reload();
  }
};

void cliTask(void *param) {
  for ( ; ; ) {
    wcli.loop();
    vTaskDelay(50);
  }
  vTaskDelete( NULL );
}

void cliTaskInit() {
  xTaskCreate(
    cliTask,          /* Task function. */
    "cliTask",        /* String with name of task. */
    10000,            /* Stack size in bytes. */
    NULL,             /* Parameter passed as input of the task */
    1,                /* Priority of the task. */
    NULL              /* Task handle. */
  );
}

/**
 * @brief WiFi CLI init and CanAirIO custom commands
 **/
void cliInit() {
  wcli.setCallback(new mESP32WifiCLICallbacks());
  wcli.setSilentMode(true);
  wcli.disableConnectInBoot();
  wcli.begin();
  // Main Commands:
  wcli.term->add("reboot", &wcli_reboot, "\tperform a ESP32 reboot");
  wcli.term->add("clear", &wcli_clear, "\tfactory settings reset. (needs confirmation)");
  wcli.term->add("debug", &wcli_debug, "\tenable debug mode");
  wcli.term->add("stime", &wcli_stime, "\tset the sample time (seconds)");
  wcli.term->add("spins", &wcli_uartpins, "\tset the UART pins TX RX");
  wcli.term->add("stype", &wcli_stype, "\tset the sensor type (UART)");
  wcli.term->add("sgeoh", &wcli_sgeoh, "\tset geohash. Type help for more details.");
  wcli.term->add("kset", &wcli_kset, "\tset preference key (e.g on/off or 1/0 or text)");
  wcli.term->add("klist", &wcli_klist, "\tlist valid preference keys");
  wcli.term->add("info", &wcli_info, "\tget device information");
  wcli.term->add("exit", &wcli_exit, "\texit of the setup mode. AUTO EXIT in 10 seg! :)");
  wcli.term->add("setup", &wcli_setup, "\tTYPE THIS WORD to enter to SAFE MODE setup\n");
  // Configuration loop:
  // 10 seconds for reconfiguration or first use case.
  // for reconfiguration type disconnect and switch the "output" mode
  uint32_t start = millis();
  while (setup_mode || (millis() - start < setup_time)) wcli.loop();
  Serial.println();
  if (setup_time==0) Serial.println("==>[INFO] Settings saved. Booting..\r\n");
  else Serial.println("==>[INFO] Time for initial setup over. Booting..\r\n");
  cliTaskInit();
}