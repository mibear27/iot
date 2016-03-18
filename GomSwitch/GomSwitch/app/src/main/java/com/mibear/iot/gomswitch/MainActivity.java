package com.mibear.iot.gomswitch;
//
import org.json.JSONException;
import org.json.JSONObject;
//
import java.io.InputStream;
import java.io.OutputStream;
import java.io.IOException;
import java.util.ArrayList;
import java.util.Calendar;
import java.util.Set;
import java.util.UUID;
import java.util.concurrent.ArrayBlockingQueue;
//
import android.app.AlertDialog;
import android.app.ProgressDialog;
import android.os.Bundle;
import android.os.Build;
import android.os.Handler;
import android.os.Message;
import android.content.ComponentName;
import android.content.Intent;
import android.content.Context;
import android.content.DialogInterface;
import android.content.SharedPreferences;
import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothDevice;
import android.bluetooth.BluetoothSocket;
import android.widget.CompoundButton;
import android.widget.Switch;
import android.util.Log;
//
import android.support.v7.app.AppCompatActivity;

// -----------------------------------------------------------------------
//
// -----------------------------------------------------------------------
public class MainActivity extends AppCompatActivity {
    //
    protected final static String LOG = "mibear";

    //
    protected final static int INTERVAL_STATE = 10000;

    //
    protected final static int CMD_BTON      =  1;
    protected final static int CMD_BTSETTING =  2;
    protected final static int CMD_START     = 11;
    protected final static int CMD_RESTART   = 12;
    protected final static int CMD_GETSTATE  = 21;


    // -----------------------------------------------------------------------
    //
    // -----------------------------------------------------------------------

    //
    protected Context mContext = null;
    protected ProgressDialog mWait = null;

    //
    protected BluetoothAdapter mAdapter = null;
    protected MbBluetooth mMbBt = null;
    protected MbHandler mMbHd = null;
    protected int mCnt = 0;

    //
    protected Switch mSwitch = null;

    // -----------------------------------------------------------------------
    // util
    // -----------------------------------------------------------------------

    protected void mlWait( String msg, DialogInterface.OnCancelListener cancel ) {
        if( mWait != null ) { mWait.dismiss(); mWait = null; }

        if( Build.VERSION.SDK_INT < Build.VERSION_CODES.HONEYCOMB ) { mWait = new ProgressDialog( mContext ); }
        else { mWait = new ProgressDialog( mContext, AlertDialog.THEME_HOLO_LIGHT ); }

        if( Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP ) { mWait = new ProgressDialog( mContext ); }
        else if( Build.VERSION.SDK_INT >= Build.VERSION_CODES.ICE_CREAM_SANDWICH ) { mWait = new ProgressDialog( mContext, AlertDialog.THEME_DEVICE_DEFAULT_LIGHT ); }
        else if( Build.VERSION.SDK_INT < Build.VERSION_CODES.HONEYCOMB ) { mWait = new ProgressDialog( mContext ); }
        else { mWait = new ProgressDialog( mContext, AlertDialog.THEME_HOLO_LIGHT ); }

        mWait.setTitle( "" );
        mWait.setMessage( msg );
        mWait.setCanceledOnTouchOutside( false );

        if( cancel != null ) { mWait.setOnCancelListener( cancel ); }

        mWait.show();
    }

    protected void mlWait( String msg ) {
        mlWait( msg, null );
    }

    public static AlertDialog.Builder getAlert( Context context ) {
        AlertDialog.Builder alert;
        if( Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP ) { alert = new AlertDialog.Builder( context ); }
        else if( Build.VERSION.SDK_INT >= Build.VERSION_CODES.ICE_CREAM_SANDWICH ) { alert = new AlertDialog.Builder( context, AlertDialog.THEME_DEVICE_DEFAULT_LIGHT ); }
        else if( Build.VERSION.SDK_INT < Build.VERSION_CODES.HONEYCOMB ) { alert = new AlertDialog.Builder( context ); }
        else { alert = new AlertDialog.Builder( context, AlertDialog.THEME_HOLO_LIGHT ); }
        return alert;
    }

    public void mlMsg( String title, String msg, DialogInterface.OnClickListener ok, boolean is_cancel ) {
        AlertDialog.Builder alert = getAlert( mContext );
        alert.setPositiveButton( android.R.string.ok, ok );
        alert.setTitle( title );
        alert.setMessage( msg );
        alert.setCancelable( is_cancel );
        alert.show();
    }

