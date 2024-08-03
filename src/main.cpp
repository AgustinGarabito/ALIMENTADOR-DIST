#include <Arduino.h>
#include <LiquidCrystal_I2C.h>
#include <Servo.h>
#include "config.h"
#include <EEPROM.h>

// VARIABLES WIFI
byte cont = 0;
byte max_intentos = 50;

// LIBRERIAS WIFI, BLYNK Y NTP
#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h>
#include <NTPClient.h>
#include <WiFiUdp.h>

// CONFIGURAR SERVIDOR NTP PARA OBTENER HORA EN TIEMPO REAL
WiFiUDP udp;
NTPClient timeClient(udp, NTPSERVER, GMT_3, INTERVALO);

// LED INTEGRADO
const int ledPin = 2;

// SERVO
Servo myservo;
int vecesAlimentadoHoy;
String fechaHora;

// BUZZER
const int PIN_BUZZ = 13;
int melodia = 0; // MELODIA DEFAULT

// DISPLAY LCD
LiquidCrystal_I2C lcd(0x27,16,2);
String horaInicio;

// MOTOR VIBRACION
const int PIN_VIBRACION = 14;

// BOTONES
const int botonUp = 3;
const int botonDown = 0;
const int botonConfirm = 2;
const int botonBack = 1;

// VARIABLES MENU
int opcionSeleccionada = 0;
const int totalOpciones = 2;

enum Estado {
  MENU_PRINCIPAL,
  MENU_CRONOGRAMA,
  MENU_COMIDA,
  SELECCIONAR_DIGITO,
  AJUSTAR_HORA
};

// ESTADO ACTUAL Y TECLA
Estado estadoActual = MENU_PRINCIPAL;

// VARIABLES CRONOGRAMA
String horaDia = "10:00:00";
String horaNoche = "20:00:00";
String horaS;
int digitoSeleccionado = 0;
int horario = 0;
int digito;

// VARIABLES EEPROM
const int EEPROM_SIZE = 512;
const int MAX_STRING_LENGTH = 20;
const int EEPROM_HORA_DIA_ADDR = 0;
const int EEPROM_HORA_NOCHE_ADDR = EEPROM_HORA_DIA_ADDR + MAX_STRING_LENGTH;

// FUNCIONES
void melodiaBuzzer(int melodia);
void inicializarDisplay();
void iniciarConexiones();
void dispensar();
void dispensadorAutomatico();
void melodia1();
void melodia2();
void melodia3();
void sacudirServo();
void motorVibracion(int duracion);
void botonArriba();
void botonAbajo();
void botonConfirmar();
void botonAtras();
void mostrarMenuPrincipal();
void mostrarMenuComida();
void mostrarMenuCronograma();
void mostrarSeleccionarDigito();
void escribirEEPROM(int address, String data);
String leerEEPROM(int address);

// FUNCION PARA ALIMENTAR MEDIANTE EL PIN VIRTUAL V1
BLYNK_WRITE(V1) {
  // LEE EL PARAMETRO
  int value = param.asInt();
  if (value && vecesAlimentadoHoy < 2) {
    dispensar();
  } 
}

// FUNCION PARA CAMBIAR MELODIA V2
BLYNK_WRITE(V2) {
  // LEE EL PARAMETRO
  int value = param.asInt();
  melodia = value;
}

// RESPALDO VECES ALIMENTADO EN CASO DE CORTE DE ENERGIA
BLYNK_WRITE(V3) {
  // LEE EL PARAMETRO
  vecesAlimentadoHoy = param.asInt();
}

// RESPALDO ULTIMA COMIDA
BLYNK_WRITE(V4) {
  // LEE EL PARAMETRO
  fechaHora = param.asString();
}

// AUMENTO / DECREMENTO DE ALIMENTO MANUAL
BLYNK_WRITE(V5) {
  // LEE EL PARAMETRO Y LO ACTUALIZA (LA VARIABLE Y EN LA APP)
  vecesAlimentadoHoy = param.asInt();
  Blynk.virtualWrite(V3,vecesAlimentadoHoy);
}

// RECUPERACION DESDE BLYNK (VECES ALIMENTADO Y MELODIA)
BLYNK_CONNECTED(){
  Blynk.syncVirtual(V3);
  Blynk.syncVirtual(V2);
  Blynk.syncVirtual(V5);
  Blynk.syncVirtual(V4);
}

void setup() {
  // DEFINICION DE PINES
  pinMode(PIN_BUZZ, OUTPUT);
  pinMode(ledPin, OUTPUT);
  
  // EEPROM (SOLO HACER UNA VEZ, DESPUES COMENTAR)
  EEPROM.begin(EEPROM_SIZE);
  //writeStringToEEPROM(EEPROM_HORA_DIA_ADDR, horaDia);
  //writeStringToEEPROM(EEPROM_HORA_NOCHE_ADDR, horaNoche);

  // SERVO
  myservo.attach(15);
  myservo.write(20); 

  // SERIAL
  Serial.begin(115200);

  // DISPLAY
  lcd.init();
  lcd.backlight();
  lcd.clear();
  lcd.setCursor(2, 0);
  lcd.print("Iniciando...");

  // MOTOR VIBRACION
  pinMode(PIN_VIBRACION, OUTPUT);

  // BOTONES
  pinMode(botonUp, INPUT);
  pinMode(botonDown, INPUT);
  pinMode(botonConfirm, INPUT);
  pinMode(botonBack, INPUT);

  // CONEXIONES
  iniciarConexiones();
}

