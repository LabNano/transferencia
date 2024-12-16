
/***********************************************************
 *                                                         *
 *        * O R N E L A S  C O R P O R A T I O N *         *
 *                                                         *
 ***********************************************************
 * Projeto: Sitema de Transferencia                        *
 * Placa - Hardware: Arduino Uno, Termo Shield             *
 * Data: 28/10/2017                                        *
 * Autor: Vinícius Ornelas                                 *
 * Professor: Rodrigo Gribel e Leonardo Campos             *
 * Usuários: Laboratório de Nanomateriais - UFMG           *
 *                                                         *
 * Descrição: Projeto feito para controlar temperatura de  *
 * uma plataforma de aquecimento local, para construção de *
 * hetero estruturas de materiais 2D na vertical, chamado  *
 * de Sistema de Transferência.                            *
 * Este projeto, é totalmente dedicado aos alunos da UFMG, *
 * e usuários do laboratório de nanomateriais de carbono.  *
 *                                                         *
 *            (C) Copyright 2017 Ornelas Corp.             *
 ***********************************************************
 *              OBSERVAÇÕES DO PROJETO                     *
 ***********************************************************
 * O Termistor foi ligado ao arduino pelo método do Fator  *
 * Beta, conforme tutorial do site:                        *
 * http://labdegaragem.com/profiles/blogs/tutorial-como    *
 * -utilizar-o-termistor-ntc-com-arduino                   *
 *                                                         *
 * Dados do Termistor:                                     *
 * Termistor NTC de vidro selado de 100k - 1%              *
 * Utilização: Próprio para Hotend e Mesa Aquecida         *
 * Especificações:                                         *
 * Temperatura de Operação: -40º a 350º C                  *
 * Marca: MJResistance                                     *
 * Número modelo: FJMB1-100-4000-1                         *
 * Resistëncia a  25ºC: 100K                               *
 * Beta: 4000 (R25 / 50ºC)                                 *
 * Tolerância: 1%                                          *
 * Diâmetro: 2.0mm + /-0.2mm                               *
 * Comprimento terminais: 35mm + /-5mm                     *
 ***********************************************************
 */

//----------------------------

//BIBLIOTECAS:
#include <AutoPID.h>
#include <LiquidCrystal.h>

//----------------------------

//Variavéis para comunicação com Arduino:
const byte buffSize = 40;
char inputBuffer[buffSize];
const char startMarker = '<';
const char endMarker = '>';
byte bytesRecvd = 0;
boolean readInProgress = false;
boolean newDataFromPC = false;

char messageFromPC[buffSize] = {0};
int intFromPC = 0;
float floatFromPC = 0.0; // fraction of servo range to move

//----------------------------

//Variaveis do PID Relay:
int WindowSize = 1000;
unsigned long windowStartTime;
double temperature, setPoint, outputVal;
unsigned long lastTempUpdate; //tracks clock time of last temp update
bool relayState;

//----------------------------

//DEFINIÇÕES:
#define I_BTadd  9  //Entrada Sinal Botão Adiciona
#define I_BTsub  10 //Entrada Sinal Botão Subtrai
#define I_BTset  13 //Entrada Sinal Botão Confirma
#define O_Refri  7  //Sinal Saida Bomba de Refrigeranção
#define O_Vacuo  8  //Sinal Saida Bomba de Vácuo
#define O_LedVM  A0 //Sinal Saida Led Vermelho
#define O_LedVD  A1 //Sinal Saida Led Verde
#define RELAY_PIN 6 //Sinal Saida Aquecimento Heater

#define TEMP_READ_DELAY 20 //can only read digital temp sensor every ~750ms

//pid settings and gains
#define OUTPUT_MIN 0
#define OUTPUT_MAX 200
#define KP .12
#define KI .0003
#define KD 0

//----------------------------

//CONFIGURAÇÃO DO THERMISTOR:
#define PINOTERMISTOR A2         
// Valor do termistor na temperatura nominal
#define TERMISTORNOMINAL 100000      
// Temp. nominal descrita no Manual
#define TEMPERATURENOMINAL 25   
// Número de amostragens para 
#define NUMAMOSTRAS 3
// Beta do nosso Termistor
#define BCOEFFICIENT 4000
// valor do resistor em série
#define SERIESRESISTOR 10000
//---------------------------- 

//MEMORIAS:
float Temp = 10;
int Temp_Min = 40;
int amostra[NUMAMOSTRAS];
int i;
int test = 0; //Bit recebe dados via serial para teste

