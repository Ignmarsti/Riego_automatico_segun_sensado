//#include <Time.h>
//#include <TimeLib.h>

#include <Wire.h>////Libreria del I2C 
#include "RTClib.h"///Libreria del RTC
#include <DHT.h>///Libreria sensor temperatura


#define rele 7///Definimos el puerto donde conectaremos el relé 
#define pinsensorT 2////Definimos el puerto donde conectaremos el sensor de temperatura
#define TipoSensor DHT11/////Definimos el tipo de sensor que es
#define sensor_humedad_tierra 17///Definimos el pin del higrómetro de suelo
#define ldr_pin 16///Definimos el pin A2 para el sensor de luz

//////Objetos/////////

RTC_DS1307 rtc;
DateTime hoy;
DHT dht(pinsensorT, TipoSensor);////Indicamos el puerto al que esta conectado y el tipo de sensor que es


////////VARIABLES GLOBALES//////////
unsigned int bandera_riego=0;
unsigned int bandera_riego_2=0;
unsigned int bandera_verano=0;
unsigned int i=0;

unsigned int duracion_riego=10000;

unsigned int horariego1=7;
unsigned int horariego2=12;
unsigned int horariego3=18;
unsigned int horariegoextra=22;

unsigned int ldr=0;
unsigned int humedad_tierra=0;
//int hora_actual;
unsigned int ano,mes,dia,hora,minuto,segundo;

unsigned long tiempo;
unsigned long tiempo2=0;;

float h, t;

String DiasdelaSemana[7] = { "Domingo", "Lunes", "Martes", "Miercoles", "Jueves", "Viernes", "Sabado" };
String MesesdelAno[12] = { "Enero", "Febrero", "Marzo", "Abril", "Mayo",  "Junio", "Julio","Agosto","Septiembre","Octubre","Noviembre","Diciembre" };

void riego_automatico(void);

//////Configuracion del reloj///////
//SDA A4 Línea I2C de transmisión de datos
//SCL A5 Línea I2C de la señal de reloj
//GND GND 
//VCC VCC
//SQW Este pin sirve para obtener una señal cuadrada, como no lo usaremos no lo declaramos
///////////////////////////////////

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  Wire.begin();////Inicializamos el I2C
  rtc.begin();/////Inicializamos el RTC
  dht.begin();////Inicializamos el sensor de Temperatura

  tiempo=millis();
  
  SREG = (SREG & 0b01111111);      //Esta instrucción deshabilita a las interrupciones globales sin modificar el resto de las interrupciones.
  TCNT2 = 0;     //Limpiar el registro que guarda la cuenta del Timer-2
  TIMSK2 =TIMSK2|0b00000001;      //Habilitación de la bandera 0 del registro que habilita la interrupción por desbordamiento del TIMER2.
  TCCR2B=0b00000111;///Configuramos el prescaler del tmr2 en 1024 (cada tick=0,128 ms --> cada vez que se desborda el timer2 han pasado 255*tick=32,64 ms
  SREG = (SREG & 0b01111111) | 0b10000000; //Habilitamos interrupciones
  
  if (! rtc.isrunning()) {
    Serial.println("No se ha iniciado correctamente el módulo RTC");
    //ESTAS LINEAS SOLO SE USARÁN EN CASO DE QUE VAYAMOS A CARGAR EL CÓDIGO POR PRIMERA VEZ EN EL ARDUINO.
    //UNA VEZ SE HAYA COMPILADO UNA VEZ, DEBREMOS COMENTAR LA LINEA Y VOLVER A SUBIRLO
    //ESTA SENTENCIA ES PARA ESTABLECER LA HORA ACTUAL MEDIANTE LA HORA DE NUESTRO ORDENADOR
    //rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    rtc.adjust(DateTime(2020, 11, 21, 19, 0, 0));
  }

  //Definimos los puertos
  pinMode(rele, OUTPUT);///Puerto del relé
  

}

