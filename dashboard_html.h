#ifndef DASHBOARD_HTML_H
#define DASHBOARD_HTML_H

#include <Arduino.h>

const char DASHBOARD_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="es">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0, maximum-scale=1.0, user-scalable=no">
  <title>Honda Civic '99 - Live Scanner</title>
  <style>
    :root {
      --bg-dark: #0a0d14;
      --card-bg: rgba(18, 24, 38, 0.85);
      --card-border: rgba(255, 255, 255, 0.08);
      --accent-cyan: #00f0ff;
      --accent-orange: #ff9900;
      --accent-red: #ff3366;
      --accent-green: #00ff88;
      --text-main: #f0f4fc;
      --text-muted: #7c8ba1;
    }
    * {
      box-sizing: border-box;
      margin: 0;
      padding: 0;
      font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, Helvetica, Arial, sans-serif;
      -webkit-tap-highlight-color: transparent;
    }
    body {
      background: radial-gradient(circle at 50% 10%, #151d2f 0%, var(--bg-dark) 100%);
      color: var(--text-main);
      min-height: 100vh;
      display: flex;
      flex-direction: column;
      overflow-x: hidden;
    }
    header {
      padding: 12px 18px;
      display: flex;
      justify-content: space-between;
      align-items: center;
      background: rgba(10, 13, 20, 0.85);
      backdrop-filter: blur(12px);
      border-bottom: 1px solid var(--card-border);
      position: sticky;
      top: 0;
      z-index: 100;
    }
    .brand {
      display: flex;
      align-items: center;
      gap: 10px;
    }
    .brand-icon {
      width: 28px;
      height: 28px;
      background: linear-gradient(135deg, #e60000, #990000);
      border-radius: 6px;
      display: flex;
      align-items: center;
      justify-content: center;
      font-weight: 900;
      font-size: 16px;
      box-shadow: 0 0 12px rgba(230,0,0,0.5);
    }
    .brand-title {
      font-size: 15px;
      font-weight: 700;
      letter-spacing: 0.5px;
      text-transform: uppercase;
    }
    .brand-subtitle {
      font-size: 10px;
      color: var(--text-muted);
      letter-spacing: 1px;
    }
    .status-badge {
      display: flex;
      align-items: center;
      gap: 6px;
      background: rgba(0, 240, 255, 0.1);
      border: 1px solid rgba(0, 240, 255, 0.3);
      padding: 5px 12px;
      border-radius: 20px;
      font-size: 11px;
      font-weight: 600;
      color: var(--accent-cyan);
      transition: all 0.3s;
    }
    .status-dot {
      width: 8px;
      height: 8px;
      border-radius: 50%;
      background: var(--accent-cyan);
      box-shadow: 0 0 8px var(--accent-cyan);
      animation: pulse 1.5s infinite;
    }
    .status-badge.error {
      background: rgba(255, 51, 102, 0.1);
      border-color: rgba(255, 51, 102, 0.3);
      color: var(--accent-red);
    }
    .status-badge.error .status-dot {
      background: var(--accent-red);
      box-shadow: 0 0 8px var(--accent-red);
    }
    @keyframes pulse {
      0%, 100% { opacity: 1; transform: scale(1); }
      50% { opacity: 0.4; transform: scale(0.85); }
    }
    nav {
      display: flex;
      background: rgba(18, 24, 38, 0.5);
      border-bottom: 1px solid var(--card-border);
      padding: 6px;
      gap: 8px;
    }
    .nav-btn {
      flex: 1;
      padding: 10px;
      background: transparent;
      border: none;
      border-radius: 8px;
      color: var(--text-muted);
      font-weight: 600;
      font-size: 13px;
      cursor: pointer;
      display: flex;
      justify-content: center;
      align-items: center;
      gap: 6px;
      transition: all 0.2s;
    }
    .nav-btn.active {
      background: rgba(0, 240, 255, 0.15);
      color: var(--accent-cyan);
      border: 1px solid rgba(0, 240, 255, 0.3);
    }
    main {
      padding: 16px;
      flex: 1;
      max-width: 900px;
      margin: 0 auto;
      width: 100%;
    }
    .tab-content {
      display: none;
    }
    .tab-content.active {
      display: block;
    }
    .gauges-grid {
      display: grid;
      grid-template-columns: 1fr 1fr;
      gap: 16px;
      margin-bottom: 16px;
    }
    @media (max-width: 640px) {
      .gauges-grid {
        grid-template-columns: 1fr;
      }
    }
    .card {
      background: var(--card-bg);
      border: 1px solid var(--card-border);
      border-radius: 16px;
      padding: 16px;
      backdrop-filter: blur(10px);
      box-shadow: 0 8px 24px rgba(0, 0, 0, 0.3);
    }
    .gauge-card {
      display: flex;
      flex-direction: column;
      align-items: center;
      position: relative;
    }
    .gauge-svg {
      width: 220px;
      height: 180px;
    }
    .gauge-bg {
      fill: none;
      stroke: rgba(255, 255, 255, 0.08);
      stroke-width: 14;
      stroke-linecap: round;
    }
    .gauge-fill {
      fill: none;
      stroke: url(#cyan-grad);
      stroke-width: 14;
      stroke-linecap: round;
      stroke-dasharray: 440;
      stroke-dashoffset: 440;
      transition: stroke-dashoffset 0.15s ease-out;
    }
    .gauge-value-text {
      font-size: 44px;
      font-weight: 800;
      letter-spacing: -1px;
      fill: var(--text-main);
      text-anchor: middle;
    }
    .gauge-unit-text {
      font-size: 14px;
      font-weight: 600;
      fill: var(--text-muted);
      text-anchor: middle;
      text-transform: uppercase;
    }
    .gauge-title {
      font-size: 13px;
      font-weight: 600;
      color: var(--text-muted);
      text-transform: uppercase;
      letter-spacing: 1px;
      margin-top: -10px;
    }
    .mini-grid {
      display: grid;
      grid-template-columns: repeat(auto-fit, minmax(130px, 1fr));
      gap: 12px;
      margin-bottom: 16px;
    }
    .mini-card {
      background: var(--card-bg);
      border: 1px solid var(--card-border);
      border-radius: 12px;
      padding: 12px;
      display: flex;
      flex-direction: column;
    }
    .mini-label {
      font-size: 11px;
      color: var(--text-muted);
      font-weight: 600;
      text-transform: uppercase;
      margin-bottom: 6px;
    }
    .mini-value {
      font-size: 22px;
      font-weight: 800;
      color: var(--text-main);
    }
    .mini-unit {
      font-size: 12px;
      font-weight: 500;
      color: var(--text-muted);
      margin-left: 2px;
    }
    .bar-container {
      width: 100%;
      height: 6px;
      background: rgba(255, 255, 255, 0.1);
      border-radius: 3px;
      margin-top: 8px;
      overflow: hidden;
    }
    .bar-fill {
      height: 100%;
      width: 0%;
      background: var(--accent-cyan);
      border-radius: 3px;
      transition: width 0.2s, background 0.3s;
    }
    .temp-cold { background: #3b82f6 !important; }
    .temp-ok { background: #10b981 !important; }
    .temp-hot { background: #ef4444 !important; }

    /* Diagnostic tab */
    .cel-banner {
      display: flex;
      align-items: center;
      gap: 16px;
      padding: 16px;
      border-radius: 12px;
      margin-bottom: 16px;
      border: 1px solid rgba(255, 255, 255, 0.1);
    }
    .cel-banner.no-faults {
      background: rgba(0, 255, 136, 0.08);
      border-color: rgba(0, 255, 136, 0.2);
    }
    .cel-banner.has-faults {
      background: rgba(255, 51, 102, 0.15);
      border-color: rgba(255, 51, 102, 0.4);
    }
    .cel-lamp {
      width: 48px;
      height: 48px;
      border-radius: 10px;
      display: flex;
      align-items: center;
      justify-content: center;
      font-weight: 900;
      font-size: 11px;
      text-transform: uppercase;
      background: #2a3447;
      color: #6b7280;
      transition: all 0.3s;
    }
    .cel-banner.has-faults .cel-lamp {
      background: #ff9900;
      color: #000;
      box-shadow: 0 0 20px rgba(255, 153, 0, 0.8);
      animation: pulse 1s infinite;
    }
    .cel-info h3 {
      font-size: 16px;
      font-weight: 700;
      margin-bottom: 2px;
    }
    .cel-info p {
      font-size: 12px;
      color: var(--text-muted);
    }
    .dtc-list {
      display: flex;
      flex-direction: column;
      gap: 10px;
      margin-bottom: 20px;
    }
    .dtc-item {
      background: rgba(255, 255, 255, 0.03);
      border: 1px solid var(--card-border);
      border-left: 4px solid var(--accent-orange);
      padding: 12px 16px;
      border-radius: 8px;
    }
    .dtc-header {
      display: flex;
      justify-content: space-between;
      align-items: center;
      margin-bottom: 4px;
    }
    .dtc-code {
      font-weight: 800;
      font-size: 15px;
      color: var(--accent-orange);
    }
    .dtc-name {
      font-weight: 600;
      font-size: 14px;
    }
    .dtc-desc {
      font-size: 12px;
      color: var(--text-muted);
      margin-top: 4px;
    }
    .action-row {
      display: flex;
      gap: 12px;
      flex-wrap: wrap;
    }
    .btn {
      flex: 1;
      min-width: 160px;
      padding: 14px 20px;
      border-radius: 10px;
      font-weight: 700;
      font-size: 13px;
      text-transform: uppercase;
      letter-spacing: 0.5px;
      border: none;
      cursor: pointer;
      display: flex;
      align-items: center;
      justify-content: center;
      gap: 8px;
      transition: all 0.2s;
    }
    .btn-primary {
      background: linear-gradient(135deg, #00b4db, #0083b0);
      color: #fff;
      box-shadow: 0 4px 14px rgba(0, 180, 219, 0.3);
    }
    .btn-danger {
      background: linear-gradient(135deg, #ff416c, #ff4b2b);
      color: #fff;
      box-shadow: 0 4px 14px rgba(255, 65, 108, 0.3);
    }
    .btn:active {
      transform: scale(0.98);
    }
    .control-row {
      display: flex;
      justify-content: space-between;
      align-items: center;
      margin-top: 16px;
      padding-top: 16px;
      border-top: 1px solid var(--card-border);
      font-size: 13px;
      color: var(--text-muted);
    }
    /* Switch */
    .switch {
      position: relative;
      display: inline-block;
      width: 44px;
      height: 24px;
    }
    .switch input { opacity: 0; width: 0; height: 0; }
    .slider {
      position: absolute; cursor: pointer; top: 0; left: 0; right: 0; bottom: 0;
      background-color: #2b3345; transition: .3s; border-radius: 24px;
    }
    .slider:before {
      position: absolute; content: ""; height: 18px; width: 18px; left: 3px; bottom: 3px;
      background-color: white; transition: .3s; border-radius: 50%;
    }
    input:checked + .slider { background-color: var(--accent-cyan); }
    input:checked + .slider:before { transform: translateX(20px); }

    /* Dashboard Sections & Indicators */
    .section-header {
      font-size: 12px;
      font-weight: 800;
      color: var(--accent-cyan);
      text-transform: uppercase;
      letter-spacing: 1px;
      margin: 20px 0 10px 4px;
      display: flex;
      align-items: center;
      gap: 8px;
    }
    .section-header::after {
      content: '';
      flex: 1;
      height: 1px;
      background: var(--card-border);
    }
    .status-strip {
      display: flex;
      gap: 8px;
      flex-wrap: wrap;
      justify-content: center;
      margin-bottom: 16px;
    }
    .status-pill {
      display: inline-flex;
      align-items: center;
      gap: 6px;
      padding: 6px 12px;
      border-radius: 20px;
      font-size: 11px;
      font-weight: 700;
      background: rgba(255, 255, 255, 0.04);
      color: var(--text-muted);
      border: 1px solid var(--card-border);
      transition: all 0.25s;
    }
    .status-pill.active {
      background: rgba(0, 240, 255, 0.15);
      color: var(--accent-cyan);
      border-color: rgba(0, 240, 255, 0.4);
      box-shadow: 0 0 10px rgba(0, 240, 255, 0.2);
    }
    .vtec-pill {
      font-size: 12px;
      letter-spacing: 1px;
    }
    .vtec-pill.active {
      background: linear-gradient(135deg, #00ff88, #00b4db) !important;
      color: #000 !important;
      border-color: #00ff88 !important;
      box-shadow: 0 0 18px rgba(0, 255, 136, 0.8) !important;
      animation: pulse 0.8s infinite;
    }
    .o2-lean { background: #3b82f6 !important; }
    .o2-stoich { background: #10b981 !important; }
    .o2-rich { background: #ff9900 !important; }

    /* Console tab */
    .verdict {
      display: flex; gap: 14px; align-items: flex-start;
      padding: 16px; border-radius: 14px; margin-bottom: 16px;
      border: 1px solid var(--card-border); background: var(--card-bg);
      transition: all 0.3s;
    }
    .verdict-icon {
      min-width: 44px; height: 44px; border-radius: 12px;
      display: flex; align-items: center; justify-content: center;
      font-size: 22px; font-weight: 900; background: #2a3447;
    }
    .verdict h3 { font-size: 15px; margin-bottom: 4px; }
    .verdict p { font-size: 12px; color: var(--text-muted); line-height: 1.5; }
    .verdict.ok   { border-color: rgba(0,255,136,0.35); background: rgba(0,255,136,0.07); }
    .verdict.ok .verdict-icon   { background: rgba(0,255,136,0.2); color: var(--accent-green); }
    .verdict.warn { border-color: rgba(255,153,0,0.4); background: rgba(255,153,0,0.08); }
    .verdict.warn .verdict-icon { background: rgba(255,153,0,0.2); color: var(--accent-orange); }
    .verdict.bad  { border-color: rgba(255,51,102,0.4); background: rgba(255,51,102,0.08); }
    .verdict.bad .verdict-icon  { background: rgba(255,51,102,0.2); color: var(--accent-red); }
    .steps { display: grid; grid-template-columns: repeat(3, 1fr); gap: 8px; margin-bottom: 16px; }
    .step {
      background: var(--card-bg); border: 1px solid var(--card-border); border-radius: 12px;
      padding: 10px; text-align: center; font-size: 11px; color: var(--text-muted);
      text-transform: uppercase; font-weight: 600; letter-spacing: 0.5px;
    }
    .step b { display: block; font-size: 20px; margin-bottom: 4px; color: #4b5563; }
    .step.ok b { color: var(--accent-green); }
    .step.bad b { color: var(--accent-red); }
    .step.warn b { color: var(--accent-orange); }
    .stat-grid { display: grid; grid-template-columns: repeat(auto-fit, minmax(105px, 1fr)); gap: 8px; margin-bottom: 16px; }
    .stat { background: rgba(255,255,255,0.03); border: 1px solid var(--card-border); border-radius: 10px; padding: 8px 10px; }
    .stat span { display: block; font-size: 10px; color: var(--text-muted); text-transform: uppercase; font-weight: 600; }
    .stat b { font-size: 18px; font-variant-numeric: tabular-nums; }
    .hexline {
      font-family: ui-monospace, Menlo, Consolas, monospace; font-size: 12px;
      background: #05070b; border: 1px solid var(--card-border); border-radius: 8px;
      padding: 8px 10px; color: var(--accent-cyan); word-break: break-all; margin-bottom: 8px;
    }
    .console {
      font-family: ui-monospace, Menlo, Consolas, monospace; font-size: 11px; line-height: 1.55;
      background: #05070b; border: 1px solid var(--card-border); border-radius: 10px;
      padding: 10px; height: 320px; overflow-y: auto; white-space: pre-wrap; word-break: break-all;
      color: #b6c2d6; margin-bottom: 12px;
    }
    .console .l-ok { color: var(--accent-green); }
    .console .l-bad { color: var(--accent-red); }
    .console .l-warn { color: var(--accent-orange); }
    .console .l-hex { color: var(--accent-cyan); }
    .btn-ghost { background: rgba(255,255,255,0.06); color: var(--text-main); border: 1px solid var(--card-border); }
    .btn-small { min-width: 0; padding: 10px 12px; font-size: 11px; }
  </style>
</head>
<body>

  <header>
    <div class="brand">
      <div class="brand-icon">H</div>
      <div>
        <div class="brand-title">Civic '99 Scanner</div>
        <div class="brand-subtitle">PGM-FI DLC 3-PIN &bull; ESP32-C3</div>
      </div>
    </div>
    <div id="statusBadge" class="status-badge">
      <div class="status-dot"></div>
      <span id="statusText">Conectando...</span>
    </div>
  </header>

  <nav>
    <button class="nav-btn active" onclick="switchTab('gauges')">
      <svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><circle cx="12" cy="12" r="10"/><path d="M16.2 7.8l-4.2 4.2"/></svg>
      Instrumentos
    </button>
    <button class="nav-btn" onclick="switchTab('diag')">
      <svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><path d="M10.29 3.86L1.82 18a2 2 0 0 0 1.71 3h16.94a2 2 0 0 0 1.71-3L13.71 3.86a2 2 0 0 0-3.42 0z"/><line x1="12" y1="9" x2="12" y2="13"/><line x1="12" y1="17" x2="12.01" y2="17"/></svg>
      Diagnóstico (DTC)
    </button>
    <button class="nav-btn" onclick="switchTab('console')">
      <svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><polyline points="4 17 10 11 4 5"/><line x1="12" y1="19" x2="20" y2="19"/></svg>
      Consola
    </button>
  </nav>

  <main>
    <!-- TAB 1: GAUGES -->
    <div id="tabGauges" class="tab-content active">
      <!-- STATUS PILLS STRIP -->
      <div class="status-strip">
        <div id="pillVtec" class="status-pill vtec-pill">&#9889; VTEC</div>
        <div id="pillGear" class="status-pill">MARCHA: <b id="valGear">--</b></div>
        <div id="pillPn" class="status-pill">P/N: <b id="valPn">--</b></div>
        <div id="pillAc" class="status-pill">A/C: <b id="valAc">OFF</b></div>
        <div id="pillBrake" class="status-pill">FRENO: <b id="valBrake">OFF</b></div>
      </div>

      <div class="gauges-grid">
        <!-- RPM GAUGE -->
        <div class="card gauge-card">
          <svg class="gauge-svg" viewBox="0 0 200 160">
            <defs>
              <linearGradient id="cyan-grad" x1="0%" y1="0%" x2="100%" y2="0%">
                <stop offset="0%" stop-color="#00f0ff" />
                <stop offset="70%" stop-color="#0083b0" />
                <stop offset="100%" stop-color="#ff3366" />
              </linearGradient>
            </defs>
            <path class="gauge-bg" d="M 25 140 A 75 75 0 1 1 175 140" />
            <path id="rpmPath" class="gauge-fill" d="M 25 140 A 75 75 0 1 1 175 140" />
            <text id="rpmValue" class="gauge-value-text" x="100" y="115">0</text>
            <text class="gauge-unit-text" x="100" y="135">RPM</text>
          </svg>
          <div class="gauge-title">Tacómetro de Motor</div>
        </div>

        <!-- SPEED GAUGE -->
        <div class="card gauge-card">
          <svg class="gauge-svg" viewBox="0 0 200 160">
            <path class="gauge-bg" d="M 25 140 A 75 75 0 1 1 175 140" />
            <path id="speedPath" class="gauge-fill" d="M 25 140 A 75 75 0 1 1 175 140" />
            <text id="speedValue" class="gauge-value-text" x="100" y="115">0</text>
            <text class="gauge-unit-text" x="100" y="135">KM/H</text>
          </svg>
          <div class="gauge-title">Velocidad (VSS)</div>
        </div>
      </div>

      <!-- SECCIÓN A: MOTOR Y ESTADOS DISCRETOS -->
      <div class="section-header">Sección A &bull; Motor y Estados Discretos</div>
      <div class="mini-grid">
        <div class="mini-card">
          <div class="mini-label">Solenoide VTEC</div>
          <div class="mini-value" id="aVtecVal" style="font-size:17px;">INACTIVO</div>
          <div class="bar-container"><div id="aVtecBar" class="bar-fill"></div></div>
        </div>
        <div class="mini-card">
          <div class="mini-label">Marcha Transmisión</div>
          <div class="mini-value" id="aGearVal">--</div>
          <div class="bar-container"><div id="aGearBar" class="bar-fill" style="width:100%; background:var(--accent-cyan);"></div></div>
        </div>
        <div class="mini-card">
          <div class="mini-label">Clutch A/C</div>
          <div class="mini-value" id="aAcVal" style="font-size:17px;">APAGADO</div>
          <div class="bar-container"><div id="aAcBar" class="bar-fill"></div></div>
        </div>
        <div class="mini-card">
          <div class="mini-label">Interruptor Freno</div>
          <div class="mini-value" id="aBrakeVal" style="font-size:17px;">LIBRE</div>
          <div class="bar-container"><div id="aBrakeBar" class="bar-fill"></div></div>
        </div>
        <div class="mini-card">
          <div class="mini-label">Knock / Detonación</div>
          <div class="mini-value"><span id="aKnockVal">0</span><span class="mini-unit">/ 5</span></div>
          <div class="bar-container"><div id="aKnockBar" class="bar-fill"></div></div>
        </div>
      </div>

      <!-- SECCIÓN B: ADMISIÓN, ESCAPE Y ELÉCTRICOS -->
      <div class="section-header">Sección B &bull; Admisión, Escape y Eléctricos</div>
      <div class="mini-grid">
        <div class="mini-card">
          <div class="mini-label">Refrigerante (ECT)</div>
          <div class="mini-value"><span id="ectValue">--</span><span class="mini-unit">&deg;C</span></div>
          <div class="bar-container"><div id="ectBar" class="bar-fill"></div></div>
        </div>
        <div class="mini-card">
          <div class="mini-label">Aire Admisión (IAT)</div>
          <div class="mini-value"><span id="iatValue">--</span><span class="mini-unit">&deg;C</span></div>
          <div class="bar-container"><div id="iatBar" class="bar-fill"></div></div>
        </div>
        <div class="mini-card">
          <div class="mini-label">Presión MAP (kPa)</div>
          <div class="mini-value"><span id="mapValue">--</span><span class="mini-unit">kPa</span></div>
          <div class="bar-container"><div id="mapBar" class="bar-fill"></div></div>
        </div>
        <div class="mini-card">
          <div class="mini-label">Vacío Múltiple (inHg)</div>
          <div class="mini-value"><span id="mapInHgVal">--</span><span class="mini-unit">inHg</span></div>
          <div class="bar-container"><div id="mapInHgBar" class="bar-fill" style="background:#3b82f6;"></div></div>
        </div>
        <div class="mini-card">
          <div class="mini-label">Presión BARO (Exterior)</div>
          <div class="mini-value"><span id="baroVal">--</span><span class="mini-unit">kPa</span></div>
          <div class="bar-container"><div id="baroBar" class="bar-fill" style="width:100%; background:#6366f1;"></div></div>
        </div>
        <div class="mini-card">
          <div class="mini-label">Acelerador (TPS)</div>
          <div class="mini-value"><span id="tpsValue">--</span><span class="mini-unit">%</span></div>
          <div class="bar-container"><div id="tpsBar" class="bar-fill"></div></div>
        </div>
        <div class="mini-card">
          <div class="mini-label">Sensor Oxígeno (O2)</div>
          <div class="mini-value"><span id="o2Val">--</span><span class="mini-unit">V</span></div>
          <div class="bar-container"><div id="o2Bar" class="bar-fill"></div></div>
        </div>
        <div class="mini-card">
          <div class="mini-label">Batería ECU</div>
          <div class="mini-value"><span id="battValue">--</span><span class="mini-unit">V</span></div>
          <div class="bar-container"><div id="battBar" class="bar-fill"></div></div>
        </div>
        <div class="mini-card">
          <div class="mini-label">Carga Alternador (ALT)</div>
          <div class="mini-value"><span id="altVal">--</span><span class="mini-unit">%</span></div>
          <div class="bar-container"><div id="altBar" class="bar-fill"></div></div>
        </div>
        <div class="mini-card">
          <div class="mini-label">Carga Eléctrica (ELD)</div>
          <div class="mini-value"><span id="eldVal">--</span><span class="mini-unit">A</span></div>
          <div class="bar-container"><div id="eldBar" class="bar-fill" style="background:#00f0ff;"></div></div>
        </div>
      </div>

      <!-- SECCIÓN C: INYECCIÓN, COMBUSTIBLE Y ENCENDIDO -->
      <div class="section-header">Sección C &bull; Inyección, Combustible y Encendido</div>
      <div class="mini-grid">
        <div class="mini-card">
          <div class="mini-label">Avance Chispa (IGN)</div>
          <div class="mini-value"><span id="ignVal">--</span><span class="mini-unit">&deg;BTDC</span></div>
          <div class="bar-container"><div id="ignBar" class="bar-fill" style="background:#f59e0b;"></div></div>
        </div>
        <div class="mini-card">
          <div class="mini-label">Tiempo Inyección (INJ)</div>
          <div class="mini-value"><span id="injVal">--</span><span class="mini-unit">ms</span></div>
          <div class="bar-container"><div id="injBar" class="bar-fill" style="background:#ec4899;"></div></div>
        </div>
        <div class="mini-card">
          <div class="mini-label">Trim Corto (STFT)</div>
          <div class="mini-value"><span id="sftVal">--</span><span class="mini-unit">%</span></div>
          <div class="bar-container"><div id="sftBar" class="bar-fill"></div></div>
        </div>
        <div class="mini-card">
          <div class="mini-label">Trim Largo (LTFT)</div>
          <div class="mini-value"><span id="lftVal">--</span><span class="mini-unit">%</span></div>
          <div class="bar-container"><div id="lftBar" class="bar-fill"></div></div>
        </div>
        <div class="mini-card">
          <div class="mini-label">Válvula Ralentí (IACV)</div>
          <div class="mini-value"><span id="iacvVal">--</span><span class="mini-unit">%</span></div>
          <div class="bar-container"><div id="iacvBar" class="bar-fill"></div></div>
        </div>
        <div class="mini-card">
          <div class="mini-label">Flujo Aire Estimado (MAF)</div>
          <div class="mini-value"><span id="mafVal">--</span><span class="mini-unit">g/s</span></div>
          <div class="bar-container"><div id="mafBar" class="bar-fill" style="background:#10b981;"></div></div>
        </div>
        <div class="mini-card">
          <div class="mini-label">Frecuencia K-Line</div>
          <div class="mini-value"><span id="hzValue">--</span><span class="mini-unit">Hz</span></div>
          <div class="bar-container"><div style="width: 100%; background: var(--accent-cyan);" class="bar-fill"></div></div>
        </div>
      </div>
    </div>

    <!-- TAB 2: DIAGNOSTIC / DTCS -->
    <div id="tabDiag" class="tab-content">
      <div id="celBanner" class="cel-banner no-faults">
        <div id="celLamp" class="cel-lamp">CHECK</div>
        <div class="cel-info">
          <h3 id="celStatusTitle">Sin Códigos de Falla</h3>
          <p id="celStatusDesc">El sistema PGM-FI no reporta anomalías activas.</p>
        </div>
      </div>

      <div class="card" style="margin-bottom: 16px;">
        <h4 style="margin-bottom: 12px; font-size: 14px; text-transform: uppercase; color: var(--text-muted);">Códigos Almacenados en ECU</h4>
        <div id="dtcContainer" class="dtc-list">
          <div style="color: var(--text-muted); font-size: 13px; text-align: center; padding: 20px;">
            Presiona "Escanear Códigos" para interrogar la memoria del ECU.
          </div>
        </div>

        <div class="action-row">
          <button class="btn btn-primary" onclick="readDTCs()">
            <svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><path d="M21.5 2v6h-6M21.34 15.57a10 10 0 1 1-.57-8.38l5.67-5.67"/></svg>
            Escanear Códigos
          </button>
          <button class="btn btn-danger" onclick="confirmClearDTCs()">
            <svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><polyline points="3 6 5 6 21 6"/><path d="M19 6v14a2 2 0 0 1-2 2H7a2 2 0 0 1-2-2V6m3 0V4a2 2 0 0 1 2-2h4a2 2 0 0 1 2 2v2"/></svg>
            Borrar Códigos (Reset CEL)
          </button>
        </div>
      </div>

      <div class="card">
        <h4 style="margin-bottom: 8px; font-size: 14px; text-transform: uppercase; color: var(--text-muted);">Herramientas de Conexión</h4>
        <div class="action-row" style="margin-top: 8px;">
          <button class="btn" style="background: rgba(255,255,255,0.06); color: var(--text-main);" onclick="reinitKLine()">
            Re-inicializar K-Line (Wake-up)
          </button>
        </div>
        <div class="control-row">
          <span>Modo Demo (Simulación de sensores sin auto)</span>
          <label class="switch">
            <input type="checkbox" id="demoSwitch" onchange="toggleDemo(this.checked)">
            <span class="slider"></span>
          </label>
        </div>
      </div>
    </div>

    <!-- TAB 3: CONSOLA K-LINE -->
    <div id="tabConsole" class="tab-content">
      <div id="verdict" class="verdict">
        <div id="verdictIcon" class="verdict-icon">?</div>
        <div>
          <h3 id="verdictTitle">Analizando...</h3>
          <p id="verdictDesc">Esperando datos del ESP32.</p>
        </div>
      </div>

      <div class="steps">
        <div id="stepTx" class="step"><b>&#9679;</b>ESP32 &rarr; Línea</div>
        <div id="stepEcho" class="step"><b>&#9679;</b>Eco L9637D</div>
        <div id="stepEcu" class="step"><b>&#9679;</b>Respuesta ECU</div>
      </div>

      <div class="card" style="margin-bottom: 16px;">
        <h4 style="margin-bottom: 10px; font-size: 14px; text-transform: uppercase; color: var(--text-muted);">Pruebas</h4>
        <div class="action-row">
          <button id="btnEcho" class="btn btn-primary" onclick="runEchoTest()">Prueba de Eco (hardware)</button>
          <button id="btnRetry" class="btn btn-ghost" onclick="reinitKLine()">Intentar conexión ECU</button>
        </div>
        <div id="echoResult" style="font-size: 12px; color: var(--text-muted); margin-top: 10px;">
          La prueba de eco envía 16 bytes y verifica que regresen por RX. Funciona aunque la ECU no conteste.
        </div>
        <div class="control-row" style="margin-top: 12px;">
          <span>Fórmula RPM (Tipo ECU):</span>
          <select id="ecuTypeSelect" onchange="changeEcuType(this.value)" style="background:#1e2638; color:var(--text-main); border:1px solid var(--card-border); padding:6px 10px; border-radius:8px; font-weight:600; font-size:12px;">
            <option value="2">OBD2 (Civic 96-00: raw / 4)</option>
            <option value="1">OBD1 (Civic 92-95: 1875000 / raw)</option>
          </select>
        </div>
      </div>

      <div class="card" style="margin-bottom: 16px;">
        <h4 style="margin-bottom: 10px; font-size: 14px; text-transform: uppercase; color: var(--text-muted);">Contadores</h4>
        <div class="stat-grid">
          <div class="stat"><span>Estado</span><b id="sState">--</b></div>
          <div class="stat"><span>Intentos</span><b id="sInit">0</b></div>
          <div class="stat"><span>Comandos</span><b id="sSent">0</b></div>
          <div class="stat"><span>Resp. OK</span><b id="sOk">0</b></div>
          <div class="stat"><span>Timeouts</span><b id="sTimeout">0</b></div>
          <div class="stat"><span>Error CRC</span><b id="sCrc">0</b></div>
          <div class="stat"><span>Eco OK</span><b id="sEchoOk">0</b></div>
          <div class="stat"><span>Eco Mal</span><b id="sEchoFail">0</b></div>
          <div class="stat"><span>Bytes ECU</span><b id="sRx">0</b></div>
          <div class="stat"><span>Uptime</span><b id="sUp">0s</b></div>
        </div>
        <div style="font-size: 11px; color: var(--text-muted); margin-bottom: 4px;">ÚLTIMO ERROR</div>
        <div id="sErr" class="hexline" style="color: var(--accent-orange);">--</div>
        <div style="font-size: 11px; color: var(--text-muted); margin-bottom: 4px;">ÚLTIMA TRAMA RECIBIDA</div>
        <div id="sLastRx" class="hexline">--</div>
      </div>

      <div class="card">
        <div style="display:flex; justify-content: space-between; align-items:center; margin-bottom: 10px;">
          <h4 style="font-size: 14px; text-transform: uppercase; color: var(--text-muted);">Log en vivo</h4>
          <label style="font-size: 11px; color: var(--text-muted); display:flex; gap:6px; align-items:center;">
            <input type="checkbox" id="autoScroll" checked> Auto-scroll
          </label>
        </div>
        <div id="console" class="console"></div>
        <div class="action-row">
          <button class="btn btn-ghost btn-small" onclick="document.getElementById('console').innerHTML=''">Limpiar pantalla</button>
          <button class="btn btn-ghost btn-small" onclick="resetCounters()">Reiniciar contadores</button>
          <button class="btn btn-ghost btn-small" onclick="copyLog()">Copiar log</button>
        </div>
      </div>
    </div>
  </main>

  <script>
    const GAUGE_CIRCUMFERENCE = 280; // approximate arc length for gauge path
    let lastUpdate = Date.now();
    let frameCount = 0;
    let isDemo = false;

    function switchTab(tabId) {
      const tabs = { gauges: 'tabGauges', diag: 'tabDiag', console: 'tabConsole' };
      const order = ['gauges', 'diag', 'console'];
      document.querySelectorAll('.nav-btn').forEach(btn => btn.classList.remove('active'));
      document.querySelectorAll('.tab-content').forEach(c => c.classList.remove('active'));
      document.querySelectorAll('.nav-btn')[order.indexOf(tabId)].classList.add('active');
      document.getElementById(tabs[tabId]).classList.add('active');
      if (tabId === 'console') fetchDiag();
    }

    function setGauge(pathId, value, maxVal) {
      const path = document.getElementById(pathId);
      const pct = Math.min(Math.max(value / maxVal, 0), 1);
      // Path total stroke-dasharray is 440; arc is ~280
      const offset = 440 - (pct * 280);
      path.style.strokeDashoffset = offset;
    }

    async function fetchTelemetry() {
      try {
        const resp = await fetch('/api/data', { cache: 'no-store' });
        if (!resp.ok) throw new Error("HTTP " + resp.status);
        const d = await resp.json();

        // Status badge
        const badge = document.getElementById('statusBadge');
        const text = document.getElementById('statusText');
        if (d.connected) {
          badge.className = "status-badge";
          text.textContent = d.demo ? "MODO DEMO" : "K-LINE ACTIVO";
        } else {
          badge.className = "status-badge error";
          text.textContent = "SIN COMUNICACIÓN";
        }

        // Top Status Pills
        const pillV = document.getElementById('pillVtec');
        if (pillV) pillV.className = 'status-pill vtec-pill' + (d.vtec ? ' active' : '');
        const valG = document.getElementById('valGear');
        if (valG) valG.textContent = d.gear === 0 ? 'P/N' : d.gear + 'ª';
        const valPn = document.getElementById('valPn');
        if (valPn) valPn.textContent = d.pn ? 'P/N' : 'D';
        const valAc = document.getElementById('valAc');
        if (valAc) {
          valAc.textContent = d.ac ? 'ON' : 'OFF';
          document.getElementById('pillAc').className = 'status-pill' + (d.ac ? ' active' : '');
        }
        const valBrk = document.getElementById('valBrake');
        if (valBrk) {
          valBrk.textContent = d.brake ? 'ON' : 'OFF';
          document.getElementById('pillBrake').className = 'status-pill' + (d.brake ? ' active' : '');
        }

        // Large Gauges
        document.getElementById('rpmValue').textContent = d.rpm;
        setGauge('rpmPath', d.rpm, 8000);

        document.getElementById('speedValue').textContent = d.vss;
        setGauge('speedPath', d.vss, 220);

        // SECCIÓN A: MOTOR Y ESTADOS DISCRETOS
        const aV = document.getElementById('aVtecVal');
        if (aV) {
          aV.textContent = d.vtec ? '⚡ ACTIVO' : 'INACTIVO';
          aV.style.color = d.vtec ? 'var(--accent-green)' : 'var(--text-muted)';
          const b = document.getElementById('aVtecBar');
          b.style.width = d.vtec ? '100%' : '0%';
          b.style.background = '#00ff88';
        }
        const aG = document.getElementById('aGearVal');
        if (aG) aG.textContent = d.gear === 0 ? 'P / N' : (d.gear + 'ª Marcha');
        const aAc = document.getElementById('aAcVal');
        if (aAc) {
          aAc.textContent = d.ac ? 'ON' : 'OFF';
          aAc.style.color = d.ac ? 'var(--accent-cyan)' : 'var(--text-muted)';
          document.getElementById('aAcBar').style.width = d.ac ? '100%' : '0%';
        }
        const aBrk = document.getElementById('aBrakeVal');
        if (aBrk) {
          aBrk.textContent = d.brake ? 'PRESIONADO' : 'LIBRE';
          aBrk.style.color = d.brake ? 'var(--accent-orange)' : 'var(--text-muted)';
          document.getElementById('aBrakeBar').style.width = d.brake ? '100%' : '0%';
        }
        const aKnk = document.getElementById('aKnockVal');
        if (aKnk) {
          aKnk.textContent = d.knock;
          const kb = document.getElementById('aKnockBar');
          kb.style.width = (d.knock / 5 * 100) + '%';
          kb.style.background = d.knock > 0 ? 'var(--accent-red)' : 'var(--accent-green)';
        }

        // SECCIÓN B: ADMISIÓN, ESCAPE Y ELÉCTRICOS
        document.getElementById('ectValue').textContent = d.ect;
        const ectBar = document.getElementById('ectBar');
        ectBar.style.width = Math.min(Math.max((d.ect / 130) * 100, 0), 100) + '%';
        if (d.ect < 75) ectBar.className = "bar-fill temp-cold";
        else if (d.ect <= 98) ectBar.className = "bar-fill temp-ok";
        else ectBar.className = "bar-fill temp-hot";

        document.getElementById('iatValue').textContent = d.iat;
        document.getElementById('iatBar').style.width = Math.min(Math.max((d.iat / 80) * 100, 0), 100) + '%';

        document.getElementById('mapValue').textContent = d.map;
        document.getElementById('mapBar').style.width = Math.min(Math.max((d.map / 110) * 100, 0), 100) + '%';

        const mapHg = document.getElementById('mapInHgVal');
        if (mapHg && d.mapInHg !== undefined) {
          mapHg.textContent = d.mapInHg.toFixed(1);
          document.getElementById('mapInHgBar').style.width = Math.min(Math.max((d.mapInHg / 25) * 100, 0), 100) + '%';
        }

        const baroEl = document.getElementById('baroVal');
        if (baroEl && d.baro) baroEl.textContent = d.baro;

        document.getElementById('tpsValue').textContent = d.tps;
        document.getElementById('tpsBar').style.width = Math.min(Math.max(d.tps, 0), 100) + '%';

        const o2El = document.getElementById('o2Val');
        if (o2El && d.o2 !== undefined) {
          o2El.textContent = d.o2.toFixed(2);
          const ob = document.getElementById('o2Bar');
          ob.style.width = Math.min(Math.max((d.o2 / 1.0) * 100, 0), 100) + '%';
          if (d.o2 < 0.40) ob.className = "bar-fill o2-lean";
          else if (d.o2 <= 0.60) ob.className = "bar-fill o2-stoich";
          else ob.className = "bar-fill o2-rich";
        }

        document.getElementById('battValue').textContent = d.batt.toFixed(1);
        document.getElementById('battBar').style.width = Math.min(Math.max(((d.batt - 10) / 5) * 100, 0), 100) + '%';

        const altEl = document.getElementById('altVal');
        if (altEl && d.altFr !== undefined) {
          altEl.textContent = d.altFr;
          document.getElementById('altBar').style.width = d.altFr + '%';
        }

        const eldEl = document.getElementById('eldVal');
        if (eldEl && d.eld !== undefined) {
          eldEl.textContent = d.eld.toFixed(1);
          document.getElementById('eldBar').style.width = Math.min(d.eld, 100) + '%';
        }

        // SECCIÓN C: INYECCIÓN, COMBUSTIBLE Y ENCENDIDO
        const ignEl = document.getElementById('ignVal');
        if (ignEl && d.ign !== undefined) {
          ignEl.textContent = d.ign.toFixed(1);
          document.getElementById('ignBar').style.width = Math.min(Math.max((d.ign / 45) * 100, 0), 100) + '%';
        }

        const injEl = document.getElementById('injVal');
        if (injEl && d.inj !== undefined) {
          injEl.textContent = d.inj.toFixed(2);
          document.getElementById('injBar').style.width = Math.min(Math.max((d.inj / 18) * 100, 0), 100) + '%';
        }

        const sftEl = document.getElementById('sftVal');
        if (sftEl && d.sft !== undefined) {
          sftEl.textContent = (d.sft > 0 ? '+' : '') + d.sft;
          const sb = document.getElementById('sftBar');
          sb.style.width = Math.min(Math.abs(d.sft) * 2.5, 100) + '%';
          sb.style.background = Math.abs(d.sft) < 10 ? '#10b981' : '#f59e0b';
        }

        const lftEl = document.getElementById('lftVal');
        if (lftEl && d.lft !== undefined) {
          lftEl.textContent = (d.lft > 0 ? '+' : '') + d.lft;
          const lb = document.getElementById('lftBar');
          lb.style.width = Math.min(Math.abs(d.lft) * 2.5, 100) + '%';
          lb.style.background = Math.abs(d.lft) < 10 ? '#10b981' : '#f59e0b';
        }

        const iacvEl = document.getElementById('iacvVal');
        if (iacvEl && d.iacv !== undefined) {
          iacvEl.textContent = d.iacv;
          document.getElementById('iacvBar').style.width = d.iacv + '%';
        }

        const mafEl = document.getElementById('mafVal');
        if (mafEl && d.maf !== undefined) {
          mafEl.textContent = d.maf.toFixed(1);
          document.getElementById('mafBar').style.width = Math.min((d.maf / 70) * 100, 100) + '%';
        }

        // FPS / Hz calculation
        frameCount++;
        const now = Date.now();
        if (now - lastUpdate >= 1000) {
          document.getElementById('hzValue').textContent = (frameCount * 1000 / (now - lastUpdate)).toFixed(1);
          frameCount = 0;
          lastUpdate = now;
        }

        if (d.demo !== undefined && isDemo !== d.demo) {
          isDemo = d.demo;
          document.getElementById('demoSwitch').checked = isDemo;
        }
      } catch (e) {
        document.getElementById('statusBadge').className = "status-badge error";
        document.getElementById('statusText').textContent = "ERROR WIFI";
      }
    }

    async function readDTCs() {
      const container = document.getElementById('dtcContainer');
      container.innerHTML = '<div style="color:var(--text-muted); padding:20px; text-align:center;">Consultando ECU por K-Line...</div>';
      try {
        const resp = await fetch('/api/dtc');
        const data = await resp.json();
        const banner = document.getElementById('celBanner');
        const title = document.getElementById('celStatusTitle');
        const desc = document.getElementById('celStatusDesc');

        if (!data.codes || data.codes.length === 0) {
          banner.className = "cel-banner no-faults";
          title.textContent = "Sin Códigos de Falla";
          desc.textContent = "El sistema PGM-FI no reporta anomalías activas.";
          container.innerHTML = '<div style="color:var(--accent-green); padding:16px; font-weight:600; text-align:center;">✓ Memoria de diagnóstico limpia. No hay códigos de error.</div>';
        } else {
          banner.className = "cel-banner has-faults";
          title.textContent = `${data.codes.length} Código(s) Detectado(s)`;
          desc.textContent = "Atención: La luz Check Engine se encuentra activa en el tablero.";
          
          let html = '';
          data.codes.forEach(c => {
            html += `
              <div class="dtc-item">
                <div class="dtc-header">
                  <span class="dtc-code">Código ${c.code}</span>
                  <span class="dtc-name">${c.name}</span>
                </div>
                <div class="dtc-desc">${c.desc}</div>
              </div>
            `;
          });
          container.innerHTML = html;
        }
      } catch (e) {
        container.innerHTML = '<div style="color:var(--accent-red); padding:16px;">Error de comunicación al solicitar códigos.</div>';
      }
    }

    async function confirmClearDTCs() {
      if (!confirm("¿Deseas resetear los códigos de falla y reiniciar la memoria del ECU?\n\nNota: Haz esto con el motor detenido y la llave en posición ON (IGN II).")) {
        return;
      }
      try {
        const resp = await fetch('/api/clear_dtc', { method: 'POST' });
        const res = await resp.json();
        if (res.success) {
          alert("Comando de reseteo enviado con éxito al ECU.");
          readDTCs();
        } else {
          alert("Error: El ECU no confirmó el reseteo.");
        }
      } catch (e) {
        alert("Error de conexión al enviar comando de reseteo.");
      }
    }

    async function reinitKLine() {
      try {
        const resp = await fetch('/api/reinit', { method: 'POST' });
        const res = await resp.json();
        alert(res.success ? "¡La ECU respondió! Conexión establecida."
                          : "La ECU no respondió. Revisa la pestaña Consola para ver el detalle.");
        fetchDiag();
      } catch (e) {
        alert("Error al intentar reiniciar K-Line.");
      }
    }

    async function toggleDemo(enabled) {
      isDemo = enabled;
      await fetch('/api/demo?enable=' + (enabled ? '1' : '0'), { method: 'POST' });
    }

    // ======================= CONSOLA K-LINE =======================
    let logSeq = 0;
    let lastDiag = null;

    function setStep(id, cls) {
      document.getElementById(id).className = 'step ' + (cls || '');
    }

    function setVerdict(cls, icon, title, desc) {
      document.getElementById('verdict').className = 'verdict ' + cls;
      document.getElementById('verdictIcon').textContent = icon;
      document.getElementById('verdictTitle').textContent = title;
      document.getElementById('verdictDesc').textContent = desc;
    }

    function analyze(d) {
      const echoTestDone = d.echoTestSent > 0;
      const echoTestGood = echoTestDone && d.echoTestOk === d.echoTestSent;
      const echoWorks = d.echoOk > 0 || echoTestGood;
      const echoBroken = (d.cmdSent > 0 && d.echoOk === 0) || (echoTestDone && d.echoTestRx === 0);

      setStep('stepTx', d.cmdSent > 0 || echoTestDone ? 'ok' : '');
      setStep('stepEcho', echoWorks ? (echoTestDone && !echoTestGood ? 'warn' : 'ok') : (echoBroken ? 'bad' : ''));
      setStep('stepEcu', d.cmdOk > 0 ? 'ok' : (d.cmdCrcFail > 0 || d.rxBytes > 0 ? 'warn' : (d.cmdSent > 0 ? 'bad' : '')));

      if (d.connected && d.cmdOk > 0) {
        setVerdict('ok', '✓', 'Comunicación con la ECU funcionando',
          `${d.cmdOk} respuestas válidas. Protocolo: OBD${d.ecuType} (${d.ecuType === 2 ? "Civic '96-'00 / raw÷4" : "Civic '92-'95"}).`);
      } else if (d.cmdSent === 0 && !echoTestDone) {
        setVerdict('', '…', 'Esperando primer intento', 'Pon la llave en ON. El ESP32 intenta conectar cada 2 segundos.');
      } else if (echoBroken && !echoWorks) {
        setVerdict('bad', '✗', 'Problema de hardware: no hay eco',
          'Lo que el ESP32 transmite no regresa por RX. Revisa: GPIO5→pin 4 (TX), pin 1 (RX)→GPIO4, ' +
          '12V en pin 7 (VS), alimentación del pin 3 (VCC), tierra común y que pin 6 llegue a la K-Line.');
      } else if (echoTestDone && !echoTestGood) {
        setVerdict('warn', '!', 'Eco con errores',
          `Regresaron ${d.echoTestOk}/${d.echoTestSent} bytes correctos. Posible ruido, pull-up incorrecto o VCC del L9637D fuera de rango (pide 4.5-5.5V).`);
      } else if (d.cmdCrcFail > 0 || d.rxBytes > 0) {
        setVerdict('warn', '!', 'La ECU contesta pero con errores',
          'Llegan bytes de la ECU pero la trama no es válida. Revisa la última trama recibida y envíala para analizarla.');
      } else if (echoWorks) {
        setVerdict('warn', '!', 'Hardware OK, pero la ECU no contesta',
          'Tu circuito transmite bien (hay eco). La ECU no responde: verifica llave en ON, que sea el pin correcto del conector, ' +
          'y prueba quitar/cambiar la resistencia pull-up de 1kΩ (la ECU podría no poder bajar la línea).');
      } else {
        setVerdict('', '?', 'Analizando...', 'Presiona "Prueba de Eco" para verificar el hardware.');
      }
    }

    function classify(line) {
      if (/EXITOSA|PERFECTO/.test(line)) return 'l-ok';
      if (/SIN ECO|NADA regresa|NO CONTESTA|no respondi/.test(line)) return 'l-bad';
      if (/INCOMPLETO|corrupto|CRC|PARCIAL|esperaba|Timeout/.test(line)) return 'l-warn';
      if (/RX|ECO RX|DTC/.test(line)) return 'l-hex';
      return '';
    }

    async function fetchDiag() {
      try {
        const resp = await fetch('/api/diag?since=' + logSeq, { cache: 'no-store' });
        const d = await resp.json();
        lastDiag = d;
        document.getElementById('sState').textContent = d.state;
        document.getElementById('sInit').textContent = `${d.initSuccess}/${d.initAttempts}`;
        document.getElementById('sSent').textContent = d.cmdSent;
        document.getElementById('sOk').textContent = d.cmdOk;
        document.getElementById('sTimeout').textContent = d.cmdTimeout;
        document.getElementById('sCrc').textContent = d.cmdCrcFail;
        document.getElementById('sEchoOk').textContent = d.echoOk;
        document.getElementById('sEchoFail').textContent = d.echoFail;
        document.getElementById('sRx').textContent = d.rxBytes;
        document.getElementById('sUp').textContent = d.uptime + 's';
        document.getElementById('sErr').textContent = d.lastError || '--';
        document.getElementById('sLastRx').textContent = d.lastRx || '--';
        const sel = document.getElementById('ecuTypeSelect');
        if (sel && document.activeElement !== sel && d.ecuType) sel.value = d.ecuType;
        analyze(d);

        if (d.seq < logSeq) logSeq = 0; // el ESP32 se reinició
        const con = document.getElementById('console');
        d.lines.forEach(l => {
          const div = document.createElement('div');
          div.textContent = l;
          const c = classify(l);
          if (c) div.className = c;
          con.appendChild(div);
        });
        while (con.childNodes.length > 400) con.removeChild(con.firstChild);
        if (d.lines.length && document.getElementById('autoScroll').checked) con.scrollTop = con.scrollHeight;
        logSeq = d.seq;
      } catch (e) { /* ignorar, se reintenta */ }
    }

    async function runEchoTest() {
      const btn = document.getElementById('btnEcho');
      const out = document.getElementById('echoResult');
      btn.disabled = true;
      out.textContent = 'Probando...';
      try {
        const r = await (await fetch('/api/echotest', { method: 'POST' })).json();
        if (r.rx === 0) {
          out.innerHTML = `<b style="color:var(--accent-red)">✗ No regresó nada (0/${r.sent}).</b> El problema está en el circuito ESP32 ↔ L9637D ↔ K-Line.`;
        } else if (r.ok === r.sent) {
          out.innerHTML = `<b style="color:var(--accent-green)">✓ Eco perfecto (${r.ok}/${r.sent}).</b> El ESP32 y el L9637D funcionan. Si la ECU no contesta, el problema está del lado del carro o del protocolo.`;
        } else {
          out.innerHTML = `<b style="color:var(--accent-orange)">! Eco parcial: ${r.ok}/${r.sent} correctos (${r.rx} recibidos).</b> Hay ruido o niveles eléctricos incorrectos.`;
        }
      } catch (e) {
        out.textContent = 'Error de conexión con el ESP32.';
      }
      btn.disabled = false;
      fetchDiag();
    }

    async function resetCounters() {
      await fetch('/api/reset_counters', { method: 'POST' });
      fetchDiag();
    }

    async function changeEcuType(val) {
      await fetch('/api/set_ecu_type?type=' + val, { method: 'POST' });
      fetchDiag();
    }

    function copyLog() {
      const txt = document.getElementById('console').innerText;
      const extra = lastDiag ? `\n--- ${JSON.stringify(Object.assign({}, lastDiag, { lines: undefined }))}` : '';
      const full = txt + extra;
      if (navigator.clipboard && window.isSecureContext) {
        navigator.clipboard.writeText(full).then(() => alert('Log copiado.'));
      } else {
        const ta = document.createElement('textarea');
        ta.value = full; document.body.appendChild(ta); ta.select();
        try { document.execCommand('copy'); alert('Log copiado.'); } catch (e) { alert('No se pudo copiar.'); }
        document.body.removeChild(ta);
      }
    }

    // Consola: refrescar cada 1 s solo si la pestaña está visible
    setInterval(() => {
      if (document.getElementById('tabConsole').classList.contains('active')) fetchDiag();
    }, 1000);

    // Poll telemetry at ~10 Hz (every 100 ms)
    setInterval(fetchTelemetry, 100);
  </script>
</body>
</html>
)rawliteral";

#endif // DASHBOARD_HTML_H
