#include <TimerOne.h>

#define BACK_PIN 3
#define LEFT_PIN 4
#define START_PIN 5
#define RIGHT_PIN 9
#define DOWN_PIN 10
#define FWD_PIN 11
#define UP_PIN 12

#define SWITCH_PIN A5

//used wake up game if it falls asleep
volatile uint8_t forceAwake = 0;
void startPinController()
{
	static uint8_t x = 0;
	x++;
	
	//pulse start pin during every other interval to force awake
	if ((x & 1) && forceAwake)
		digitalWrite(START_PIN, HIGH);
	else
		digitalWrite(START_PIN, LOW);
}

//
void pulsePin(uint8_t pin, uint32_t duration)
{
	digitalWrite(pin, HIGH);
	delay(duration);
	digitalWrite(pin, LOW);
}

//
void autoPilot()
{
	//wait for switch to be pressed
	while(!getDebouncedButtonState());

	forceAwake = 1;
	
	//home
	//assumes that crane is already fully raised
	pulsePin(RIGHT_PIN, 3000);
	pulsePin(BACK_PIN, 3000);
	
	//center
	pulsePin(LEFT_PIN, 1000);
	pulsePin(FWD_PIN, 1000);
	
	//dance
	pulsePin(LEFT_PIN, 1000);
	delay(500);
	pulsePin(RIGHT_PIN, 1000);
	delay(500);
	pulsePin(FWD_PIN, 1000);
	delay(500);
	pulsePin(BACK_PIN, 1000);
	delay(500);
	pulsePin(RIGHT_PIN, 1000);
	delay(500);
	
	//attempt grab
	pulsePin(DOWN_PIN, 3000);
	pulsePin(UP_PIN, 3000);

	//go to drop off
	pulsePin(FWD_PIN, 3000);
	pulsePin(LEFT_PIN, 3000);
	
	//do drop
	pulsePin(DOWN_PIN, 1000);
	
	//back up
	pulsePin(UP_PIN, 1050);

	forceAwake = 0;
}

//
void controlPin(uint8_t pin)
{
	//wait for btn to be pressed
	while(!getDebouncedButtonState());

	//start action
	digitalWrite(pin, HIGH);

	//wait for btn to be released
	while(getDebouncedButtonState());
	
	//stop action
	digitalWrite(pin, LOW);
}

//
void manualOverride()
{
	//wait for switch to be released
	while(getDebouncedButtonState());

	//control each action one at a time
	while(1)
	{
		controlPin(RIGHT_PIN);
		controlPin(LEFT_PIN);
		controlPin(BACK_PIN);
		controlPin(FWD_PIN);
		controlPin(DOWN_PIN);
		controlPin(UP_PIN);
	}
}

//state for button debounce
volatile uint32_t lastButtonTime;
volatile uint8_t lastButtonState;
ISR(PCINT1_vect)
{
	uint8_t instantaneous = digitalRead(SWITCH_PIN) == LOW;

	getDebouncedButtonState(); //latch last state if applicable
	
	lastButtonTime = millis();
	lastButtonState = instantaneous;
}

//state is validated/updated at request time
#define DEBOUNCE_THRESHOLD 25
uint8_t getDebouncedButtonState()
{
	static uint8_t debouncedButtonState = 0;
	
	if (millis() - lastButtonTime > DEBOUNCE_THRESHOLD)
		debouncedButtonState = lastButtonState;

	return debouncedButtonState;
}

//
void setupButton()
{
	pinMode(SWITCH_PIN, INPUT_PULLUP);
	delay(100);
	
	//init state
	lastButtonState = digitalRead(SWITCH_PIN) == LOW;
	lastButtonTime = 0;

	PCICR |= _BV(PCIE1); //enable pin change interrupts
	PCMSK1 = 0b00100000;
}

//
void setup()
{
	pinMode(BACK_PIN, OUTPUT);
	pinMode(LEFT_PIN, OUTPUT);
	pinMode(START_PIN, OUTPUT);
	pinMode(RIGHT_PIN, OUTPUT);
	pinMode(DOWN_PIN, OUTPUT);
	pinMode(FWD_PIN, OUTPUT);
	pinMode(UP_PIN, OUTPUT);
	
	Timer1.initialize(200000);
	Timer1.attachInterrupt(startPinController);
	
	setupButton();

	if (!getDebouncedButtonState())
	{
		while(1)
			autoPilot();
	}
	else
	{
		forceAwake = 1;
		manualOverride();
	}
}

//
void loop() {}
