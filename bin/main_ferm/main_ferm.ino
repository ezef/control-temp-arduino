/*
Control de Temperatura para Fermentador de Cerveza
HISTORY:

091214:
  -histeresis a 0.6
  -guarda tempSet en la EEPROM
 
 Backlight conectada al pin 2 para poder activarla y desactivarla desde el codigo 
 
  */

#include <LiquidCrystal.h>
#include <EEPROM.h>
#include <tempo.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#define RS 8
#define E 7
#define D4 6
#define D3 5
#define D2 4
#define D1 3
#define LIGHT 2
#define ANALOGBUTTONS0 0
#define ANALOGBUTTONS1 1
#define RELAY2 11
#define RELAY1 12
#define TEMP1 2
#define HISTERESIS 0.6
#define ADDR 0 //direccion de la EEPROM para la tempSet
#define ADDR1 1 // direccion de la EEPROM para la tempSet1
#define BUSONEWIRE 9

LiquidCrystal lcd(RS , E , D4 , D3 , D2 , D1);
int laststate[] = {
  0,0,0,0,0,0}; // de funcion lecturabotones()
int i,j;
int a0,a1;
int tempSet,auxTempSet;
int tempSet1,auxTempSet1;
int minTemp,maxTemp;
float tempD;
float temp1;

boolean tempSetChange, //variable que indica si se ha cambiado la tref pero no se ha guardado.
        blinkState,
        tempSetChange1;

Tempo t_boton(100); //temporizador para lectura de botones
Tempo t_temp(30*1000); // temporizador para la lectura de temperatura
Tempo t_blink(300); // temporizador para el parpadeo del lcd

OneWire oneWire(BUSONEWIRE);
DallasTemperature sensors(&oneWire);


void setup() {
  lcd.begin(16, 2);  

  Serial.begin(9600);
  

  pinMode(LIGHT,OUTPUT);
  pinMode(RELAY1,OUTPUT);
  pinMode(RELAY2,OUTPUT);

  digitalWrite(LIGHT,HIGH);
  digitalWrite(14,HIGH); //pullup de a0 para lectura de los botones
  digitalWrite(15,HIGH); //pullup de a1 para lectura de los botones

  i=0; //columna de cursor
  j=1; //fila de cursor  
 
  minTemp=0;  //temps minimas y maximas de seteo.
  maxTemp=25;

  tempSetChange= false; // variable para indicar si la setTemp fue cambiada
  tempSetChange1= false;
  blinkState= false;    //variable para indicar el estado del parpadeo en la funcion parpadeartexto()
  tempSetChange1=false;


  tempSet=EEPROM.read(ADDR); //leo la temp guardada en la EEPROM.
  tempSet1=EEPROM.read(ADDR1);
  auxTempSet = tempSet;

  lcd.print("   El Mason te  ");
  lcd.setCursor(0,1);        
  lcd.print("    la pone!    ");
  
  delay(2500);
  lcd.clear();
  
  lcd.print("F1:     F2:     ");
  lcd.setCursor(0,1);
  lcd.print("S1:    S2:    ");

  lcd.setCursor(3,1);
  lcd.print(tempSet);
  lcd.setCursor(12,1);
  control_sens(TEMP1);
  control_sensado_18b20(BUSONEWIRE);
      
   
}

void loop() {
  
  
  if (t_boton.state()){ // realiza la lectura de los botones  y actualiza el lcd
    lcd.setCursor(4,1);
    lecturabotones();       
  }

  if (tempSetChange){ //si la tempSet cambia,  parpadea hasta apretar enter 
    parpadeartexto(auxTempSet,3);
  }
    if (tempSetChange1){ //si la tempSet cambia,  parpadea hasta apretar enter 
    parpadeartexto(auxTempSet1,10);
  }

  if (t_temp.state()){ // realiza la lectura de la temperatura, actualiza el lcd y comanda los relays.
    control_sens(TEMP1);
	control_sensado_18b20(BUSONEWIRE);
    
        
  }  

}

void control_sens(int pin){  
  tempD=0.0;
  float actual=0.0;
  float ant=analogRead(pin)*0.48828125;
  float alpha=0.8;
  for(int i= 0;i < 30;i++) // leo 30 veces para tomar una mejor lectura final promedio
  { 
    actual=analogRead(pin)*0.48828125;
    tempD += (actual)*alpha + (1-alpha)*ant;
    ant=actual;
  }  
  lcd.setCursor(3,0);
  lcd.print(tempD=tempD/30);  
  control_comandar();
}

