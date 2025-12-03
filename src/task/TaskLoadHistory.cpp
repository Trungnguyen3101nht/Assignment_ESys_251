#include "TaskLoadHistory.h"

void loadSchedulesFromFile()
{
    File file = LittleFS.open("/schedules.dat", "r");
    if (!file)
    {
        Serial.println("Failed to open file for reading");
        return;
    }

    DynamicJsonDocument doc(2048);

    DeserializationError error = deserializeJson(doc, file);
    if (error)
    {
        Serial.println("Failed to parse schedules JSON");
        file.close();
        return;
    }

    JsonArray scheduleArray = doc["schedules"];
    schedules = DLinkedList();

    for (JsonObject scheduleObj : scheduleArray)
    {
        Schedule schedule;
        schedule.id = scheduleObj["id"];
        schedule.state = scheduleObj["state"];
        schedule.time = scheduleObj["time"].as<String>();

        JsonArray daysArray = scheduleObj["days"];
        for (int i = 0; i < MAX_DAYS; i++)
        {
            if (i < daysArray.size())
            {
                schedule.days[i] = daysArray[i].as<String>();
            }
            else
            {
                schedule.days[i] = "";
            }
        }

        JsonArray actionsArray = scheduleObj["actions"];
        schedule.actionCount = actionsArray.size();
        for (int i = 0; i < actionsArray.size(); i++)
        {
            schedule.actions[i].relayId = actionsArray[i]["relayId"];
            schedule.actions[i].action = actionsArray[i]["action"].as<String>();
        }

        schedule.lastTriggered = scheduleObj["lastTriggered"].as<String>();

        schedules.add(schedule);
    }

    file.close();
    Serial.println("Schedules loaded successfully");
}

void loadGpsFromFile()
{
    // Nếu file chưa tồn tại, tạo file trống
    if (!LittleFS.exists("/gps.dat"))
    {
        Serial.println("File gps.dat không tồn tại, tạo mới...");
        File file = LittleFS.open("/gps.dat", "w");
        if (!file)
        {
            Serial.println("Không thể tạo file gps.dat!");
            return;
        }
        file.close();
    }

    File file = LittleFS.open("/gps.dat", "r");
    if (!file)
    {
        Serial.println("Không thể mở file gps.dat để đọc");
        return;
    }

    if (file.size() == 0)
    {
        // Serial.println("File gps.dat rỗng");
        file.close();
        return;
    }

    String fileContent = file.readString();
    file.close();

    DynamicJsonDocument doc(2048); // Tăng nếu file lớn
    DeserializationError error = deserializeJson(doc, "[" + fileContent + "]");

    if (error)
    {
        Serial.print("deserializeJson() failed: ");
        Serial.println(error.f_str());
        return;
    }

    JsonArray array = doc.as<JsonArray>();
    for (JsonObject entry : array)
    {
        String output;
        serializeJson(entry, output);

        if (client.connected())
        {
            publishData("history", output.c_str());
            vTaskDelay(500 / portTICK_PERIOD_MS); // delay nhỏ để tránh watchdog
        }
    }

    // Xóa file sau khi gửi xong
    LittleFS.remove("/gps.dat");
    Serial.println("GPS data sent and file removed");
}

void loadTemp_HumiFromFile()
{
    // Nếu file chưa tồn tại, tạo file trống
    if (!LittleFS.exists("/temp_humi.dat"))
    {
        Serial.println("File temp_humi.dat không tồn tại, tạo mới...");
        File file = LittleFS.open("/temp_humi.dat", "w");
        if (!file)
        {
            Serial.println("Không thể tạo file temp_humi.dat!");
            return;
        }
        file.close();
    }

    File file = LittleFS.open("/temp_humi.dat", "r");
    if (!file)
    {
        Serial.println("Không thể mở file temp_humi.dat để đọc");
        return;
    }

    if (file.size() == 0)
    {
        // Serial.println("File temp_humi.dat rỗng");
        file.close();
        return;
    }

    String fileContent = file.readString();
    file.close();

    DynamicJsonDocument doc(4096); // Tăng nếu file lớn
    DeserializationError error = deserializeJson(doc, "[" + fileContent + "]");

    if (error)
    {
        Serial.print("deserializeJson() failed: ");
        Serial.println(error.f_str());
        return;
    }

    String output;
    serializeJson(doc.as<JsonArray>(), output);

    Serial.println(output); // in debug
    if (client.connected())
    {
        publishData("history", output.c_str());
    }

    // Xóa file sau khi gửi
    LittleFS.remove("/temp_humi.dat");
    Serial.println("Temp/Humi data sent and file removed");
}
void TaskLoadHistory(void *pvParameters)
{
    loadSchedulesFromFile();

    while (true)
    {
        if (WiFi.status() == WL_CONNECTED)
        {
            if (client.connected())
            {
                loadGpsFromFile();
                loadTemp_HumiFromFile();
                // Task này chạy liên tục, delay tránh watchdog
                vTaskDelay(5000 / portTICK_PERIOD_MS);
            }
            // else
            // {
            //     client.connect(); // reconnect nếu cần
            //     vTaskDelay(1000 / portTICK_PERIOD_MS);
            // }
        }
        else
        {
            vTaskDelay(1000 / portTICK_PERIOD_MS);
        }
    }
}