    public void mlMsg( String title, String msg, DialogInterface.OnClickListener ok, DialogInterface.OnClickListener cancel, boolean is_cancel ) {
        AlertDialog.Builder alert = getAlert( mContext );
        alert.setTitle( title );
        alert.setMessage( msg );
        alert.setPositiveButton( android.R.string.ok, ok );
        alert.setNegativeButton( android.R.string.cancel, cancel );
        alert.setCancelable( is_cancel );
        alert.show();
    }

    // -----------------------------------------------------------------------
    //
    // -----------------------------------------------------------------------

    @Override
    protected void onCreate( Bundle savedInstanceState ) {
        super.onCreate( savedInstanceState );
        setContentView( R.layout.activity_main );

        //
        mContext = this;

        //
        if( mAdapter == null ) {
            mAdapter = BluetoothAdapter.getDefaultAdapter();
        }

        mSwitch = (Switch)findViewById( R.id.power );
        mSwitch.setOnCheckedChangeListener( new CompoundButton.OnCheckedChangeListener() {
            public void onCheckedChanged( CompoundButton buttonView, boolean isChecked ) {
                if( mMbHd != null ) {
                    mlWait( "설정중입니다", null );
                    mMbHd.setSwitch( isChecked );
                }
            }
        });
    }

    @Override
    protected void onResume() {
        super.onResume();

        // check

        if( mAdapter == null ) {
            mlMsg( "알림", "불루투스 장비 없음 !!!", new DialogInterface.OnClickListener() {
                public void onClick( DialogInterface dialog, int which ) {
                    finish();
                }
            }, false );
            return;
        }

        if( !mAdapter.isEnabled() ) {
            Intent intent = new Intent( BluetoothAdapter.ACTION_REQUEST_ENABLE );
            startActivityForResult( intent, CMD_BTON );
            return;
        }

        BluetoothDevice dev = selectDevice();
        if( dev == null ) {
            findDevice();
            return;
        }

        mHandler.sendMessage( mHandler.obtainMessage( CMD_START ) );
    }

    @Override
    protected void onPause() {
        super.onPause();
        mHandler.removeCallbacksAndMessages( null );
        stopDevice();
    }

    @Override
    public void onActivityResult( int req, int res, Intent data ) {
//        Log.d( LOG, "req : " + req + " / res : " + res );
        switch( req ) {
            case CMD_BTON: {
                if( res == RESULT_OK ) {
                    BluetoothDevice dev = selectDevice();
                    if( dev == null ) { findDevice(); break; }
                    mHandler.sendMessage( mHandler.obtainMessage( CMD_START ) );
                }
                else { finish(); }
                break;
            }
            case CMD_BTSETTING: {
                findDevice();
                break;
            }
        }
    }

    // -----------------------------------------------------------------------
    // bluetooth
    // -----------------------------------------------------------------------

    protected BluetoothDevice selectDevice() {
        SharedPreferences sp = mContext.getSharedPreferences( "user", 0 );
        String addr = sp.getString( "addr", null );

        BluetoothDevice dev = null;
        if( addr != null && addr.length() > 0 ) {
            Set<BluetoothDevice> devices = mAdapter.getBondedDevices();
            for( BluetoothDevice device : devices ) {
                if( device.getAddress().equals( addr ) ) { // "98:D3:31:20:76:C8" ) ) {
                    dev = device;
                    break;
                }
            }
        }
        return dev;
    }

    protected void findDevice() {
        ArrayList<String> l_name = new ArrayList<String>();
        ArrayList<String> l_addr = new ArrayList<String>();

        Set<BluetoothDevice> devices = mAdapter.getBondedDevices();
        for( BluetoothDevice device : devices ) {
            l_name.add( device.getName() );
            l_addr.add( device.getAddress() );
        }
        l_name.add( "등록" );
        l_name.add( "종료" );

        final ArrayList<String> addrs = l_addr;
        AlertDialog.Builder dlg = getAlert( mContext );
        dlg.setTitle( "블루투스 장비를 선택해주세요" );
        dlg.setItems( l_name.toArray( new String[ l_name.size() ] ), new DialogInterface.OnClickListener() {
            public void onClick( DialogInterface dialog, int which ) {
                if( which == addrs.size() ) {
                    Intent intent = new Intent( Intent.ACTION_MAIN );
                    intent.setComponent( new ComponentName( "com.android.settings", "com.android.settings.bluetooth.BluetoothSettings" ) );
                    startActivityForResult( intent, CMD_BTSETTING );
                    return;
                }
                else if( which > addrs.size() ) {
                    finish();
                    return;
                }

                SharedPreferences.Editor spe = mContext.getSharedPreferences( "user", 0 ).edit();
                spe.putString( "addr", addrs.get( which ) );
                spe.commit();

                mHandler.sendMessage( mHandler.obtainMessage( CMD_START ) );
            }
        } );
        dlg.setCancelable( false );
        dlg.show();
    }

