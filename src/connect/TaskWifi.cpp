#include "TaskWifi.h"

bool reconnect = false;

void TaskWifi(void *pvParameters)
{
    if (WIFI_SSID.isEmpty() || WIFI_PASS.isEmpty())
    {
        vTaskDelete(NULL);
    }

    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID.c_str(), WIFI_PASS.c_str());

    while (WiFi.status() != WL_CONNECTED)
    {
        vTaskDelay(delay_connect / portTICK_PERIOD_MS);
    }

    String ipAddress = WiFi.localIP().toString();
    Serial.println("Connected to WiFi");
    Serial.println(ipAddress);

    // publishWifiInfo();

    while (true)
    {
        if (WiFi.status() != WL_CONNECTED && reconnect)
        {
            updateWifiConfig();
        }
        else if (WiFi.status() == WL_CONNECTED && reconnect)
        {
            reconnect = false;

            Serial.println("📡 Reconnected with new WiFi!");
            Serial.println(WiFi.localIP());

            if (client.connected())
            {
                String info = "{\"email\":\"" + EMAIL +
                              "\",\"data\":{\"ssid\":\"" + WIFI_SSID +
                              "\",\"pass\":\"" + WIFI_PASS + "\"}}";
                publishData("wifiinfo", info); // <==== Gửi lại wifiinfo
                Serial.println("📤 Sent updated WiFi info!");
            }
        }

        vTaskDelay(delay_connect / portTICK_PERIOD_MS);
    }
}
bool saveWifiToFile(String ssid, String pass)
{
    // Load file cũ
    File file = LittleFS.open("/info.dat", "r");
    DynamicJsonDocument doc(1024);
    if (file)
    {
        deserializeJson(doc, file);
        file.close();
    }

    // update 2 trường này
    doc["WIFI_SSID"] = ssid;
    doc["WIFI_PASS"] = pass;

    // ghi lại
    file = LittleFS.open("/info.dat", "w");
    if (!file)
    {
        Serial.println("❌ Không mở được file để ghi!");
        return false;
    }

    serializeJson(doc, file);
    file.flush();
    file.close();

    Serial.println("✅ Đã update WiFi trong info.dat");
    return true;
}

void parseWifiConfig(String payload)
{
    Serial.println("📥 Nhận WiFi config:");
    Serial.println(payload);

    StaticJsonDocument<1024> doc;
    DeserializationError err = deserializeJson(doc, payload);

    if (err)
    {
        Serial.println("❌ Lỗi parse JSON!");
        return;
    }

    String newSSID = doc["data"]["ssid"] | "";
    String newPASS = doc["data"]["pass"] | "";

    if (newSSID.isEmpty() || newPASS.isEmpty())
    {
        Serial.println("❌ SSID hoặc PASS rỗng!");
        return;
    }

    Serial.println("📡 New SSID: " + newSSID);
    Serial.println("🔑 New PASS: " + newPASS);

    WIFI_SSID = newSSID;
    WIFI_PASS = newPASS;

    // lưu vào file .dat
    if (saveWifiToFile(newSSID, newPASS))
    {
        Serial.println("💾 Đã lưu WiFi mới vào info.dat");
    }

    reconnect = true;
    WiFi.disconnect(true);
    Serial.println("🔄 Switching WiFi…");
}

void updateWifiConfig()
{
    Serial.println("Updating WiFi config…");

    WiFi.disconnect(true);
    delay(1000);

    WiFi.begin(WIFI_SSID.c_str(), WIFI_PASS.c_str());

    unsigned long t = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - t < 15000)
    {
        vTaskDelay(500 / portTICK_PERIOD_MS);
    }

    if (WiFi.status() == WL_CONNECTED)
        Serial.println("✅ WiFi connected");
    else
        Serial.println("❌ WiFi connect failed");
}

void wifi_init()
{
    xTaskCreate(TaskWifi, "TaskWifi", 4096, NULL, 2, NULL);
}