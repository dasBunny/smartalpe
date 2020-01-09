#include <lmic.h> //LoraWAN Library
#include <hal/hal.h>
#include <SPI.h>
//#include <DHT.h> //Library for inside temperature/humidity sensors
#include <SoftwareSerial.h>
#include <OneWire.h> //Library for the outside temperature sensor
#include <DallasTemperature.h> //Additional library for the outside temperature sensor

// Device keys from TTN, always use MSB
static const PROGMEM u1_t NWKSKEY[16] = { 0xB4, 0x21, 0x81, 0x4F, 0xE3, 0x9E, 0xF6, 0x4A, 0xCE, 0x6D, 0x0D, 0xE4, 0x2C, 0x27, 0x2A, 0xC8 }; //Network Session Key
static const u1_t PROGMEM APPSKEY[16] = { 0xF7, 0x97, 0x6B, 0xC6, 0x69, 0xC6, 0x27, 0x88, 0x64, 0x30, 0x97, 0x63, 0xBD, 0xFF, 0x3E, 0xAF }; //App Session Key
static const u4_t DEVADDR = 0x260114EA; //Device Adress

//############ VARIOUS OTHER THINGS ############
SoftwareSerial sensor(5,6); //SoftwareSerial object for communication with the sensor arduino
byte stage = 0; //counter used when preparing the next transmission

//############ LMIC RELATED OBJECTS ############
uint8_t data[] = "0000000000"; //Data array, will be populated with measured values later
static osjob_t sendjob;
const unsigned TX_INTERVAL = 300; //Time between measurements

//############ DEFINTIONS FOR ENERGY MANAGEMENT ###########
#define BAT_MAXCHARGE 200
#define MIN_SUNLIGHT 100

//############ PIN DEFINITIONS ############
#define POWER A7
#define ONE_WIRE_BUS A2
#define DHT_pin A1
#define LDR_pin A6
#define BAT_short 7

//Pinmap for the RF95 chip
const lmic_pinmap lmic_pins = {
    .nss = 10,                       
    .rxtx = LMIC_UNUSED_PIN,
    .rst = LMIC_UNUSED_PIN,                       
    .dio = {2,3,3}, 
};

//############ TEMPERATURE SENSORS ############
OneWire oneWire(ONE_WIRE_BUS); //create OneWire Object for outside temperature sensor
DallasTemperature DS20B18(&oneWire); //create DallasTemperature object using the OneWire object
//DHT dht_object(DHT_pin,DHT22); //DHT object for the inside temperature/humidity sensors


//Events in the lora library, fired whenever something important happens
//Only used for debug purposes, except for EV_TXCOMPLETE which restarts the timer
void onEvent (ev_t ev) {
    Serial.print(os_getTime());
    Serial.print(F(": "));
    switch(ev) {
        case EV_JOINING:
            Serial.println(F("EV_JOINING"));
            break;
        case EV_JOINED:
            Serial.println(F("EV_JOINED"));
            break;
            
        case EV_JOIN_FAILED:
            Serial.println(F("EV_JOIN_FAILED"));
            break;
        case EV_REJOIN_FAILED:
            Serial.println(F("EV_REJOIN_FAILED"));
            break;
        case EV_TXCOMPLETE:
            Serial.println(F("EV_TXCOMPLETE (includes waiting for RX windows)"));
            if (LMIC.txrxFlags & TXRX_ACK)
              Serial.println(F("Received ack"));
            if (LMIC.dataLen) {
              Serial.println(F("Received "));
              Serial.println(LMIC.dataLen);
              Serial.println(F(" bytes of payload"));
            }
            // Schedule next transmission
            os_setTimedCallback(&sendjob, os_getTime()+sec2osticks(TX_INTERVAL), prepare_transmission);
            break;
        case EV_LOST_TSYNC:
            Serial.println(F("EV_LOST_TSYNC"));
            break;
        case EV_RESET:
            Serial.println(F("EV_RESET"));
            break;
        case EV_RXCOMPLETE:
            // data received in ping slot
            Serial.println(F("EV_RXCOMPLETE"));
            break;
        case EV_LINK_DEAD:
            Serial.println(F("EV_LINK_DEAD"));
            break;
        case EV_LINK_ALIVE:
            Serial.println(F("EV_LINK_ALIVE"));
            break;
        case EV_TXSTART:
            Serial.println(F("EV_TXSTART"));
            break;
        default:
            Serial.print(F("Unknown event: "));
            Serial.println((unsigned) ev);
            break;
    }
}

