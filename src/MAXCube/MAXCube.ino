#include <SPI.h>
#include <Ethernet.h>

extern "C" {
#include "max.h"
}

byte mac[] = {
    0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED
};

#define LOG(l, v) Serial.print(l);Serial.print(": ");Serial.println(v);

IPAddress server(10, 0, 0, 3);
IPAddress me(10, 0, 0, 4);
EthernetClient client;
long lastData = 0;
int n = 0;

boolean fullData = false;

void setup() {
    Serial.begin(115200);
    Serial.print("Connecting to ethernet...");
    int dhcp = Ethernet.begin(mac);
    if (dhcp == 1) {
        Serial.println("DONE");
    } else {
        Serial.println("FAILED");
    }

    Serial.println(Ethernet.localIP());

    Serial.print("Connecting to server...");

    // if you get a connection, report back via serial:
    if (client.connect(server, 62910)) {
        Serial.println("DONE");
    } else {
        // if you didn't get a connection to the server:
        Serial.println("FAILED");
    }
    lastData = millis();

}

void loop() {
    // if there are incoming bytes available
    // from the server, read them and print them:
    while (client.available()) {
        char c = client.read();
        addChar(c);
        Serial.print(c);
        lastData = millis();
    }

    /*
      // as long as there are bytes in the serial queue,
      // read them and send them out the socket if it's open:
      while (Serial.available() > 0) {
        char inChar = Serial.read();
        if (client.connected()) {
          client.print(inChar);
        }
      }
     */

    if (lastData + 100 < millis()) {
        client.println("g:");
    }

    // if the server's disconnected, stop the client:
    if (!client.connected() || lastData + 1000 < millis()) {
        Serial.println();
        Serial.println("disconnecting.");
        Serial.print("Response length: ");
        Serial.println(n);
        client.stop();
        finalizeParsing();
        char* lMessage = getOriginalLMessage();
        int len = getOriginalLMessageLength();
        for (int i = 0; i < len; i++) {
            Serial.print(lMessage[i]);
        }
        Serial.println();
        MAX_message maxLMessage = getLMessage();
        LOG("Type", maxLMessage.type);
        LOG("Length", maxLMessage.dataLength);
        for (int i = 0; i < maxLMessage.dataLength; i++) {
            Serial.print((byte) (maxLMessage.data[i]), DEC);
            Serial.print(' ');
        }
        Serial.println();
        struct ThermostatData *thermostatData = getThermostatData();
        for (int i = 0; i < getThermostatCount(); i++) {
            struct ThermostatData thermostat = thermostatData[i];
            char address[10];
            sprintf(address, "%.2X %.2X %.2X ", thermostat.RFAddress[0], thermostat.RFAddress[1], thermostat.RFAddress[2]);
            Serial.print(address);
            Serial.print(thermostat.valvePosition);
            Serial.print(' ');
            Serial.print(thermostat.setpoint, DEC);
            Serial.println();
        }
        while (true) {
            ;
        }
    }
}