void loop() {
  // MANEJAR RECONEXION WIFI
  if (WiFi.status() != WL_CONNECTED) {
    // MUESTRA POR PANTALLA
    lcd.clear();
    Serial.println("WiFi desconectado. Intentando reconectar...");
    lcd.setCursor(1, 0);
    lcd.print("Se perdio la");
    lcd.setCursor(3, 1);
    lcd.print("conexion");

    // INTENTA RECONECTAR
    iniciarConexiones();
  }

  // SI HAY CONEXION QUE TRABAJE
  if(WiFi.status() == WL_CONNECTED){
    // MANEJO DE BOTONES / MENU
    botonArriba();
    botonAbajo();
    botonConfirmar();
    botonAtras();

    // PRENDER LED INTEGRADO
    digitalWrite(ledPin, LOW);

    // PRENDER A BLYNK
    Blynk.run();

    // ACTUALIZAR HORA
    timeClient.update();

    // DISPENSA AUTOMATICAMENTE A LAS 10AM Y A LAS 20PM
    dispensadorAutomatico();

    // REINICIA EL CONTADOR AL CAMBIAR DE DIA Y ACTUALIZARLO EN LA APP
    if(timeClient.getFormattedTime() == "00:00:00"){
      vecesAlimentadoHoy = 0;
      Blynk.virtualWrite(V3,vecesAlimentadoHoy);
      Blynk.virtualWrite(V5,vecesAlimentadoHoy);
    }
  }
}

// CONEXION WIFI Y BLYNK
void iniciarConexiones(){
  // CONEXION WIFI
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  cont = 0;
  while (WiFi.status() != WL_CONNECTED && cont < max_intentos) { // ESPERA CONEXION CON UN MAXIMO DE INTENTOS
    cont++;
    delay(500);
    Serial.print(".");
  }

  Serial.println("");

  if (cont < max_intentos) {  // SI SE CONECTA IMPRIME INFORMACION   
    Serial.println("********************************************");
    Serial.print("Conectado a la red WiFi: ");
    Serial.println(WiFi.SSID());
    Serial.print("IP: ");
    Serial.println(WiFi.localIP());
    Serial.print("macAdress: ");
    Serial.println(WiFi.macAddress());
    Serial.println("*********************************************");

    // MUESTRA POR PANTALLA QUE CONECTO
    lcd.clear();
    lcd.setCursor(2, 0);
    lcd.print("Conexion");
    lcd.setCursor(2, 1);
    lcd.print("exitosa!");
    cont = 0;

    // HORA ACTUAL
    timeClient.begin();
    timeClient.update();
    
    // INICIA BLYNK
    Blynk.begin(BLYNK_AUTH_TOKEN, WIFI_SSID, WIFI_PASSWORD);
    delay(1000);

    // PRENDER A BLYNK
    Blynk.run();
    delay(5000);

    // INICIALIZAR DISPLAY
    mostrarMenuPrincipal();
    //inicializarDisplay();

    // VERIFICAR SI NO SE PERDIO UNA COMIDA
    if(timeClient.getFormattedTime() > leerEEPROM(EEPROM_HORA_DIA_ADDR) && timeClient.getFormattedTime() <= "14:00:00" && vecesAlimentadoHoy == 0){
      dispensar();
    }
    if(timeClient.getFormattedTime() > leerEEPROM(EEPROM_HORA_NOCHE_ADDR) && timeClient.getFormattedTime() <= "23:00:00" && vecesAlimentadoHoy == 1){
      dispensar();
    }

    // VERIFICAR SI NO SE PERDIO EL REINICIO
    if(timeClient.getFormattedTime() > "00:00:00" && timeClient.getFormattedTime() <= "05:00:00" && vecesAlimentadoHoy > 0){
      vecesAlimentadoHoy = 0;
      Blynk.virtualWrite(V3,vecesAlimentadoHoy);
      Blynk.virtualWrite(V5,vecesAlimentadoHoy);
    }
  }
  else { // SI NO SE CONECTA MUESTRA UN ERROR
    Serial.println("------------------------------------");
    Serial.println("Error de conexion");
    Serial.println("------------------------------------");

    // MUESTRA POR PANTALLA EL ERROR
    lcd.clear();
    lcd.setCursor(3, 0);
    lcd.print("Error al");
    lcd.setCursor(3, 1);
    lcd.print("conectar!");

    // ESPERA 30 SEG ANTES DE VOLVER A INTENTAR
    delay(30000);
    ESP.restart();
  }
}

// SELECTOR DE MELODIAS
void melodiaBuzzer(int melodia){
  if (melodia == 0){ // PIRATAS DEl CARIBE
    melodia1();
  } else if (melodia == 1){ // STAR WARS
    melodia2();
  } else if (melodia == 2){ // HARRY POTTER
    melodia3();
  }
}

