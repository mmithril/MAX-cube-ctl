#include <SPI.h>
#include <Ethernet.h>

extern "C" {
#include "max.h"
}

byte mac[] = {
    0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED
};



IPAddress server(10, 0, 0, 3);
EthernetClient client;
long lastData = 0;
int n = 0;
char recvBuff[4000];

boolean fullData = false;

void setup() {
    Serial.begin(115200);
    delay(2000);
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
        recvBuff[n++] = c;
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
        MAX_msg_list* msg_list = NULL;
        Serial.print("Parsing...");
        Serial.print(recvBuff[0]);
        parseMAXData(recvBuff, n, &msg_list);
        Serial.println("DONE");
        if (msg_list == NULL) {
            Serial.println("Nothing parsed");
        }
        int valvePositions[20];

        MAX_msg_list* msg_list_i = msg_list;
        while (msg_list_i != NULL) {
            char* md = msg_list_i->MAX_msg->data;
            Serial.print("Message type:");
            Serial.println(msg_list_i->MAX_msg->type);
            msg_list_i = msg_list_i->next;
        }
        int valvesCount = extractValvePositions(msg_list, valvePositions);
        Serial.print("Devices: ");
        Serial.println(valvesCount);
        for (int i = 0; i < valvesCount; i++) {
            Serial.print(valvePositions[i]);
            Serial.println();

        }
        msg_list_i = msg_list;
        while (msg_list_i != NULL) {
            msg_list_i = msg_list_i->next;
            free(msg_list_i->MAX_msg);
        }
        while (true) {
            ;
        }
    }
}