    protected void startDevice( BluetoothDevice dev ) {
        stopDevice();

        if( mWait == null ) {
            mlWait( "잠시만 기다려 주세요", new DialogInterface.OnCancelListener() {
                public void onCancel( DialogInterface dialog ) {
                    finish();
                }
            });
        }

        mMbBt = new MbBluetooth( dev, mHandler );
        mMbHd = new MbHandler( mMbBt );

        if( mMbBt.mlGetState() == MbBluetooth.ST_ERROR ) {
            ;
        }
    }

    protected void stopDevice() {
        if( mMbBt != null ) {
            mMbBt.mlStop();
            mMbBt = null;
        }
        mMbHd = null;
    }

    // -----------------------------------------------------------------------
    // handler
    // -----------------------------------------------------------------------

    protected void mlHandle( Message msg ) {
        if( msg.what < CMD_START ) {
            if( mMbHd == null ) { return; }
        }

        switch( msg.what ) {
            case MbBluetooth.ST_CONNECT: {
                Log.d( LOG, "connect !!!" );
                mCnt = 0;
                if( mWait != null ) { mWait.dismiss(); mWait = null; }

                mMbHd.connect();
                break;
            }
            case MbBluetooth.ST_DISCONNECT: {
                Log.d( LOG, "diconnect !!! ( " + mCnt + " )" );
                if( mCnt > 2 ) {
                    mlMsg( "알림", "연결 실패했습니다.", new DialogInterface.OnClickListener() {
                        public void onClick( DialogInterface dialog, int which ) {
                            findDevice();
                        }
                    }, false );
                    break;
                }
                mCnt++;
                mHandler.sendMessageDelayed( mHandler.obtainMessage( CMD_RESTART, 0, 0, null ), 3000 );
                break;
            }
            case MbBluetooth.ST_RECV: {
                if( mWait != null ) { mWait.dismiss(); mWait = null; }

                MbHandler.Response res = mMbHd.parse( ( String )msg.obj );
                if( res == null ) {
                    break;
                }

                switch( res.mCmd ) {
                    case MbHandler.CMD_CONNECT: {
                        mlWait( "시간설정 중입니다.", null );
                        mMbHd.setDateTime();
                        mHandler.sendMessage( mHandler.obtainMessage( CMD_GETSTATE ) );
                        break;
                    }
                    case MbHandler.CMD_DATETIME: {
                        break;
                    }
                    case MbHandler.CMD_STATE: {
                        mSwitch.setChecked( ( res.mInfo1 == 1 ) );
                        break;
                    }
                }
                break;
            }
            case CMD_START: {
                mCnt = 0;
            }
            case CMD_RESTART: {
                startDevice( selectDevice() );
                break;
            }
            case CMD_GETSTATE: {
                if( mMbHd != null ) {
                    mMbHd.getState();
                }
                mHandler.sendMessageDelayed( mHandler.obtainMessage( CMD_GETSTATE ), INTERVAL_STATE );
                break;
            }
        }
    }

    protected Handler mHandler = new Handler() { public void handleMessage( Message msg ) { mlHandle( msg ); } };

    // -----------------------------------------------------------------------
    // MbHandler
    // -----------------------------------------------------------------------
    public static class MbHandler {

        //
        public final static int CMD_STATE    =  1;
        public final static int CMD_SWTICH   =  2;
        public final static int CMD_DATETIME =  3;
        public final static int CMD_SCHEDULE =  4;
        public final static int CMD_CONNECT  = 10;

        //
        protected MbBluetooth mBt = null;
        protected int mSeq = 1;
        protected int mRes = 1;