// PIRATAS DEL CARIBE
void melodia1(){
  tone(PIN_BUZZ, 330, (1000 / 8)); // E4
  delay((1000 / 8) * 1.3);
  tone(PIN_BUZZ, 392, (1000 / 8)); // G4
  delay((1000 / 8) * 1.3);
  tone(PIN_BUZZ, 440, (1000 / 4)); // A4
  delay((1000 / 4) * 1.3);
  tone(PIN_BUZZ, 440, (1000 / 8)); // A4
  delay((1000 / 8) * 1.3);
  tone(PIN_BUZZ, 0, (1000 / 8)); // REST
  delay((1000 / 8) * 1.3);

  tone(PIN_BUZZ, 440, (1000 / 8)); // A4
  delay((1000 / 8) * 1.3);
  tone(PIN_BUZZ, 494, (1000 / 8)); // B4
  delay((1000 / 8) * 1.3);
  tone(PIN_BUZZ, 523, (1000 / 4)); // C5
  delay((1000 / 4) * 1.3);
  tone(PIN_BUZZ, 523, (1000 / 8)); // C5
  delay((1000 / 8) * 1.3);
  tone(PIN_BUZZ, 0, (1000 / 8)); // REST
  delay((1000 / 8) * 1.3);

  tone(PIN_BUZZ, 523, (1000 / 8)); // C5
  delay((1000 / 8) * 1.3);
  tone(PIN_BUZZ, 587, (1000 / 8)); // D5
  delay((1000 / 8) * 1.3);
  tone(PIN_BUZZ, 494, (1000 / 4)); // B4
  delay((1000 / 4) * 1.3);
  tone(PIN_BUZZ, 494, (1000 / 8)); // B4
  delay((1000 / 8) * 1.3);
  tone(PIN_BUZZ, 0, (1000 / 8)); // REST
  delay((1000 / 8) * 1.3);

  tone(PIN_BUZZ, 440, (1000 / 8)); // A4
  delay((1000 / 8) * 1.3);
  tone(PIN_BUZZ, 392, (1000 / 8)); // G4
  delay((1000 / 8) * 1.3);
  tone(PIN_BUZZ, 440, (1000 / 4)); // A4
  delay((1000 / 4) * 1.3);
  tone(PIN_BUZZ, 0, (1000 / 8)); // REST
  delay((1000 / 8) * 1.3);

  //

  tone(PIN_BUZZ, 330, (1000 / 8)); // E4
  delay((1000 / 8) * 1.3);
  tone(PIN_BUZZ, 392, (1000 / 8)); // G4
  delay((1000 / 8) * 1.3);
  tone(PIN_BUZZ, 440, (1000 / 4)); // A4
  delay((1000 / 4) * 1.3);
  tone(PIN_BUZZ, 440, (1000 / 8)); // A4
  delay((1000 / 8) * 1.3);
  tone(PIN_BUZZ, 0, (1000 / 8)); // REST
  delay((1000 / 8) * 1.3);

  tone(PIN_BUZZ, 440, (1000 / 8)); // A4
  delay((1000 / 8) * 1.3);
  tone(PIN_BUZZ, 494, (1000 / 8)); // B4
  delay((1000 / 8) * 1.3);
  tone(PIN_BUZZ, 523, (1000 / 4)); // C5
  delay((1000 / 4) * 1.3);
  tone(PIN_BUZZ, 523, (1000 / 8)); // C5
  delay((1000 / 8) * 1.3);
  tone(PIN_BUZZ, 0, (1000 / 8)); // REST
  delay((1000 / 8) * 1.3);

  tone(PIN_BUZZ, 523, (1000 / 8)); // C5
  delay((1000 / 8) * 1.3);
  tone(PIN_BUZZ, 587, (1000 / 8)); // D5
  delay((1000 / 8) * 1.3);
  tone(PIN_BUZZ, 494, (1000 / 4)); // B4
  delay((1000 / 4) * 1.3);
  tone(PIN_BUZZ, 494, (1000 / 8)); // B4
  delay((1000 / 8) * 1.3);
  tone(PIN_BUZZ, 0, (1000 / 8)); // REST
  delay((1000 / 8) * 1.3);

  tone(PIN_BUZZ, 440, (1000 / 8)); // A4
  delay((1000 / 8) * 1.3);
  tone(PIN_BUZZ, 392, (1000 / 8)); // G4
  delay((1000 / 8) * 1.3);
  tone(PIN_BUZZ, 440, (1000 / 4)); // A4
  delay((1000 / 4) * 1.3);
  tone(PIN_BUZZ, 0, (1000 / 8)); // REST
  delay((1000 / 8) * 1.3);

  //

  tone(PIN_BUZZ, 330, (1000 / 8)); // E4
  delay((1000 / 8) * 1.3);
  tone(PIN_BUZZ, 392, (1000 / 8)); // G4
  delay((1000 / 8) * 1.3);
  tone(PIN_BUZZ, 440, (1000 / 4)); // A4
  delay((1000 / 4) * 1.3);
  tone(PIN_BUZZ, 440, (1000 / 8)); // A4
  delay((1000 / 8) * 1.3);
  tone(PIN_BUZZ, 0, (1000 / 8)); // REST
  delay((1000 / 8) * 1.3);

  tone(PIN_BUZZ, 440, (1000 / 8)); // A4
  delay((1000 / 8) * 1.3);
  tone(PIN_BUZZ, 523, (1000 / 8)); // C5
  delay((1000 / 8) * 1.3);
  tone(PIN_BUZZ, 587, (1000 / 4)); // D5
  delay((1000 / 4) * 1.3);
  tone(PIN_BUZZ, 587, (1000 / 8)); // D5
  delay((1000 / 8) * 1.3);
  tone(PIN_BUZZ, 0, (1000 / 8)); // REST
  delay((1000 / 8) * 1.3);

  tone(PIN_BUZZ, 587, (1000 / 8)); // D5
  delay((1000 / 8) * 1.3);
  tone(PIN_BUZZ, 659, (1000 / 8)); // E5
  delay((1000 / 8) * 1.3);
  tone(PIN_BUZZ, 698, (1000 / 4)); // F5
  delay((1000 / 4) * 1.3);
  tone(PIN_BUZZ, 698, (1000 / 8)); // F5
  delay((1000 / 8) * 1.3);
  tone(PIN_BUZZ, 0, (1000 / 8)); // REST
  delay((1000 / 8) * 1.3);

  tone(PIN_BUZZ, 659, (1000 / 8)); // E5
  delay((1000 / 8) * 1.3);
  tone(PIN_BUZZ, 587, (1000 / 8)); // D5
  delay((1000 / 8) * 1.3);
  tone(PIN_BUZZ, 659, (1000 / 8)); // E5
  delay((1000 / 8) * 1.3);
  tone(PIN_BUZZ, 440, (1000 / 4)); // A4
  delay((1000 / 4) * 1.3);
  tone(PIN_BUZZ, 0, (1000 / 8)); // REST
  delay((1000 / 8) * 1.3);

  //

  tone(PIN_BUZZ, 440, (1000 / 8)); // A4
  delay((1000 / 8) * 1.3);
  tone(PIN_BUZZ, 494, (1000 / 8)); // B4
  delay((1000 / 8) * 1.3);
  tone(PIN_BUZZ, 523, (1000 / 4)); // C5
  delay((1000 / 4) * 1.3);
  tone(PIN_BUZZ, 523, (1000 / 8)); // C5
  delay((1000 / 8) * 1.3);
  tone(PIN_BUZZ, 0, (1000 / 8)); // REST
  delay((1000 / 8) * 1.3);

  tone(PIN_BUZZ, 587, (1000 / 4)); // D5
  delay((1000 / 4) * 1.3);
  tone(PIN_BUZZ, 659, (1000 / 8)); // E5
  delay((1000 / 8) * 1.3);
  tone(PIN_BUZZ, 440, (1000 / 4)); // A4
  delay((1000 / 4) * 1.3);
  tone(PIN_BUZZ, 0, (1000 / 8)); // REST
  delay((1000 / 8) * 1.3);

  tone(PIN_BUZZ, 440, (1000 / 8)); // A4
  delay((1000 / 8) * 1.3);
  tone(PIN_BUZZ, 523, (1000 / 8)); // C5
  delay((1000 / 8) * 1.3);
  tone(PIN_BUZZ, 494, (1000 / 4)); // B4
  delay((1000 / 4) * 1.3);
  tone(PIN_BUZZ, 494, (1000 / 8)); // B4
  delay((1000 / 8) * 1.3);
  tone(PIN_BUZZ, 0, (1000 / 8)); // REST
  delay((1000 / 8) * 1.3);

  tone(PIN_BUZZ, 523, (1000 / 8)); // C5
  delay((1000 / 8) * 1.3);
  tone(PIN_BUZZ, 440, (1000 / 8)); // A4
  delay((1000 / 8) * 1.3);
  tone(PIN_BUZZ, 494, (1000 / 4)); // B4
  delay((1000 / 4) * 1.3);
  tone(PIN_BUZZ, 0, (1000 / 4)); // REST
  delay((1000 / 4) * 1.3);
}

