#include <WiFi.h>
#include "configuration.h"
#include "station_utils.h"
#include "battery_utils.h"
#include "aprs_is_utils.h"
#include "boards_pinout.h"
#include "syslog_utils.h"
#include "A7670_utils.h"
#include "lora_utils.h"
#include "wifi_utils.h"
#include "gps_utils.h"
#include "bme_utils.h"
#include "display.h"
#include "utils.h"

extern Configuration        Config;
extern WiFiClient           espClient;
extern String               versionDate;
extern String               firstLine;
extern String               secondLine;
extern String               thirdLine;
extern String               fourthLine;
extern String               fifthLine;
extern String               sixthLine;
extern String               seventhLine;
extern String               iGateBeaconPacket;
extern String               iGateLoRaBeaconPacket;
extern std::vector<String>  lastHeardStation;
extern int                  rssi;
extern float                snr;
extern int                  freqError;
extern String               distance;
extern bool                 WiFiConnected;
extern int                  wxModuleType;
extern bool                 backUpDigiMode;
extern bool                 shouldSleepLowVoltage;
extern bool                 transmitFlag;

bool        statusAfterBoot     = true;
bool        beaconUpdate        = true;
uint32_t    lastBeaconTx        = 0;
uint32_t    lastScreenOn        = millis();


namespace Utils {

    void processStatus() {
        String status = Config.callsign;
        status.concat(">APLRG1");
        if (Config.beacon.path.indexOf("WIDE") == 0) {
            status.concat(",");
            status.concat(Config.beacon.path);
        }
        if (WiFi.status() == WL_CONNECTED && Config.aprs_is.active && Config.beacon.sendViaAPRSIS) {
            delay(1000);
            status.concat(",qAC:>");
            status.concat(versionDate);
            APRS_IS_Utils::upload(status);
            SYSLOG_Utils::log(2, status, 0, 0.0, 0);   // APRSIS TX
            statusAfterBoot = false;
        }
        if (statusAfterBoot && !Config.beacon.sendViaAPRSIS && Config.beacon.sendViaRF) {
            delay(2000);
            status.concat(":>https://github.com/richonguzman/LoRa_APRS_iGate ");
            status.concat(versionDate);
            STATION_Utils::addToOutputPacketBuffer(status);
            statusAfterBoot = false;
        }
    }

    String getLocalIP() {
        if (!WiFiConnected) {
            return "IP :  192.168.4.1";
        } else if (backUpDigiMode) {
            return "- BACKUP DIGI MODE -";
        } else {
            return "IP :  " + String(WiFi.localIP()[0]) + "." + String(WiFi.localIP()[1]) + "." + String(WiFi.localIP()[2]) + "." + String(WiFi.localIP()[3]);
        }        
    }

    void setupDisplay() {
        displaySetup();
        #ifdef INTERNAL_LED_PIN
            digitalWrite(INTERNAL_LED_PIN,HIGH);
        #endif
        Serial.println("\nStarting Station: " + Config.callsign + "   Version: " + versionDate);
        displayShow(" LoRa APRS", "", "", "   ( iGATE & DIGI )", "", "" , "  CA2RXU  " + versionDate, 4000);
        #ifdef INTERNAL_LED_PIN
            digitalWrite(INTERNAL_LED_PIN,LOW);
        #endif
        firstLine   = Config.callsign;
        seventhLine = "     listening...";
    }

    void activeStations() {
        fourthLine = "Stations (";
        fourthLine.concat(String(Config.rememberStationTime));
        fourthLine.concat("min) = ");
        if (lastHeardStation.size() < 10) {
            fourthLine += " ";
        }
        fourthLine.concat(String(lastHeardStation.size()));
    }

