/*
Control de Temperatura para Fermentador de Cerveza
HISTORY:

091214:
  -histeresis a 0.6
  -guarda tempSet en la EEPROM
 
 Backlight conectada al pin 2 para poder activarla y desactivarla desde el codigo
 
 para leer del LM35 utilizar en el setup:
 analogReference(INTERNAL); para setear el aref en 1.1v
  
 en el loop:
 sensorValue=0.0;
  for(int i= 0;i < 5;i++) // leo 5 veces para tomar una mejor lectura final promedio
  {
    //sensorValue += (500.0*analogRead(2)/1024);
    sensorValue += analogRead(2)/9.31;
    delay(1000);
  }
  
  sensorValue = sensorValue/5;  //promedio las diversas lecturas para entregar un solo valor de temp
 
 
 se muestra el log de temperaturas con rango absoluto entre 0 y 24 grados en el display....
 
  */

#include <LiquidCrystal.h>
#include <EEPROM.h>
#include <tempo.h>
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
#define TEMP2 3
#define HISTERESIS 0.6
#define ADDR 0 //direccion de la EEPROM para la tempSet

LiquidCrystal lcd(RS , E , D4 , D3 , D2 , D1);
int laststate[] = {
  0,0,0,0,0,0}; // de funcion lecturabotones()
int i,j;
int a0,a1;
int tempSet,auxTempSet;
int minTemp,maxTemp;

float tempC,tempD;

boolean tempSetChange, //variable que indica si se ha cambiado la tref pero no se ha guardado.
        blinkState;

Tempo t_boton(100); //temporizador para lectura de botones
Tempo t_temp(30*1000); // temporizador para la lectura de temperatura
Tempo t_blink(300); // temporizador para el parpadeo del lcd

unsigned long logTemp[8]={0,0,0,0,0,0,0,0};
byte prueba[8]= {  B00001,
                   B00001,
                   B00001,
                   B00010,
                   B00001,
                   B00001,
                   B00001,
                   B00001};


void setup() {
  lcd.begin(16, 2);
  //lcd.cursor();

  Serial.begin(9600);
  
  //analogReference(INTERNAL);//seteo la referencia interna a 1.1v para la lectura mas precisa de la temp

  pinMode(LIGHT,OUTPUT);
  pinMode(RELAY1,OUTPUT);
 

  digitalWrite(LIGHT,HIGH);
  digitalWrite(14,HIGH); //pullup de a0 para lectura de los botones
  digitalWrite(15,HIGH); //pullup de a1 para lectura de los botones

  i=0; //columna de cursor
  j=1; //fila de cursor
  
  tempC = 0.0;
  tempD = 0.0;  
  minTemp=0;  //temps minimas y maximas de seteo.
  maxTemp=25;

  tempSetChange= false; // variable para indicar si la setTemp fue cambiada
  blinkState= false;    //variable para indicar el estado del parpadeo en la funcion parpadeartexto()


  tempSet=EEPROM.read(ADDR); //leo la temp guardada en la EEPROM.
  auxTempSet = tempSet;

  lcd.print("    SOFTWARE   ");
  lcd.setCursor(0,1);        
  lcd.print("    BIRRERO    ");
  
  delay(2500);
  lcd.clear();
  
  lcd.print("Ca:      De:    ");
  lcd.setCursor(0,1);
  lcd.print("SET:      LOGTEP");

  lcd.setCursor(4,1);
  lcd.print(tempSet);
  control_sens(TEMP1);
  log_update(tempD);
  
  
  /*
  lcd.createChar(0,prueba);
  lcd.setCursor(15 , 1);
    lcd.write(byte(0));
  */
    
   
}

void loop() {

  //lcd.setCursor(i, j);
  
  if (t_boton.state()){ // realiza la lectura de los botones  y actualiza el lcd
    lcd.setCursor(4,1);
    lecturabotones();       
  }

  if (tempSetChange){ //si la tempSet cambia,  parpadea hasta apretar enter 
    parpadeartexto(auxTempSet);
  }

  if (t_temp.state()){ // realiza la lectura de la temperatura, actualiza el lcd y comanda los relays.
    control_sens(TEMP1);
    log_update(tempD);
        
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
      lcd.setCursor(7,1);
      lcd.print("ON");
      }
    else{
        if ( tempD<tempSet-HISTERESIS)
          { digitalWrite(RELAY1,LOW);
            lcd.setCursor(7,1);
            lcd.print("  ");
          }
      }
  
}

void parpadeartexto(int texto){

  if (t_blink.state()){
    lcd.setCursor(4,1);
    
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


void log_update(int temp){ // se actualiza todo el loggin de temperatura 
  temp = temp - (tempSet - 3); //ajusto la temp recibida para q se muetre entre +-3 grados de la tempSet.
  for (int i=0;i<8;i++){ // recorro cada linea del logtemp, realizo un corrimiento de 1 bit a la izq y agrego o no un bit menos significativo en 1 dependiendo de la temperatura. 
    logTemp[i] = logTemp[i] << 1;
    if (i < temp){
      logTemp[i] = logTemp[i] | B1;
    }    
  }
  log_show();
  
}
  
void log_show(){
  unsigned long porcion = B11111;
  for (int i=0;i<6;i++){     // por cada caracter(son 6) corto el logTemp cada 5 columnas y genero el char a imprimir
    byte impr[8];       
    for (int j=0;j<8;j++){  //recorro el logtemp para este caracter y corto cada porcion de cada linea
      impr[7 -j] = (logTemp[j]  & porcion) >> i*5 ;   
      
    }      
    porcion = porcion << 5;
    
    lcd.createChar(i,impr);
    lcd.setCursor(15-i , 1);
    lcd.write(byte(i));    
  }
}

void log_escalar(int diff){
  
  if(diff>0){      
    for (int j=0;j<diff;j++){
      
      for (int i=7;i>0;i--){
        logTemp[i]=logTemp[i-1];    
        
      }
      logTemp[0]=0xffffffff;    
    }
  }
  else{    
    for (int j=0;j<abs(diff);j++){      
      for (int i=0;i<7;i++){
        logTemp[i]=logTemp[i+1];       
      }
      logTemp[7]=0;    
    }    
  }  
  log_show();  
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
  
  
}

void btnRIGHT(){
  //Serial.println("right");
   
  
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
    
    log_escalar(tempSet-auxTempSet); // escala el grafico del loggin en funcion de la nueva tempSet
    
    tempSet=auxTempSet; // [TODO] guardar valor en la EEPROM
    lcd.setCursor(4,1);
    lcd.print(tempSet);
    
    EEPROM.write(ADDR,tempSet);

    control_comandar(); 
  }
} 

void btnESC(){
  //Serial.println("esc");
  if (tempSetChange){    
    auxTempSet=tempSet;  
    tempSetChange=false;
    
    lcd.setCursor(4,1);
    lcd.print(auxTempSet);    
  }
} 