// STAR WARS
void melodia2(){
  tone(PIN_BUZZ, 466, (1000 / 8)); // AS4
  delay((1000 / 8) * 1.3);
  tone(PIN_BUZZ, 466, (1000 / 8)); // AS4
  delay((1000 / 8) * 1.3);
  tone(PIN_BUZZ, 466, (1000 / 8)); // AS4
  delay((1000 / 8) * 1.3);

  tone(PIN_BUZZ, 698, (1000 / 2)); // F5
  delay((1000 / 2) * 1.3);
  tone(PIN_BUZZ, 1047, (1000 / 2)); // C6
  delay((1000 / 2) * 1.3);

  tone(PIN_BUZZ, 932, (1000 / 8)); // AS5
  delay((1000 / 8) * 1.3);
  tone(PIN_BUZZ, 880, (1000 / 8)); // A5
  delay((1000 / 8) * 1.3);
  tone(PIN_BUZZ, 784, (1000 / 8)); // G5
  delay((1000 / 8) * 1.3);
  tone(PIN_BUZZ, 1397, (1000 / 2)); // F6
  delay((1000 / 2) * 1.3);
  tone(PIN_BUZZ, 1047, (1000 / 4)); // C6
  delay((1000 / 4) * 1.3);

  tone(PIN_BUZZ, 932, (1000 / 8)); // AS5
  delay((1000 / 8) * 1.3);
  tone(PIN_BUZZ, 880, (1000 / 8)); // A5
  delay((1000 / 8) * 1.3);
  tone(PIN_BUZZ, 784, (1000 / 8)); // G5
  delay((1000 / 8) * 1.3);
  tone(PIN_BUZZ, 1397, (1000 / 2)); // F6
  delay((1000 / 2) * 1.3);
  tone(PIN_BUZZ, 1047, (1000 / 4)); // C6
  delay((1000 / 4) * 1.3);

  tone(PIN_BUZZ, 932, (1000 / 8)); // AS5
  delay((1000 / 8) * 1.3);
  tone(PIN_BUZZ, 880, (1000 / 8)); // A5
  delay((1000 / 8) * 1.3);
  tone(PIN_BUZZ, 932, (1000 / 8)); // AS5
  delay((1000 / 8) * 1.3);
  tone(PIN_BUZZ, 784, (1000 / 2)); // G5
  delay((1000 / 2) * 1.3);
  tone(PIN_BUZZ, 523, (1000 / 8)); // C5
  delay((1000 / 8) * 1.3);
  tone(PIN_BUZZ, 523, (1000 / 8)); // C5
  delay((1000 / 8) * 1.3);
  tone(PIN_BUZZ, 523, (1000 / 8)); // C5
  delay((1000 / 8) * 1.3);
  
  tone(PIN_BUZZ, 698, (1000 / 2)); // F5
  delay((1000 / 2) * 1.3);
  tone(PIN_BUZZ, 1047, (1000 / 2)); // C6
  delay((1000 / 2) * 1.3);

  tone(PIN_BUZZ, 932, (1000 / 8)); // AS5
  delay((1000 / 8) * 1.3);
  tone(PIN_BUZZ, 880, (1000 / 8)); // A5
  delay((1000 / 8) * 1.3);
  tone(PIN_BUZZ, 784, (1000 / 8)); // G5
  delay((1000 / 8) * 1.3);
  tone(PIN_BUZZ, 1397, (1000 / 2)); // F6
  delay((1000 / 2) * 1.3);
  tone(PIN_BUZZ, 1047, (1000 / 4)); // C6
  delay((1000 / 4) * 1.3);

  tone(PIN_BUZZ, 932, (1000 / 8)); // AS5
  delay((1000 / 8) * 1.3);
  tone(PIN_BUZZ, 880, (1000 / 8)); // A5
  delay((1000 / 8) * 1.3);
  tone(PIN_BUZZ, 784, (1000 / 8)); // G5
  delay((1000 / 8) * 1.3);
  tone(PIN_BUZZ, 1397, (1000 / 2)); // F6
  delay((1000 / 2) * 1.3);
  tone(PIN_BUZZ, 1047, (1000 / 4)); // C6
  delay((1000 / 4) * 1.3);

  tone(PIN_BUZZ, 932, (1000 / 8)); // AS5
  delay((1000 / 8) * 1.3);
  tone(PIN_BUZZ, 880, (1000 / 8)); // A5
  delay((1000 / 8) * 1.3);
  tone(PIN_BUZZ, 932, (1000 / 8)); // AS5
  delay((1000 / 8) * 1.3);
  tone(PIN_BUZZ, 784, (1000 / 2)); // G5
  delay((1000 / 2) * 1.3);
}

