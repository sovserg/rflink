#include <getopt.h>
#include "Base.h"
#include <wiringPi.h>
// My header files
#include "server.h"
#include "Plug.h"
#include "Mqtt.h"

//****************************************************************************************************************************************
// seting start values for global variables and strutures

void(*Reboot)(void)=0;                                                          // reset function on adres 0.
byte PKSequenceNumber=0;                                                        // 1 byte packet counter
boolean RFDebug=false;                                                          // debug RF signals with plugin 001 
boolean RFUDebug=false;                                                         // debug RF signals with plugin 254 
boolean QRFDebug=false;
byte logLevel=4;																// log level (0-nothing, 1-error log, 2-warning, 3-running status, 4-debug)

struct RawSignalStruct RawSignal={0,0,0,0,0,0L};                                // Raw signal variabelen places in a struct
 
byte PIN_RF_TX_DATA=DEFAUL_PIN_RF_TX_DATA ;										// Data to the 433Mhz transmitter on this pin (wiringpi numbering)
byte PIN_RF_RX_DATA=DEFAULT_PIN_RF_RX_DATA;										// On this input, the 433Mhz-RF signal is received. LOW when no signal. (wiringpi numbering)
 
// ===============================================================================
unsigned long RepeatingTimer=0L;
unsigned long SignalCRC=0L;                                                     // holds the bitstream value for some plugins to identify RF repeats
unsigned long SignalHash=0L;                                                    // holds the processed plugin number
unsigned long SignalHashPrevious=0L;                                            // holds the last processed plugin number


char pbuffer[PRINT_BUFFER_SIZE];                                                // Buffer for printing data
char InputBuffer_Serial[BUFSIZE];                                                		// Buffer for input data this name is uset in PluginTX_XXX function therefore this is alias to inBuf from server.cpp

// serial port emulation for print and write functions
Ser Serial;

boolean mqtt_init_need=false;

// log functions
void log(int level,std::string str, boolean endLine)  						// log withou end line
{
	if (level<=logLevel) {
		std::cout << str;
		if (endLine==true) std::cout << std::endl;
	}
	return;
}

void log(int level, std::string str) {										// log witj end line
	return log(level, str, true);
}


void setup() {
  wiringPiSetup();
  //serialOpen(BAUD);                                                           // Initialise the serial port
  pinMode(PIN_RF_RX_DATA, INPUT);                                               // Initialise in/output ports
  pinMode(PIN_RF_TX_DATA, OUTPUT);                                              // Initialise in/output ports
  pinMode(PIN_RF_TX_VCC,  OUTPUT);                                              // Initialise in/output ports
  pinMode(PIN_RF_RX_VCC,  OUTPUT);                                              // Initialise in/output ports    
  digitalWrite(PIN_RF_RX_VCC,HIGH);                                             // turn VCC to RF receiver ON
  digitalWrite(PIN_RF_RX_DATA,PUD_UP);                                    // pull-up resister on (to prevent garbage)
  
  pinMode(PIN_BSF_0,OUTPUT);                                                    // rflink board switch signal
  digitalWrite(PIN_BSF_0,HIGH);                                                 // rflink board switch signal

  Serial.ini(); // ini mutex for print line 
  
  //PKSequenceNumber++;
  PluginInit();
  PluginTXInit();
}

