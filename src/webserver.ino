/*
// parses and processes webpages
// if the webpage has %SOMETHING% or %SOMETHINGELSE% it will replace those strings with the ones defined
String processor(const String &var)
{
  if (var == "FIRMWARE")
  {
    return FIRMWARE_VERSION;
  }

  if (var == "FREEFLASH")
  {
    return humanReadableSize((LittleFS.totalBytes() - LittleFS.usedBytes()));
  }

  if (var == "USEDFLASH")
  {
    return humanReadableSize(LittleFS.usedBytes());
  }

  if (var == "TOTALFLASH")
  {
    return humanReadableSize(LittleFS.totalBytes());
  }

  if (var == "SLIDERVALUE")
  {
    return sliderValue;
  }
  return String();
}

void configureWebServer()
{
  // configure web server

  // run handleUpload function when any file is uploaded
  server->onFileUpload(handleUpload);

  server->on("/listfiles", HTTP_GET, [](AsyncWebServerRequest *request)
             {
    String logmessage = "Client:" + request->client()->remoteIP().toString() + " " + request->url();
    if (checkUserWebAuth(request)) {
        logmessage += " Auth: Success";
        Serial.println(logmessage);
        request->send(200, "text/plain", listFiles(true, 1, maxGIFsPerPage)); // Explicitly pass page and pageSize
    } else {
        logmessage += " Auth: Failed";
        Serial.println(logmessage);
        return request->requestAuthentication();
    } });

  server->on("/list", HTTP_GET, [](AsyncWebServerRequest *request)
             {
    int page = 1; // Default to the first page
    if (request->hasParam("page")) {
        page = request->getParam("page")->value().toInt();
    }

    //Serial.printf("Requested page: %d\n", page); // Debug log

    String fileList = listFiles(true, page, maxGIFsPerPage); // Generate the table content
    request->send(200, "text/html", fileList); });

  server->on("/file", HTTP_GET, [](AsyncWebServerRequest *request)
             {
    String logmessage = "Client:" + request->client()->remoteIP().toString() + " " + request->url();
    if (checkUserWebAuth(request)) {
      logmessage += " Auth: Success";
      Serial.println(logmessage);

      if (request->hasParam("name") && request->hasParam("action")) {
        String fileName = "/"+String(request->getParam("name")->value() );
        const char *fileAction = request->getParam("action")->value().c_str();

        logmessage = "Client:" + request->client()->remoteIP().toString() + " " + request->url() + "?name=" + String(fileName) + "&action=" + String(fileAction);

        if (!LittleFS.exists(fileName)) {
          Serial.println(logmessage + " ERROR: file does not exist");
          request->send(400, "text/plain", "ERROR: file does not exist");
        } else {
          Serial.println(logmessage + " file exists");
         
          if (strcmp(fileAction, "download") == 0) {
            logmessage += " downloaded";
            request->send(LittleFS, fileName, "application/octet-stream");
          } else if (strcmp(fileAction, "delete") == 0) {
            logmessage += " deleted";
            LittleFS.remove(fileName);
            request->send(200, "text/plain", "Deleted File: " + String(fileName));
          }
          else if (strcmp(fileAction, "play") == 0) {
            requestedGifPath = fileName; // Store the requested GIF path
            //gif.close();
            //dma_display->fillScreen(dma_display->color565(0, 0, 0));
            gifFile = FILESYSTEM.open(fileName);
            logmessage += " opening";
          }
           else if (strcmp(fileAction, "show") == 0) {
            logmessage += " previewing"; 
            delay(100);
            request->send(LittleFS, fileName, "image/gif");
           }
          else {
            logmessage += " ERROR: invalid action param supplied";
            request->send(400, "text/plain", "ERROR: invalid action param supplied");
          }
          Serial.println(logmessage);
        }
      } else {
        request->send(400, "text/plain", "ERROR: name and action params required");
      }
    } else {
      logmessage += " Auth: Failed";
      Serial.println(logmessage);
      return request->requestAuthentication();
    } });
}

// handles uploads to the filserver
void handleUpload(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final)
{
  // make sure authenticated before allowing upload
  if (checkUserWebAuth(request))
  {
    String logmessage = "Client:" + request->client()->remoteIP().toString() + " " + request->url();
    Serial.println(logmessage);

    if (!index)
    {
      logmessage = "Upload Start: " + String(filename);
      // open the file on first call and store the file handle in the request object
      request->_tempFile = LittleFS.open("/" + filename, "w");
      Serial.println(logmessage);
    }

    if (len)
    {
      // stream the incoming chunk to the opened file
      request->_tempFile.write(data, len);
      logmessage = "Writing file: " + String(filename) + " index=" + String(index) + " len=" + String(len);
      Serial.println(logmessage);
    }

    if (final)
    {
      logmessage = "Upload Complete: " + String(filename) + ",size: " + String(index + len);
      // close the file handle as the upload is now done
      request->_tempFile.close();
      Serial.println(logmessage);
      request->redirect("/");
    }
  }
  else
  {
    Serial.println("Auth: Failed");
    return request->requestAuthentication();
  }
}
*/