// HARRY POTHER
void melodia3(){
  tone(PIN_BUZZ, 0, (1000 / 2)); // REST
  delay((1000 / 2) * 1.3);
  tone(PIN_BUZZ, 294, (1000 / 4)); // D4
  delay((1000 / 4) * 1.3);

  tone(PIN_BUZZ, 392, (1000 / 4)); // G4
  delay((1000 / 4) * 1.3);
  tone(PIN_BUZZ, 466, (1000 / 8)); // AS4
  delay((1000 / 8) * 1.3);
  tone(PIN_BUZZ, 440, (1000 / 4)); // A4
  delay((1000 / 4) * 1.3);

  tone(PIN_BUZZ, 392, (1000 / 2)); // G4
  delay((1000 / 2) * 1.3);
  tone(PIN_BUZZ, 587, (1000 / 4)); // D5
  delay((1000 / 4) * 1.3);

  tone(PIN_BUZZ, 523, (1000 / 2)); // C5
  delay((1000 / 2) * 1.3);

  tone(PIN_BUZZ, 440, (1000 / 2)); // A4
  delay((1000 / 2) * 1.3);

  tone(PIN_BUZZ, 392, (1000 / 4)); // G4
  delay((1000 / 4) * 1.3);
  tone(PIN_BUZZ, 466, (1000 / 8)); // AS4
  delay((1000 / 8) * 1.3);
  tone(PIN_BUZZ, 440, (1000 / 4)); // A4
  delay((1000 / 4) * 1.3);

  tone(PIN_BUZZ, 294, (1000 / 1)); // D4
  delay((1000 / 1) * 1.3);

  tone(PIN_BUZZ, 294, (1000 / 4)); // D4
  delay((1000 / 4) * 1.3);

  tone(PIN_BUZZ, 392, (1000 / 4)); // G4
  delay((1000 / 4) * 1.3);
  tone(PIN_BUZZ, 466, (1000 / 8)); // AS4
  delay((1000 / 8) * 1.3);
  tone(PIN_BUZZ, 440, (1000 / 4)); // A4
  delay((1000 / 4) * 1.3);

  tone(PIN_BUZZ, 392, (1000 / 2)); // G4
  delay((1000 / 2) * 1.3);
  tone(PIN_BUZZ, 587, (1000 / 4)); // D5
  delay((1000 / 4) * 1.3);

  tone(PIN_BUZZ, 698, (1000 / 2)); // F5
  delay((1000 / 2) * 1.3);
  tone(PIN_BUZZ, 659, (1000 / 4)); // E5
  delay((1000 / 4) * 1.3);

  tone(PIN_BUZZ, 622, (1000 / 2)); // DS5
  delay((1000 / 2) * 1.3);
  tone(PIN_BUZZ, 494, (1000 / 4)); // B4
  delay((1000 / 4) * 1.3);

  tone(PIN_BUZZ, 622, (1000 / 4)); // DS5
  delay((1000 / 4) * 1.3);
  tone(PIN_BUZZ, 587, (1000 / 8)); // D5
  delay((1000 / 8) * 1.3);
  tone(PIN_BUZZ, 554, (1000 / 4)); // CS5
  delay((1000 / 4) * 1.3);

  tone(PIN_BUZZ, 277, (1000 / 2)); // CS4
  delay((1000 / 2) * 1.3);
  tone(PIN_BUZZ, 494, (1000 / 4)); // B4
  delay((1000 / 4) * 1.3);

  tone(PIN_BUZZ, 392, (1000 / 1)); // G4
  delay((1000 / 1) * 1.3);
}

