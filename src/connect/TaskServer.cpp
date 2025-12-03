#include "TaskServer.h"

AsyncWebServer server(httpPort);
AsyncWebSocket ws("/ws");

bool isServerRunning = false;

void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len)
{
    if (type == WS_EVT_CONNECT)
    {
        Serial.printf("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
    }
    else if (type == WS_EVT_DISCONNECT)
    {
        Serial.printf("WebSocket client #%u disconnected\n", client->id());
    }
    else if (type == WS_EVT_DATA)
    {
        AwsFrameInfo *info = (AwsFrameInfo *)arg;

        if (info->opcode == WS_TEXT && info->final)
        {
            String message;
            message.reserve(len);
            message = String((char *)data).substring(0, len);
            parseJson(message, true);
        }
    }
}

void connnectWSV()
{
    ws.onEvent(onEvent);
    server.addHandler(&ws);

    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
              { if (LittleFS.exists("/index.html")) {
    request->send(LittleFS, "/index.html", "text/html");
} else {
    request->send(500, "text/plain", "Missing index.html");
} });

    server.on("/script.js", HTTP_GET, [](AsyncWebServerRequest *request)
              { if (LittleFS.exists("/index.html")) {
    request->send(LittleFS, "/script.js", "application/javascript");
} else {
    request->send(500, "text/plain", "Missing js file");
} });

    server.on("/styles.css", HTTP_GET, [](AsyncWebServerRequest *request)
              { if (LittleFS.exists("/index.html")) {
    request->send(LittleFS, "/styles.css", "text/css");
} else {
    request->send(500, "text/plain", "Missing css file");
} });

    server.begin();
    isServerRunning = true;

    Serial.println("HTTP + WebSocket Server Started");
}

void TaskServer(void *pvParameters)
{

    connnectWSV();

    while (true)
    {
        ws.cleanupClients(); // RẤT QUAN TRỌNG, tránh crash
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}

void server_init()
{
    xTaskCreate(TaskServer, "TaskServer", 8192, NULL, 1, NULL);
}
