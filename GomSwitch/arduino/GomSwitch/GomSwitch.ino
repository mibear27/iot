//
#include <SoftwareSerial.h>
#include <ArduinoJson.h>
#include <QueueList.h>
//
#include <MsTimer2.h>
#include <TimeLib.h>

///////////////////////////////////////////////////////////////////
// common
///////////////////////////////////////////////////////////////////

#define MAX_BUF 128

///////////////////////////////////////////////////////////////////
// pin
///////////////////////////////////////////////////////////////////

#define PIN_SERIAL_TX 10
#define PIN_SERIAL_RX 11
#define PIN_SWITCH     9

///////////////////////////////////////////////////////////////////
// SerialCommand
///////////////////////////////////////////////////////////////////
class SerialCommand
{
protected:
    SoftwareSerial m_cSerial;
    QueueList<String*> m_qSend;
    StaticJsonBuffer<MAX_BUF>* m_pJb;
    boolean m_isRelease;
    
public:
    SerialCommand( int tx = PIN_SERIAL_TX, int rx = PIN_SERIAL_RX ) :
        m_cSerial( tx, rx ), m_pJb( NULL )
    {
    }
    
    void setup()
    {
      m_cSerial.begin( 9600 );
      m_cSerial.setTimeout( 10 );
      m_isRelease = false;
    }
    
    JsonObject* process()
    {
      if( !m_isRelease ) {
        while( m_cSerial.available() )
        {
            String r = m_cSerial.readStringUntil( '\n' );
            
            int s = r.indexOf( "{" );
            if( s == -1 ) { send( "{\"cmd\":\"err\",\"err\":1}" ); break; }
            
            int e = r.indexOf( "}", s );
            if( e == -1 ) { send( "{\"cmd\":\"err\",\"err\":2}" ); break; }
            
            String json = r.substring( s, e + 1 );

            if( m_pJb ) { delete m_pJb; m_pJb = NULL; }
            m_pJb = new StaticJsonBuffer<MAX_BUF>();
            if( m_pJb == NULL ) { send( "{\"cmd\":\"err\",\"err\":4}" ); break; }
            
            JsonObject* jo = &m_pJb->parseObject( json );
            if( !jo->success() ) { send( "{\"cmd\":\"err\",\"err\":3}" ); break; }
                        
            m_isRelease = true;
            return jo;
        }
      }
      
      while( !m_qSend.isEmpty() )
      {
          String* send = m_qSend.pop();
          if( send )
          {
//            Serial.println( "send : " + *send );
            m_cSerial.print( *send );
            m_cSerial.print( '\n' );
            delete send;
          }
      }

      return NULL;
    }

    void release( JsonObject* js, int result ) {
      char buf[MAX_BUF];
      (*js)["result"] = result;
      js->printTo( buf, MAX_BUF );
      m_qSend.push( new String( buf ) );
      m_isRelease = false;
    }
    
    void send( String cmd )
    {
        m_qSend.push( new String( cmd ) );
    }
};

// 
SerialCommand gCmd;

///////////////////////////////////////////////////////////////////
// Schedule
///////////////////////////////////////////////////////////////////
class Schedule
{
protected:

  unsigned long m_ulStart;
  unsigned long m_ulTime;

public:

  Schedule()
    : m_ulStart( 0 ), m_ulTime( 0 )
  {}

  void set( String st )
  {
    int y = st.substring(  0,  4 ).toInt();
    int m = st.substring(  4,  6 ).toInt();
    int d = st.substring(  6,  8 ).toInt();
    int h = st.substring(  8, 10 ).toInt();
    int n = st.substring( 10, 12 ).toInt();
    int s = st.substring( 12, 14 ).toInt();

    setTime( h, n, s, d, m, y );
    m_ulStart = millis();
  }

  void setSchTime( unsigned long mask )
  {
    m_ulTime = mask;

    for( int i = 0; i < 32; i++ ) {
      if( m_ulTime & ( (unsigned long)1 << i ) ) {
        Serial.println( "ON : " + (String)i );
      }
    }
  }
  
  unsigned long getSchTime() {
    return m_ulTime;
  }

  void checkTime()
  {
    if( m_ulStart == 0 ) { return; }

    // time mask check
    if( m_ulTime > 0 ) {
      int h = hour();
      
      int v = LOW;
      if( m_ulTime & ( (unsigned long)1 << h ) ) { v = HIGH; }

      Serial.println( "check : " + (String)h + ", " + v );

      int s = digitalRead( PIN_SWITCH );
      if( s != v ) {
        digitalWrite( PIN_SWITCH, v );
        Serial.println( "change switch : " + (String)v );
      }
    }
  }
};

Schedule gSch;

///////////////////////////////////////////////////////////////////
// 
///////////////////////////////////////////////////////////////////

void checkTime()
{
  gSch.checkTime();
}

void interruptProcess()
{
  digitalWrite( PIN_SWITCH, HIGH );
}
///////////////////////////////////////////////////////////////////
// 
///////////////////////////////////////////////////////////////////

void setup() {
  Serial.begin( 9600 );
  gCmd.setup();

  //
  pinMode( PIN_SWITCH, OUTPUT );
  digitalWrite( PIN_SWITCH, HIGH );

  //
  attachInterrupt( digitalPinToInterrupt( 2 ), interruptProcess, RISING );

  //
  gSch.setSchTime( (long)1 << 17 );
  
  //
  MsTimer2::set( 1000, checkTime );
  MsTimer2::start();
}

void loop() {
  JsonObject* jo = gCmd.process();
  if( jo != NULL ) {
    const char* str = (*jo)["cmd"];
    String cmd( str );
    int result = 0;
    
    Serial.println( cmd );

    if( cmd.compareTo( "state" ) == 0 ) {
      int state = digitalRead( PIN_SWITCH );
      (*jo)["state"] = state;
    }
    else if( cmd.compareTo( "switch" ) == 0 ) {
      int state = (*jo)["state"];
      if( state > 0 ) { state = HIGH; }
      else { state = LOW; }
      digitalWrite( PIN_SWITCH, state );
    }
    else if( cmd.compareTo( "schedule" ) == 0 ) {
      long mask = (*jo)["timemask"];
      if( mask >= 0 ) { gSch.setSchTime( mask ); }
      else { (*jo)["timemask"] = gSch.getSchTime(); }
    }
    else if( cmd.compareTo( "datetime" ) == 0 ) {
      String s = (*jo)["datetime"];
      gSch.set( s );
    }
    
    gCmd.release( jo, result );
  }
}
