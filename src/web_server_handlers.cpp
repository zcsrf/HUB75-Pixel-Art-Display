// Put the code that handles web server calls
#include "web_server_handles.h"
String humanReadableSize(const size_t bytes)
{
    if (bytes < 1024)
        return String(bytes) + " B";
    else if (bytes < (1024 * 1024))
        return String(bytes / 1024.0) + " KB";
    else if (bytes < (1024 * 1024 * 1024))
        return String(bytes / 1024.0 / 1024.0) + " MB";
    else
        return String(bytes / 1024.0 / 1024.0 / 1024.0) + " GB";
}

String listFiles(bool ishtml, int page = 1, int pageSize = config.gifConfig.maxGIFsPerPage)
{
    String returnText = "";
    int fileIndex = 0;
    int startIndex = (page - 1) * pageSize;
    int endIndex = startIndex + pageSize;

    File root = FILESYSTEM.open("/");
    File foundfile = root.openNextFile();

    if (ishtml)
    {
        returnText += "<!DOCTYPE HTML><html lang=\"en\"><head>";
        returnText += "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">";
        returnText += "<meta charset=\"UTF-8\">";
        returnText += "</head><body>";
        returnText += "<table><tr><th>Name</th><th>Size</th><th>Preview</th><th>Actions</th></tr>";
    }

    while (foundfile)
    {
        if (fileIndex >= startIndex && fileIndex < endIndex)
        {
            if (ishtml)
            {
                returnText += "<tr><td>" + String(foundfile.name()) + "</td>";
                returnText += "<td>" + humanReadableSize(foundfile.size()) + "</td>";
                returnText += "<td><img src=\"/file?name=" + String(foundfile.name()) + "&action=show\" width=\"64\"></td>";
                returnText += "<td>";
                returnText += "<button onclick=\"downloadDeleteButton('" + String(foundfile.name()) + "', 'play')\">Play</button>";
                returnText += "<button onclick=\"downloadDeleteButton('" + String(foundfile.name()) + "', 'download')\">Download</button>";
                returnText += "<button onclick=\"downloadDeleteButton('" + String(foundfile.name()) + "', 'delete')\">Delete</button>";
                returnText += "</td></tr>";
            }
        }
        fileIndex++;
        foundfile = root.openNextFile();
    }

    if (ishtml)
    {
        returnText += "</table>";
        // Add the Home button
        returnText += "<button onclick=\"window.location.href='/'\">Home</button>";
        // Add the Previous button if applicable
        if (page > 1)
        {
            returnText += "<button onclick=\"window.location.href='/list?page=" + String(page - 1) + "'\">Previous</button>";
        }
        // Add the Next button if there are more files
        if (fileIndex > endIndex)
        {
            returnText += "<button onclick=\"window.location.href='/list?page=" + String(page + 1) + "'\">Next</button>";
        }
        // GIF select page
        returnText += "<script>";
        returnText += "function downloadDeleteButton(filename, action) {";
        returnText += "    console.log(`downloadDeleteButton called with filename: ${filename}, action: ${action}`);";
        returnText += "    const url = `/file?name=${filename}&action=${action}`;";
        returnText += "    if (action === 'delete') {";
        returnText += "        fetch(url).then(response => response.text()).then(data => {";
        returnText += "            console.log(data);";
        returnText += "            alert('File deleted successfully!');";
        returnText += "            location.reload();";
        returnText += "        }).catch(error => {";
        returnText += "            console.error('Error deleting file:', error);";
        returnText += "            alert('Failed to delete file.');";
        returnText += "        });";
        returnText += "    } else if (action === 'download') {";
        returnText += "        window.open(url, '_blank');";
        returnText += "    } else if (action === 'play') {";
        returnText += "        fetch(url).then(response => response.text()).then(data => {";
        returnText += "            console.log(data);";
        returnText += "            alert('Playing file...');";
        returnText += "        }).catch(error => {";
        returnText += "            console.error('Error playing file:', error);";
        returnText += "            ";
        returnText += "        });";
        returnText += "    }";
        returnText += "}";
        returnText += "</script>";
        returnText += "</body></html>";
    }

    root.close();
    return returnText;
}

void handleToggleGif()
{

    if (server.args() == 1)
    {
        if (server.argName(0) == "state")
        {
            config.display.gifEnabled = (server.arg(0) == "on");
            server.send(200);
            preferences.putBool("gifState", config.display.gifEnabled);
        }
        else
        {
            server.send(200);
        }
    }
    else
    {
        server.send(400, "text/plain", "Missing 'state' parameter");
    }
}

