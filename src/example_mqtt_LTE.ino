/*
 * Botletics SIM7000 - OAuth2 Authenticated File Upload via HTTPS
 * 1. Authenticates with server to get JWT token
 * 2. Uploads file using Bearer token
 *
 * Author: Built from Botletics IoT Example
 * Date: 2026
 */

#include "BotleticsSIM7000.h"
#include <SDCard.h>
#include <SPI.h>
#include <SoftwareSerial.h>

// ==================== PIN DEFINITIONS ====================
#define PWRKEY 19
#define RST 20
#define TX 47  // Microcontroller RX
#define RX 48   // Microcontroller TX
#define LED 10

#define SD_PIN_CS 4
#define SD_PIN_SCLK 5
#define SD_PIN_MOSI 6
#define SD_PIN_MISO 7

#define APN "fast-tmobile.com"

// ==================== MODEM SETUP ====================
SoftwareSerial modemSS = SoftwareSerial(TX, RX);
SoftwareSerial *modemSerial = &modemSS;
SDCARD sdCard(SD_PIN_CS, SD_PIN_SCLK, SD_PIN_MOSI, SD_PIN_MISO);

Botletics_modem_LTE modem = Botletics_modem_LTE();

// ==================== SERVER CONFIG ====================
#define UPLOAD_SERVER "localhost"
#define UPLOAD_PORT 8080
#define TOKEN_ENDPOINT "/api/token"
#define UPLOAD_ENDPOINT "/api/report/test"

// ==================== AUTHENTICATION CONFIG ====================
char USERNAME[] = "testuser";
char PASSWORD[] = "testuserpw";
int NODE_ID = 1;
char jwtToken[512] = ""; // Store JWT token here
bool tokenValid = false;

// ==================== FILE CONFIG ====================
char fileName[] = "/example.wav";
File uploadFile;
uint32_t fileSize = 0;
char URL[512];
char boundary[] = "----WebKitBoundary";
char imei[16] = {0};

#define CHUNK_SIZE 1024
#define TOKEN_BUFFER_SIZE 1024

// ==================== SETUP ====================
void setup() {
  Serial.begin(9600);
  Serial.println(F("\n*** SIM7000 OAuth2 Authenticated Upload ***\n"));

  pinMode(LED, OUTPUT);
  digitalWrite(LED, LOW);
  pinMode(RST, OUTPUT);
  digitalWrite(RST, HIGH);

  // Initialize SD card
  if (!sdCard.begin(SD_PIN_CS)) {
    Serial.println(F("ERROR: SD card initialization failed!"));
    while (1)
      ;
  }
  Serial.println(F("SD card initialized OK"));

  // Check if file exists
  if (!sdCard.exists(fileName)) {
    Serial.print(F("ERROR: File not found: "));
    Serial.println(fileName);
    while (1)
      ;
  }

  uploadFile = sdCard.open(fileName, FILE_READ);
  fileSize = uploadFile.size();
  uploadFile.close();

  Serial.print(F("File: "));
  Serial.println(fileName);
  Serial.print(F("Size: "));
  Serial.print(fileSize);
  Serial.println(F(" bytes\n"));

  // Power on modem
  modem.powerOn(PWRKEY);
  delay(3000);

  // Initialize modem communication
  moduleSetup();

  // Get IMEI for device identification
  uint8_t imeiLen = modem.getIMEI(imei);
  if (imeiLen > 0) {
    Serial.print(F("Device IMEI: "));
    Serial.println(imei);
  }

  // Set full functionality
  modem.setFunctionality(1);

  // Configure APN
  modem.setNetworkSettings(F(APN));
  modem.setHTTPSRedirect(true);

  Serial.println(F("Setup complete. Ready for authentication.\n"));
}

// ==================== MAIN LOOP ====================
void loop() {
  // Connect to network
  if (!netStatus()) {
    Serial.println(F("Not connected to network, retrying..."));
    delay(2000);
    return;
  }

  Serial.println(F("Connected to LTE network!\n"));

  // Step 1: Authenticate and get JWT token
  Serial.println(F("=== STEP 1: Authenticating with OAuth2 ==="));
  if (!authenticate_OAuth2()) {
    Serial.println(F("✗ Authentication failed!"));
    delay(5000);
    return;
  }

  Serial.println(F("✓ Authentication successful!\n"));
  digitalWrite(LED, HIGH);
  delay(500);
  digitalWrite(LED, LOW);
  delay(500);

  // Step 2: Upload file with JWT token
  Serial.println(F("=== STEP 2: Uploading file with Bearer token ==="));
  if (uploadFile_HTTPS()) {
    Serial.println(F("\n✓ File uploaded successfully!"));
    digitalWrite(LED, HIGH);
    delay(2000);
    digitalWrite(LED, LOW);
  } else {
    Serial.println(F("\n✗ File upload failed!"));
  }

  // Infinite loop - reset to try again
  Serial.println(F("\nEntering infinite loop. Reset to try again."));
  while (1)
    ;
}

