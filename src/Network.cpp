/*
Network.cpp
Inkplate 6 Arduino library
David Zovko, Borna Biro, Denis Vajak, Zvonimir Haramustek @ e-radionica.com
September 24, 2020
https://github.com/e-radionicacom/Inkplate-6-Arduino-library

For support, please reach over forums: forum.e-radionica.com/en
For more info about the product, please check: www.inkplate.io

This code is released under the GNU Lesser General Public License v3.0: https://www.gnu.org/licenses/lgpl-3.0.en.html
Please review the LICENSE file included with this example.
If you have any questions about licensing, please contact techsupport@e-radionica.com
Distributed as-is; no warranty is given.
*/

#include "Network.h"

#include <HTTPClient.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>

namespace Project
{
    void Network::begin()
    {
        // Initiating wifi, like in BasicHttpClient example
        Serial.println(F("Setting wifi mode..."));
        WiFi.mode(WIFI_STA);

        Serial.println(F("Starting wifi network..."));
        WiFi.begin(ssid, pass);

        int cnt = 0;
        Serial.print(F("Waiting for WiFi to connect..."));
        while ((WiFi.status() != WL_CONNECTED))
        {
            Serial.print(F("."));
            delay(1000);
            ++cnt;

            WiFi.reconnect();
            delay(5000);

            if (cnt == 10)
            {
                Serial.println(" Can't connect to WIFI, restarting.");
                delay(100);
                ESP.restart();
            }
        }
        Serial.println(F(" connected."));

        // Find internet time
        setTime();
    }

    // Function to get all war data from web
    void Network::waitForConnection()
    {
        // If not connected to wifi reconnect wifi
        if (WiFi.status() != WL_CONNECTED)
        {
            WiFi.reconnect();

            delay(5000);

            int cnt = 0;
            Serial.print(F("Waiting for WiFi to reconnect..."));
            while ((WiFi.status() != WL_CONNECTED))
            {
                // Prints a dot every second that wifi isn't connected
                Serial.print(F("."));
                delay(1000);
                ++cnt;

                WiFi.reconnect();
                delay(5000);

                if (cnt == 10)
                {
                    Serial.println(" Can't connect to WIFI, restart initiated.");
                    delay(100);
                    ESP.restart();
                }
            }
            Serial.println(F(" connected."));
        }
    }

    void Network::setTime()
    {
        // Used for setting correct time
        configTime(0, 0, "pool.ntp.org", "time.nist.gov");

        Serial.print(F("Waiting for NTP time sync: "));
        time_t nowSecs = time(nullptr);
        while (nowSecs < 8 * 3600 * 2)
        {
            delay(500);
            Serial.print(F("."));
            yield();
            nowSecs = time(nullptr);
        }

        Serial.println();

        // Used to store time info
        struct tm timeinfo;
        gmtime_r(&nowSecs, &timeinfo);

        Serial.print(F("Current time: "));
        Serial.print(asctime(&timeinfo));
    }
}