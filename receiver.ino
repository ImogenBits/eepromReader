#include "SerialTransfer.h"
SerialTransfer myTransfer;

typedef struct {
	uint32_t instruction;
	uint32_t pageNum;
	uint32_t len;
	byte data[128];
} Data;

//Breadboard mapping
/*
static int WE = 22;
static int OE = 28;
static int CE = 30;

//           D0, D1, D2...            ...D7
int IO[8] = {47, 48, 49, 35, 34, 33, 32, 31};
//                A0, A1, A2...                                       ...A14, A15, A16
int adress[17] = {46, 45, 44, 43, 42, 41, 40, 39, 25, 26, 29, 27, 38, 24, 23, 37, 36};
*/

//Shield mapping
static int WE = 25;
static int OE = 39;
static int CE = 43;

//           D0, D1, D2...            ...D7
static int IO[8] = {46, 48, 50, 53, 51, 49, 47, 45};
//                A0, A1, A2...                                       ...A14, A15, A16
static int adress[17] = {44, 42, 40, 38, 36, 34, 32, 30, 33, 35, 41, 37, 28, 31, 29, 26, 24};


void declarePins() {
  for (int i = 0; i < 8; i++) {
    pinMode(IO[i], OUTPUT);
  }
  for(int i = 0; i < 17; i++) {
    pinMode(adress[i], OUTPUT);
  }
  pinMode(WE, OUTPUT);
  pinMode(OE, OUTPUT);
  pinMode(CE, OUTPUT);
  digitalWrite(OE, HIGH);
  digitalWrite(WE, HIGH);
  digitalWrite(CE, LOW);
  

}

byte readFromAddress(long adr) {
  digitalWrite(WE, HIGH);
  for(int i = 0; i < 17; i++) {
    if(adr % 2 == 0) {
      digitalWrite(adress[i], LOW);
    }
    else {
      digitalWrite(adress[i], HIGH);
    }
    adr = adr >> 1;
  }
  for (int i = 0; i < 8; i++) {
    digitalWrite(IO[i], LOW);
    pinMode(IO[i], INPUT);
  }
  digitalWrite(OE, LOW);
  delayMicroseconds(1);
  byte value = 0;
  for (int i = 7; i >= 0; i--) {
    value = (value << 1) + digitalRead(IO[i]);
  }
  digitalWrite(OE, HIGH);
  for (int i = 0; i < 8; i++) {
    pinMode(IO[i], OUTPUT);
  }
  return value;
}

void writeAddress(long address, byte value) {
	for(int j = 0; j < 7; j++) {
		digitalWrite(adress[j], address & 1);
		address >>= 1;
	}

	for(int j = 0; j < 8; j++) {
		digitalWrite(IO[j], value & 1);
		value >>= 1;
	}

	digitalWrite(WE, LOW);
	delayMicroseconds(5);
	digitalWrite(WE, HIGH);
	delayMicroseconds(5);
}


/*void writePage(uint32_t pageNum, byte values[128]) {
	long startAdr = pageNum * 128;
	for (int i = 0; i < 128; i ++) {
		long adr = startAdr + i;
		for(int j = 0; j < 7; j++) {
			digitalWrite(adress[j], adr & 1);
			adr = adr >> 1;
		}

		byte currValue = values[i];
		for(int j = 0; j < 8; j++) {
			digitalWrite(IO[j], currValue & 1);
			currValue = currValue >> 1;
		}
		//delay(1000);
		digitalWrite(WE, LOW);
		//delay(1000);
		delayMicroseconds(5);
		digitalWrite(WE, HIGH);
		delayMicroseconds(5);
	}
	delay(11);
}*/

void writePage(long startAdr, byte value[128]) {
  long adr = startAdr;
  for(int i = 0; i < 17; i++) {
    digitalWrite(adress[i], (adr & 1));
    adr = adr >> 1;
  }
  
  for (int i = 0; i < 128; i ++) {
    long adr = startAdr + i;
    for(int j = 0; j < 7; j++) {
      digitalWrite(adress[j], (adr & 1));
      adr = adr >> 1;
    }

    byte currValue = value[i];
    for(int j = 0; j < 8; j++) {
      if(currValue % 2 == 0) {
        digitalWrite(IO[j], LOW);
      } else {
        digitalWrite(IO[j], HIGH);
      }
      currValue = currValue >> 1;
    }
    digitalWrite(WE, LOW);
    delayMicroseconds(5);
    digitalWrite(WE, HIGH);
    delayMicroseconds(5);
  }
  delay(11);
}

void writePage(Data pageData) {
	byte dataArray[128];
	for (int i = 0; i < pageData.len; i++) {
		dataArray[i] = pageData.data[i];
	}
	for (int i = pageData.len; i < 128; i++) {
		dataArray[i] = 0x00;
	}
	writePage(pageData.pageNum * 128, dataArray);
}

void readPage(Data &data) {
	data.len = 128;
	for (long i = 0; i < data.len; i++) {
		data.data[i] = readFromAddress((long) (data.pageNum * 128) + i);
	}
}

void disableDataProtect() {
	writeAddress(0x5555, 0xAA);
	writeAddress(0x2AAA, 0x55);
	writeAddress(0x5555, 0x80);
	writeAddress(0x5555, 0xAA);
	writeAddress(0x2AAA, 0x55);
	writeAddress(0x5555, 0x20);
	delay(100);
}

void setup() {
	declarePins();
	Serial.begin(115200);
	myTransfer.begin(Serial);
	delay(10);
  	//disableDataProtect();
    pinMode(LED_BUILTIN, OUTPUT);
    for (int i = 0; i < 3; i++) {
        digitalWrite(LED_BUILTIN, HIGH);
        delay(500);
        digitalWrite(LED_BUILTIN, LOW);
        delay(500);
    }
}

void loop() {
	if (myTransfer.available()) {
        pinMode(LED_BUILTIN, OUTPUT);
        for (int i = 0; i < 2; i++) {
            digitalWrite(LED_BUILTIN, HIGH);
            delay(1000);
            digitalWrite(LED_BUILTIN, LOW);
            delay(1000);
        }
		//receive data
		Data data;
		int receivedBytes = 0;
		myTransfer.rxObj(data.instruction, sizeof(data.instruction), receivedBytes);
		receivedBytes += sizeof(data.instruction);
		myTransfer.rxObj(data.pageNum, sizeof(data.pageNum), receivedBytes);
		receivedBytes += sizeof(data.pageNum);
		myTransfer.rxObj(data.len, sizeof(data.len), receivedBytes);
		receivedBytes += sizeof(data.len);
		myTransfer.rxObj(data.data, data.len, receivedBytes);
		receivedBytes += data.len;

		//process data
		if (data.instruction == 1) {
			writePage(data);

			//send return message
			myTransfer.txObj(data.pageNum, sizeof(data.pageNum), 0);
			myTransfer.sendData(sizeof(data.pageNum));

		} else if (data.instruction == 2) {
			readPage(data);

			//send return message
			int sendBytes = 0;
			myTransfer.txObj(data.pageNum, sizeof(data.pageNum), sendBytes);
			sendBytes += sizeof(data.pageNum);
			myTransfer.txObj(data.len, sizeof(data.len), sendBytes);
			sendBytes += sizeof(data.len);
			myTransfer.txObj(data.data, sizeof(data.data), sendBytes);
			sendBytes += sizeof(data.data);
			myTransfer.sendData(sendBytes);
		}
	}
}