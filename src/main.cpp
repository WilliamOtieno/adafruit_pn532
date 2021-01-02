
/*
    This will wait for any ISO14443A card or tag, and
    depending on the size of the UID will attempt to read from it.
   
    If the card has a 4-byte UID it is probably a Mifare
    Classic card, and the following steps are taken:
   
    - Authenticate block 4 (the first block of Sector 1) using
      the default KEYA of 0XFF 0XFF 0XFF 0XFF 0XFF 0XFF
    - If authentication succeeds, we can then read any of the
      4 blocks in that sector (though only block 4 is read here)
/**************************************************************************/

#include <Arduino.h>
#include <Wire.h>
#include <SPI.h>
#include <Adafruit_PN532.h>

// Using the breakout with SPI, define the pins for SPI communication.
#define PN532_SCK  (2)
#define PN532_MOSI (3)
#define PN532_SS   (4)
#define PN532_MISO (5)



// Use this line for a breakout with a software SPI connection
// Declare the Adafruit Object as breakOut
Adafruit_PN532 breakOut(PN532_SCK, PN532_MISO, PN532_MOSI, PN532_SS);



void setup(void) {

    Serial.begin(115200);
    Serial.println("BreakOut v1.0");
    Serial.println("Attempting to locate chip");

    // Initialize Adafruit Object as nfc
    breakOut.begin();

    uint32_t versiondata = breakOut.getFirmwareVersion();
    if (! versiondata) {
        Serial.println("Didn't find PN532 board");
        Serial.print("Ascertain all physical connections");
        while (1); // halt
    }
    // Upon successful retrieval of data...
    // Print out some info about the board
    Serial.print("Found chip PN532"); Serial.println((versiondata>>24) & 0xFF, HEX);
    Serial.print("Firmware version: "); Serial.print((versiondata>>16) & 0xFF, DEC);
    Serial.print('.'); Serial.println((versiondata>>8) & 0xFF, DEC);

    // configure board to read RFID tags
    breakOut.SAMConfig();

    Serial.println("Waiting for an ISO14443A Card ...");
}


void loop(void) {
    uint8_t success;
    uint8_t uid[] = { 0, 0, 0, 0, 0, 0, 0 };  // Buffer to store the returned UID
    uint8_t uidLength;                        // Length of the UID (4 or 7 bytes depending on ISO14443A card type)

    // Wait for an ISO14443A type cards.  When one is found
    // 'uid' will be populated with the UID, and uidLength will indicate
    // if the uid is 4 bytes (Mifare Classic) or 7 bytes (Mifare Ultralight)
    success = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength);

    if (success) {
        // Display some basic information about the card
        Serial.println("Splendid! Found an ISO14443A card");
        Serial.print("  UID Length: ");Serial.print(uidLength, DEC);Serial.println(" bytes");
        Serial.print("  UID Value: ");
        nfc.PrintHex(uid, uidLength);
        Serial.println("");

        if (uidLength == 4)
        {
            // We probably have a Mifare Classic card ...
            Serial.println("Mifare Card obtained (4 byte UID)");

            // Now we need to try to authenticate it for read/write access
            // Try with the factory default KeyA: 0xFF 0xFF 0xFF 0xFF 0xFF 0xFF
            Serial.println("Trying to authenticate block 4 with default KEYA value");
            uint8_t keya[6] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };

            // Start with block 4 (the first block of sector 1) since sector 0
            // contains the manufacturer data and it's probably better just
            // to leave it alone unless you know what you're doing
            success = nfc.mifareclassic_AuthenticateBlock(uid, uidLength, 4, 0, keya);

            if (success)
            {
                Serial.println("Sector 1 (Blocks 4..7) has been authenticated");
                uint8_t data[16];

                // If you want to write something to block 4 to test with, uncomment
                // the following line and this text should be read back in a minute
                //memcpy(data, (const uint8_t[]){ 'P', 'A', 'V', 'E', ' ', 'S', 'L', 'N', ' ', 'C', 'A', 'R', 'D', ' ', 'I', 'D' }, sizeof data);
                //success = nfc.mifareclassic_WriteDataBlock (4, data);

                // Try to read the contents of block 4
                success = nfc.mifareclassic_ReadDataBlock(4, data);

                if (success)
                {
                    // Data seems to have been read ... spit it out
                    Serial.println("Reading Block 4:");
                    nfc.PrintHexChar(data, 16);
                    Serial.println("");

                    // Wait a bit before reading the card again
                    delay(1000);
                }
                else
                {
                    Serial.println("Ooops ... unable to read the requested block.  Try another key?");
                }
            }
            else
            {
                Serial.println("Ooops ... authentication failed: Try another key?");
            }
        }

    }
}
