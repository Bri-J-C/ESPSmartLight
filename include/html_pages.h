#pragma once

#include <Arduino.h>

static const char HTML_HEADER[] PROGMEM = R"rawliteral(
<!DOCTYPE html><html><head>
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>Smart Light Config</title>
<style>
*{box-sizing:border-box;margin:0;padding:0}
body{font-family:system-ui,-apple-system,sans-serif;background:#1a1a2e;color:#e0e0e0;padding:20px;max-width:600px;margin:0 auto}
h1{color:#00d4ff;margin-bottom:20px;font-size:1.5em}
h2{color:#00d4ff;margin:18px 0 10px;font-size:1.1em;border-bottom:1px solid #333;padding-bottom:5px}
.card{background:#16213e;border-radius:8px;padding:20px;margin-bottom:15px;border:1px solid #0f3460}
label{display:block;margin:10px 0 4px;font-size:0.9em;color:#aaa}
input,select{width:100%;padding:10px;border:1px solid #0f3460;border-radius:4px;background:#1a1a2e;color:#e0e0e0;font-size:1em}
input:focus,select:focus{outline:none;border-color:#00d4ff}
button,.btn{display:inline-block;padding:12px 24px;border:none;border-radius:4px;cursor:pointer;font-size:1em;margin:5px 2px;text-decoration:none;color:#fff}
.btn-primary{background:#00d4ff;color:#1a1a2e;font-weight:bold}
.btn-danger{background:#e74c3c}
.btn-secondary{background:#555}
.btn-success{background:#27ae60}
.btn-sm{padding:8px 16px;font-size:0.85em}
.status-row{display:flex;justify-content:space-between;padding:8px 0;border-bottom:1px solid #222}
.status-label{color:#aaa}
.status-value{color:#00d4ff;font-weight:bold}
.online{color:#27ae60}.offline{color:#e74c3c}
.scan-list{max-height:200px;overflow-y:auto;margin:8px 0}
.scan-item{padding:8px;background:#1a1a2e;margin:2px 0;border-radius:4px;cursor:pointer;display:flex;justify-content:space-between}
.scan-item:hover{background:#0f3460}
.signal{font-size:0.8em;color:#aaa}
.actions{margin-top:15px;text-align:center}
#scanResults{display:none}
.toast{position:fixed;top:20px;right:20px;padding:12px 20px;border-radius:4px;background:#27ae60;color:#fff;display:none;z-index:999}
</style></head><body>
)rawliteral";

static const char HTML_FOOTER[] PROGMEM = R"rawliteral(
<div class="toast" id="toast"></div>
</body></html>
)rawliteral";

static const char HTML_CONFIG_PAGE[] PROGMEM = R"rawliteral(
<h1>Smart Light Setup</h1>
<form method="POST" action="/save">
<div class="card">
<h2>WiFi</h2>
<label>SSID</label>
<div style="display:flex;gap:5px">
<input type="text" name="wifi_ssid" id="ssid" value="%WIFI_SSID%" required>
<button type="button" class="btn-secondary btn-sm" onclick="scanWifi()">Scan</button>
</div>
<div id="scanResults"><div class="scan-list" id="scanList"></div></div>
<label>Password</label>
<input type="password" name="wifi_pass" placeholder="%WIFI_PASS_PH%">
</div>

<div class="card">
<h2>MQTT</h2>
<label>Broker Host</label>
<input type="text" name="mqtt_host" value="%MQTT_HOST%">
<label>Port</label>
<input type="number" name="mqtt_port" value="%MQTT_PORT%">
<label>Username</label>
<input type="text" name="mqtt_user" value="%MQTT_USER%">
<label>Password</label>
<input type="password" name="mqtt_pass" placeholder="%MQTT_PASS_PH%">
<label>Root Topic</label>
<input type="text" name="mqtt_root" value="%MQTT_ROOT%" placeholder="e.g. home/floor/room/switches">
</div>

<div class="card">
<h2>Device</h2>
<label>Device Name</label>
<input type="text" name="device_name" value="%DEVICE_NAME%">
<label>Relay GPIO Pin</label>
<input type="number" name="relay_pin" value="%RELAY_PIN%" min="0" max="21">
<label>Button GPIO Pin</label>
<input type="number" name="button_pin" value="%BUTTON_PIN%" min="0" max="21">
<label>Boot State</label>
<select name="boot_state">
<option value="0" %BOOT_OFF%>OFF</option>
<option value="1" %BOOT_ON%>ON</option>
<option value="2" %BOOT_LAST%>Last State</option>
</select>
</div>

<div class="actions">
<button type="submit" class="btn btn-primary">Save & Reboot</button>
</div>
</form>

<script>
function scanWifi(){
  document.getElementById('scanResults').style.display='block';
  document.getElementById('scanList').innerHTML='Scanning...';
  pollScan();
}
function pollScan(){
  fetch('/scan').then(r=>r.json()).then(data=>{
    if(data.scanning){setTimeout(pollScan,1000);return;}
    var list=document.getElementById('scanList');
    list.innerHTML='';
    if(!Array.isArray(data)||!data.length){list.innerHTML='<div style="padding:8px;color:#aaa">No networks found</div>';return;}
    data.sort((a,b)=>b.rssi-a.rssi);
    data.forEach(n=>{
      var div=document.createElement('div');
      div.className='scan-item';
      div.onclick=function(){document.getElementById('ssid').value=n.ssid;};
      var bars=n.rssi>-50?'####':n.rssi>-70?'### ':n.rssi>-80?'##  ':'#   ';
      var left=document.createElement('span');
      left.textContent=(n.enc?'* ':'')+n.ssid;
      var right=document.createElement('span');
      right.className='signal';
      right.textContent=bars+' '+n.rssi+'dBm';
      div.appendChild(left);
      div.appendChild(right);
      list.appendChild(div);
    });
  }).catch(()=>{document.getElementById('scanList').innerHTML='Scan failed';});
}
</script>
)rawliteral";

static const char HTML_STATUS_PAGE[] PROGMEM = R"rawliteral(
<h1>%DEVICE_NAME%</h1>
<div class="card">
<h2>Status</h2>
<div class="status-row"><span class="status-label">WiFi</span><span class="status-value %WIFI_CLASS%">%WIFI_STATUS%</span></div>
<div class="status-row"><span class="status-label">IP Address</span><span class="status-value">%IP_ADDR%</span></div>
<div class="status-row"><span class="status-label">MQTT</span><span class="status-value %MQTT_CLASS%">%MQTT_STATUS%</span></div>
<div class="status-row"><span class="status-label">Relay</span><span class="status-value">%RELAY_STATE%</span></div>
<div class="status-row"><span class="status-label">Uptime</span><span class="status-value">%UPTIME%</span></div>
<div class="status-row"><span class="status-label">Firmware</span><span class="status-value">%FW_VERSION%</span></div>
<div class="status-row"><span class="status-label">Free Heap</span><span class="status-value">%FREE_HEAP%</span></div>
</div>

<div class="actions">
<button class="btn btn-secondary" onclick="doAction('/restart')">Restart</button>
<a class="btn btn-primary" href="/config">Reconfigure</a>
<a class="btn btn-secondary" href="/logs">Logs</a>
<button class="btn btn-danger" onclick="if(confirm('Factory reset?'))doAction('/reset')">Factory Reset</button>
</div>

<script>
function doAction(url){
  fetch(url,{method:'POST'}).then(()=>{
    if(url==='/toggle'){setTimeout(()=>location.reload(),500);}
    else{document.body.innerHTML='<h1 style="text-align:center;margin-top:40vh;color:#00d4ff">Restarting...</h1>';}
  });
}
setInterval(()=>{
  fetch('/status').then(r=>r.json()).then(d=>{
    document.querySelectorAll('.status-value').forEach(el=>{
      let key=el.previousElementSibling.textContent;
      if(d[key]!==undefined){el.textContent=d[key];}
    });
  });
},5000);
</script>
)rawliteral";