        //
        public static class Response {
            public int    mCmd    = 0;
            public int    mSeq    = 0;
            public int    mResult = 0;
            public int    mInfo1  = 0;
            public int    mInfo2  = 0;
            public Object mData   = null;
        }

        // -----------------------------------------------------------------------
        //
        // -----------------------------------------------------------------------

        public MbHandler( MbBluetooth bt ) {
            mBt = bt;
        }

        protected void updateSeq() {
            mRes = mSeq++;
            if( mSeq > 99999 ) { mSeq = 1; }
        }

        public boolean connect() {
            JSONObject json = new JSONObject();
            try {
                json.put( "cmd", "connect" );
                json.put( "seq", mSeq );
                updateSeq();
            } catch( JSONException e ) { return false; }
            return mBt.mlCommand( json.toString() );
        }

        public boolean setDateTime() {
            Calendar date = Calendar.getInstance();
            String st = String.format( "%04d%02d%02d%02d%02d%02d",
                date.get( Calendar.YEAR ), date.get( Calendar.MONTH ) + 1, date.get( Calendar.DAY_OF_MONTH ),
                date.get( Calendar.HOUR_OF_DAY ), date.get( Calendar.MINUTE ), date.get( Calendar.SECOND ) );

            JSONObject json = new JSONObject();
            try {
                json.put( "cmd", "datetime" );
                json.put( "seq", mSeq );
                json.put( "datetime", st );
                updateSeq();
            } catch( JSONException e ) { return false; }
            return mBt.mlCommand( json.toString() );
        }

        public boolean setSwitch( boolean v ) {
            JSONObject json = new JSONObject();
            try {
                json.put( "cmd", "switch" );
                json.put( "seq", mSeq );
                json.put( "state", ( v ? 1 : 0 ) );
                updateSeq();
            } catch( JSONException e ) { return false; }
            return mBt.mlCommand( json.toString() );
        }

        public boolean setSchTime( int timemask ) {
            JSONObject json = new JSONObject();
            try {
                json.put( "cmd", "schedule" );
                json.put( "seq", mSeq );
                json.put( "timemask", timemask & 0xFFFFFFFF );
                updateSeq();
            } catch( JSONException e ) { return false; }
            return mBt.mlCommand( json.toString() );
        }

        public boolean getSchTime() {
            JSONObject json = new JSONObject();
            try {
                json.put( "cmd", "schedule" );
                json.put( "seq", mSeq );
                json.put( "timemask", 0xFFFFFFFF );
                updateSeq();
            } catch( JSONException e ) { return false; }
            return mBt.mlCommand( json.toString() );
        }

        public Response parse( String str ) {
            Response res = new Response();
            try {
                JSONObject json = new JSONObject( str );
                String cmd = json.getString( "cmd" );

                if( cmd.equals( "state" ) ) {
                    res.mCmd = CMD_STATE;
                    res.mInfo1 = json.getInt( "state" );
                }
                else if( cmd.equals( "switch" ) ) {
                    res.mCmd = CMD_SWTICH;
                    res.mInfo1 = json.getInt( "state" );
                }
                else if( cmd.equals( "datetime" ) ) {
                    res.mCmd = CMD_DATETIME;
                }
                else if( cmd.equals( "schedule" ) ) {
                    res.mCmd = CMD_SCHEDULE;
                    res.mInfo1 = json.getInt( "timemask" );
                }
                else if( cmd.equals( "connect" ) ) {
                    res.mCmd = CMD_CONNECT;
                }

                res.mSeq = json.getInt( "seq" );
                res.mResult = json.getInt( "result" );
            } catch( JSONException e ) {
                return null;
            }

//            Log.d( LOG, "response cmd : " + res.mCmd + ", seq : " + res.mSeq + ", result : " + res.mResult + " / res : " + mRes );
            if( mRes != res.mSeq ) {
                return null;
            }

            return res;
        }

        // -------------------------------------------------------------------
        // command
        // -------------------------------------------------------------------

        public boolean getState() {
            JSONObject json = new JSONObject();
            try {
                json.put( "cmd", "state" );
                json.put( "seq", mSeq );
                updateSeq();
            } catch( JSONException e ) { return false; }
            return mBt.mlCommand( json.toString() );
        }
    }

