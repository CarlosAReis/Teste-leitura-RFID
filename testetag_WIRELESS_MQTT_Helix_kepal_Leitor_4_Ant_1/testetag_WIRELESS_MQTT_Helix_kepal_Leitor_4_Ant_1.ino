#include <PubSubClient.h>
#include <WiFi.h>
#include <SPI.h>
#include <MFRC522.h>
// definir os valores para cada palavra 

#define SS_PIN    21
#define RST_PIN   22
#define SIZE_BUFFER     18
#define MAX_SIZE_BLOCK  16
#define greenPin     12
#define redPin       32


//**************************************
//*********** MQTT CONFIG **************
//**************************************
const char *mqtt_server = "52.14.171.32";  ///helix ip
const int mqtt_port = 1883;//1883,1026,4041
const char *mqtt_user = "NnB7tFj8KgQdkjg";
const char *mqtt_pass = "KoTg3FGuYMMwSxW";
const char *root_topic_subscribe = "qkk2sNHLWr2MFm6/input";// "qkk2sNHLWr2MFm6/input"
const char *root_topic_publish = "Antenna001";
const char* dispositivoID = "LEITOR_004";// id do dispositivo 
String leituraTAG = " ";
String Novaleitura = " ";
//char aux[16];
//**************************************
//*********** WIFICONFIG ***************
//**************************************
const char* ssid = "Violetas"; //suarede
const char* password = "reis151293";//suasenha


//**************************************
//*********** GLOBALES   ***************
//**************************************
WiFiClient espClient;
PubSubClient client(espClient);
char msg[100];
long count=0;


//**************************************
//*********** RFID READER CONFIG ***************
//**************************************
//used in authentication
MFRC522::MIFARE_Key key;
//authentication return status code
MFRC522::StatusCode status;
// Defined pins to module RC522
MFRC522 mfrc522(SS_PIN, RST_PIN);


//************************
//** F U N C I O N E S ***
//************************
void callback(char* topic, byte* payload, unsigned int length);
void reconnect();
void setup_wifi();

//**************************************
//*********** CREDITS ***************
//**************************************
//credits - We got the RFID code from: https://www.instructables.com/ESP32-With-RFID-Access-Control/
//https://github.com/miguelbalboa/rfid/blob/master/examples/ReadAndWrite/ReadAndWrite.ino
// WE LEARNED HOW TO USE MQTT IN IOTICOS.ORG - COM VIDEOS NO YOUTUBE



 //**************************************
 //**************************************
//*********** DUMP BYTE ARRAY**************
//**************************************
//**************************************
void dump_byte_array(byte *buffer, byte bufferSize) {
    for (byte i = 0; i < bufferSize; i++) {
        Serial.print(buffer[i] < 0x10 ? " 0" : " ");
        Serial.print(buffer[i], HEX);
    }
}

//**************************************
//*********** SETUP ***************
//**************************************
void setup() {
  // put your setup code here, to run once:

  Serial.begin(9600);
  SPI.begin(); // Init SPI bus
  pinMode(greenPin, OUTPUT);
  pinMode(redPin, OUTPUT);


  setup_wifi();
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);
//seting up the wireless connection 
  
  reconnect();

  // Init MFRC522
  mfrc522.PCD_Init(); 

  // Prepare the key (used both as key A and as key B)
    // using FFFFFFFFFFFFh which is the default at chip delivery from the factory
    for (byte i = 0; i < 6; i++) {
        key.keyByte[i] = 0xFF;
    }

 Serial.println();
Serial.print(F("Using key (for A and B):"));
    dump_byte_array(key.keyByte, MFRC522::MF_KEY_SIZE);
  Serial.println();
  Serial.println("Approach your card on the reader...");
  Serial.println();
}