void prepare_transmission(){
  /*  This function is called by the timer. It toggles on the 
   *  sensor arduino, requests data and sets the stage to 1, so the next steps
   *  can be executed in the loop
   */ 
  digitalWrite(POWER, HIGH); //Enable power for sensor board
  sensor.flush();
  while(sensor.available()){ //Discard all old serial bytes
    Serial.print(F("Discarding: "));
    Serial.println(sensor.read());
  }
  sensor.write('R'); //Request data command for the sensor
  stage=1;
  Serial.println(F("Triggered stage 1"));
  //The next steps are handled in the stages in the loop
}

byte outsideTemp(){
  /*
   * The outside temperature sensor is connected via OneWire
   * The DallasTemperature library is used here to make reading the data easier
   */
    int outsideTemperature = (int) DS20B18.getTempCByIndex(0); //Reading sensor value
    Serial.print(F("Outside Temperature: "));
    Serial.print(outsideTemperature);
    Serial.println(F(" Celsius"));
    if(outsideTemperature >=127) return 127; //Temperatures above 127 Degrees Celcius are totally unrealistic and can be discarded
    if(outsideTemperature <=-127) return 255; //Temperatures below -127 Degrees Celcius are also unrealistic
    if(0 <= outsideTemperature < 127) return (byte) outsideTemperature; //if the temperature is positive and below 127 degrees it can be transmitted without conversions
    if(-127 < outsideTemperature < 0) return (byte) 127 + outsideTemperature; //Negative values are counted from 10000001 upwards
    return 255;
}

byte sunlight(){
    int ldr = analogRead(LDR_pin); //Measuring the voltage on the LDR pin
    ldr = ldr/4; //Sacrifizing accuracy for less airtime by removing the two least significant bits
    if(ldr>255) return 255; //error prevention if 'ldr' is larger than expected
    return (byte) ldr;
}

//old temperature/humidity code, not working
/*byte insideTemperature(){
  int temp = (int) dht_object.readTemperature();
  Serial.print(F("Inside Temperature: "));
  Serial.print(temp);
  Serial.println(F("Celsius"));
  if(temp >= 127) return 127;
  if(temp <= -127) return 255;
  if(0 <= temp < 127) return (byte) temp;
  if(-127 < temp < 0) return (byte) 128 - temp;
  return 255;
}

byte insideHumidity(){
  int humid = dht_object.readHumidity();
  Serial.print(F("Inside Humidity: "));
  Serial.print(humid);
  Serial.println(F("%"));
  if(humid >= 127) return 127;
  if(humid <= -127) return 255;
  if(0 <= humid < 127) return (byte) humid;
  if(-127 < humid < 0) return (byte) 128 - humid;
  return 255;
}*/

//Function that starts the actual transmission
void sendData(osjob_t* j){
  /* This function is called when all measurement data has been gathered
   * and the array is ready to be send. 
   */
  //TODO Cancel previous job if it isn't done in time
    // Check if there is not a current TX/RX job running
    if (LMIC.opmode & OP_TXRXPEND) {
        Serial.println(F("OP_TXRXPEND, not sending"));
    } else {
        // Prepare upstream data transmission at the next possible time.
        LMIC_setTxData2(1, data, sizeof(data)-1, 0);
        Serial.println(F("Packet queued"));
    }
    // Next TX is scheduled after TX_COMPLETE event.
}