int serve_input(char* inBuf){
	strncpy(InputBuffer_Serial,inBuf,BUFSIZE);
	//InputBuffer_Serial=inBuf;											// create alias from inBuf to global bariable InputBuffer_Serial uset in PluginTX_XXX function
	byte ValidCommand=0;
	//Serial.print("20;incoming;"); 
	//Serial.println(InputBuffer_Serial); 
	log(LOG_STATUS,"Client incoming: ",false);
	log(LOG_STATUS,InputBuffer_Serial);
	
	if (strlen(InputBuffer_Serial) > 7){                                // need to see minimal 8 characters on the serial port
	   // 10;....;..;ON;
	   if (strncmp (InputBuffer_Serial,"10;",3) == 0) {                 // Command from Master to RFLink
		  // -------------------------------------------------------
		  // Handle Device Management Commands
		  // -------------------------------------------------------
		  if (strcasecmp(InputBuffer_Serial+3,"PING;")==0) {
			 sprintf(InputBuffer_Serial,"20;%02X;PONG;",PKSequenceNumber++);
			 Serial.println(InputBuffer_Serial); 
		  } else
		  if (strcasecmp(InputBuffer_Serial+3,"REBOOT;")==0) {
			 strcpy(InputBuffer_Serial,"reboot");
			 return -1;
		  } else
		  if (strncasecmp(InputBuffer_Serial+3,"RFDEBUG=O",9) == 0) {
			 if (InputBuffer_Serial[12] == 'N' || InputBuffer_Serial[12] == 'n' ) {
				RFDebug=true;                                           // full debug on
				RFUDebug=false;                                         // undecoded debug off 
				QRFDebug=false;                                        // undecoded debug off
				sprintf(InputBuffer_Serial,"20;%02X;RFDEBUG=ON;",PKSequenceNumber++);
			 } else {
				RFDebug=false;                                          // full debug off
				sprintf(InputBuffer_Serial,"20;%02X;RFDEBUG=OFF;",PKSequenceNumber++);
			 }
			 Serial.println(InputBuffer_Serial); 
		  } else                 
		  if (strncasecmp(InputBuffer_Serial+3,"RFUDEBUG=O",10) == 0) {
			 if (InputBuffer_Serial[13] == 'N' || InputBuffer_Serial[13] == 'n') {
				RFUDebug=true;                                          // undecoded debug on 
				QRFDebug=false;                                        // undecoded debug off
				RFDebug=false;                                          // full debug off
				sprintf(InputBuffer_Serial,"20;%02X;RFUDEBUG=ON;",PKSequenceNumber++);
			 } else {
				RFUDebug=false;                                         // undecoded debug off
				sprintf(InputBuffer_Serial,"20;%02X;RFUDEBUG=OFF;",PKSequenceNumber++);
			 }
			 Serial.println(InputBuffer_Serial); 
		  } else                 
		  if (strncasecmp(InputBuffer_Serial+3,"QRFDEBUG=O",10) == 0) {
			 if (InputBuffer_Serial[13] == 'N' || InputBuffer_Serial[13] == 'n') {
				QRFDebug=true;                                         // undecoded debug on 
				RFUDebug=false;                                         // undecoded debug off 
				RFDebug=false;                                          // full debug off
				sprintf(InputBuffer_Serial,"20;%02X;QRFDEBUG=ON;",PKSequenceNumber++);
			 } else {
				QRFDebug=false;                                        // undecoded debug off
				sprintf(InputBuffer_Serial,"20;%02X;QRFDEBUG=OFF;",PKSequenceNumber++);
			 }
			 Serial.println(InputBuffer_Serial); 
		  } else                 
		  if (strncasecmp(InputBuffer_Serial+3,"VERSION",7) == 0) {
			  sprintf(InputBuffer_Serial,"20;%02X;VER=1.1;REV=%02x;BUILD=%02x;",PKSequenceNumber++,REVNR, BUILDNR);
			  Serial.println(InputBuffer_Serial); 
		  } else {
			 // -------------------------------------------------------
			 // Handle Generic Commands / Translate protocol data into Nodo text commands 
			 // -------------------------------------------------------
			 // check plugins
			 if (InputBuffer_Serial[strlen(InputBuffer_Serial)-1]==';') InputBuffer_Serial[strlen(InputBuffer_Serial)-1]=0;  // remove last ";" char
			 if(PluginTXCall(0, InputBuffer_Serial)) {
				ValidCommand=1;
			 } else {
				// Answer that an invalid command was received?
				ValidCommand=2;
			 }
		  }
	   }
	} // if > 7
	if (ValidCommand != 0) {
	   if (ValidCommand==1) {
		  sprintf(InputBuffer_Serial,"20;%02X;OK;",PKSequenceNumber++);
		  Serial.println( InputBuffer_Serial );
	   } else {
		  sprintf(InputBuffer_Serial, "20;%02X;CMD UNKNOWN;", PKSequenceNumber++); // Node and packet number 
		  Serial.println( InputBuffer_Serial );
	   }   
	}
	// odemknout mutex pro vystup s RX (aby se nemotala odpoved na vstup s vystupen z projmace)
	return 1;
}