// INICIALIZAR DISPLAY
void inicializarDisplay(){
  lcd.clear();
  lcd.setCursor(1, 0);
  lcd.print("Ultima comida: ");
  lcd.setCursor(0, 1);
  lcd.print(fechaHora);
}

void dispensar(){
  // LLAMA MEDIANTE MELODIA
  melodiaBuzzer(melodia);

  sacudirServo();

  // OBETENER DIA
  timeClient.update();
  int diaSemana = timeClient.getDay();

  String diaSemanaStr = (diaSemana == 1) ? "Lunes" :
                        (diaSemana == 2) ? "Martes" :
                        (diaSemana == 3) ? "Mierc." :
                        (diaSemana == 4) ? "Jueves" :
                        (diaSemana == 5) ? "Viernes" :
                        (diaSemana == 6) ? "Sabado" :
                        "Domingo";
                        
  // GUARDA DIA/HORA Y LA ENVIA A LA APP
  fechaHora = diaSemanaStr + " " + timeClient.getFormattedTime();

  Blynk.virtualWrite(V4, fechaHora);

  // AUMENTA vecesAlimentadoHoy Y LO ACTUALIZA EN LA APP (SLIDE Y VALUE)
  vecesAlimentadoHoy++;
  Blynk.virtualWrite(V3,vecesAlimentadoHoy);
  Blynk.virtualWrite(V5,vecesAlimentadoHoy);
  Blynk.logEvent("alimentado", fechaHora);
}

// DISPENSADOR AUTOMATICO (10 AM Y 20 PM)
void dispensadorAutomatico(){
  if (((timeClient.getFormattedTime() == leerEEPROM(EEPROM_HORA_DIA_ADDR) && vecesAlimentadoHoy < 1) || ((timeClient.getFormattedTime() == leerEEPROM(EEPROM_HORA_NOCHE_ADDR)) && vecesAlimentadoHoy < 2))){
    dispensar();
  }
}

// SACUDIR SERVO
void sacudirServo() {
  // MOVIMIENTOS VAIVEN
  delay(1000);
   for (int i=0;i<5;i++){
    myservo.write(76);
    delay(100);
    myservo.write(20);
    delay(100);
  }

  // VIBRACION
  motorVibracion(3000);
  delay(3200);

  myservo.write(180); // ABRE COMPUERTA
  delay(SEGUNDOS_DISPENSADOS); // DELAY 0,400 SEG (PORCION)
  myservo.write(20);   // CIERRA COMPUERTA
  delay(200);

  // VIBRACION
  motorVibracion(3000);
  delay(3200);

  //MOVIMIENTOS VAIVEN
  for (int i=0;i<5;i++){
   myservo.write(76);
   delay(100);
   myservo.write(20);
   delay(100);
  }
}

// MOTOR VIBRACION
void motorVibracion(int duracion){
  digitalWrite(PIN_VIBRACION, HIGH);
  delay(duracion);
  digitalWrite(PIN_VIBRACION, LOW);
}