void handleToggleClock()
{
    if (server.args() == 1)
    {
        if (server.argName(0) == "state")
        {
            config.display.clockEnabled = (server.arg(0) == "on");
            server.send(200);
            preferences.putBool("clock", config.display.clockEnabled);
        }
        else
        {
            server.send(200);
        }
    }
    else
    {
        server.send(400, "text/plain", "Missing 'state' parameter");
    }
}

void handleToggleLoopGif()
{
    if (server.args() == 1)
    {
        if (server.argName(0) == "state")
        {
            config.display.loopGifEnabled = (server.arg(0) == "on");
            server.send(200);
            preferences.putBool("loopGif", config.display.loopGifEnabled);
            Serial.println("Changing Loop Gif State");
        }
        else
        {
            server.send(200);
        }
    }
    else
    {
        server.send(400, "text/plain", "Missing 'state' parameter");
    }
}

void handleToggleScrollText()
{
    if (server.args() == 1)
    {
        if (server.hasArg("state"))
        {
            config.display.scrollTextEnabled = (server.arg(0) == "on");
            server.send(200);
            preferences.putBool("scrollText", config.display.scrollTextEnabled);
        }
        else
        {
            server.send(200);
        }
    }
    else
    {
        server.send(400, "text/plain", "Missing 'state' parameter");
    }
}

void handleAdjustSlider()
{
    if (server.args() == 1)
    {
        if (server.argName(0) == "value")
        {
            config.display.displayBrightness = server.arg(0).toInt();
            server.send(200);

            dma_display->setBrightness8(config.display.displayBrightness);
            preferences.putUInt("displayBrig", config.display.displayBrightness);
        }
        else
        {
            server.send(200);
        }
    }
    else
    {
        server.send(400, "text/plain", "Missing 'value' parameter");
    }
}

void handleSetReboot()
{
    Serial.println("Rebooting ESP32: WebServer ");
    Serial.flush();
    ESP.restart();
}

void handleUpload()
{
    HTTPUpload &upload = server.upload();
    if (upload.status == UPLOAD_FILE_START)
    {
        String filename = upload.filename;
        if (!filename.startsWith("/"))
        {
            filename = "/" + filename;
        }
        Serial.print("handleFileUpload Name: ");
        Serial.println(filename);
        config.status.fsUploadFile = FILESYSTEM.open(filename, "w");
        filename = String();
    }
    else if (upload.status == UPLOAD_FILE_WRITE)
    {
        Serial.print("handleFileUpload Data: ");
        Serial.println(upload.currentSize);
        if (config.status.fsUploadFile)
        {
            config.status.fsUploadFile.write(upload.buf, upload.currentSize);
        }
    }
    else if (upload.status == UPLOAD_FILE_END)
    {
        if (config.status.fsUploadFile)
        {
            config.status.fsUploadFile.close();
        }
        Serial.print("handleFileUpload Size: ");
        Serial.println(upload.totalSize);
    }
}

void handleListFiles()
{
    int page = 1;

    if (server.hasArg("page"))
    {
        page = server.arg("page").toInt();
    }

    server.send(200, "text/html", listFiles(true, page, config.gifConfig.maxGIFsPerPage));
}

void handleListFilesAlt()
{
    File root = LittleFS.open("/");
    if (!root || !root.isDirectory())
    {
        server.send(500, "text/plain", "Failed to open /images");
        return;
    }

    File file = root.openNextFile();
    String json = "[";
    bool first = true;
    while (file)
    {
        if (!file.isDirectory())
        {
            String name = file.name();
            name.toLowerCase();

            if (name.endsWith(".gif") || name.endsWith(".jpg") || name.endsWith(".mjpeg"))
            {
                if (!first)
                    json += ",";
                json += "\"" + name + "\"";
                first = false;
            }
        }
        file = root.openNextFile();
    }
    json += "]";
    server.send(200, "application/json", json);
}

