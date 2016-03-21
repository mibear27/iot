//
#include <SoftwareSerial.h>
#include <ArduinoJson.h>
#include <QueueList.h>
//
#include <EEPROM.h>
#include <MsTimer2.h>
#include <TimeLib.h>

///////////////////////////////////////////////////////////////////
// common
///////////////////////////////////////////////////////////////////

#define MAX_BUF 128

typedef unsigned long DWORD;
typedef unsigned int  WORD;
typedef byte          BYTE;

// schedule
const int SCH_MAX_ITEM    = 5;
const int SCH_DEF_DELAY   = 60 * 60; // 1 minute
const int SCH_EEPROM_ADDR = 0;

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
        m_cSerial( tx, rx ), m_pJb( NULL ) {
    }
    
    void setup() {
      m_cSerial.begin( 9600 );
      m_cSerial.setTimeout( 10 );
      m_isRelease = false;
    }
    
    JsonObject* process() {
      if( !m_isRelease ) {
        while( m_cSerial.available() ) {
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
      
      while( !m_qSend.isEmpty() ) {
          String* send = m_qSend.pop();
          if( send ) {
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
    
    void send( String cmd ) {
      m_qSend.push( new String( cmd ) );
    }
};

// 
SerialCommand gCmd;

///////////////////////////////////////////////////////////////////
// switch
///////////////////////////////////////////////////////////////////
void ChangeSwitch( int state ) {
  int cur = digitalRead( PIN_SWITCH );
  if( cur != state ) {
    Serial.println( "#### change state : " + (String)state );
    digitalWrite( PIN_SWITCH, state );
  }
}

///////////////////////////////////////////////////////////////////
// Schedule
///////////////////////////////////////////////////////////////////
class Schedule
{
public:

  typedef struct _item {
    BYTE bType;
    BYTE bWeeks;
    WORD wStart;
    WORD wStop;
  } Item;
  
protected:

  DWORD m_dwStart;

  //
  Item  m_sItem[SCH_MAX_ITEM];
  
  //
  DWORD m_dwDelayTime;
  DWORD m_dwDelay;

public:

  Schedule() : m_dwStart( 0 ), m_dwDelayTime( SCH_DEF_DELAY ), m_dwDelay( 0 ) { 
    memset( m_sItem, 0, sizeof(Item) * SCH_MAX_ITEM );
  }

protected:

  WORD get_cur_time() {
    return (WORD)( ( ( hour() << 8 ) & 0xFF00 ) | ( minute() & 0xFF ) ); 
  }

public:

  static void init() {
    int addr = SCH_EEPROM_ADDR;
    Item item;
    memset( &item, 0, sizeof(Item) );
    for( int i = 0 ; i < SCH_MAX_ITEM; i++, addr += sizeof(Item) ) {
        EEPROM.put( addr, item );
    }
    
    DWORD t = SCH_DEF_DELAY;
    EEPROM.put( addr, t );
  }

  void setup() {
    int addr = SCH_EEPROM_ADDR;
    for( int i = 0; i < SCH_MAX_ITEM; i++, addr += sizeof(Item) ) {
      EEPROM.get( addr, m_sItem[i] );
    }
    EEPROM.get( addr, m_dwDelayTime );
    
    // test
    if( true ) {
        m_sItem[0].bType = 1;
        m_sItem[0].wStart = 0x0004;
        m_sItem[0].wStop = 0x0006;
        Serial.println( "" + (String)m_sItem[0].bType + " " + (String)m_sItem[0].wStart + " " + (String)m_sItem[0].wStop );
    }
    
    Serial.println( "delay time : " + (String)m_dwDelayTime );
  }
  
  void setCurTime( String st ) {
    int y = st.substring(  0,  4 ).toInt();
    int m = st.substring(  4,  6 ).toInt();
    int d = st.substring(  6,  8 ).toInt();
    int h = st.substring(  8, 10 ).toInt();
    int n = st.substring( 10, 12 ).toInt();
    int s = st.substring( 12, 14 ).toInt();
    setTime( h, n, s, d, m, y );
    m_dwStart = millis();
  }
  
  boolean setSchItem( int id, Item& item ) {
    if( id >= SCH_MAX_ITEM ) {
        return false;
    }
    
    memcpy( &m_sItem[id], &item, sizeof(Item) );

    int addr = id * sizeof(Item);
    EEPROM.put( addr, m_sItem[id] );

    return true;
  }

  Item* getSchItem( int id ) {
    if( id >= SCH_MAX_ITEM ) {
        return NULL;
    }

    return &m_sItem[id];
  }

  void clearSchItem() {
    memset( m_sItem, 0, sizeof(Item) * 10 );
    for( int i = 0, addr = SCH_EEPROM_ADDR; i < SCH_MAX_ITEM; i++, addr += sizeof(Item) ) {
      setSchItem( i, m_sItem[i] );
      EEPROM.put( addr, m_sItem[i] );
    }
  }
  
  boolean setDelay( boolean is_enable ) {
    if( is_enable ) { m_dwDelay = m_dwDelayTime; }
    else { m_dwDelay = 0; }
    return true;
  }
  
  boolean setDelayTime( DWORD t ) {
    m_dwDelayTime = t;
    EEPROM.put( sizeof(Item) * SCH_MAX_ITEM, m_dwDelayTime );
  }

  DWORD getDelayTime() {
    return m_dwDelayTime;
  }

  void checkTime() {
    // check, delay
    if( m_dwDelay > 0 ) {
        m_dwDelay--;
        if( m_dwDelay > 0 ) { return; }
        Serial.println( "delay end !!!" );
    }

    // check, schedule
//    if( m_dwStart == 0 || second() != 0 ) { return; }
    if( second() != 0 ) { return; }
    
    // check
    WORD ct = get_cur_time(); 
    Serial.println( ct, HEX );

    boolean is_on = false;
    for( int i = 0; i < 10; i++ ) {
      if( m_sItem[i].bType != 1 ) {
        continue;
      }

      if( m_sItem[i].wStart <= m_sItem[i].wStop ) {
        if( m_sItem[i].wStart <= ct && ct <= m_sItem[i].wStop ) {
          is_on = true;
          break;
        }
      }
      else {
        if( m_sItem[i].wStart <= ct && ct <= 0x173B ) {
          is_on = true;
          break;
        }
        else if( 0 <= ct && ct <= m_sItem[i].wStop ) {
          is_on = true;
          break;
        }
      }
    }

    Serial.println( "state : " + (String)is_on );
    ChangeSwitch( is_on ? HIGH : LOW );
  }
};

Schedule gSch;

///////////////////////////////////////////////////////////////////
// 
///////////////////////////////////////////////////////////////////

void checkTime() {
  gSch.checkTime();
}

volatile boolean gInt = false;
void interruptProcess() {
  gInt = true;
}

///////////////////////////////////////////////////////////////////
// 
///////////////////////////////////////////////////////////////////

void setup() {
//  Schedule::init();
  
  Serial.begin( 9600 );
  gCmd.setup();
  gSch.setup();

  //
  pinMode( PIN_SWITCH, OUTPUT );
  digitalWrite( PIN_SWITCH, HIGH );

  //
//  attachInterrupt( digitalPinToInterrupt( 2 ), interruptProcess, RISING );
  
  //
  MsTimer2::set( 1000, checkTime );
  MsTimer2::start();
}

void loop() {
  JsonObject* pjo = gCmd.process();
  if( pjo != NULL ) {
    JsonObject& jo = *pjo;
    const char* str = jo["cmd"];
    String cmd( str );
    int result = 0;
    
    Serial.println( cmd );

    if( cmd.compareTo( "state" ) == 0 ) {
      int state = digitalRead( PIN_SWITCH );
      jo["state"] = state;
    }
    else if( cmd.compareTo( "switch" ) == 0 ) {
      int state = jo["state"];
      if( state > 0 ) { state = HIGH; }
      else { state = LOW; }
      digitalWrite( PIN_SWITCH, state );
      gSch.setDelay( true );
    }
    else if( cmd.compareTo( "schedule" ) == 0 ) {
      int id = jo["id"];
      if( id < SCH_MAX_ITEM ) {
        Schedule::Item item;
        memset( &item, 0, sizeof(Schedule::Item) );

        if( jo.containsKey( "type" ) && jo.containsKey( "start" ) && jo.containsKey( "stop" ) ) {
          item.bType = jo["type"];
          item.wStart = jo["start"];
          item.wStop = jo["stop"];
          //
          boolean ck = true;;
          if( item.wStart > 0x1700 ||( item.wStart & 0xFF ) > 0x3B ) {
              ck = false;
          }
          if( item.wStop > 0x1700 || ( item.wStop & 0xFF ) > 0x3B ) {
              ck = false;
          }

          Serial.println( "" + (String)item.bType + " / start : " + (String)item.wStart + " / stop : " + (String)item.wStop );
          
          if( ck ) { gSch.setSchItem( id, item ); }
          else { result = 1; }
        }
        else {
          Schedule::Item* p_item = gSch.getSchItem( id );
          jo["type"] = p_item->bType;
          jo["start"] = p_item->wStart;
          jo["stop"] = p_item->wStop;
        }
      }
      else { result = 1; }
    }
    else if( cmd.compareTo( "delay" ) == 0 ) {
      if( jo.containsKey( "time" ) ) {
          DWORD t = jo["time"];
          if( t > 0 ) { gSch.setDelayTime( t ); }
          else { result = 1; }
      }
      else { jo["time"] = gSch.getDelayTime(); }
      
      if( jo.containsKey( "cancle" ) ) {
          gSch.setDelay( false );
      }
    }
    else if( cmd.compareTo( "datetime" ) == 0 ) {
      String s = jo["datetime"];
      gSch.setCurTime( s );
    }
    else if( cmd.compareTo( "factory" ) == 0 ) {
      if( jo.containsKey( "reset" ) ) {
        Schedule::init();
      }
    }
    
    gCmd.release( pjo, result );
  }

  if( gInt ) {
    gInt = false;
    gSch.setDelay( true );
    ChangeSwitch( HIGH );
  }
}