//**************************************
//*********** LOOP ARDUINO ***************
//**************************************
void loop() {
  

if (WiFi.status() != WL_CONNECTED){
  Serial.println("connection lost");
  delay(1000);
  setup_wifi();
}
   if (!client.connected()) {
   reconnect();
  }
  client.loop();
  if ( ! mfrc522.PICC_IsNewCardPresent()) 
  {
    return;
  }
  // Select a card
  if ( ! mfrc522.PICC_ReadCardSerial()) 
  {
    return;
  }

//check if connected, reads and send data to the broker
 if (client.connected()){
    readingData();
    String str = "{\"tag_id\":\"" + leituraTAG + "\",\"sensor_id\":\"" + dispositivoID + "\"}";
    str.toCharArray(msg,100);
    client.publish(root_topic_publish,msg);
    delay(300);
  }
  
  
  
  //call menu function and retrieve the desired option
 /* int op = menu();

 if(op == 0)
      readingData();
  else if(op == 1) 
      writingData();
  else {
    Serial.println(F("Incorrect Option!"));
    return;
  }
*/

  //instructs the PICC when in the ACTIVE state to go to a "STOP" state
  mfrc522.PICC_HaltA(); 
  // "stop" the encryption of the PCD, it must be called after communication with authentication, otherwise new communications can not be initiated
  mfrc522.PCD_StopCrypto1(); 

  //client.loop();
}




//*****************************
//***    CONEXION WIFI      ***
//*****************************
void setup_wifi(){
  delay(10);
  // Nos conectamos a nuestra red Wifi
  Serial.println();
  Serial.print("Conectando a ssid: ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("Conectado a red WiFi!");
  Serial.println("Dirección IP: ");
  Serial.println(WiFi.localIP());
}


//*****************************
//***    CONEXION MQTT      ***
//*****************************

void reconnect() {

  while (!client.connected()) {
    Serial.print("Intentando conexión Mqtt...");
    // Creamos un cliente ID
    String clientId = dispositivoID;
   // clientId += String(random(0xffff), HEX);
    // Intentamos conectar
    //if (client.connect(clientId.c_str(),mqtt_user,mqtt_pass))
    if (client.connect(clientId.c_str())) {
      Serial.println("Conectado!");
      // Nos suscribimos
      if(client.subscribe(root_topic_subscribe)){
        Serial.println("Suscripcion ok");
      }else{
        Serial.println("fallo Suscripciión");
      }
    } else {
      Serial.print("falló :( con error -> ");
      Serial.print(client.state());
      Serial.println(" Intentamos de nuevo en 5 segundos");
      delay(5000);
    }
  }
}

//**************************************
//**************************************
//*********** READ DATA FROM CARD OR TAG **************
//**************************************
//**************************************


void readingData()
{
  
  //prepare the key - all keys are set to FFFFFFFFFFFFh
  for (byte i = 0; i < 6; i++) key.keyByte[i] = 0xFF;
  
  //buffer for read data
  //byte buffer[SIZE_BUFFER] = {0};
 byte buffer[SIZE_BUFFER];
  //the block to operate
  byte block = 1;
 // byte size = SIZE_BUFFER; 
  byte size = sizeof(buffer);
  
  //authenticates the block to operate
  status = (MFRC522::StatusCode) mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, block, &key, &(mfrc522.uid)); //line 834 of MFRC522.cpp file
  if (status != MFRC522::STATUS_OK) {
    Serial.print(F("Authentication failed: "));
    Serial.println(mfrc522.GetStatusCodeName(status));
    digitalWrite(redPin, HIGH);
    delay(1000);
    digitalWrite(redPin, LOW);
    return;
  }
//read data from block
 status = (MFRC522::StatusCode) mfrc522.MIFARE_Read(block, buffer, &size);
  //status = (MFRC522::StatusCode) mfrc522.MIFARE_Read(block, aux, &size);
  if(status != MFRC522::STATUS_OK)
  {
    Serial.print(F("Reading failed: "));
    Serial.println(mfrc522.GetStatusCodeName(status));
    digitalWrite(redPin, HIGH);
    delay(2000);
    digitalWrite(redPin, LOW);
    return;
  }
  else{
      digitalWrite(greenPin, HIGH);
      delay(2000);
      digitalWrite(greenPin, LOW);
  }

  Serial.print(F("\nData from block ["));
  Serial.print(block);
  Serial.print(F("]: "));

 //prints read data
 String myString = String((char *)buffer);
  for (uint8_t i = 0; i < MAX_SIZE_BLOCK; i++)
  {
       Serial.write(buffer[i]);
      
  }
  Serial.println();
  Serial.println(" leitura da tag ");
  Novaleitura = myString.substring(1,15);
///inString = inString.replace("\n","");
delay(100);
 leituraTAG = Novaleitura.substring(0,Novaleitura.length()- (Novaleitura.length()-Novaleitura.indexOf(" ")));
  Serial.println(leituraTAG);
}


