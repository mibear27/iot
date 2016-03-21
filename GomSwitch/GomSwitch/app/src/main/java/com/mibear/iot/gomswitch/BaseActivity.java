package com.mibear.iot.gomswitch;
//
import android.app.AlertDialog;
import android.app.ProgressDialog;
import android.content.Context;
import android.content.DialogInterface;
import android.os.Build;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
//
import android.support.v7.app.AppCompatActivity;

// -----------------------------------------------------------------------
//
// -----------------------------------------------------------------------
public class BaseActivity extends AppCompatActivity {
    //
    protected final static String LOG = "mibear";


    // -------------------------------------------------------------------
    //
    // -------------------------------------------------------------------

    //
    protected Context mContext = null;
    protected ProgressDialog mWait = null;

    // -------------------------------------------------------------------
    // util
    // -------------------------------------------------------------------

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

    // -------------------------------------------------------------------
    //
    // -------------------------------------------------------------------

    @Override
    protected void onCreate( Bundle savedInstanceState ) {
        super.onCreate( savedInstanceState );
        setContentView( R.layout.activity_main );

        //
        mContext = this;
    }

    @Override
    protected void onPause() {
        super.onPause();
        mHandler.removeCallbacksAndMessages( null );
    }

    // -------------------------------------------------------------------
    // handler
    // -------------------------------------------------------------------

    protected void mlHandle( Message msg ) {
    }

    protected Handler mHandler = new Handler() { public void handleMessage( Message msg ) { mlHandle( msg ); } };
}