    void checkBeaconInterval() {
        uint32_t lastTx = millis() - lastBeaconTx;
        if (lastBeaconTx == 0 || lastTx >= Config.beacon.interval * 60 * 1000) {
            beaconUpdate = true;    
        }

        if (beaconUpdate) {
            if (!Config.display.alwaysOn && Config.display.timeout != 0) {
                displayToggle(true);
            }
            
            STATION_Utils::deleteNotHeard();

            activeStations();

            String beaconPacket             = iGateBeaconPacket;
            String secondaryBeaconPacket    = iGateLoRaBeaconPacket;
            if (Config.bme.active && wxModuleType != 0) {
                String sensorData = BME_Utils::readDataSensor();
                beaconPacket += sensorData;
                secondaryBeaconPacket += sensorData;
            } else if (Config.bme.active && wxModuleType == 0) {
                beaconPacket += ".../...g...t...r...p...P...h..b.....";
                secondaryBeaconPacket += ".../...g...t...r...p...P...h..b.....";
            }
            beaconPacket            += Config.beacon.comment;
            secondaryBeaconPacket   += Config.beacon.comment;

            #if defined(BATTERY_PIN) || defined(HAS_AXP192) || defined(HAS_AXP2101)
                if (Config.battery.sendInternalVoltage || Config.battery.monitorInternalVoltage) {
                    float internalVoltage       = BATTERY_Utils::checkInternalVoltage();
                    String internalVoltageInfo  = String(internalVoltage,2) + "V";
                    if (Config.battery.sendInternalVoltage) {
                        beaconPacket            += " Batt=";
                        beaconPacket            += internalVoltageInfo;
                        secondaryBeaconPacket   += " Batt=";
                        secondaryBeaconPacket   += internalVoltageInfo;
                        sixthLine               = "    (Batt=";
                        sixthLine               += internalVoltageInfo;
                        sixthLine               += ")";
                    }
                    if (Config.battery.monitorInternalVoltage && internalVoltage < Config.battery.internalSleepVoltage) {
                        beaconPacket            += " **IntBatWarning:SLEEP**";
                        secondaryBeaconPacket   += " **IntBatWarning:SLEEP**";
                        shouldSleepLowVoltage   = true;
                    }                    
                }
            #endif

            #ifndef HELTEC_WP
                if (Config.battery.sendExternalVoltage || Config.battery.monitorExternalVoltage) {
                    float externalVoltage       = BATTERY_Utils::checkExternalVoltage();
                    String externalVoltageInfo  = String(externalVoltage,2) + "V";
                    if (Config.battery.sendExternalVoltage) {
                        beaconPacket            += " Ext=";
                        beaconPacket            += externalVoltageInfo;
                        secondaryBeaconPacket   += " Ext=";
                        secondaryBeaconPacket   += externalVoltageInfo;
                        sixthLine               = "    (Ext V=";
                        sixthLine               += externalVoltageInfo;
                        sixthLine               += ")";
                    }
                    if (Config.battery.monitorExternalVoltage && externalVoltage < Config.battery.externalSleepVoltage) {
                        beaconPacket            += " **ExtBatWarning:SLEEP**";
                        secondaryBeaconPacket   += " **ExtBatWarning:SLEEP**";
                        shouldSleepLowVoltage   = true;
                    }
                }
            #endif

            if (Config.aprs_is.active && Config.beacon.sendViaAPRSIS && !backUpDigiMode) {
                Utils::println("-- Sending Beacon to APRSIS --");
                displayShow(firstLine, secondLine, thirdLine, fourthLine, fifthLine, sixthLine, "SENDING IGATE BEACON", 0); 
                seventhLine = "     listening...";
                #ifdef HAS_A7670
                    A7670_Utils::uploadToAPRSIS(beaconPacket);
                #else
                    APRS_IS_Utils::upload(beaconPacket);
                #endif
            }

            if (Config.beacon.sendViaRF || backUpDigiMode) {
                Utils::println("-- Sending Beacon to RF --");
                displayShow(firstLine, secondLine, thirdLine, fourthLine, fifthLine, sixthLine, "SENDING DIGI BEACON", 0);
                seventhLine = "     listening...";
                STATION_Utils::addToOutputPacketBuffer(secondaryBeaconPacket);
            }

            lastBeaconTx = millis();
            lastScreenOn = millis();
            beaconUpdate = false;
        }

        if (statusAfterBoot) {
            processStatus();
        }
    }