// MOSTRAR MENU PRINCIPAL
void mostrarMenuPrincipal() {
  lcd.clear();
  switch (opcionSeleccionada) {
    case 0:
      lcd.setCursor(0, 0);
      lcd.print("> Cronograma");
      lcd.setCursor(0, 1);
      lcd.print("  Ult. Comida");
      break;
    case 1:
      lcd.setCursor(0, 0);
      lcd.print("  Cronograma");
      lcd.setCursor(0, 1);
      lcd.print("> Ult. Comida");
      break;
  }
}


// MOSTRAR MENU CRONOGRAMA
void mostrarMenuCronograma(){
  lcd.clear();
  switch (opcionSeleccionada) {
    case 0:
      lcd.setCursor(0, 0);
      lcd.print("> Dia");
      lcd.setCursor(0, 1);
      lcd.print("  Noche");
      break;
    case 1:
      lcd.setCursor(0, 0);
      lcd.print("  Dia");
      lcd.setCursor(0, 1);
      lcd.print("> Noche");
      break;
  }
}

// MOSTRAR MENU SELECCIONAR DIGITO
void mostrarSeleccionarDigito(String horaS){
  lcd.clear();
  String horaActual;

  lcd.setCursor(0, 0);
	horaActual = horaS.substring(0,5);
  lcd.print(horaActual);
  if(digitoSeleccionado > 1){
   	lcd.setCursor(digitoSeleccionado + 1, 1);
  } else {
   	lcd.setCursor(digitoSeleccionado, 1);
  }
  lcd.print("^");
}

// MOSTRAR MENU COMIDA
void mostrarMenuComida(){
  lcd.clear();
  lcd.setCursor(1, 0);
  lcd.print("Ultima comida: ");
  lcd.setCursor(0, 1);
  lcd.print(fechaHora);
}

// BOTON ARRIBA
void botonArriba(){
  // BOTON UP
  if (digitalRead(botonUp) == LOW) {
    delay(200);
	switch(estadoActual){
    case MENU_PRINCIPAL:
	   	if (opcionSeleccionada > 0) {
       	opcionSeleccionada--;
    	}
    	mostrarMenuPrincipal();
	  break;
    case MENU_CRONOGRAMA:
      if (opcionSeleccionada > 0) {
        opcionSeleccionada--;
      }
      mostrarMenuCronograma();
    break;
    case SELECCIONAR_DIGITO:
      if (digitoSeleccionado > 0) { 
        digitoSeleccionado--;
      }
      if(horario == 0){
        String readHoraDia = leerEEPROM(EEPROM_HORA_DIA_ADDR);
        mostrarSeleccionarDigito(readHoraDia);
        //mostrarSeleccionarDigito(horaDia);
      } else {
        String readHoraNoche = leerEEPROM(EEPROM_HORA_NOCHE_ADDR);
        mostrarSeleccionarDigito(readHoraNoche);
        //mostrarSeleccionarDigito(horaNoche);
      }
        
    break;
    case AJUSTAR_HORA:
      int mayor = 10;
      if(digitoSeleccionado == 0 && horaS.charAt(1) - '0' < 4){
        mayor = 3;
      } else if(digitoSeleccionado == 0) {
        mayor = 2;
      }
      if(digitoSeleccionado == 1 && horaS.charAt(0) - '0' == 2){
        mayor = 4;
      }
      if(digitoSeleccionado == 2){
        mayor = 6;
      }
      if (digito < mayor - 1) { 
        digito++;
      if(digitoSeleccionado > 1){
        horaS.setCharAt(digitoSeleccionado + 1, '0' + digito);
      } else {
        horaS.setCharAt(digitoSeleccionado, '0' + digito);
      }  	 
    }
      mostrarSeleccionarDigito(horaS);
    break;
    }
  }
}

// BOTON ABAJO
void botonAbajo(){
  // BOTON DOWN
  if (digitalRead(botonDown) == LOW) {
    delay(200);
	  switch(estadoActual){
      case MENU_PRINCIPAL:
	   	  if (opcionSeleccionada < totalOpciones - 1) { 
       	opcionSeleccionada++;
    	  }
    	  mostrarMenuPrincipal();
	    break;
	    case MENU_CRONOGRAMA:
		    if (opcionSeleccionada < totalOpciones - 1) { 
       	  opcionSeleccionada++;
    	  }
    	  mostrarMenuCronograma();
	    break;
      case SELECCIONAR_DIGITO:
      	if (digitoSeleccionado < 4 - 1) { 
       		digitoSeleccionado++;
    	  }
		    if(horario == 0){
          String readHoraDia = leerEEPROM(EEPROM_HORA_DIA_ADDR);
      		mostrarSeleccionarDigito(readHoraDia);
      		//mostrarSeleccionarDigito(horaDia);
        } else {
          String readHoraNoche = leerEEPROM(EEPROM_HORA_NOCHE_ADDR);
        	mostrarSeleccionarDigito(readHoraNoche);
        	//mostrarSeleccionarDigito(horaNoche);
        }
      break;
      case AJUSTAR_HORA:
      if (digito > 0) { 
          digito--;
          if(digitoSeleccionado > 1){
            horaS.setCharAt(digitoSeleccionado + 1, '0' + digito);
          } else {
            horaS.setCharAt(digitoSeleccionado, '0' + digito);
          }     
        }
        mostrarSeleccionarDigito(horaS);
      break;
    }
  }
}

