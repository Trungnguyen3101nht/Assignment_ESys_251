#include "globals.h"

AsyncWebServer server_1(80);

String NAME_DEVICE;
String WIFI_SSID;
String WIFI_PASS;
String IO_USERNAME;
String IO_KEY;
String EMAIL;
String username_ota;
String password_ota;

// --- tạo file default ---
void createDefaultInfoFile()
{
    DynamicJsonDocument doc(1024);

    doc["NAME_DEVICE"] = "";
    doc["WIFI_SSID"] = "";
    doc["WIFI_PASS"] = "";
    doc["IO_USERNAME"] = "";
    doc["IO_KEY"] = "";
    doc["EMAIL"] = "";
    doc["username_ota"] = "";
    doc["password_ota"] = "";

    File file = LittleFS.open("/info.dat", "w"); // ghi đè, không append
    if (!file)
    {
        Serial.println("Không thể tạo file mới!");
        return;
    }

    serializeJson(doc, file);
    file.flush();
    file.close();

    Serial.println("Đã tạo file info.dat mặc định");
}

void loadInfoFromFile()
{
    if (!LittleFS.exists("/info.dat"))
    {
        Serial.println("File không tồn tại, tạo file mới...");
        createDefaultInfoFile();
        Serial.println("Đã tạo file vừa tạo...");
    }

    File file = LittleFS.open("/info.dat", "r");
    if (!file)
    {
        Serial.println("Không thể mở file để đọc, tạo file mới...");
        createDefaultInfoFile();
        return;
    }

    DynamicJsonDocument doc(1024);
    DeserializationError error = deserializeJson(doc, file);
    file.close();

    if (error)
    {
        Serial.println("Lỗi JSON, tạo file mới...");
        createDefaultInfoFile();
        return;
    }

    NAME_DEVICE = doc["NAME_DEVICE"] | "";
    WIFI_SSID = doc["WIFI_SSID"] | "";
    WIFI_PASS = doc["WIFI_PASS"] | "";
    IO_USERNAME = doc["IO_USERNAME"] | "";
    IO_KEY = doc["IO_KEY"] | "";
    EMAIL = doc["EMAIL"] | "";
    username_ota = doc["username_ota"] | "";
    password_ota = doc["password_ota"] | "";
}