    void checkDisplayInterval() {
        uint32_t lastDisplayTime = millis() - lastScreenOn;
        if (!Config.display.alwaysOn && lastDisplayTime >= Config.display.timeout * 1000) {
            displayToggle(false);
        }
    }

    void validateFreqs() {
        if (Config.loramodule.txFreq != Config.loramodule.rxFreq && abs(Config.loramodule.txFreq - Config.loramodule.rxFreq) < 125000) {
            Serial.println("Tx Freq less than 125kHz from Rx Freq ---> NOT VALID");
            displayShow("Tx Freq is less than ", "125kHz from Rx Freq", "device will autofix", "and then reboot", 1000);
            Config.loramodule.txFreq = Config.loramodule.rxFreq; // Inform about that but then change the TX QRG to RX QRG and reset the device
            Config.writeFile();
            ESP.restart();
        }
    }

    void typeOfPacket(const String& packet, const uint8_t packetType) {
        String sender = packet.substring(0,packet.indexOf(">"));
        switch (packetType) {
            case 0: // LoRa-APRS
                fifthLine = "LoRa Rx ----> APRS-IS";
                break;
            case 1: // APRS-LoRa
                fifthLine = "APRS-IS ----> LoRa Tx";
                break;
            case 2: // Digi
                fifthLine = "LoRa Rx ----> LoRa Tx";
                break;
        }
        for (int i = sender.length(); i < 9; i++) {
            sender += " ";
        }
        sixthLine = sender;
        String seventhLineHelper = "RSSI:";
        seventhLineHelper += String(rssi);
        seventhLineHelper += "dBm SNR: ";
        seventhLineHelper += String(snr);
        seventhLineHelper += "dBm";

        int firstColonIndex = packet.indexOf(":");
        if (packet[firstColonIndex + 1] == ':') {
            sixthLine += "> MESSAGE";
            seventhLine = seventhLineHelper;
        } else if (packet[firstColonIndex + 1] == '>') {
            sixthLine += "> NEW STATUS";
            seventhLine = seventhLineHelper;
        } else if (packet[firstColonIndex + 1] == '!' || packet[firstColonIndex + 1] == '=' || packet[firstColonIndex + 1] == '@') {
            sixthLine += "> GPS BEACON";
            if (!Config.syslog.active) {
                GPS_Utils::getDistanceAndComment(packet);       // to be checked!!!
            }
            seventhLine = "RSSI:";
            seventhLine += String(rssi);
            seventhLine += "dBm";
            if (rssi <= -100) {
                seventhLine += " ";
            } else {
                seventhLine += "  ";
            }
            if (distance.indexOf(".") == 1) {
                seventhLine += " ";
            }
            seventhLine += "D:";
            seventhLine += distance;
            seventhLine += "km";
        } else if (packet[firstColonIndex + 1] == '`' || packet[firstColonIndex + 1] == '\'') {
            sixthLine += ">  MIC-E";
            seventhLine = seventhLineHelper;
        } else if (packet[firstColonIndex + 1] == ';') {
            sixthLine += ">  OBJECT";
            seventhLine = seventhLineHelper;
        } else if (packet.indexOf(":T#") >= 10 && packet.indexOf(":=/") == -1) {
            sixthLine += "> TELEMETRY";
            seventhLine = seventhLineHelper;
        } else {
            sixthLine += "> ??????????";
            seventhLine = seventhLineHelper;
        }
    }

    void print(const String& text) {
        if (!Config.tnc.enableSerial) {
            Serial.print(text);
        }
    }