// BOTON CONFIRMAR
void botonConfirmar(){
  // BOTON CONFIRM
  if (digitalRead(botonConfirm) == LOW) {
    delay(200);
    switch(estadoActual){
      case MENU_PRINCIPAL:
	   	  if(opcionSeleccionada == 0){
			    //opcionSeleccionada = 0;
      		mostrarMenuCronograma();
			    estadoActual = MENU_CRONOGRAMA;
    	  } else{
			    opcionSeleccionada = 0;
      		mostrarMenuComida();
			    estadoActual = MENU_COMIDA;
    	  }
	  break;
	  case MENU_CRONOGRAMA:
		  if(opcionSeleccionada == 0){
			  //opcionSeleccionada = 0;
			  horario = 0;
        String readHoraDia = leerEEPROM(EEPROM_HORA_DIA_ADDR);
      	//mostrarSeleccionarDigito(horaDia);
      	mostrarSeleccionarDigito(readHoraDia);
			  estadoActual = SELECCIONAR_DIGITO;
    	} else{
			  opcionSeleccionada = 0;
			  horario = 1;
        String readHoraNoche = leerEEPROM(EEPROM_HORA_NOCHE_ADDR);
      	//mostrarSeleccionarDigito(horaNoche);
      	mostrarSeleccionarDigito(readHoraNoche);
			  estadoActual = SELECCIONAR_DIGITO;
    	}
	  break;
	  case SELECCIONAR_DIGITO:
		  if(horario == 0){
        String readHoraDia = leerEEPROM(EEPROM_HORA_DIA_ADDR);
			  horaS = readHoraDia;
			  //horaS = horaDia;
      } else {
        String readHoraNoche = leerEEPROM(EEPROM_HORA_NOCHE_ADDR);
			  //horaS = horaNoche; 	
			  horaS = readHoraNoche; 	
      }
		  if(digitoSeleccionado > 1){
          digito = horaS.charAt(digitoSeleccionado + 1) - '0';
      } else {
          digito = horaS.charAt(digitoSeleccionado) - '0';
      }
      lcd.clear();
      lcd.setCursor(2,0);
      lcd.print("Seleccionado!");
      delay(400);
      mostrarSeleccionarDigito(horaS);
      estadoActual = AJUSTAR_HORA;
	  break;
	  case AJUSTAR_HORA:
      lcd.clear();
      lcd.setCursor(2,0);
      lcd.print("Modificado!");
      delay(400);
		  if(horario == 0){
        horaDia = horaS;
        escribirEEPROM(EEPROM_HORA_DIA_ADDR, horaDia);
			  mostrarSeleccionarDigito(horaDia);
      } else {
        horaNoche = horaS;
        escribirEEPROM(EEPROM_HORA_NOCHE_ADDR, horaNoche);
			  mostrarSeleccionarDigito(horaNoche);
      }
		  opcionSeleccionada = 0;
      estadoActual = SELECCIONAR_DIGITO;
	  break;
    }
   
  }
}

// BOTON ATRAS
void botonAtras(){
  // BOTON BACK
  if (digitalRead(botonBack) == LOW) {
    delay(200);
	  opcionSeleccionada = 0;
	  digitoSeleccionado = 0;
    Serial.println("Boton apretado");

    switch(estadoActual){
      case SELECCIONAR_DIGITO:
        mostrarMenuCronograma();
        estadoActual = MENU_CRONOGRAMA;
      break;
      case AJUSTAR_HORA:
        mostrarMenuCronograma();
        estadoActual = MENU_CRONOGRAMA;
      break;
      default:
        mostrarMenuPrincipal();
        estadoActual = MENU_PRINCIPAL;
      break;
      }
  }
}

// Función para escribir una cadena en la EEPROM
void escribirEEPROM(int address, String data) {
  // AJUSTA LA LONGITUD
  int longitud = data.length();
  if (longitud > MAX_STRING_LENGTH - 1) {
    longitud = MAX_STRING_LENGTH - 1;
  }

  // GUARDA LA LONGITUD
  EEPROM.write(address, longitud);

  // GUARDA 1X1
  for (int i = 0; i < longitud; i++) {
    EEPROM.write(address + 1 + i, data[i]);
  }
  // TERMINA LA CADENA CON \0
  EEPROM.write(address + 1 + longitud, '\0');
  EEPROM.commit();
}

String leerEEPROM(int address) {
  // AJUSTA LA LONGITUD
  int longitud = EEPROM.read(address);
  if (longitud > MAX_STRING_LENGTH - 1) {
    longitud = MAX_STRING_LENGTH - 1;
  }
  // VARIABLE PARA GUARDAR
  char data[longitud + 1];
  for (int i = 0; i < longitud; i++) {
    // LEE 1X1
    data[i] = EEPROM.read(address + 1 + i);
  }
  // TERMINA LA CADENA CON \0
  data[longitud] = '\0'; 
  return String(data);
}