float media;
float temperatura;

char val[9]=""; //Caracter que recebe dados da porta serial

byte Prog_Temp = 0;
byte M_Ciclo = 0;
byte M_TEMPO = 0;

//----------------------------

//PINOS DE INICIALIZAÇÃO DO DISPLAY:
const int rs = 12, en = 11, d4 = 5, d5 = 4, d6 = 3, d7 = 2;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);

//----------------------------

//input/output variables passed by reference, so they are updated automatically
AutoPID myPID(&temperature, &setPoint, &outputVal, OUTPUT_MIN, OUTPUT_MAX, KP, KI, KD);

//----------------------------

//call repeatedly in loop, only updates after a certain time interval
//returns true if update happened
bool updateTemperature() {
  if ((millis() - lastTempUpdate) > TEMP_READ_DELAY) {

    for (i=0; i< NUMAMOSTRAS; i++) {
     amostra[i] = analogRead(PINOTERMISTOR);
     //delay(10);
    }
   
    media = 0;
    for (i=0; i< NUMAMOSTRAS; i++) {
       media += amostra[i];
    }
    media /= NUMAMOSTRAS;
    // Converte o valor da tensão em resistência
    media = 1023 / media - 1;
    media = SERIESRESISTOR / media;
    
    //Faz o cálculo pela fórmula do Fator Beta
    
    temperatura = media / TERMISTORNOMINAL;     // (R/Ro)
    temperatura = log(temperatura); // ln(R/Ro)
    temperatura /= BCOEFFICIENT;                   // 1/B * ln(R/Ro)
    temperatura += 1.0 / (TEMPERATURENOMINAL + 273.15); // + (1/To)
    temperatura = 1.0 / temperatura;                 // Inverte o valor
    temperatura -= 273.15;                         // Converte para Celsius

    temperature = temperatura;

    Serial.print("<sp,");
    Serial.print(M_Ciclo);
    Serial.print(",");
    Serial.print(temperature);
    Serial.println(">");
    
    lastTempUpdate = millis();
    return true;
  }
  return false;
}//void updateTemperature

//----------------------------

void Controle_relay_PID(){

  setPoint = Temp;
  
  if(M_Ciclo == 1)
  {
    myPID.run(); //call every loop, updates automatically at certain time interval
    digitalWrite(O_LedVD, HIGH);
  /************************************************
   * turn the output pin on/off based on pid output
   ************************************************/
    if (millis() - windowStartTime > WindowSize)
    { //time to shift the Relay Window
      windowStartTime += WindowSize;
    }
    
    if (outputVal > millis() - windowStartTime) 
    {
      digitalWrite(RELAY_PIN, HIGH);
      digitalWrite(O_LedVM, HIGH);
      relayState = true;
    }
    else 
    {
      digitalWrite(RELAY_PIN, LOW);
      digitalWrite(O_LedVM, LOW);
      relayState = false;   
    }

    if (setPoint > 100)
    {
      if (temperature < (setPoint))
      {
        digitalWrite(RELAY_PIN, HIGH);
        digitalWrite(O_LedVM, HIGH);
        relayState = true;
      } 
    }
    else
    {
      if (temperature < (setPoint-12))
      {
        digitalWrite(RELAY_PIN, HIGH);
        digitalWrite(O_LedVM, HIGH);
        relayState = true;
      }
    }
  }
  else
  {
    myPID.stop();
    digitalWrite(RELAY_PIN, LOW);
    digitalWrite(O_LedVM, LOW);
    digitalWrite(O_LedVD, LOW);
    relayState = false;
  }
}//void Controle_relay_PID

//----------------------------

void Display(){

    lcd.setCursor(0, 0);
    lcd.print("Set-Point:");
    lcd.setCursor(10, 0);
    lcd.print(Temp);
    lcd.write(B11011111);
    lcd.print("C");

    if(Temp < 100)
    {
      lcd.setCursor(12, 0);
      lcd.write(B11011111);
      lcd.print("C  ");
    }

    if(Temp < 10)
    {
      lcd.setCursor(11, 0);
      lcd.write(B11011111);
      lcd.print("C   ");
    }
    
    lcd.setCursor(0, 1);
    lcd.print("Heater: ");
    lcd.setCursor(7, 1);
    lcd.print(temperatura);
    lcd.write(B11011111);
    lcd.print("C");

    if(temperatura < 100)
    {
      lcd.setCursor(12, 1);
      lcd.write(B11011111);
      lcd.print("C ");
    }
}//void Display

