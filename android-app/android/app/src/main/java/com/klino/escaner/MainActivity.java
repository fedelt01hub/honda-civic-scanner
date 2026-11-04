package com.klino.escaner;

import android.os.Bundle;
import com.getcapacitor.BridgeActivity;

public class MainActivity extends BridgeActivity {
  @Override
  public void onCreate(Bundle savedInstanceState) {
    super.onCreate(savedInstanceState);
    // Permite que el acelerón VTEC del splash suene sin que el usuario toque la pantalla
    getBridge().getWebView().getSettings().setMediaPlaybackRequiresUserGesture(false);
  }
}
