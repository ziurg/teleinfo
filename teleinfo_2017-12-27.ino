// v3.0 - Envoie régulier (tous les n relevés)
//        meme s'il n'y a pas de changement.
//      - Modifie le calcul de Papp en moyennant la valeur.
//      - Modifie le calcul du courant en moyennant la valeur.
//      - Envoi des compteurs en Wh au lieu de KWh. Plus
//        précis, mais correction à prevoir dans MySensors
#define SKETCH_NAME "Teleinfo_sensor"
#define SKETCH_VERSION "v3.0"

// uncomment to enable debuging using serial port @115200bps
#define DEBUG_ENABLED
#define MY_DEBUG

// durée entre 2 rafraichissements des données - en mili-secondes 
//#define SLEEP_TIME 5000UL // Desactivé pour un calcul précis de la puissance

// Nombre de relevés avant envoi de la valeur (moyennée le cas échéant)
#define MAXSAMPLES 50 // Environ 3s entre chaque trame téléinfo. A vérifier

#define MY_RADIO_NRF24
#define MY_NODE_ID 10

// Wait times
#define LONG_WAIT 500
#define SHORT_WAIT 50

// mysensors
#include <SPI.h>
#include <MySensors.h>

// teleinfo
#include "RunningAverage.h"
#include <SoftwareSerial.h>
#include "teleInfo.h"
#define TI_RX 4
teleInfo TI( TI_RX );

// longueur max des données qu'on reçoit
#define BUFSIZE 15

///////////////////////////////////
// infos générales
///////////////////////////////////
#define CHILD_ID_ADCO 1  // Identifiant du compteur
#define CHILD_ID_PTEC 2 // Periode tarifaire en cours
#define CHILD_ID_IINST 3 // Intensite instantanee
#define CHILD_ID_ADPS 4 // Avertissement de depassement de puissance
#define CHILD_ID_IMAX 5
#define CHILD_ID_PAPP 6 // Puissance apparente
#define CHILD_ID_HC_HC 7
#define CHILD_ID_HC_HP 8
#define CHILD_ID_HC_HC2 9 // Compteur en Wh
#define CHILD_ID_HC_HP2 10 // Compteur en Wh

MyMessage msgText( 0, V_TEXT ); // Pour ADCO, PTEC, ADPS et IMAX
MyMessage msgCURRENT( 0, V_CURRENT ); // Pour IINST
MyMessage msgKWH( 0, V_KWH ); // (en fait c'est des WH) pour compteurs HC et HP
MyMessage msgWATT( 0, V_WATT ); // (pas vrai c'est des VA!) poour PAPP

///////////////////////////////////

teleInfo_t last; // dernière lecture téléinfo
// On utilise la moyenne sur toute la periode entre 2 envois
RunningAverage PappRA(MAXSAMPLES);
RunningAverage IinstRA(MAXSAMPLES);
int samples = 0; //nombre de trames recues
//float HC = 0.;

void setup() {
#ifdef DEBUG_ENABLED
  Serial.begin( 115200 );
#endif

  sendSketchInfo(SKETCH_NAME, SKETCH_VERSION);
  Serial.print(SKETCH_NAME);
  Serial.println(SKETCH_VERSION);
  wait(LONG_WAIT);
  
  teleInfo_t currentTI;

  // Présentation à la gateway
  present( CHILD_ID_ADCO, S_INFO, "ADCO" );
  present( CHILD_ID_PTEC, S_INFO, "PTEC" );
  present( CHILD_ID_IINST, S_MULTIMETER , "IINST");
  present( CHILD_ID_ADPS, S_INFO, "ADPS" );
  present( CHILD_ID_IMAX, S_INFO , "IMAX");
  present( CHILD_ID_PAPP, S_POWER, "PAPP" );
  //
  present( CHILD_ID_HC_HC, S_POWER, "HC" );
  present( CHILD_ID_HC_HP, S_POWER, "HP" );
  present( CHILD_ID_HC_HC2, S_POWER, "HC2" );
  present( CHILD_ID_HC_HP2, S_POWER, "HP2" );
  
  PappRA.clear();
  IinstRA.clear();
}

void loop() {
  teleInfo_t currentTI;
  
  // read téléinfo
  currentTI = TI.get();
  samples++;
  Serial.print("Sample n ");
  Serial.println(samples);

  // update average
  PappRA.addValue(currentTI.PAPP);
  IinstRA.addValue(currentTI.IINST);

  if (samples == MAXSAMPLES)
  {
    samples = 0;
    Serial.println("Nombre atteint, envoie des donnees");
    #ifdef DEBUG_ENABLED
      Serial.println("Running Average :");
      Serial.println(PappRA.getAverage(), 3);
      Serial.println(IinstRA.getAverage(), 3);
      Serial.print("ADCO :");
      Serial.println(currentTI.ADCO);
      Serial.print("PTEC :");
      Serial.println(currentTI.PTEC);
      Serial.print("ADPS :");
      Serial.println(currentTI.ADPS);
      Serial.print("IMAX :");
      Serial.println(currentTI.IMAX);
      Serial.print("HC_HC :");
      //HC = float(currentTI.HC_HC)/1000.;
      Serial.println(float(currentTI.HC_HC)/1000.);
      Serial.print("HC_HP :");
      //HC = float(currentTI.HC_HP)/1000.;
      Serial.println(float(currentTI.HC_HP)/1000.);
    #endif
    
    //sendTI( currentTI.ADCO, msgText, CHILD_ID_ADCO ); // ADCO
    send( msgText.setSensor( CHILD_ID_ADCO ).set( currentTI.ADCO ) ); // ADCO
    
    //sendTI( currentTI.PTEC, msgText, CHILD_ID_PTEC ); // PTEC
    send( msgText.setSensor( CHILD_ID_PTEC ).set( currentTI.PTEC ) ); // PTEC
    
    //sendTI( currentTI.ADPS, msgText, CHILD_ID_ADPS ); // ADPS
    send( msgText.setSensor( CHILD_ID_ADPS ).set( currentTI.ADPS ) ); // ADPS
    
    //sendTI( currentTI.IMAX, msgText, CHILD_ID_IMAX ); // IMAX
    send( msgText.setSensor( CHILD_ID_IMAX ).set( currentTI.IMAX ) ); // IMAX
    
    volatile uint32_t lastIinst = IinstRA.getAverage();
    send( msgCURRENT.setSensor( CHILD_ID_IINST ).set( lastIinst ) ); // IINST
    
    volatile uint32_t HC = float(currentTI.HC_HC)/1000.;
    send( msgKWH.setSensor( CHILD_ID_HC_HC ).set( HC,3 ) ); // HC
    
    volatile uint32_t HP= float(currentTI.HC_HP)/1000.;
    send( msgKWH.setSensor( CHILD_ID_HC_HP ).set( HP,3 ) ); // HP
    
    volatile uint32_t HCprec = float(currentTI.HC_HC);
    send( msgKWH.setSensor( CHILD_ID_HC_HC2 ).set( HCprec ) ); // HC
    
    volatile uint32_t HPprec= float(currentTI.HC_HP);
    send( msgKWH.setSensor( CHILD_ID_HC_HP2 ).set( HPprec ) ); // HP
    
    volatile uint32_t lastPapp= PappRA.getAverage();
    send( msgWATT.setSensor( CHILD_ID_PAPP ).set( lastPapp ) ); // HP
    
    PappRA.clear();
    IinstRA.clear();
  }
  //delay( SLEEP_TIME );
  //gw.sleep( SLEEP_TIME );
}