void help(char *argv) {
  printf("Usage: %s [options]\n\
Options:\n\
 -p, --port           TCP port number: 1-65535\n\
 -t, --pin_tx         TX_PIN - transmitter pin (by wiringpi numbering)\n\
 -r, --pin_rx         RX_PIN - receiver pin (by wiringpi numbering)\n\
 -l, --log_level      log level number: 0-nothing, 1-error log, 2-warning, 3-running status, 4-debug\n\
 -H, --mqtt_host      MQTT Server (default: localhost)\n\
 -U, --mqtt_username  MQTT Username\n\
 -P, --mqtt_password  MQTT Password\n\
 -T, --mqtt_topic     MQTT topic (default: \"/devices/rflink\")\n\
\n\
     --help           display this help and exit\n", argv);
}

int common_send(const char* buf, const int size)
{
	int ret = 1;
	log(LOG_STATUS, "common send: ",false);
	log(LOG_STATUS, buf);
	if (mosq != NULL) {
	    ret = mqtt_send(buf, size);
	}
	if (clientSocket != -1) {
	    ret = socket_send(buf, size);
	}
	return ret;
}


int main(int argc, char *argv[]) {
	int port=DEFAULT_TCP_PORT;
	int scan_log=logLevel;				// form sscanf("%d", int) who make with int not byte
	int scan_pin_tx=PIN_RF_TX_DATA;
	int scan_pin_rx=PIN_RF_RX_DATA;
	boolean mqtt_init_need=false;
	
	static struct option long_options[] = {
          {"help",     no_argument, 0, 'h'},
          {"port",     required_argument, 0, 'p'},
          {"pin_tx",    required_argument, 0, 't'},
          {"pin_rx",    required_argument, 0, 'r'},
          {"log_level", required_argument, 0, 'l'},
          {"mqtt_host", required_argument, 0, 'H'},
          {"mqtt_username", required_argument, 0, 'U'},
          {"mqtt_password", required_argument, 0, 'P'},
          {"mqtt_topic", required_argument, 0, 'T'},
          {0, 0, 0, 0}
        };
        int c;

        while (1) {
          int option_index = 0;
          c = getopt_long (argc, argv, "hp:t:r:l:H:U:P:T:",
                       long_options, &option_index);
          if (c == -1) break;

          switch (c) {
            case 'h':
              help(argv[0]);
              return 0;
            case 'p':
              if (sscanf(optarg,"%d",&port)==EOF) {
                help(argv[0]);
                return false;
              }
              break;

            case 't':
              if (sscanf(optarg,"%d",&scan_pin_tx)==EOF) {
                help(argv[0]);
                return false;
              }
              break;

            case 'r':
              if (sscanf(optarg,"%d",&scan_pin_rx)==EOF) {
                help(argv[0]);
                return false;
              }
              break;

            case 'l':
              if (sscanf(optarg,"%d",&scan_log)==EOF or !(scan_log>=0 and scan_log<=4) ) {
                help(argv[0]);
                return false;
              }
              break;

            case 'H':
              mqtt_host=optarg;
              mqtt_init_need=true;
              break;

            case 'U':
              mqtt_username=optarg;
              mqtt_init_need=true;
              break;

            case 'P':
              mqtt_pw=optarg;
              mqtt_init_need=true;
              break;

            case 'T':
              mqtt_topic=optarg;
              mqtt_init_need=true;
              break;

            case '?':
              help(argv[0]);
              break;

            default:
              abort ();
          }
        }
	logLevel=(byte) scan_log;
	PIN_RF_TX_DATA=(byte) scan_pin_tx;
	PIN_RF_RX_DATA=(byte) scan_pin_rx;

	printf("Log level: %d\n",logLevel);
	sprintf(pbuffer,"TCP port: %d",port);
	log(LOG_STATUS, pbuffer);
	sprintf(pbuffer,"TX pin: %d",PIN_RF_TX_DATA);
	log(LOG_STATUS, pbuffer);
	sprintf(pbuffer,"RX pin: %d",PIN_RF_RX_DATA);
	log(LOG_STATUS, pbuffer);

	setup();
	if (mqtt_init_need) {
	  printf("MQTT init with server: %s, topic: %s, username: %s\n",mqtt_host, mqtt_topic, mqtt_username);
	  mqtt_setup(mqtt_host, mqtt_topic, mqtt_username, mqtt_pw);
	}
	Serial.set_send_function(common_send); // enable Serial.println common send function
	StartScanEventTheader();
	TCPserver(port);
}
/*********************************************************************************************/