//----------------------------

void Botoes(){
  
  if(digitalRead(I_BTadd) == 1 && digitalRead(I_BTadd) != 0)
  {
    Temp++;
    
    if(Temp > 200)
    {
      Temp = 200;
    }
    delay(200);
  }

  if(digitalRead(I_BTsub) == 1 && digitalRead(I_BTsub) != 0)
  {
    Temp--;
    
    if(Temp < 0)
    {
      Temp = 0;
    }
    delay(200);
  }

  if(digitalRead(I_BTset) == 1 && digitalRead(I_BTset) != 0)
  {
    M_Ciclo = !M_Ciclo;
    delay(200);
  }
}//void Botoes

//----------------------------

void Inicializa_Sistema(){
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("UFMG-Dep. Fisica");
  lcd.setCursor(1, 1);
  lcd.print("Lab. Nanomat.");  

  delay(1000);

  lcd.clear();
  lcd.setCursor(3, 0);
  lcd.print("Sistema de");
  lcd.setCursor(1, 1);
  lcd.print("Transferencia");

  delay(1000);

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("By  Ornelas");
  lcd.setCursor(2, 1);
  lcd.print("Corporation");  
  
  delay(1000);
}

//----------------------------

void getDataFromPC() {

    // receive data from PC and save it into inputBuffer
    
  if(Serial.available() > 0) {

    char x = Serial.read();

      // the order of these IF clauses is significant
      
    if (x == endMarker) {
      readInProgress = false;
      newDataFromPC = true;
      inputBuffer[bytesRecvd] = 0;
      parseData();
    }
    
    if(readInProgress) {
      inputBuffer[bytesRecvd] = x;
      bytesRecvd ++;
      if (bytesRecvd == buffSize) {
        bytesRecvd = buffSize - 1;
      }
    }

    if (x == startMarker) { 
      bytesRecvd = 0; 
      readInProgress = true;
    }
  }
}

//=============
 
void parseData() {

    // split the data into its parts
    
  char * strtokIndx; // this is used by strtok() as an index
  
  strtokIndx = strtok(inputBuffer,",");      // get the first part - the string
  strcpy(messageFromPC, strtokIndx); // copy it to messageFromPC
  
  strtokIndx = strtok(NULL, ","); // this continues where the previous call left off
  intFromPC = atoi(strtokIndx);     // convert this part to an integer
  
  strtokIndx = strtok(NULL, ","); 
  floatFromPC = atof(strtokIndx);     // convert this part to a float

}

//=============

void configIno(){

  if (strcmp(messageFromPC, "sp") == 0 && newDataFromPC){
    M_Ciclo = intFromPC;
    Temp = floatFromPC;
    newDataFromPC = false;
    }
}

//----------------------------

void setup() {

  while (!updateTemperature()) {} //wait until temp sensor updated

  //if temperature is more than 4 degrees below or above setpoint, OUTPUT will be set to min or max respectively
  //myPID.setBangBang(0.01);
  myPID.setBangBang(0.001, 0.001);
  //set PID update interval to 4000ms
  myPID.setTimeStep(1000);

  Serial.begin(9600);

 //INICIALIZA O LCD 
  lcd.begin(16, 2);

  analogReference(EXTERNAL);

  windowStartTime = millis();
  
  //DEFINIÇÃO DOS PINOS
  pinMode(I_BTadd,INPUT);
  pinMode(I_BTsub,INPUT);
  pinMode(I_BTset,INPUT);
  
  pinMode(O_Refri,OUTPUT);
  pinMode(O_Vacuo,OUTPUT);
  pinMode(O_LedVM,OUTPUT);
  pinMode(O_LedVD,OUTPUT);

  
  //ESTADOS INICIAIS
  digitalWrite(O_Refri,0);
  digitalWrite(O_Vacuo,0);
  digitalWrite(O_LedVD,1);

  Inicializa_Sistema();

  Serial.println("<Arduino is ready>");
  
}//void setup

//----------------------------

void loop() {
  Display();
  Botoes();
  updateTemperature();
  Controle_relay_PID();
  getDataFromPC();
  configIno();
  //digitalWrite(O_LedVD, myPID.atSetPoint(1)); //light up LED when we're at setpoint +-1 degree
  
}//void loop