    void println(const String& text) {
        if (!Config.tnc.enableSerial) {
            Serial.println(text);
        }
    }

    void checkRebootMode() {
        if (Config.rebootMode && Config.rebootModeTime > 0) {
            Serial.println("(Reboot Time Set to " + String(Config.rebootModeTime) + " hours)");
        }
    }

    void checkRebootTime() {
        if (Config.rebootMode && Config.rebootModeTime > 0) {
            if (millis() > Config.rebootModeTime * 60 * 60 * 1000) {
                Serial.println("\n*** Automatic Reboot Time Restart! ****\n");
                ESP.restart();
            }
        }
    }

    void checkSleepByLowBatteryVoltage(uint8_t mode) {
        if (shouldSleepLowVoltage) {
            if (mode == 0) {    // at startup
                delay(3000);
            }
            Serial.println("\n\n*** Sleeping Low Battey Voltage ***\n\n");
            esp_sleep_enable_timer_wakeup(30 * 60 * 1000000); // sleep 30 min
            if (mode == 1) {    // low voltage detected after a while
                displayToggle(false);
            }
            #ifdef VEXT_CTRL
                #ifndef HELTEC_WSL_V3
                    digitalWrite(VEXT_CTRL, LOW);
                #endif
            #endif
            LoRa_Utils::sleepRadio();
            transmitFlag = true;
            delay(100);
            esp_deep_sleep_start();
        }
    }

    bool checkValidCallsign(const String& callsign) {
        if (callsign == "WLNK-1") return true;
        
        String cleanCallsign;
        if (callsign.indexOf("-") > 0) {    // SSID Validation
            cleanCallsign = callsign.substring(0, callsign.indexOf("-"));
            String ssid = callsign.substring(callsign.indexOf("-") + 1);
            if (ssid.indexOf("-") != -1 || ssid.length() > 2) return false;
            for (int i = 0; i < ssid.length(); i++) {
                if (!isAlphaNumeric(ssid[i])) return false;
            }
        } else {
            cleanCallsign = callsign;
        }

        if (cleanCallsign.length() < 4 || cleanCallsign.length() > 6) return false;

        if (cleanCallsign.length() < 6 && isAlpha(cleanCallsign[0]) && isDigit(cleanCallsign[1]) && isAlpha(cleanCallsign[2]) && isAlpha(cleanCallsign[3]) ) {
            cleanCallsign = " " + cleanCallsign;    // A0AA --> _A0AA
        }

        if (!isDigit(cleanCallsign[2]) || !isAlpha(cleanCallsign[3])) {     // __0A__ must be validated
            if (cleanCallsign[0] != 'R' && !isDigit(cleanCallsign[1]) && !isAlpha(cleanCallsign[2])) return false;    // to accepto R0A___
        }

        bool isValid = false;
        if ((isAlphaNumeric(cleanCallsign[0]) || cleanCallsign[0] == ' ') && isAlpha(cleanCallsign[1])) {
            isValid = true;     //  AA0A (+A+A) + _A0AA (+A) + 0A0A (+A+A)
        } else if (isAlpha(cleanCallsign[0]) && isDigit(cleanCallsign[1])) {
            isValid = true;     //  A00A (+A+A)
        } else if (cleanCallsign[0] == 'R' && cleanCallsign.length() == 6 && isDigit(cleanCallsign[1]) && isAlpha(cleanCallsign[2]) && isAlpha(cleanCallsign[3]) && isAlpha(cleanCallsign[4])) {
            isValid = true;     //  R0AA (+A+A)
        }
        if (!isValid) return false;   // also 00__ avoided

        if (cleanCallsign.length() > 4) {   // to validate ____AA
            for (int i = 5; i <= cleanCallsign.length(); i++) {
                if (!isAlpha(cleanCallsign[i - 1])) return false;
            }
        }
        return true;
    }

}