// --- xóa file an toàn ---
void deleteInfoFile()
{
    // đảm bảo file không đang mở ở nơi khác trước khi xóa
    if (LittleFS.exists("/info.dat"))
    {
        // debug
        Serial.println("Xóa file info.dat ...");
        bool ok = LittleFS.remove("/info.dat");
        if (ok)
            Serial.println("Đã xóa file info.dat");
        else
            Serial.println("Xóa file thất bại!");
    }
}

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>ESP32 Configuration</title>
    <style>
        body { font-family: Arial, sans-serif; background-color: #f2f2f2; margin: 0; padding: 20px; display: flex; justify-content: center; align-items: center; height: 100vh; }
        .container { background-color: white; padding: 20px; border-radius: 8px; box-shadow: 0 0 10px rgba(0, 0, 0, 0.1); width: 300px; }
        h2 { text-align: center; color: #333; }
        label { margin-top: 10px; display: block; color: #666; }
        input[type="text"], input[type="password"], input[type="email"] { width: 100%; padding: 10px; margin-top: 5px; border: 1px solid #ccc; border-radius: 4px; box-sizing: border-box; }
        input[type="submit"] { background-color: #4CAF50; color: white; border: none; padding: 10px; border-radius: 4px; cursor: pointer; width: 100%; margin-top: 15px; }
        input[type="submit"]:hover { background-color: #45a049; }
        .ota-title { margin-top: 20px; font-size: 18px; text-align: center; color: #333; }
    </style>
</head>
<body>
    <div class="container">
        <h2>ESP32 Configuration</h2>
        <form action="/save" method="post">
            <label for="ssid">Name Device :</label>
            <input type="text" name="name_device" id="name_device" required>

            <label for="ssid">WiFi SSID:</label>
            <input type="text" name="ssid" id="ssid" required>
            
            <label for="pass">WiFi Password:</label>
            <input type="password" name="pass" id="pass" required>

            <label for="mqtt_user">MQTT Username:</label>
            <input type="text" name="mqtt_user" id="mqtt_user" required>
            
            <label for="mqtt_key">MQTT Key:</label>
            <input type="text" name="mqtt_key" id="mqtt_key" required>

            <label for="email">Email:</label>
            <input type="email" name="email" id="email" required>

            <div class="ota-title">OTA Information</div>
            <label for="username">Username:</label>
            <input type="text" name="username" id="username" required>

            <label for="password">Password:</label>
            <input type="password" name="password" id="password" required>

            <input type="submit" value="Save">
        </form>
    </div>
</body>
</html>
)rawliteral";

void startAccessPoint()
{
    LED_ACP();
    // set mode rõ ràng trước khi tạo AP
    WiFi.mode(WIFI_AP);
    WiFi.softAP(SSID_AP);
    Serial.println("Access Point Started");
    Serial.print("AP IP address: ");
    Serial.println(WiFi.softAPIP());

    server_1.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
                { request->send_P(200, "text/html", index_html); });

    server_1.on("/save", HTTP_POST, [](AsyncWebServerRequest *request)
                {
        NAME_DEVICE = request->getParam("name_device", true)->value();
        WIFI_SSID = request->getParam("ssid", true)->value();
        WIFI_PASS = request->getParam("pass", true)->value();
        IO_USERNAME = request->getParam("mqtt_user", true)->value();
        IO_KEY = request->getParam("mqtt_key", true)->value();
        EMAIL = request->getParam("email", true)->value();
        username_ota = request->getParam("username", true)->value();
        password_ota = request->getParam("password", true)->value();

        DynamicJsonDocument doc(512);
        doc["NAME_DEVICE"] = NAME_DEVICE;
        doc["WIFI_SSID"] = WIFI_SSID;
        doc["WIFI_PASS"] = WIFI_PASS;
        doc["IO_USERNAME"] = IO_USERNAME;
        doc["IO_KEY"] = IO_KEY;
        doc["EMAIL"] = EMAIL;
        doc["username_ota"] = username_ota;
        doc["password_ota"] = password_ota;

        File configFile = LittleFS.open("/info.dat", "w");
        if (configFile) {
            serializeJson(doc, configFile);
            configFile.flush();
            configFile.close();
            request->send(200, "text/html", "Configuration has been saved. ESP32 will restart...");
            delay(2000);
            Serial.println("=== CONFIG LOADED ===");
Serial.println(NAME_DEVICE);
Serial.println(WIFI_SSID);
Serial.println(WIFI_PASS);
Serial.println(IO_USERNAME);
Serial.println(IO_KEY);
Serial.println(EMAIL);
Serial.println(username_ota);
Serial.println(password_ota);
Serial.println("=====================");

            ESP.restart();
        } else {
            request->send(500, "text/html", "Unable to save the configuration.");
        } });

    server_1.begin();
}

void TaskResetDevice(void *pvParameters)
{
    // delay nhỏ để tránh đọc sai chân ngay sau khởi động
    vTaskDelay(500 / portTICK_PERIOD_MS);

    // đảm bảo chân BOOT có pullup, tránh floating
#ifdef IS_LEAF
    pinMode(BOOT, INPUT_PULLUP);
#else
    pinMode(BOOT, INPUT_PULLUP); // mặc định dùng pullup; nếu mạch của bạn có pull-down thì điều chỉnh
#endif

    unsigned long buttonPressStartTime = 0;

    while (true)
    {
        int boot_state = digitalRead(BOOT);
        // debug để biết trạng thái nút
        // Serial.println có thể nhiều log, xem mức log khi cần
        // Serial.print("BOOT: "); Serial.println(boot_state);

        if (boot_state == LOW) // nút được giữ (active low)
        {
            if (buttonPressStartTime == 0)
            {
                buttonPressStartTime = millis();
                Serial.println("BOOT pressed (start timer)");
            }
            else if (millis() - buttonPressStartTime > 5000)
            {
                Serial.println("BOOT held >5s -> Xóa file và restart");
                // trước khi xóa, đảm bảo không có File đang mở ở chỗ khác
                deleteInfoFile();
                delay(200); // cho hệ thống ổn định trước khi restart
                ESP.restart();
                // vTaskDelete(NULL); // không cần thiết sau restart
            }
        }
        else
        {
            if (buttonPressStartTime != 0)
            {
                Serial.println("BOOT released (reset timer)");
            }
            buttonPressStartTime = 0;
        }
        vTaskDelay(200 / portTICK_PERIOD_MS); // delay đủ lâu để tránh chiếm CPU
    }
}

void reset_device()
{
    // nếu đã tạo task rồi, có thể cần kiểm tra; nhưng giữ cách gọi như trước
    xTaskCreate(TaskResetDevice, "TaskResetDevice", 4096, NULL, 1, NULL);
}

bool check_info()
{
    // Nếu bạn cần define IS_LEAF để thao tác khác, giữ nguyên
    // pinMode(BOOT, INPUT_PULLUP); // đã set trong TaskResetDevice

    // mount LittleFS phải đã thực hiện trước khi gọi check_info()
    loadInfoFromFile();

    // tạo task reset (ở đây sẽ set pinMode trong task)
    reset_device();

    // nếu config bất kỳ rỗng -> bật AP
    if (NAME_DEVICE.isEmpty() || WIFI_SSID.isEmpty() || WIFI_PASS.isEmpty() || IO_USERNAME.isEmpty() || IO_KEY.isEmpty() || EMAIL.isEmpty() || username_ota.isEmpty() || password_ota.isEmpty())
    {
        startAccessPoint();
        return false;
    }

    for (int i = 0; i < 6; i++)
    {
        pinMode(relay_pins[i], OUTPUT);
        digitalWrite(relay_pins[i], LOW);
    }
    return true;
}