void loop() {
  hoy = rtc.now();///Recuperamos la fecha actual 
  
  ano=hoy.year();
  mes=hoy.month();
  dia=hoy.day();
  hora=hoy.hour();
  minuto=hoy.minute();
  segundo=hoy.second();

  if(tiempo-tiempo2>=5000)
  {
    tiempo2=tiempo;
    Serial.print(ano);
    Serial.print('/');
    Serial.print(mes);
    Serial.print('/');
    Serial.print(dia);
    //Serial.print('(');
    //Serial.print(DiasdelaSemana[hoy.dayOfTheWeek()]);
    //Serial.print(')');
    Serial.print(' ');
    Serial.print(hora);
    Serial.print(':');
    Serial.print(minuto);
    Serial.print(':');
    Serial.print(segundo);
    Serial.println();
  }
  
  riego_automatico();

  
}


ISR(TIMER2_OVF_vect){///////////Atencion a la interrupcion por desbordamiento de timer2
  i++;
  if(i=200)
  {
    ldr = map(analogRead(ldr_pin), 1023, 0, 100, 0);///Esta función lo que hace es convertir un valor entre 0 y 1023 en un valor equivalente entre 0 y 100. De esta manera pondremos la cantidad de luz en tanto por ciento
  }
  if(i=500)
  {
    int humedad_tierra = analogRead(sensor_humedad_tierra);
    Serial.print("Humedad de la tierra: ");
    Serial.print(humedad_tierra);
    Serial.println("  (Recuerde que un valor entre 0 sería con el sensor sumergido en agua, y un valor de 1023 sería con el sensor al aire)");
  }
  if(i=900)///Si han pasado 27 segundos aproximadamente, leemos la temperatura y la humedad
  {
    h = dht.readHumidity();//Lectura de humedad relativa
    t = dht.readTemperature();////En grados celsius por defecto
    Serial.print("Humedad relativa: ");
    Serial.println(h);
    Serial.print("Temperatura: ");
    Serial.println(t);
  }
  if(i=1000)///A los 32,64 segundos se comprueban los valores
  {
    i=0;
    Serial.print("Los valores de Temperatura, Humedad del ambiente, Cantidad de luz, Humedad de la tierra son; ");
    Serial.print(t);
    Serial.print(", ");
    Serial.print(h);
    Serial.print(", ");
    Serial.print(ldr);
    Serial.print('%');
    Serial.print(", ");
    Serial.print(humedad_tierra);
    if(35<=t<=40&&ldr>38&&(600<=humedad_tierra<=800)||humedad_tierra>800)///////////AQUI VA LA CONDICION DE HUMEDAD,Tª Y HUMEDAD RELATIVA PARA QUE SE PRODUZCA EL RIEGO
    {
    bandera_riego_2=1;
    digitalWrite(rele, LOW);///CREO QUE MI RELE SE ACTIVA EN BAJA
    Serial.print("Regando a las ");
    Serial.print(hora);
    Serial.print(":");
    Serial.print(minuto);
    Serial.print(":");
    Serial.println(segundo);
    delay(duracion_riego);
    digitalWrite(rele,HIGH);
    }
  }

}

void riego_automatico(){
  if(mes==6||mes==7||mes==8)
  {
    bandera_verano=1;
  }
  else bandera_verano=0;

  if((hora==horariego1||hora==horariego2||hora==horariego3)&&bandera_riego==0)
  {
    bandera_riego=1;
    digitalWrite(rele, LOW);///CREO QUE MI RELE SE ACTIVA EN BAJA
    Serial.print("Regando a las ");
    Serial.println(hora);
    Serial.println(":");
    Serial.println(minuto);
    Serial.println(":");
    Serial.println(segundo);
    delay(duracion_riego);
    digitalWrite(rele,HIGH);
  }
  if((hora==(horariego1+1)||hora==(horariego2+1)||hora==(horariego3+1)||hora==(horariegoextra+1)))
  {
    bandera_riego=0;
  }
  if(bandera_verano==1&&hora==horariegoextra&&bandera_riego==0)
  {
    bandera_riego=1;
    digitalWrite(rele, LOW);///CREO QUE MI RELE SE ACTIVA EN BAJA
    Serial.print("Regando a las ");
    Serial.println(hora);
    Serial.println(":");
    Serial.println(minuto);
    Serial.println(":");
    Serial.println(segundo);
    delay(duracion_riego);
    digitalWrite(rele,HIGH);
  }
}