    // -----------------------------------------------------------------------
    // MbBluetooth
    // -----------------------------------------------------------------------
    public static class MbBluetooth extends Thread {
        //
        public final static int ST_INIT       = 0;
        public final static int ST_CONNECT    = 1;
        public final static int ST_DISCONNECT = 2;
        public final static int ST_RECV       = 3;
        public final static int ST_SEND       = 4;
        public final static int ST_ERROR      = 5;

        //
        protected final static UUID BT_UUID = UUID.fromString( "00001101-0000-1000-8000-00805F9B34FB" );

        //
        protected BluetoothDevice mDevice = null;
        protected BluetoothSocket mSocket = null;
        protected Handler mHandler = null;
        protected int mState = ST_INIT;
        protected boolean mIsRun = true;

        //
        protected ArrayBlockingQueue<String> mQueue = new ArrayBlockingQueue<String>( 10 );

        //
        public MbBluetooth( BluetoothDevice dev, Handler handler ) {
            mDevice = dev;
            mHandler = handler;

            //
            try {
                mSocket = mDevice.createRfcommSocketToServiceRecord( BT_UUID );
            } catch( Exception e ) { Log.e( LOG, "createRfcommSocketToServiceRecord() failed" ); mSocket = null; mState = ST_ERROR; }

            if( mSocket != null ) {
                setDaemon( true );
                start();
            }
        }

        // -----------------------------------------------------------------------
        // export
        // -----------------------------------------------------------------------

        public int mlGetState() {
            return mState;
        }

        public void mlStop() {
            mIsRun = false;
            if( mSocket != null ) {
                try {
                    mSocket.close();
                } catch( IOException e ) { ; }
            }
        }

        public boolean mlCommand( String cmd ) {
            boolean ret = false;

            try {
                ret = mQueue.add( cmd );
                if( !ret ) {
                    Log.e( LOG, "command put failed !!!" );
                }
            } catch( Exception e ) { ret = false; }
            return ret;
        }

        // -----------------------------------------------------------------------
        // thread
        // -----------------------------------------------------------------------

        public void run() {
            //
            BluetoothSocket socket = mSocket;
            OutputStream os = null;
            InputStream is = null;

            //
            int len;
            byte[] buf = new byte[512];
            String req = "";
            String cmd = "";

            //
            mState = ST_CONNECT;
            while( mIsRun ) {
                switch( mState ) {
                    case ST_CONNECT: {
                        try {
                            socket.connect();
                            is = socket.getInputStream();
                            os = socket.getOutputStream();
                            mState = ST_SEND;
                            mHandler.sendMessage( mHandler.obtainMessage( ST_CONNECT, 0, 0, null ) );
                        } catch( Exception e ) {
                            Log.e( LOG, "connect failed() failed", e );
                            mState = ST_DISCONNECT;
                            break;
                        }
                        break;
                    }
                    case ST_DISCONNECT: {
                        mIsRun = false;
                        break;
                    }
                    case ST_SEND: {
                        try {
                            cmd = mQueue.take();
                            Log.d( LOG, "send : " + cmd );
                        } catch( InterruptedException e ) {
                            Log.e( LOG, "queue", e );
                            break;
                        }

                        try {
                            os.write( cmd.getBytes() );
                        } catch( IOException e ) {
                            Log.e( LOG, "disconnected", e );
                            mState = ST_DISCONNECT;
                            break;
                        }

                        mState = ST_RECV;
                        break;
                    }
                    case ST_RECV: {
                        len = 0;
                        req = "";

                        while( true ) {
                            try {
                                len = is.read( buf );
                                req += new String( buf, 0, len );

                                if( req.indexOf( "\n" ) >= 0 ) {
                                    Log.d( LOG, "recv : " + req );
                                    mHandler.sendMessage( mHandler.obtainMessage( ST_RECV, 0, 0, new String( req ) ) );
                                    break;
                                }
                            } catch( IOException e ) {
                                Log.e( LOG, "disconnected", e );
                                mState = ST_DISCONNECT;
                                break;
                            } catch( NullPointerException e  ) {
                                break;
                            }
                        }

                        if( mState == ST_DISCONNECT ) { break; }
                        mState = ST_SEND;
                        break;
                    }
                }
            }

            if( mSocket != null ) {
                try {
                    mSocket.close();
                } catch( IOException e ) { ; }
            }

            if( mState == ST_DISCONNECT ) {
                mHandler.sendMessage( mHandler.obtainMessage( ST_DISCONNECT, 0, 0, null ) );
            }
        }
    }
}