//**************************************
//**************************************
//*********** WRITE DATA CON CARD OR TAG **************
//**************************************
//**************************************


void writingData()
{

  // waits 30 seconds dor data entry via Serial 
  Serial.setTimeout(30000L) ;     
  Serial.println(F("Enter the data to be written with the '#' character at the end \n[maximum of 16 characters]:"));

  //prepare the key - all keys are set to FFFFFFFFFFFFh
  for (byte i = 0; i < 6; i++) key.keyByte[i] = 0xFF;

  //buffer para armazenamento dos dados que iremos gravar
  //buffer for storing data to write
  byte buffer[MAX_SIZE_BLOCK] = "";
  byte block = 1; //the block to operate
  byte dataSize; //size of data (bytes)

  //recover on buffer the data from Serial
  //all characters before chacactere '#'
  dataSize = Serial.readBytesUntil('#', (char*)buffer, MAX_SIZE_BLOCK);
  //void positions that are left in the buffer will be filled with whitespace
  for(byte i=dataSize; i < MAX_SIZE_BLOCK; i++)
  {
    buffer[i] = ' ';
  }
 
   //authenticates the block to operate
  status = (MFRC522::StatusCode) mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, block, &key, &(mfrc522.uid));

//for (byte i=size
  if (status != MFRC522::STATUS_OK) {
    Serial.print(F("PCD_Authenticate() failed: "));
    Serial.println(mfrc522.GetStatusCodeName(status));
    digitalWrite(redPin, HIGH);
    delay(1000);
    digitalWrite(redPin, LOW);
    return;
  }
  
  //Writes in the block
  status = mfrc522.MIFARE_Write(block, buffer, MAX_SIZE_BLOCK);
  if (status != MFRC522::STATUS_OK) {
    Serial.print(F("MIFARE_Write() failed: "));
    Serial.println(mfrc522.GetStatusCodeName(status));
    digitalWrite(redPin, HIGH);
    delay(2000);
    digitalWrite(redPin, LOW);
    return;
  }
  else{
    Serial.println(F("MIFARE_Write() success: "));
    digitalWrite(greenPin, HIGH);
    delay(2000);
    digitalWrite(greenPin, LOW);
  }
 
}

//**************************************
//**************************************
//*********** MENU TO OPERATION CHOICE **************
//**************************************
//**************************************

int menu()
{
  Serial.println(F("\nChoose an option:"));
  Serial.println(F("0 - Reading data"));
  Serial.println(F("1 - Writing data\n"));

  //waits while the user does not start data
  while(!Serial.available())
  {
      Serial.println(F("waiting\n"));
    };
  
  //retrieves the chosen option
  int op = (int)Serial.read();
  int zeroasciitable = 48; 
  //remove all characters after option (as \n per example)
  while(Serial.available()) 
  {
    if(Serial.read() == '\n')
    {
      break; 
    }
    else{
      Serial.read();
    }
    
  }
  return (op - zeroasciitable);//subtract 48 from read value, 48 is the zero from ascii table
}

//*****************************
//***       CALLBACK        ***
//*****************************

void callback(char* topic, byte* payload, unsigned int length){
  String incoming = "";
  Serial.print("Mensaje recibido desde -> ");
  Serial.print(topic);
  Serial.println("");
  for (int i = 0; i < length; i++) {
    incoming += (char)payload[i];
  }
  incoming.trim();
  Serial.println("Mensaje -> " + incoming);

}