// ==================== OAUTH2 AUTHENTICATION ====================
bool authenticate_OAuth2() {
  Serial.println(F("Sending authentication request..."));

  // Build authentication payload (URL-encoded form data)
  char authPayload[256];
  sprintf(authPayload, "username=%s&password=%s&grant_type=password", USERNAME,
          PASSWORD);

  uint32_t payloadLen = strlen(authPayload);

  // Build HTTP request for token endpoint
  sprintf(
      URL,
      "POST %s HTTP/1.1\r\nHost: %s:%d\r\nContent-Type: "
      "application/x-www-form-urlencoded\r\nContent-Length: %lu\r\nConnection: "
      "close\r\n\r\n",
      TOKEN_ENDPOINT, UPLOAD_SERVER, UPLOAD_PORT, payloadLen);

  // Open HTTPS connection
  if (!modem.postData(UPLOAD_SERVER, UPLOAD_PORT, "HTTPS", URL)) {
    Serial.println(F("ERROR: Failed to connect to token endpoint!"));
    return false;
  }

  delay(500);

  // Send authentication payload
  modemSS.write((uint8_t *)authPayload, payloadLen);
  Serial.print(F("Sent credentials: "));
  Serial.println(authPayload);

  delay(2000);

  // Read response and extract JWT token
  if (parseTokenResponse()) {
    Serial.print(F("JWT Token received: "));
    Serial.println(jwtToken);
    tokenValid = true;
    return true;
  }

  return false;
}

// ==================== PARSE JWT TOKEN FROM RESPONSE ====================
bool parseTokenResponse() {
  uint32_t timeout = 5000;
  uint32_t startTime = millis();
  char responseBuffer[TOKEN_BUFFER_SIZE];
  uint16_t bufferIndex = 0;

  Serial.println(F("Waiting for token response..."));

  // Read response from modem
  while ((millis() - startTime) < timeout) {
    if (modemSS.available()) {
      char c = modemSS.read();
      if (bufferIndex < TOKEN_BUFFER_SIZE - 1) {
        responseBuffer[bufferIndex++] = c;
      }
      startTime = millis(); // Reset timeout on each character
    }
  }
  responseBuffer[bufferIndex] = '\0';

  Serial.println(F("\n--- Token Response ---"));
  Serial.println(responseBuffer);
  Serial.println(F("--- End Response ---\n"));

  // Extract access_token from JSON response
  // Looking for: "access_token":"eyJ0eXAiOiJKV1QiLC..."
  const char *tokenStart = strstr(responseBuffer, "\"access_token\":\"");
  if (!tokenStart) {
    Serial.println(F("ERROR: access_token not found in response!"));
    return false;
  }

  // Move pointer past the "access_token":"
  tokenStart += 16;

  // Find the closing quote
  const char *tokenEnd = strchr(tokenStart, '"');
  if (!tokenEnd) {
    Serial.println(F("ERROR: Token format invalid!"));
    return false;
  }

  // Extract token
  uint16_t tokenLen = tokenEnd - tokenStart;
  if (tokenLen > sizeof(jwtToken) - 1) {
    Serial.println(F("ERROR: Token too long!"));
    return false;
  }

  strncpy(jwtToken, tokenStart, tokenLen);
  jwtToken[tokenLen] = '\0';

  Serial.print(F("Token extracted: "));
  Serial.print(tokenLen);
  Serial.println(F(" characters"));

  return true;
}