void handleVersionFlash()
{
    // This functions will provide a JSON of home page dynamic values
    String message = "{\"v\":\"";
    message += "1:2:3";
    message += "\"";

    message += ",\"ff\":\"";
    message += String(humanReadableSize((LittleFS.totalBytes() - LittleFS.usedBytes())));
    message += "\"";

    message += ",\"uf\":\"";
    message += String(humanReadableSize(LittleFS.usedBytes()));
    message += "\"";

    message += ",\"tf\":\"";
    message += String(humanReadableSize(LittleFS.totalBytes()));
    message += "\"";

    message += ",\"gs\":\"";
    message += String(config.display.gifEnabled);
    message += "\"";

    message += ",\"cs\":\"";
    message += String(config.display.clockEnabled);
    message += "\"";

    message += ",\"ls\":\"";
    message += String(config.display.loopGifEnabled);
    message += "\"";

    message += ",\"ts\":\"";
    message += String(config.display.scrollTextEnabled);
    message += "\"";

    message += ",\"ds\":\"";
    message += String(config.display.displayBrightness);
    message += "\"";

    // Text Color
    message += ",\"str\":\"";
    message += String(config.status.textColor.red);
    message += "\"";
    message += ",\"stg\":\"";
    message += String(config.status.textColor.green);
    message += "\"";
    message += ",\"stb\":\"";
    message += String(config.status.textColor.blue);
    message += "\"";

    // Scroll Text Size
    message += ",\"stf\":\"";
    message += String(config.status.scrollText.scrollFontSize);
    message += "\"";

    // Scroll Text Speed
    message += ",\"sts\":\"";
    message += String(config.status.scrollText.scrollSpeed);
    message += "\"";

    // Scroll Text
    message += ",\"stt\":\"";
    message += String(config.status.scrollText.scrollText);

    message += "\"}";

    server.send(200, "application/json", message);
}

void handleFileRequest()
{
    bool gifEnabledTemp = config.display.gifEnabled;

    config.display.gifEnabled = false;

    if (server.hasArg("name") && server.hasArg("action"))
    {
        String fileName = "/" + server.arg("name");
        String fileAction = server.arg("action");

        String logmessage = "Client: " + server.client().remoteIP().toString() + " " + server.uri() +
                            "?name=" + fileName + "&action=" + fileAction;

        if (!LittleFS.exists(fileName))
        {
            Serial.println(logmessage + " ERROR: file does not exist");
            server.send(400, "text/plain", "ERROR: file does not exist");
            return;
        }

        Serial.println(logmessage + " file exists");

        if (fileAction == "download")
        {
            logmessage += " downloaded";
            File downloadFile = LittleFS.open(fileName, "r");
            server.streamFile(downloadFile, "application/octet-stream");
            downloadFile.close();
        }
        else if (fileAction == "delete")
        {
            LittleFS.remove(fileName);
            logmessage += " deleted";
            server.send(200, "text/plain", "Deleted File: " + fileName);
        }
        else if (fileAction == "play")
        {
            config.status.gif.requestedGifPath = fileName;
            logmessage += " opening";
            server.send(200, "text/plain", "GIF will play on end: " + fileName);
        }
        else if (fileAction == "show")
        {
            logmessage += " previewing";

            File imageFile = LittleFS.open(fileName, "r");
            server.streamFile(imageFile, "image/gif");
            imageFile.close();
        }
        else
        {
            logmessage += " ERROR: invalid action param supplied";
            server.send(400, "text/plain", "ERROR: invalid action param supplied");
        }

        Serial.println(logmessage);
    }
    else
    {
        server.send(400, "text/plain", "ERROR: name and action params required");
    }

    config.display.gifEnabled = gifEnabledTemp;
}

void handleSetColor()
{
    if (server.args() == 3)
    {
        if (server.hasArg("r") && server.hasArg("g") && server.hasArg("b"))
        {
            config.status.textColor.red = server.arg("r").toInt();
            config.status.textColor.green = server.arg("g").toInt();
            config.status.textColor.blue = server.arg("b").toInt();

            server.send(200);
        }
        else
        {
            server.send(400, "text/plain", "Missing parameters");
        }
    }
    else
    {
        server.send(400, "text/plain", "Missing parameter");
    }
}

void handleSetScrollText()
{

    if (server.hasArg("text"))
    {
        config.status.scrollText.scrollText = server.arg("text");
    }
    if (server.hasArg("fontSize"))
    {
        config.status.scrollText.scrollFontSize = server.arg("fontSize").toInt();
    }
    if (server.hasArg("speed"))
    {
        config.status.scrollText.scrollSpeed = server.arg("speed").toInt();
    }

    if (!server.hasArg("text") && !server.hasArg("fontSize") && !server.hasArg("speed"))
    {
        server.send(400, "text/plain", "Missing parameters");
    }
    else
    {
        server.send(200);
    }
}