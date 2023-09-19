// PIR sensor pin
const int pirPin = 2;

void setup() {
    // Initialize serial communication
    Serial.begin(9600);

    // Configure PIR sensor pin as input
    pinMode(pirPin, INPUT);
}

void loop() {
    if (digitalRead(pirPin) == HIGH) {
        Serial.println("1");
    }
    else {
        Serial.println("0");
    }
    delay(100);
}
