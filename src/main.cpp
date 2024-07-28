#include <Arduino.h>
#include <LiquidCrystal_I2C.h>
#include <Servo.h>
#include "config.h"

// VARIABLES WIFI
byte cont = 0;
byte max_intentos = 50;

// LIBRERIAS WIFI, BLYNK Y NTP
#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h>
#include <NTPClient.h>
#include <WiFiUdp.h>

// VARIABLES BLYNK
BlynkTimer timer;

// CONFIGURAR SERVIDOR NTP PARA OBTENER HORA EN TIEMPO REAL
WiFiUDP udp;
NTPClient timeClient(udp, NTPSERVER, GMT_3, INTERVALO);

// LED INTEGRADO
const int ledPin = 2;

// SERVO
Servo myservo;
int vecesAlimentadoHoy = 0;
String fechaHora = " ";

// BUZZER
const int PIN_BUZZ = 13;
int melodia = 0; // MELODIA DEFAULT

// DISPLAY LCD
LiquidCrystal_I2C lcd(0x27,16,2);
String horaInicio;

// MOTOR VIBRACION
const int PIN_VIBRACION = 14;

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
bool verificarConexionBlynk();
void timerBlynk();

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

// RESPALDO VECES ALIMENTADO EN CASO DE CORTE DE ENERGIA
BLYNK_WRITE(V4) {
  // LEE EL PARAMETRO
  vecesAlimentadoHoy = param.asInt();
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
}

void setup() {
  // DEFINICION DE PINES
  pinMode(PIN_BUZZ, OUTPUT);
  pinMode(ledPin, OUTPUT);
  
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

  // BLYNK TIMER
  timer.setInterval(60000, timerBlynk);

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

  // MANEJAR RECONEXION BLYNK
  if (!verificarConexionBlynk()) {
    Serial.println("Reconectando a Blynk...");
    Blynk.begin(BLYNK_AUTH_TOKEN, WIFI_SSID, WIFI_PASSWORD);
    delay(1000);

    // PRENDER BLYNK
    Blynk.run();
    delay(5000);

    // VERIFICAR SI NO SE PERDIO UNA COMIDA
    if(timeClient.getFormattedTime() > "10:00:00" && timeClient.getFormattedTime() <= "14:00:00" && vecesAlimentadoHoy == 0){
      dispensar();
    }
    if(timeClient.getFormattedTime() > "20:00:00" && timeClient.getFormattedTime() <= "23:00:00" && vecesAlimentadoHoy == 1){
      dispensar();
    }

    // VERIFICAR SI NO SE PERDIO EL REINICIO
    if(timeClient.getFormattedTime() > "00:00:00" && timeClient.getFormattedTime() <= "05:00:00" && vecesAlimentadoHoy > 0){
      vecesAlimentadoHoy = 0;
      Blynk.virtualWrite(V3,vecesAlimentadoHoy);
      Blynk.virtualWrite(V5,vecesAlimentadoHoy);
    }
  }

  // PRENDER LED INTEGRADO
  digitalWrite(ledPin, LOW);

  // PRENDER A BLYNK
  Blynk.run();

  // PRENDER TIMER
  timer.run();

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

// CONEXION WIFI Y BLYNK
void iniciarConexiones(){
  // CONEXION WIFI
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED and cont < max_intentos) { // ESPERA CONEXION CON UN MAXIMO DE INTENTOS
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

    // SI SE REALIZO LA CONEXION QUE HAGA ESTO
    if (WiFi.status() == WL_CONNECTED) {
      // HORA ACTUAL
      timeClient.begin();
      timeClient.update();

      // INICIALIZAR DISPLAY
      inicializarDisplay();
    }

    // INICIA BLYNK
    Blynk.begin(BLYNK_AUTH_TOKEN, WIFI_SSID, WIFI_PASSWORD);
    delay(1000);

    // PRENDER A BLYNK
    Blynk.run();
    delay(5000);

    // VERIFICAR SI NO SE PERDIO UNA COMIDA
    if(timeClient.getFormattedTime() > "10:00:00" && timeClient.getFormattedTime() <= "14:00:00" && vecesAlimentadoHoy == 0){
      dispensar();
    }
    if(timeClient.getFormattedTime() > "20:00:00" && timeClient.getFormattedTime() <= "23:00:00" && vecesAlimentadoHoy == 1){
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

// VERIFICAR CONEXION BLYNK
bool verificarConexionBlynk() {
  return Blynk.connected();
}

// TIMER BLYNK
void timerBlynk() {
  timeClient.update();
  String horaActual = " | Hora Act. " + timeClient.getFormattedTime();
  String horaDeInicio = "Hora inicio "+ horaInicio;
  if(fechaHora == " "){
    Blynk.virtualWrite(V4, horaDeInicio + horaActual);
  } else {
    Blynk.virtualWrite(V4, fechaHora + horaActual);
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
  lcd.print("Hora de inicio: ");
  lcd.setCursor(4, 1);
  timeClient.update();
  horaInicio = timeClient.getFormattedTime();
  lcd.print(horaInicio);
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

  // ACTUALIZA LA PANTALLA
  lcd.clear();
  lcd.setCursor(1, 0);
  lcd.print("Ultima comida");
  lcd.setCursor(0, 1);
  lcd.print(diaSemanaStr);
  lcd.setCursor(8, 1);
  lcd.print(timeClient.getFormattedTime());

  // GUARDA DIA/HORA Y LA ENVIA A LA APP
  fechaHora = diaSemanaStr + " " + timeClient.getFormattedTime();

  // AUMENTA vecesAlimentadoHoy Y LO ACTUALIZA EN LA APP (SLIDE Y VALUE)
  vecesAlimentadoHoy++;
  Blynk.virtualWrite(V3,vecesAlimentadoHoy);
  Blynk.virtualWrite(V5,vecesAlimentadoHoy);
}

// DISPENSADOR AUTOMATICO (10 AM Y 20 PM)
void dispensadorAutomatico(){
  if ((timeClient.getFormattedTime() == "10:00:00" || timeClient.getFormattedTime() == "20:00:00" ) && vecesAlimentadoHoy < 2){
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
