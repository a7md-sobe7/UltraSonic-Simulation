#define TRIG_PIN 12
#define ECHO_PIN 8
#define LED_PIN 7

#define DANGER_DISTANCE 5   // cm

volatile bool waitingForRising = true;
volatile bool measurementReady = false;

volatile uint16_t startTime = 0;
volatile uint16_t endTime = 0;
volatile uint32_t overflowCount = 0;
volatile uint32_t pulseDuration = 0;

void setup() {
  Serial.begin(9600);

  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(LED_PIN, OUTPUT);

  // Timer1 → 1 µs resolution
  TCCR1A = 0;
  TCCR1B = 0;
  TCCR1B |= (1 << CS11);     // Prescaler = 8
  TIMSK1 |= (1 << TOIE1);    // Overflow interrupt

  // Pin Change Interrupt for D8
  PCICR |= (1 << PCIE0);
  PCMSK0 |= (1 << PCINT0);

  sei();
}

void loop() {
  // Simulated distance loop
  for (int distance = 50; distance >= 5; distance -= 3) {

    Serial.print("Distance: ");
    Serial.print(distance);
    Serial.println(" cm");

    int zone;
    switch (distance) {
      case 50:
      case 47:
      case 44:
      case 41:
      case 38:
      case 35:
        zone = 1; // safe ZONE
        break;
      case 32:
      case 29:
      case 26:
      case 23:
        zone = 2; // take care ZONE
        break;
      case 20:
      case 17:
      case 16:
      case 14:
        zone = 3; // warning zone
        break;
      case 11:
      case 8:
      case 5:
        zone = 4; // DANGER ZONE
        break;
              default:
                zone = 0; // Path clear
                break;
    }

    // تنفيذ الـ LED حسب الـ zone
    switch (zone) {
      case 1:
        blinkLED(600);
        Serial.println("safe ZONE");
        break;
      case 2:
        blinkLED(500);
        Serial.println("take care ZONE !!!");
        break;
      case 3:
        blinkLED(400);
        Serial.println("warning ZONE");
        break;
      case 4:
        blinkLED(200);
        Serial.println("DANGER ZONE");
        break;
      case 0:
        digitalWrite(LED_PIN, LOW);
        Serial.println("Path Clear");
        break;
    }

    Serial.println("--------------------");
   // delay(250); // delay between the change in the distance
  }
  triggerUltrasonic();
  /* actual simulator calculations
  if (measurementReady) {
    noInterrupts();
    uint32_t duration = pulseDuration;
    measurementReady = false;
    interrupts();

    float distance = duration / 58.0; 
  }
  */
  
}

void blinkLED(int delayTime) {
  digitalWrite(LED_PIN, HIGH);
  delay(delayTime);
  digitalWrite(LED_PIN, LOW);
  delay(delayTime);
}

void triggerUltrasonic() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
}

ISR(PCINT0_vect) {
  if (digitalRead(ECHO_PIN)) {
    if (waitingForRising) {
      startTime = TCNT1;
      overflowCount = 0;
      waitingForRising = false;
    }
  } else {
    if (!waitingForRising) {
      endTime = TCNT1;
      pulseDuration =
        ((uint32_t)overflowCount << 16) + endTime - startTime;
      measurementReady = true;
      waitingForRising = true;
    }
  }
}

ISR(TIMER1_OVF_vect) {
  overflowCount++;
}