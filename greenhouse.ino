// Template ID, Device Name and Auth Token are provided by the Blynk.Cloud
// See the Device Info tab, or Template settings
#define BLYNK_TEMPLATE_ID "xxx"
#define BLYNK_DEVICE_NAME "xxx"
#define BLYNK_AUTH_TOKEN "xxx"


// Comment this out to disable prints and save space
#define BLYNK_PRINT Serial
#define ASCIUTTO 530
#define BAGNATO 450

#include <ESP8266_Lib.h>
#include <BlynkSimpleShieldEsp8266.h>
#include "TimeLib.h"

char auth[] = BLYNK_AUTH_TOKEN;

// Your WiFi credentials.
// Set password to "" for open networks.
char ssid[] = "xxx";
char pass[] = "xxx";


ESP8266 wifi(&Serial);

//DHT11 Sensor:
#include "DHT.h"
#define DHTPIN 12     // Sensore collegato al PIN 12
#define DHTTYPE DHT22   // Sensore DHT 11
DHT dht(DHTPIN, DHTTYPE);

//I2C LCD:
#include <LiquidCrystal_I2C.h> // Libreria LCD I2C
LiquidCrystal_I2C lcd(0x27, 20, 4);// LCD I2C address

char *res = malloc(5);

//        VALORI DA SETTARE DA SMARTPHONE

int att_ventola = 30;  //Valore temperatura attivazione ventola
int att_irrigazione = 75; //Valore dell'igrometro al quale il relay sarà ON
int att_luce = 540;  //Valore lumen attivazione luce
int att_riscaldatore = 12;  //valore di attivazione riscaldamento

// PARAMETRI DA SETTARE A PIACIMENTO  
int valore_limite_sensore_terra = 1;

//        VARIABILI DI ESECUZIONE
int irriga = 0;
int apertura = 0;

//        VALORI LCD

int temp = 0; //temp
int umdtrr = 0; //umidità terra
int h = 0;  //umidità
//char buf[15];

WidgetLED led_riscaldamento(V2);

BLYNK_CONNECTED() {
  Serial.println("Connesso");
  Blynk.sendInternal("rtc", "sync");
}

BLYNK_WRITE(InternalPinRTC) {
  setTime(param.asLong());
}

//controllo ventola
BLYNK_WRITE(V0){  
  att_ventola = param.asInt();
}

//controllo irrigazione
BLYNK_WRITE(V1){  
  att_irrigazione = param.asInt();
}

//controllo riscaldatore
BLYNK_WRITE(V7){  
  att_riscaldatore = param.asInt();
}

//controllo pistone
BLYNK_WRITE(V10){  
  apertura = param.asInt();
    
}

//controllo irrigazione led
BLYNK_WRITE(V6){  
  irriga = param.asInt();
    
}

