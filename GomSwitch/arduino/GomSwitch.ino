#include <SoftwareSerial.h>
#include <ArduinoJson.h>
#include <QueueList.h>

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
// 
///////////////////////////////////////////////////////////////////

void setup() {
  Serial.begin( 9600 );
  gCmd.setup();

  //
  pinMode( PIN_SWITCH, OUTPUT );
  digitalWrite( PIN_SWITCH, HIGH );
}

void loop() {
  JsonObject* jo = gCmd.process();
  if( jo != NULL ) {
    const char* str = (*jo)["cmd"];
    String cmd( str );
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
    
    gCmd.release( jo, 0 );
  }
}