void setup() {
    pinMode(POWER, OUTPUT);
    pinMode(A0, OUTPUT);
    digitalWrite(POWER, LOW);
    digitalWrite(A0, HIGH);
    while (!Serial); // wait for Serial to be initialized
    Serial.begin(115200); //Begin debug serial
    delay(100); 
    Serial.println(F("Starting"));
    sensor.begin(9600); //Begin connections between the sensors
    DS20B18.begin(); //start external temperature sensor
    //dht_object.begin();
    //Serial.println(dht_object.readTemperature());


//>>>>>>>> All setup code below this is configuring the LMIC library <<<<<<<<<<<
    // LMIC init
    os_init();
    // Reset the MAC state. Session and pending data transfers will be discarded.
    LMIC_reset();

    // Set static session parameters. Instead of dynamically establishing a session
    // by joining the network, precomputed session parameters are be provided.
    #ifdef PROGMEM
    // On AVR, these values are stored in flash and only copied to RAM
    // once. Copy them to a temporary buffer here, LMIC_setSession will
    // copy them into a buffer of its own again.
    uint8_t appskey[sizeof(APPSKEY)];
    uint8_t nwkskey[sizeof(NWKSKEY)];
    memcpy_P(appskey, APPSKEY, sizeof(APPSKEY));
    memcpy_P(nwkskey, NWKSKEY, sizeof(NWKSKEY));
    LMIC_setSession (0x13, DEVADDR, nwkskey, appskey);
    #else
    // If not running an AVR with PROGMEM, just use the arrays directly
    LMIC_setSession (0x13, DEVADDR, NWKSKEY, APPSKEY);
    #endif

    #if defined(CFG_eu868)
    //Defining different radio frequency channels the library can use
    LMIC_setupChannel(0, 868100000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band
    LMIC_setupChannel(1, 868300000, DR_RANGE_MAP(DR_SF12, DR_SF7B), BAND_CENTI);      // g-band
    LMIC_setupChannel(2, 868500000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band
    LMIC_setupChannel(3, 867100000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band
    LMIC_setupChannel(4, 867300000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band
    LMIC_setupChannel(5, 867500000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band
    LMIC_setupChannel(6, 867700000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band
    LMIC_setupChannel(7, 867900000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band
    LMIC_setupChannel(8, 868800000, DR_RANGE_MAP(DR_FSK,  DR_FSK),  BAND_MILLI);      // g2-band
    // TTN defines an additional channel at 869.525Mhz using SF9 for class B
    // devices' ping slots. LMIC does not have an easy way to define set this
    // frequency and support for class B is spotty and untested, so this
    // frequency is not configured here.
    #endif

    // Disable link check validation
    LMIC_setLinkCheckMode(0);

    // TTN uses SF9 for its RX2 window.
    LMIC.dn2Dr = DR_SF9;

    // Set data rate and transmit power for uplink
    LMIC_setDrTxpow(DR_SF7,14);

    // Start job
    prepare_transmission();
}

void loop() {
  /*
   * Some functions of the lorawan library should be run as often as possible. os_runloop_once() at the end
   * of the loop is taking care of this.
   * Sometimes wait times are occuring during the preparation of a new transmission. To ensure os_runloop_once() is 
   * called often enough, preparing a transmission works uses different stages in the loop.
   * 
   * 
   */
  if(stage==1){ //Handling the data read from the sensor arduino
    Serial.print(F("Available bytes: ")); //Serial is used for debug messages to check if data is transmitted properly
    Serial.println(sensor.available());
    Serial.print(F("Next serial:"));
    Serial.println(sensor.peek());
    for(int j=0; j<=sensor.available(); j++){ //Checking if the available bytes contain the start indicator (0xFF)
      if(sensor.peek() == 0xFF){ //Stop removing if the byte matches
        Serial.println(F("Break!"));
        break;
      }
      sensor.read(); //Remove byte if it does not match
    }
    if(sensor.available()>=6 && sensor.peek()==0xFF){ //if the first byte is the start byte and all needed bytes are received
      sensor.read(); //Discard start byte
      for(int i=0; i<5; i++){ //copy the 5 bytes  from the sensor arduino into the data[] array
        int dataTemp=sensor.read();
        Serial.println(dataTemp);
        data[i]=(uint8_t) dataTemp; 
      }
      DS20B18.requestTemperatures(); //Request a temperature reading from the outside DS20B18 sensor in preparation for stage 2
      Serial.println(F("Stage 2 fnished"));
      stage=2;
      sensor.flush();
    }
  }
  if(stage==2){
    Serial.println(F("Stage 2"));
    //Adding the data recorded by this device to the data array, index 0-4 have been set in stage 1
    data[5]=(uint8_t)89; //battery level, currently hardcoded due to missing energy management
    data[6]=(uint8_t)15; //inside temperature, currently hardcoded value
    data[7]=(uint8_t)87; //inside humidity, currently hardcoded value
    data[8]=outsideTemp(); //Outside temperature
    data[9]=sunlight(); //Get sunlight
    stage=0; //Reset stage
    sendData(&sendjob); //Queue data transmission
    digitalWrite(POWER, LOW); //Disable power supply for the sensor board
  }

  
    os_runloop_once();
}
