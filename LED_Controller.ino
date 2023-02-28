const int TOLERANCE_VALUE = 20;
const uint32_t BAUD = 115200;
const int SLEEP_TIME = 16;

struct RGB
{
  uint8_t red = 255;
  uint8_t green = 255;
  uint8_t blue = 255;
};

RGB calcRGBValue(uint8_t brightness, uint16_t position) // Takes a brightness value and the position in a color spectrum.
{
  // I have adapted some of the code for this function from a previous project which can be found here: https://github.com/Razerfish/ble-cat-lamp/blob/master/Main/lighting.cpp
  
  // Contstrain brightness to 0-100
  uint8_t bPercent = map(brightness, 0, 255, 0, 100);

  float ratio = float(position) / 1024.0f;

  // Find our precise location in a 6 sector gradient with each sector being 256 units long
  uint16_t normalized = ratio * 256 * 6;

  // Find the distance from the start of our sector
  uint16_t range = normalized % 256;

  RGB color;

  // Switch depending on which sector we're in
  switch (normalized / 256)
  {
  case 0: // Red(255, 0, 0) -> Yellow (255, 255, 0)
    color.red = 255;
    color.green = range;
    color.blue = 0;
    break;
  case 1: // Yellow(255, 255, 0) -> Green(0, 255, 0)
    color.red = 255 - range;
    color.green = 255;
    color.blue = 0;
    break;
  case 2: // Green(0, 255, 0) -> Cyan(0, 255, 255)
    color.red = 0;
    color.green = 255;
    color.blue = range;
    break;
  case 3: // Cyan(0, 255, 255) -> Blue(0, 0, 255)
    color.red = 0;
    color.green = 255 - range;
    color.blue = 255;
    break;
  case 4: // Blue(0, 0, 255) -> Magenta(255, 0, 255)
    color.red = range;
    color.green = 0;
    color.blue = 255;
    break;
  case 5: // Magenta(255, 0, 255) -> Red(255, 0, 0)
    color.red = 255;
    color.green = 0;
    color.blue = 255 - range;
    break;
  }

  // Adjust by brightness
  color.red = uint8_t(float(color.red) * (float(bPercent) / 100.0f));
  color.green = uint8_t(float(color.green) * (float(bPercent) / 100.0f));
  color.blue = uint8_t(float(color.blue) * (float(bPercent) / 100.0f));

  return color;
}

void setup()
{
  Serial.begin(BAUD);
  pinMode(A0, INPUT); // Potentiometer analog input.
  pinMode(2, INPUT_PULLUP); // Push button digital input.

  pinMode(4, OUTPUT); // Monochrome LED digital output.

  pinMode(6, OUTPUT); // RGB LED red analog output.
  pinMode(5, OUTPUT); // RGB LED green analog output.
  pinMode(3, OUTPUT); // RGB LED blue analog output.
}

static bool controlMode = true; // false = controlling hue, true = controlling brightness.
static bool buttonReleased = true; // Used to prevent extremely fast toggleing when holding the push button.
static uint16_t lastPotValue = 1024; // Used for holding value when switching control modes. Default value is set to one out of range of the possible pot values so it instantly updates on startup.

// These variables are static so we can log them every loop even when they aren't updated, calculated or used.
static uint8_t brightness = 255;
static uint16_t hue = 0;
static RGB color;

void loop()
{
  // Check control mode toggle button.
  if (digitalRead(2) == LOW && buttonReleased)
  {
    // If this is the first loop that the button is pressed toggle the control mode and set buttonReleased to false.
    controlMode = !controlMode;
    buttonReleased = false;

    // Write to mode indicator LED.
    (controlMode) ? digitalWrite(4, LOW) : digitalWrite(4, HIGH);
  }
  else if (digitalRead(2) == HIGH && !buttonReleased)
  {
    // Set button released to true one button has been released.
    buttonReleased = true;
  }

  uint16_t potValue = analogRead(A0);
  if (potValue < lastPotValue - TOLERANCE_VALUE || potValue > lastPotValue + TOLERANCE_VALUE) // Only update if the difference between the current pot value and the last pot value exceeds a tolerance range.
  {
    if (controlMode) // Update hue
    {
      hue = constrain(potValue, 0, 1023); // Ensure that we are within an allowable range, probably not strictly nessesary but y'know, just in case.
    }
    else // Update brightness
    {
      brightness = map(constrain(potValue, 0, 1023), 0, 1023, 0, 255);
    }

    // Update color and write to RGB LED
    color = calcRGBValue(brightness, hue);

    // Write to RGB LED.
    analogWrite(6, color.red);
    analogWrite(5, color.green);
    analogWrite(3, color.blue);
  }

  // Log values to serial stream. This bit is messy but I just need to get this done.
  String logMessage = "";
  (controlMode) ? logMessage += "Control mode: Hue\n" : logMessage += "Control mode: brightness\n";
  logMessage += "buttonReleased: ";
  (buttonReleased) ? logMessage += "true\n" : "false\n";
  logMessage += "lastPotValue: " + String(lastPotValue) + "\n";

  logMessage += "Brightness: " + String(brightness) + "\n";
  logMessage += "Hue: " + String(hue) + "\n";
  logMessage += "Color: (" + String(color.red) + ", " + String(color.green) + ", " + String(color.blue) + ")\n";

  logMessage += "potValue: " + String(potValue) + "\n";

  Serial.print(logMessage);

  delay(SLEEP_TIME);
}