void control_comandar(){
    if(tempD > tempSet+HISTERESIS){
      digitalWrite(RELAY1,HIGH);
      lcd.setCursor(5,1);
      lcd.print("ON");
      }
    else{
        if ( tempD<tempSet-HISTERESIS)
          { digitalWrite(RELAY1,LOW);
            lcd.setCursor(5,1);
            lcd.print("  ");
          }
      }  
}

void  control_sensado_18b20(int pin){
  sensors.requestTemperatures();
  temp1=sensors.getTempCByIndex(0);
  control_comandar_18b20();
  
}

void control_comandar_18b20(){  
      if(temp1 > tempSet1+HISTERESIS){
      digitalWrite(RELAY2,HIGH);
      lcd.setCursor(12,1);
      lcd.print("ON");
      }
    else{
        if ( temp1<tempSet-HISTERESIS)
          { digitalWrite(RELAY2,LOW);
            lcd.setCursor(12,1);
            lcd.print("  ");
          }
      }
}



void parpadeartexto(int texto,int posicion){
  if (t_blink.state()){
    lcd.setCursor(posicion,1);
    
    if (blinkState){ 
      lcd.print("  ");
      blinkState=false;
    }
    else {
      lcd.print(texto);
      blinkState=true; 
    }    
  }

  
  

}

void lecturabotones() 
{ 
  a0 = analogRead(ANALOGBUTTONS0);
  a1 = analogRead(ANALOGBUTTONS1);

  if ( a0 < 50) {
    if ( laststate[0] == 0 ){      //boton left     
      laststate[0] = 1;     
      btnLEFT();

    }     
  } 
  else {laststate[0] = 0;}


  if (( a0 < 400) && ( a0 > 300)){
    if ( laststate[1] == 0 ){      //boton down     
      laststate[1] = 1;
      btnDOWN();      
    }   
  } 
  else {laststate[1] = 0;}


  if (( a0 < 800) && ( a0 > 600)){
    if ( laststate[2] == 0 ){      //boton up     
      laststate[2] = 1;
      btnUP();     
    }
  } 
  else {laststate[2] = 0;}


  if ( a1 < 50) {
    if ( laststate[3] == 0 ){      //boton enter     
      laststate[3] = 1;
      btnENTER();  
    } 
  }
  else {laststate[3] = 0;}


  if (( a1 < 400) && ( a1 > 300)){
    if ( laststate[4] == 0 ){      //boton esc     
      laststate[4] = 1;
      btnESC();  
    } 
  } 
  else {laststate[4] = 0;}


  if (( a1 < 800) && ( a1 > 600)){
    if ( laststate[5] == 0 ){      //boton derecha     
      laststate[5] = 1;
      btnRIGHT();  
    }   
  } 
  else {laststate[5] = 0;}

}



void btnLEFT(){
  //Serial.println("left");
    if(auxTempSet1 > minTemp){
    auxTempSet1--;
    tempSetChange1=true;
  }
  
}

void btnRIGHT(){
  //Serial.println("right");
     if (auxTempSet1 < maxTemp){
    auxTempSet1++;
    tempSetChange1=true;
  }
  
} 

void btnDOWN(){
  //Serial.println("down");
  if(auxTempSet > minTemp){
    auxTempSet--;
    tempSetChange=true;
  }

} 

void btnUP(){
  //Serial.println("up");
  if (auxTempSet < maxTemp){
    auxTempSet++;
    tempSetChange=true;
  }

} 

void btnENTER(){
  //Serial.println("enter");
  if (tempSetChange){     // en caso de cambiar la temperatura de referencia esta espera confirmacion presionando enter... 
    tempSetChange=false;     
    
    tempSet=auxTempSet;
    lcd.setCursor(3,1);
    lcd.print(tempSet);
    
    EEPROM.write(ADDR,tempSet);

    control_comandar(); 
  }  
  if (tempSetChange1){
    tempSetChange1=false;     
    
    tempSet1=auxTempSet1;
    lcd.setCursor(10,1);
    lcd.print(tempSet);
    
    EEPROM.write(ADDR1,tempSet1);

    control_comandar_18b20();
  }
} 

void btnESC(){
  //Serial.println("esc");
  
  if (tempSetChange){    
    auxTempSet=tempSet;  
    tempSetChange=false;
    
    lcd.setCursor(3,1);
    lcd.print(auxTempSet);    
  }  
   if (tempSetChange1){    
    auxTempSet1=tempSet1;  
    tempSetChange1=false;
    
    lcd.setCursor(10,1);
    lcd.print(auxTempSet1);    
  }
} 









