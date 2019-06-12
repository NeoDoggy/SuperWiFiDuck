#include <Wire.h>

#define I2C_ADDR 0x31

#define BUFFER_SIZE 512
#define PACKETS_SIZE 32

#define RESPONSE_OK 0x00
#define RESPONSE_PROCESSING 0x01
#define RESPONSE_REPEAT 0x02
#define RESPONSE_I2C_ERROR 0xFF

#define WAITING_TIME 25

int  bytesSent  = 0;
bool connection = false;

char* prevMessage = NULL;

uint8_t response = RESPONSE_OK;
int     repeats  = 0;

bool testConnection() {
    sendParseRequest();
    return connection;
}

void sendParseRequest() {
    Wire.requestFrom(I2C_ADDR, 3);

    if (Wire.available()) {
        response   = Wire.read();
        repeats    = int(Wire.read()) << 8 | int(Wire.read());
        connection = true;
    } else {
        Serial.println("Request error :(");
        connection = false;
        response   = RESPONSE_I2C_ERROR;
    }
}

uint8_t getResponse() {
    return response;
}

void checkRepeat() {
    sendParseRequest();

    while (getResponse() == RESPONSE_REPEAT) {
        sendMessage(prevMessage);
        sendParseRequest();
    }
}

void wait() {
    sendParseRequest();

    while (getResponse() == RESPONSE_PROCESSING) {
        delay(WAITING_TIME);
        Serial.print('.');
        sendParseRequest();
    }

    bytesSent = 0;
}

void sendMessage(const char* msg) {
    if (!connection) return;

    Serial.print("Sending message: ");
    Serial.print(msg);

    unsigned int msgLen = strlen(msg);
    int transmissions   = msgLen / PACKETS_SIZE;

    if (msgLen % PACKETS_SIZE != 0) ++transmissions;

    unsigned int j = 0;

    for (int i = 0; i < transmissions; i++) {
        if (bytesSent + PACKETS_SIZE > BUFFER_SIZE) wait();

        Wire.beginTransmission(0x31);

        for (unsigned int k = 0; k < PACKETS_SIZE && j < msgLen; k++) {
            Wire.write(msg[j]);
            ++j;
            ++bytesSent;
        }

        Wire.endTransmission();
    }

    wait();

    while (repeats > 0) resendPrevMessage();

    if (response == RESPONSE_OK) prevMessage = (char*)msg;

    Serial.println("Done");
}

void resendPrevMessage() {
    if (!connection) return;

    Serial.print("Resending message: ");
    Serial.print(prevMessage);

    unsigned int msgLen = strlen(prevMessage);
    int transmissions   = msgLen / PACKETS_SIZE;

    if (msgLen % PACKETS_SIZE != 0) ++transmissions;

    unsigned int j = 0;

    for (int i = 0; i < transmissions; i++) {
        if (bytesSent + PACKETS_SIZE > BUFFER_SIZE) wait();

        Wire.beginTransmission(0x31);

        for (unsigned int k = 0; k < PACKETS_SIZE && j < msgLen; k++) {
            Wire.write(prevMessage[j]);
            ++j;
            ++bytesSent;
        }

        Wire.endTransmission();
    }

    wait();

    Serial.println("Done");
}

void setup() {
    Serial.begin(115200);
    Serial.println();

    delay(2000);

    Wire.begin(5, 4); // join i2c bus (address optional for master)

    Serial.println("Started");

    if (testConnection()) {
        Serial.println("Connected!");

        connection = true;

        delay(2000);

        sendMessage("String NaN\n");
        sendMessage("REPEAT 5\n");
        checkRepeat();
        sendMessage("String  Batman!\n");

        // sendMessage("DEFAULTDELAY 1000\n");
        // sendMessage("GUI r\n");
        // sendMessage("STRING notepad\n");
        // sendMessage("ENTER\n");

        // sendMessage("DEFAULTDELAY 0\n");

        // for (int i = 0; i<10; i++) {
        //    sendMessage("STRING 01234567890 01234567890 01234567890 01234567890 01234567890 01234567890 01234567890 01234567890 01234567890 01234567890 01234567890 01234567890 01234567890 01234567890 01234567890 01234567890 01234567890 01234567890 01234567890 01234567890 01234567890 01234567890 01234567890 01234567890 01234567890 01234567890 01234567890 01234567890 01234567890 01234567890 01234567890 01234567890 01234567890 01234567890 01234567890 01234567890 01234567890 01234567890 01234567890 01234567890 01234567890 01234567890 01234567890 01234567890 01234567890 01234567890 01234567890 01234567890 01234567890 01234567890 01234567890 01234567890 01234567890 01234567890 01234567890 01234567890 01234567890 01234567890 01234567890 01234567890 01234567890 01234567890 01234567890 01234567890 01234567890 01234567890 01234567890 01234567890 01234567890 01234567890 01234567890 01234567890 01234567890 01234567890 01234567890 01234567890 01234567890 01234567890 01234567890 01234567890 01234567890 01234567890 01234567890 01234567890 01234567890 01234567890 01234567890 01234567890 01234567890 01234567890 01234567890 01234567890 01234567890 01234567890 01234567890 01234567890 01234567890 01234567890 01234567890 01234567890 01234567890 01234567890 01234567890 01234567890 01234567890 01234567890 01234567890 01234567890 01234567890 01234567890 01234567890 01234567890 01234567890 01234567890 01234567890 01234567890 01234567890 01234567890 01234567890 01234567890 01234567890 01234567890 01234567890 01234567890 01234567890 01234567890 01234567890 01234567890 01234567890 01234567890 01234567890 01234567890 01234567890 01234567890 01234567890 01234567890 01234567890 01234567890 01234567890 01234567890 01234567890 01234567890 01234567890 01234567890 01234567890 01234567890 01234567890 01234567890 01234567890 01234567890 01234567890 01234567890 01234567890 01234567890 01234567890 01234567890 01234567890 01234567890 01234567890 01234567890 01234567890 01234567890 01234567890 01234567890 01234567890 01234567890 01234567890 01234567890 01234567890 01234567890\n");
        //    sendMessage("ENTER\n");
        // }
    } else {
        Serial.println("Connection error");
    }
}

void loop() {}