void repeatMe(int secondi) {

//    sprintf(buf, "%02d/%02d %02d:%02d", day(), month(), hour(), minute());
    
    if (apertura == 1) {
      digitalWrite(5, HIGH); // motore up
    } else {
      digitalWrite(5, LOW); // motore down
    }
  
    // Scrittura valori su app blynk
    Blynk.virtualWrite(V3, temp);
    Blynk.virtualWrite(V4, umdtrr);
    Blynk.virtualWrite(V5, h);
    Blynk.virtualWrite(V8, hour());
    readSoil();

        
    // irrigazione parte se umidità sotto soglia e non in pausa per aver appena irrigato
    if (irriga == 1) { 
      digitalWrite(9, LOW); // Attiva irrigazione
      Serial.println("STO IRRIGANDO");
    }
    else {
      digitalWrite(9, HIGH); //disattiva irrigazione
    }

//                                                            LUCE

    int Lumen = analogRead (A0); // Lumen come intero della lettura del pin A0
    // orario attivazione 07,00 disattivazione 19
    if (Lumen > att_luce && (hour() > 7 && hour() < 19)){
      digitalWrite (11, LOW); // Se il valore di Lumen è superiore a 750 attiva relè
    }
    else {
      digitalWrite (11, HIGH); // altrimenti spegne relè
    }
  
    // Lettura umidità e temperatura fatta dal sensore sensore DHT11
    h = dht.readHumidity();
    temp = dht.readTemperature();
    
    //                                                            VENTOLA
  
    // Misura la temperatura e se maggiore di 27 gradi attiva relè per accendere la ventola
    if (temp >= att_ventola)
      digitalWrite (10, LOW); // Attiva ventola
    else
      digitalWrite (10, HIGH); // Spegni ventola
  
    //                                                         RISCALDAMENTO
  
    // Se temperatura inferiore a 10 gradi accende il riscaldamento
    if (temp <= att_riscaldatore ){
      digitalWrite (7, LOW); // Attiva riscaldatore
      led_riscaldamento.on();
    }
    else if (temp >= att_riscaldatore + 2){
      digitalWrite (7, HIGH); // Spegni riscaldatore
      led_riscaldamento.off();
    }
    
    // impostare cursore sulla prima riga:
    lcd.setCursor(0, 0);
    if (hour() > 6 && hour() < 19) {
      lcd.print("Buongiorno!");
    }
    else {
      lcd.print("Buonasera!");
    }
    lcd.setCursor(0, 1);
    lcd.print("Temperatura :  ");
    lcd.print(temp);
    lcd.print("C");
  
    lcd.setCursor(0, 2);
    lcd.print("Umidita'aria : ");
    lcd.print(h);
    lcd.print("%");
  
    // imposta il cursore sulla quarta riga:
    lcd.setCursor(0, 3);
    lcd.print("Umidita'terra: ");
    lcd.print(umdtrr);
    lcd.print("% ");
}

void setup()
{
  // Debug console
  Serial.begin(9600);
  delay(10);

  pinMode(13, OUTPUT);//alimentazione dht11
  // PIN 6 al LED serbatoio acqua livello basso

  // pin gestione apertura
  pinMode(5, OUTPUT);
  
  pinMode(6, OUTPUT);

  //PIN 7 al relè 4 cavetto riscaldatore
  pinMode(7, OUTPUT);

  // PIN 9 al Relè 1 pompa acqua irrigazione
  pinMode(9, OUTPUT);

  // PIN 10 al relè 2 ventolina estrazione aria calda
  pinMode(10, OUTPUT);

  // PIN 11 al relè 3 stringa led illuminazione
  pinMode(11, OUTPUT);

  // PIN 8 PER CONTROLLO SENSORE IGROMETRO
  pinMode(8, OUTPUT);
  digitalWrite(8, HIGH);


  //I2C LCD
  lcd.begin(20, 4);


  // Inizializza sensore DHT11
  dht.begin();
  lcd.init ();
  lcd.clear ();
  lcd.backlight();
  delay(200);

  readSoil();

  Blynk.begin(auth, wifi, ssid, pass);

}

String pad(int n) {
  sprintf(res, "%02d", n);// comando per avere lo zero iniziale su valore di una cifra es 01,02,03...
  return String(res);
}

void readSoil(){
    // Igrometro
    digitalWrite(8, HIGH);
    delay(200);
    int igro = analogRead(A3); // Legge il valore analogico dell'unidità del terreno
    //int umidity = map(igro, BAGNATO, ASCIUTTO, 100,0); // converto il valore analogico in percentuale
    //umdtrr=constrain (umdtrr,0,100);
    int umidity = map(igro, 1024, 300, 0, 100);
    umdtrr = map(umidity, 75, 125, 0, 100);
    umdtrr=constrain (umdtrr,0,100);
    digitalWrite(8, LOW);
}

void loop()
{
  if (Blynk.connected()){
    Blynk.run();

    int secondi= second();
    if (secondi == 0){
      Blynk.sendInternal("rtc", "sync");
    }
    // ogni 10 secondi si esegue la funzione repeatMe. Questa funzione esegue tutte le operazioni della serra
    if (secondi == 0 || secondi == 10 || secondi == 20 || secondi == 30 || secondi == 40 || secondi == 50) {
      repeatMe(secondi);
    }
  }
  else {
    Serial.println("Disconnesso dall'app!");
    Serial.println("tentativo di riconnessione...");
    Blynk.connect();
 }
}