// ==================== FILE UPLOAD WITH JWT ====================
bool uploadFile_HTTPS() {
  if (!tokenValid || strlen(jwtToken) == 0) {
    Serial.println(F("ERROR: No valid JWT token available!"));
    return false;
  }

  Serial.println(F("Starting authenticated file upload..."));

  uploadFile = sdCard.open(fileName, FILE_READ);
  if (!uploadFile) {
    Serial.println(F("ERROR: Cannot open file for upload!"));
    return false;
  }

  uint32_t contentLength = calculateContentLength();

  Serial.print(F("Content-Length: "));
  Serial.println(contentLength);

  // Build HTTPS request with Authorization Bearer token
  sprintf(
      URL,
      "POST %s HTTP/1.1\r\nHost: %s:%d\r\nAuthorization: Bearer "
      "%s\r\nContent-Type: multipart/form-data; boundary=%s\r\nContent-Length: "
      "%lu\r\nConnection: close\r\n\r\n",
      UPLOAD_ENDPOINT, UPLOAD_SERVER, UPLOAD_PORT, jwtToken, boundary,
      contentLength);

  // Open HTTPS connection
  if (!modem.postData(UPLOAD_SERVER, UPLOAD_PORT, "HTTPS", URL)) {
    Serial.println(F("ERROR: Failed to connect to upload endpoint!"));
    uploadFile.close();
    return false;
  }

  delay(500);

  // Send multipart form fields
  sendMultipartField("node_id", itoa(NODE_ID, (char *)malloc(12), 10), false);
  sendMultipartFileHeaders();

  // Send file data in chunks
  uint32_t bytesRead = 0;
  uint8_t buffer[CHUNK_SIZE];

  while (uploadFile.available() && bytesRead < fileSize) {
    uint16_t n = uploadFile.read(buffer, CHUNK_SIZE);

    if (n > 0) {
      sendDataChunk(buffer, n);
      bytesRead += n;

      if (bytesRead % 10240 == 0) {
        Serial.print(F("Uploaded: "));
        Serial.print(bytesRead / 1024);
        Serial.println(F("KB"));
      }
    }
  }

  // Send closing boundary
  sendMultipartFooter();

  uploadFile.close();

  Serial.print(F("Total uploaded: "));
  Serial.print(bytesRead);
  Serial.println(F(" bytes"));

  delay(2000);

  // Check response from modem
  flushModemResponse();

  return true;
}

// ==================== HELPER FUNCTIONS ====================

void sendMultipartField(const char *fieldName, const char *fieldValue,
                        bool isLastField) {
  char fieldHeader[256];
  sprintf(fieldHeader,
          "--%s\r\nContent-Disposition: form-data; name=\"%s\"\r\n\r\n%s\r\n",
          boundary, fieldName, fieldValue);

  modemSS.write((uint8_t *)fieldHeader, strlen(fieldHeader));

  Serial.print(F("Sent field '"));
  Serial.print(fieldName);
  Serial.print(F("': "));
  Serial.println(fieldValue);
}

void sendMultipartFileHeaders() {
  char header[300];
  sprintf(header,
          "--%s\r\nContent-Disposition: form-data; name=\"file\"; "
          "filename=\"%s\"\r\nContent-Type: application/octet-stream\r\n\r\n",
          boundary, fileName);
  modemSS.write((uint8_t *)header, strlen(header));

  Serial.print(F("Sent file headers: "));
  Serial.print(strlen(header));
  Serial.println(F(" bytes"));
}

void sendDataChunk(uint8_t *data, uint16_t len) {
  for (uint16_t i = 0; i < len; i++) {
    modemSS.write(data[i]);
  }
}

void sendMultipartFooter() {
  char footer[128];
  sprintf(footer, "\r\n--%s--\r\n", boundary);
  modemSS.write((uint8_t *)footer, strlen(footer));
  Serial.println(F("Sent closing boundary"));
}

uint32_t calculateContentLength() {
  char nodeIdStr[12];
  itoa(NODE_ID, nodeIdStr, 10);

  uint32_t totalLength = 0;
  totalLength += strlen(boundary) + 4;
  totalLength += 47;
  totalLength += strlen(nodeIdStr) + 2;
  totalLength += strlen(boundary) + 4;
  totalLength += strlen(fileName) + 100;
  totalLength += fileSize;
  totalLength += 2;
  totalLength += strlen(boundary) + 6;

  Serial.print(F("Content-Length: "));
  Serial.println(totalLength);

  return totalLength;
}

void flushModemResponse() {
  uint32_t timeout = 5000;
  uint32_t startTime = millis();

  Serial.println(F("\n--- Upload Response ---"));
  while (modemSS.available()) {
    char c = modemSS.read();
    Serial.write(c);
    if (millis() - startTime > timeout)
      break;
  }
  Serial.println(F("--- End Response ---\n"));
}

bool netStatus() {
  int n = modem.getNetworkStatus();
  Serial.print(F("Network status: "));
  if (n == 1) {
    Serial.println(F("Registered (home)"));
    return true;
  }
  if (n == 5) {
    Serial.println(F("Registered (roaming)"));
    return true;
  }
  Serial.println(F("Not registered"));
  return false;
}

void moduleSetup() {
  modemSS.begin(115200);
  Serial.println(F("Setting modem baud to 9600..."));
  modemSS.println("AT+IPR=9600");
  delay(100);
  modemSS.begin(9600);

  if (!modem.begin(modemSS)) {
    Serial.println(F("ERROR: Couldn't find modem!"));
    while (1)
      ;
  }

  Serial.println(F("Modem found!"));

  uint8_t type = modem.type();
  if (type == SIM7000) {
    Serial.println(F("Device: SIM7000 LTE"));
  }
}
