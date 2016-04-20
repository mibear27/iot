#define ST_INIT            1
#define ST_TIMER_ON        2
#define ST_TIMER_OFF       3
#define ST_SCH_ON          4
#define ST_SCH_OFF         5
#define ST_SENSOR_ON       6
#define ST_SENSOR_OFF      7
#define ST_TEST_NORMAL   101
#define ST_TEST_FLASHING 102
#define ST_TEST_BRIGHT   103
#define ST_SYS_ERROR     800

#define MB_TIME  20
#define MB_MAX  120

#define MODE_NORMAL   0
#define MODE_FLASHING 1
#define MODE_BRIGHT   2

#define IDX_R 0
#define IDX_G 1
#define IDX_B 2
#define IDX_T 3

#define MASK_ON( idx )  ( m_iState |= ( 1 << idx ) )
#define MASK_OFF( idx ) ( m_iState &= ~( 1 << idx ) )
#define MASK_IS( idx )  ( m_iState & ( 1 << idx ) )
#define MASK_SET( idx, v ) if( v ) { MASK_ON( idx ); } else { MASK_OFF( idx ); }
#define MASK_TOGGLE( idx ) if( MASK_IS( idx ) ) { MASK_OFF( idx ); } else { MASK_ON( idx ); }

class LedState
{
protected:

  uint8_t  m_iPin[3]; // pin RGB
  uint32_t m_iTime;
  uint8_t  m_iMode;
  uint8_t  m_iState;
  uint8_t  m_iValue;
    
protected:

  void set( boolean r = false, boolean g = false, boolean b = false, int v = 0 ) {
    analogWrite( m_iPin[IDX_R], r ? v : 0 );
    analogWrite( m_iPin[IDX_G], g ? v : 0 );
    analogWrite( m_iPin[IDX_B], b ? v : 0 );
  }
  
  void setMode( int mode, boolean r, boolean g, boolean b, int v ) {
    m_iMode = mode;
    switch( m_iMode ) {
      case MODE_NORMAL: {
        set( r, g, b, v );
        break;
      }
      case MODE_FLASHING: {
        m_iValue = v;
        MASK_SET( IDX_R, r );
        MASK_SET( IDX_G, g );
        MASK_SET( IDX_B, b );
        set( r, g, b, m_iValue );
        break;
      }
      case MODE_BRIGHT: {
        m_iValue = 1;
        MASK_SET( IDX_R, r );
        MASK_SET( IDX_G, g );
        MASK_SET( IDX_B, b );
        set( r, g, b, m_iValue );
        break;
      }
    }
  }
  
public:

  LedState( int r, int g, int b ) {
    m_iPin[IDX_R] = (uint8_t)r;
    m_iPin[IDX_G] = (uint8_t)g;
    m_iPin[IDX_B] = (uint8_t)b;
  }
  
  void setup() {
    m_iMode = MODE_NORMAL;
    m_iState = m_iValue = 0;
    m_iTime = 0;
    
    pinMode( m_iPin[IDX_R], OUTPUT );
    pinMode( m_iPin[IDX_G], OUTPUT );
    pinMode( m_iPin[IDX_B], OUTPUT );
  }
  
  void update() {
    uint32_t ct = millis();
    switch( m_iMode ) {
      case MODE_FLASHING: {
        if( abs( ct - m_iTime ) < 1000 ) { break; }
        m_iTime = ct;
        //
        boolean r = false, g = false, b = false;
        if( !MASK_IS( IDX_T ) ) {
          if( MASK_IS( IDX_R ) ) { r = true; }
          if( MASK_IS( IDX_G ) ) { g = true; }
          if( MASK_IS( IDX_B ) ) { b = true; }
        }
        MASK_TOGGLE( IDX_T );
        set( r, g, b, m_iValue );
        break;
      }
      case MODE_BRIGHT: {
        if( abs( ct - m_iTime ) < MB_TIME ) { break; }
        m_iTime = ct;
        
        boolean r = false, g = false, b = false;
        m_iValue += 1;
        if( m_iValue > MB_MAX ) { m_iValue = 1; }
        //
        if( MASK_IS( IDX_R ) ) { r = true; }
        if( MASK_IS( IDX_G ) ) { g = true; }
        if( MASK_IS( IDX_B ) ) { b = true; }
        set( r, g, b, m_iValue );
        break;
      }
    }
  }
  
  void set( int st ) {
    switch( st ) {
      case ST_INIT: {
        setMode( MODE_FLASHING, true, false, false, 30 );
        break;
      }
      case ST_TIMER_ON: {
        setMode( MODE_FLASHING, false, false, true, 10 );
        break;
      }
      case ST_TIMER_OFF: {
        setMode( MODE_FLASHING, false, true, false, 10 );
        break;
      }
      case ST_SCH_ON: {
        setMode( MODE_NORMAL, false, false, true, 10 );
        break;
      }
      case ST_SCH_OFF: {
        setMode( MODE_NORMAL, true, true, true, 0 );
        break;
      }
      case ST_SENSOR_ON: {
        setMode( MODE_BRIGHT, false, false, true, 1 );
        break;
      }
      case ST_SENSOR_OFF: {
        setMode( MODE_BRIGHT, false, true, false, 1 );
        break;
      }
      case ST_SYS_ERROR: {
        setMode( MODE_FLASHING, true, true, true, 30 );       
        break;
      }
      case ST_TEST_NORMAL: {
        setMode( MODE_NORMAL,false, false, true, 1 );
        break;
      }
      case ST_TEST_FLASHING: {
        setMode( MODE_FLASHING, true, false, true, 1 );
        break;
      }
      case ST_TEST_BRIGHT: {
        setMode( MODE_BRIGHT, true, false, true, 0 );
        break;
      }
    }
  }
};

//
LedState gLed( 11, 10, 9 );

void setup() {
  Serial.begin( 9600 );
  gLed.setup();
}

uint32_t gTime = 99999;
int gTest = ST_INIT;

void loop() {
  gLed.update();

  uint32_t ct = millis();
  if( abs( ct - gTime ) >= 10000 ) {
    gTime = ct;
    int st = gTest;
    switch( gTest ) {
      case ST_INIT: {
        Serial.println( "ST_INIT" );
        gTest = ST_TIMER_ON;
        break;
      }
      case ST_TIMER_ON: {
        Serial.println( "ST_TIMER_ON" );
        gTest = ST_TIMER_OFF;
        break;
      }
      case ST_TIMER_OFF: {
        Serial.println( "ST_TIMER_OFF" );
        gTest = ST_SCH_ON;
        break;
      }
      case ST_SCH_ON: {
        Serial.println( "ST_SCH_ON" );
        gTest = ST_SCH_OFF;
        break;
      }
      case ST_SCH_OFF: {
        Serial.println( "ST_SCH_OFF" );
        gTest = ST_SENSOR_ON;
        break;
      }
      case ST_SENSOR_ON: {
        Serial.println( "ST_SENSOR_ON" );
        gTest = ST_SENSOR_OFF;
        break;
      }
      case ST_SENSOR_OFF: {
        Serial.print( "ST_SENSOR_OFF" );
        gTest = ST_SYS_ERROR;
        break;
      }
      case ST_SYS_ERROR: {
        Serial.println( "ST_SYS_ERROR" );
        gTest = ST_INIT;
        break;
      }
    }
    gLed.set( st );
  }
}
