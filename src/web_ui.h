#pragma once

static const char INDEX_HTML[] = R"rawliteral(<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <meta http-equiv="Cache-Control" content="no-cache, no-store, must-revalidate">
  <meta http-equiv="Pragma" content="no-cache">
  <meta http-equiv="Expires" content="0">
  <title>Master Helper - ESP32 Control Center</title>
  <style>
    :root {
      --bg: #0f172a;
      --surface: #1e293b;
      --surface-hover: #334155;
      --primary: #38bdf8;
      --primary-hover: #0ea5e9;
      --accent: #818cf8;
      --success: #10b981;
      --warning: #f59e0b;
      --danger: #ef4444;
      --text: #f8fafc;
      --text-dim: #94a3b8;
      --border: #334155;
    }
    * { box-sizing: border-box; margin: 0; padding: 0; font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, sans-serif; }
    body { background: var(--bg); color: var(--text); min-height: 100vh; padding: 1.5rem 1rem; }
    .container { max-width: 900px; margin: 0 auto; }
    header { display: flex; flex-wrap: wrap; justify-content: space-between; align-items: center; margin-bottom: 2rem; padding-bottom: 1rem; border-bottom: 1px solid var(--border); gap: 1rem; }
    .title-group h1 { font-size: 1.6rem; color: var(--primary); display: flex; align-items: center; gap: 0.5rem; }
    .title-group p { font-size: 0.85rem; color: var(--text-dim); }
    .badge { display: inline-flex; align-items: center; gap: 0.4rem; padding: 0.35rem 0.75rem; border-radius: 9999px; font-size: 0.8rem; font-weight: 600; }
    .badge-green { background: rgba(16, 185, 129, 0.2); color: #34d399; border: 1px solid rgba(16, 185, 129, 0.3); }
    .badge-blue { background: rgba(56, 189, 248, 0.2); color: #38bdf8; border: 1px solid rgba(56, 189, 248, 0.3); }
    .badge-yellow { background: rgba(245, 158, 11, 0.2); color: #fbbf24; border: 1px solid rgba(245, 158, 11, 0.3); }
    .badge-red { background: rgba(239, 68, 68, 0.2); color: #f87171; border: 1px solid rgba(239, 68, 68, 0.3); }
    .tabs { display: flex; gap: 0.5rem; margin-bottom: 1.5rem; border-bottom: 1px solid var(--border); padding-bottom: 0.5rem; overflow-x: auto; }
    .tab-btn { background: none; border: none; color: var(--text-dim); padding: 0.6rem 1.2rem; font-size: 0.95rem; font-weight: 600; cursor: pointer; border-radius: 0.5rem; transition: all 0.2s; white-space: nowrap; }
    .tab-btn:hover { color: var(--text); background: var(--surface-hover); }
    .tab-btn.active { color: var(--primary); background: rgba(56, 189, 248, 0.15); }
    .card { background: var(--surface); border: 1px solid var(--border); border-radius: 0.75rem; padding: 1.5rem; margin-bottom: 1.5rem; box-shadow: 0 4px 6px -1px rgba(0,0,0,0.2); }
    .card-title { font-size: 1.15rem; font-weight: 600; margin-bottom: 1rem; color: var(--text); display: flex; align-items: center; justify-content: space-between; }
    .grid-2 { display: grid; grid-template-columns: repeat(auto-fit, minmax(280px, 1fr)); gap: 1rem; }
    .stat-box { background: rgba(15, 23, 42, 0.6); border: 1px solid var(--border); border-radius: 0.5rem; padding: 1rem; }
    .stat-label { font-size: 0.8rem; text-transform: uppercase; letter-spacing: 0.05em; color: var(--text-dim); margin-bottom: 0.4rem; }
    .stat-value { font-size: 1.1rem; font-weight: 600; color: var(--text); }
    .sub-stat { font-size: 0.85rem; color: var(--text-dim); margin-top: 0.3rem; }
    .form-group { margin-bottom: 1.2rem; }
    label { display: block; font-size: 0.85rem; font-weight: 500; margin-bottom: 0.4rem; color: var(--text); }
    input[type="text"], input[type="password"], select { width: 100%; padding: 0.75rem 1rem; background: rgba(15, 23, 42, 0.8); border: 1px solid var(--border); border-radius: 0.5rem; color: var(--text); font-size: 0.95rem; outline: none; transition: border-color 0.2s; }
    input[type="text"]:focus, input[type="password"]:focus, select:focus { border-color: var(--primary); }
    .helper-text { color: var(--text-dim); display: block; margin-top: 0.3rem; font-size: 0.8rem; }
    .section-desc { color: var(--text-dim); font-size: 0.9rem; margin-bottom: 1.5rem; line-height: 1.5; }
    .btn-group { display: flex; gap: 1rem; flex-wrap: wrap; }
    .btn { display: inline-flex; align-items: center; justify-content: center; gap: 0.5rem; padding: 0.75rem 1.5rem; font-size: 0.95rem; font-weight: 600; border-radius: 0.5rem; border: none; cursor: pointer; transition: all 0.2s; }
    .btn-primary { background: var(--primary); color: #0f172a; }
    .btn-primary:hover:not(:disabled) { background: var(--primary-hover); }
    .btn-success { background: var(--success); color: #fff; }
    .btn-success:hover:not(:disabled) { opacity: 0.9; }
    .btn-danger { background: var(--danger); color: #fff; }
    .btn-danger:hover:not(:disabled) { opacity: 0.9; }
    .btn:disabled { opacity: 0.5; cursor: not-allowed; }
    .alert { padding: 1rem; border-radius: 0.5rem; margin-bottom: 1rem; font-size: 0.9rem; display: flex; align-items: center; gap: 0.75rem; }
    .alert-info { background: rgba(56, 189, 248, 0.15); border: 1px solid rgba(56, 189, 248, 0.3); color: #38bdf8; }
    .alert-success { background: rgba(16, 185, 129, 0.15); border: 1px solid rgba(16, 185, 129, 0.3); color: #34d399; }
    .alert-warning { background: rgba(245, 158, 11, 0.15); border: 1px solid rgba(245, 158, 11, 0.3); color: #fbbf24; }
    .alert-danger { background: rgba(239, 68, 68, 0.15); border: 1px solid rgba(239, 68, 68, 0.3); color: #f87171; }
    .code-inline { font-family: monospace; background: rgba(0,0,0,0.3); padding: 0.1rem 0.3rem; border-radius: 0.25rem; font-size: 0.85rem; }
    .progress-bar-bg { width: 100%; height: 0.75rem; background: rgba(15, 23, 42, 0.8); border-radius: 9999px; overflow: hidden; margin-top: 0.75rem; border: 1px solid var(--border); }
    .progress-bar-fill { height: 100%; width: 0%; background: var(--primary); transition: width 0.3s; }
    .progress-info { display: flex; justify-content: space-between; font-size: 0.85rem; color: var(--text-dim); margin-top: 0.5rem; }
    .hidden { display: none !important; }
    .mb-15 { margin-bottom: 1.5rem; }
    .manifest-doc { margin-top: 1.5rem; padding: 1rem; background: rgba(0,0,0,0.2); border-radius: 0.5rem; font-size: 0.8rem; border: 1px dashed var(--border); }
    .manifest-doc pre { margin-top: 0.5rem; color: var(--accent); }
  </style>
</head>
<body>
<div class="container">
  <header>
    <div class="title-group">
      <h1>&#9889; Master Helper</h1>
      <p id="sub-title">ESP32-C3 Control Center &amp; OTA Gateway</p>
    </div>
    <div id="net-badge" class="badge badge-yellow">&#8987; Connecting...</div>
  </header>

  <div class="tabs">
    <button class="tab-btn active" onclick="switchTab('tab-dash', this)">Dashboard</button>
    <button class="tab-btn" onclick="switchTab('tab-wifi', this)">Wi-Fi &amp; Settings</button>
    <button class="tab-btn" onclick="switchTab('tab-ota', this)">GitHub OTA Updater</button>
    <button class="tab-btn" onclick="switchTab('tab-manual', this)">Manual Flash</button>
  </div>

  <!-- DASHBOARD TAB -->
  <div id="tab-dash" class="tab-content">
    <div id="conn-helper-alert" class="alert alert-warning hidden">
      <span>&#9888; <strong>Notice:</strong> If connected to <code>MasterHelper-AP</code> on a smartphone, please temporarily disable <strong>Mobile Data (4G/5G)</strong> so your browser routes traffic directly to <code>192.168.4.1</code>.</span>
    </div>

    <div class="card">
      <div class="card-title">System Overview</div>
      <div class="grid-2">
        <div class="stat-box">
          <div class="stat-label">Device Name</div>
          <div class="stat-value" id="stat-name">MasterHelper</div>
        </div>
        <div class="stat-box">
          <div class="stat-label">Running Version</div>
          <div class="stat-value" id="stat-ver">...</div>
        </div>
        <div class="stat-box">
          <div class="stat-label">Current Hash</div>
          <div class="stat-value code-inline" id="stat-hash">...</div>
        </div>
        <div class="stat-box">
          <div class="stat-label">Active Boot Partition</div>
          <div class="stat-value" id="stat-partition">...</div>
        </div>
        <div class="stat-box">
          <div class="stat-label">IP Address</div>
          <div class="stat-value" id="stat-ip">...</div>
        </div>
        <div class="stat-box">
          <div class="stat-label">Wi-Fi Mode &amp; RSSI</div>
          <div class="stat-value" id="stat-rssi">...</div>
        </div>
        <div class="stat-box">
          <div class="stat-label">Free Heap RAM</div>
          <div class="stat-value" id="stat-heap">...</div>
        </div>
        <div class="stat-box">
          <div class="stat-label">Device Uptime</div>
          <div class="stat-value" id="stat-uptime">...</div>
        </div>
      </div>
    </div>
  </div>

  <!-- WI-FI & SETTINGS TAB -->
  <div id="tab-wifi" class="tab-content hidden">
    <div class="card">
      <div class="card-title">Configuration &amp; Settings</div>
      <div id="cfg-alert" class="alert hidden"></div>

      <div class="form-group">
        <label for="cfg-device-name">Device Name</label>
        <input type="text" id="cfg-device-name" placeholder="e.g. MasterHelper">
      </div>

      <div class="form-group">
        <label for="cfg-ssid">Wi-Fi Network Name (SSID)</label>
        <input type="text" id="cfg-ssid" placeholder="Enter Wi-Fi SSID">
      </div>

      <div class="form-group">
        <label for="cfg-pass">Wi-Fi Password</label>
        <input type="password" id="cfg-pass" placeholder="Enter Wi-Fi Password">
      </div>

      <div class="form-group">
        <label for="cfg-github-url">GitHub Update Manifest URL (version.json)</label>
        <input type="text" id="cfg-github-url" placeholder="https://raw.githubusercontent.com/user/repo/main/compiled/version.json">
        <small class="helper-text">
          Points to raw JSON manifest in your GitHub repository containing version, commit hash, and firmware download link.
        </small>
      </div>

      <div class="btn-group">
        <button class="btn btn-primary" onclick="saveConfig(false)">Save Settings</button>
        <button class="btn btn-success" onclick="saveConfig(true)">Save &amp; Reconnect</button>
      </div>
    </div>

    <div class="card">
      <div class="card-title">System Controls</div>
      <button class="btn btn-danger" onclick="restartDevice()">Restart ESP32</button>
    </div>
  </div>

  <!-- GITHUB OTA UPDATER TAB -->
  <div id="tab-ota" class="tab-content hidden">
    <div class="card">
      <div class="card-title">GitHub Firmware Updater</div>
      <p class="section-desc">
        The device checks the remote <span class="code-inline">version.json</span> manifest file. 
        Updates are triggered <strong>only when the version number changes</strong> and the remote commit hash does not match the currently running build.
      </p>

      <div class="grid-2 mb-15">
        <div class="stat-box">
          <div class="stat-label">Currently Running Build</div>
          <div class="stat-value" id="ota-cur-ver">Version: ...</div>
          <div class="sub-stat">Hash: <span class="code-inline" id="ota-cur-hash">...</span></div>
        </div>

        <div class="stat-box">
          <div class="stat-label">Remote GitHub Build</div>
          <div class="stat-value" id="ota-rem-ver">Version: (Not checked)</div>
          <div class="sub-stat">Hash: <span class="code-inline" id="ota-rem-hash">-</span></div>
        </div>
      </div>

      <div id="ota-check-result" class="alert hidden"></div>

      <div class="btn-group mb-15">
        <button id="btn-check-ota" class="btn btn-primary" onclick="checkOtaUpdate()">
          &#128269; Check for Updates
        </button>
        <button id="btn-run-ota" class="btn btn-success" onclick="startOtaUpdate()" disabled>
          &#11015; Install Default Update (main.bin)
        </button>
      </div>

      <!-- Test / Alternate Builds Section -->
      <div id="test-builds-section" class="card hidden" style="background: rgba(15, 23, 42, 0.5); border-style: dashed; margin-top: 1rem;">
        <div class="card-title">&#129514; Available Test &amp; Alternate Builds</div>
        <p class="section-desc">Select an experimental build or test binary from the repository:</p>
        <div class="form-group">
          <label for="select-test-build">Select Binary from compiled/:</label>
          <select id="select-test-build" class="select-build" onchange="onTestBuildSelected()"></select>
        </div>
        <div id="test-build-details" class="stat-box mb-15">
          <div class="stat-label">Selected Build Info</div>
          <div id="test-build-info" style="font-size:0.85rem; color:var(--text-dim); line-height: 1.5;">...</div>
        </div>
        <button id="btn-flash-test" class="btn btn-primary" onclick="startTestBuildUpdate()">
          &#9889; Flash Selected Test Build
        </button>
      </div>

      <!-- OTA Progress Area -->
      <div id="ota-progress-area" class="hidden">
        <div class="progress-info">
          <span id="ota-status-text">Downloading update...</span>
          <span id="ota-percent-text">0%</span>
        </div>
        <div class="progress-bar-bg">
          <div id="ota-progress-bar" class="progress-bar-fill"></div>
        </div>
      </div>

      <div class="manifest-doc">
        <h4>Expected GitHub version.json Format:</h4>
        <pre>{
  "version": "1.0.1",
  "hash": "8f3b92a",
  "bin_url": "https://raw.githubusercontent.com/user/repo/main/compiled/main.bin",
  "notes": "Added new helper endpoints and stability fixes",
  "test_builds": [
    { "name": "Beta 1", "version": "1.0.1-b1", "hash": "abc", "bin_url": "..." }
  ]
}</pre>
      </div>
    </div>
  </div>

  <!-- MANUAL FLASH TAB -->
  <div id="tab-manual" class="tab-content hidden">
    <div class="card">
      <div class="card-title">Manual Firmware Upload</div>
      <p class="section-desc">
        Directly upload a compiled binary file (<span class="code-inline">.bin</span>) into the inactive OTA partition over local Wi-Fi without GitHub.
      </p>

      <div class="form-group">
        <label for="firmware-file">Select Compiled Firmware (.bin)</label>
        <input type="file" id="firmware-file" accept=".bin">
      </div>

      <button id="btn-upload-file" class="btn btn-primary" onclick="uploadFirmwareManual()">
        &#9889; Flash Firmware File
      </button>

      <div id="manual-progress-area" class="hidden" style="margin-top: 1.5rem;">
        <div class="progress-info">
          <span id="manual-status-text">Uploading firmware...</span>
          <span id="manual-percent-text">0%</span>
        </div>
        <div class="progress-bar-bg">
          <div id="manual-progress-bar" class="progress-bar-fill"></div>
        </div>
      </div>
    </div>
  </div>
</div>

<script>
let lastManifest = null;
let pollTimer = null;
let testBuildsList = [];

function switchTab(tabId, btn) {
  document.querySelectorAll('.tab-content').forEach(el => el.classList.add('hidden'));
  document.querySelectorAll('.tab-btn').forEach(el => el.classList.remove('active'));
  const target = document.getElementById(tabId);
  if (target) target.classList.remove('hidden');
  if (btn) {
    btn.classList.add('active');
  } else {
    const el = document.querySelector('[onclick*="' + tabId + '"]');
    if (el) el.classList.add('active');
  }
}
window.switchTab = switchTab;

async function fetchStatus() {
  const connHelper = document.getElementById('conn-helper-alert');
  try {
    const res = await fetch('/api/status', { cache: 'no-store' });
    if (!res.ok) {
      const netBadge = document.getElementById('net-badge');
      if (netBadge) {
        netBadge.className = 'badge badge-yellow';
        netBadge.textContent = 'API Error: ' + res.status;
      }
      return;
    }
    const data = await res.json();
    if (connHelper) connHelper.classList.add('hidden');

    document.getElementById('stat-name').textContent = data.device_name || 'MasterHelper';
    document.getElementById('stat-ver').textContent = 'v' + data.version;
    document.getElementById('stat-hash').textContent = data.hash || 'unknown';
    document.getElementById('stat-partition').textContent = data.partition || 'ota_0';
    document.getElementById('stat-ip').textContent = data.ip;
    document.getElementById('stat-rssi').textContent = (data.mode || 'STA') + (data.rssi ? ' (' + data.rssi + ' dBm)' : '');
    document.getElementById('stat-heap').textContent = (data.free_heap / 1024).toFixed(1) + ' KB';

    const sec = data.uptime || 0;
    const h = Math.floor(sec / 3600);
    const m = Math.floor((sec % 3600) / 60);
    const s = sec % 60;
    document.getElementById('stat-uptime').textContent = h + 'h ' + m + 'm ' + s + 's';

    document.getElementById('ota-cur-ver').textContent = 'Version: v' + data.version;
    document.getElementById('ota-cur-hash').textContent = data.hash || 'none';

    const netBadge = document.getElementById('net-badge');
    if (data.mode === 'AP' || data.mode === 'AP_MODE') {
      netBadge.className = 'badge badge-yellow';
      netBadge.innerHTML = '&#128246; SoftAP: ' + data.ip;
    } else {
      netBadge.className = 'badge badge-green';
      netBadge.innerHTML = '&#10004; Wi-Fi: ' + data.ip;
    }
  } catch (e) {
    console.error('Error fetching status', e);
    const netBadge = document.getElementById('net-badge');
    if (netBadge) {
      netBadge.className = 'badge badge-yellow';
      netBadge.innerHTML = '&#9888; Retrying...';
    }
    if (connHelper) connHelper.classList.remove('hidden');
  }
}

async function loadConfig() {
  try {
    const res = await fetch('/api/config', { cache: 'no-store' });
    if (!res.ok) return;
    const data = await res.json();
    document.getElementById('cfg-device-name').value = data.device_name || '';
    document.getElementById('cfg-ssid').value = data.wifi_ssid || '';
    document.getElementById('cfg-github-url').value = data.github_url || '';
  } catch (e) {
    console.error('Error loading config', e);
  }
}

async function saveConfig(reconnect) {
  const alertEl = document.getElementById('cfg-alert');
  alertEl.className = 'alert alert-info';
  alertEl.textContent = 'Saving configuration...';
  alertEl.classList.remove('hidden');

  const payload = {
    device_name: document.getElementById('cfg-device-name').value.trim(),
    wifi_ssid: document.getElementById('cfg-ssid').value.trim(),
    wifi_pass: document.getElementById('cfg-pass').value,
    github_url: document.getElementById('cfg-github-url').value.trim(),
    reconnect: reconnect
  };

  try {
    const res = await fetch('/api/config', {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify(payload)
    });
    if (res.ok) {
      alertEl.className = 'alert alert-success';
      alertEl.textContent = reconnect ? 'Settings saved! Reconnecting Wi-Fi...' : 'Settings saved successfully!';
      setTimeout(fetchStatus, 1500);
    } else {
      alertEl.className = 'alert alert-danger';
      alertEl.textContent = 'Failed to save settings.';
    }
  } catch (e) {
    alertEl.className = 'alert alert-danger';
    alertEl.textContent = 'Network error saving settings: ' + e;
  }
}

async function checkOtaUpdate() {
  const btn = document.getElementById('btn-check-ota');
  const alertEl = document.getElementById('ota-check-result');
  const runBtn = document.getElementById('btn-run-ota');

  btn.disabled = true;
  btn.innerHTML = '&#8987; Checking GitHub...';
  alertEl.className = 'alert alert-info';
  alertEl.textContent = 'Contacting GitHub manifest URL...';
  alertEl.classList.remove('hidden');

  try {
    const res = await fetch('/api/ota/check', { method: 'POST' });
    const data = await res.json();
    btn.disabled = false;
    btn.innerHTML = '&#128269; Check for Updates';

    document.getElementById('ota-rem-ver').textContent = 'Version: ' + (data.remote_version ? 'v' + data.remote_version : 'Unknown');
    document.getElementById('ota-rem-hash').textContent = data.remote_hash || '-';

    if (data.update_available) {
      lastManifest = data;
      alertEl.className = 'alert alert-success';
      alertEl.innerHTML = '<strong>Update Available!</strong> New Version: v' + data.remote_version + ' | Hash: <span class="code-inline">' + data.remote_hash + '</span><br>Changelog: ' + (data.notes || 'No release notes.');
      runBtn.disabled = false;
    } else {
      lastManifest = null;
      runBtn.disabled = true;
      alertEl.className = 'alert alert-info';
      alertEl.textContent = data.message || 'Firmware is already up to date.';
    }

    const testSection = document.getElementById('test-builds-section');
    const testSelect = document.getElementById('select-test-build');
    testBuildsList = data.test_builds || [];

    if (testBuildsList.length > 0) {
      testSection.classList.remove('hidden');
      testSelect.innerHTML = testBuildsList.map((tb, idx) => 
        '<option value="' + idx + '">' + (tb.name || tb.version) + ' (v' + tb.version + ')</option>'
      ).join('');
      onTestBuildSelected();
    } else {
      testSection.classList.add('hidden');
    }
  } catch (e) {
    btn.disabled = false;
    btn.innerHTML = '&#128269; Check for Updates';
    alertEl.className = 'alert alert-danger';
    alertEl.textContent = 'Failed to check updates: ' + e;
  }
}

function onTestBuildSelected() {
  const select = document.getElementById('select-test-build');
  const idx = parseInt(select.value, 10);
  const infoEl = document.getElementById('test-build-info');
  if (isNaN(idx) || !testBuildsList[idx]) return;

  const tb = testBuildsList[idx];
  infoEl.innerHTML = '<strong>Name:</strong> ' + (tb.name || 'Unnamed Test') + '<br>' +
    '<strong>Version:</strong> v' + tb.version + '<br>' +
    '<strong>Hash:</strong> <span class="code-inline">' + tb.hash + '</span><br>' +
    '<strong>URL:</strong> ' + tb.bin_url + '<br>' +
    '<strong>Notes:</strong> ' + (tb.notes || 'No notes provided.');
}

async function startOtaUpdate() {
  if (!confirm('Download and install this update now? The ESP32 will reboot.')) return;
  const pArea = document.getElementById('ota-progress-area');
  const pBar = document.getElementById('ota-progress-bar');
  const pText = document.getElementById('ota-status-text');
  const pPct = document.getElementById('ota-percent-text');

  pArea.classList.remove('hidden');
  pBar.style.width = '0%';
  pPct.textContent = '0%';
  pText.textContent = 'Starting OTA update...';

  try {
    const res = await fetch('/api/ota/update', {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({})
    });
    const data = await res.json();
    if (!data.success) {
      alert('Update failed to start: ' + data.message);
      pArea.classList.add('hidden');
      return;
    }
    pollOtaStatus();
  } catch (e) {
    alert('Failed to initiate OTA: ' + e);
    pArea.classList.add('hidden');
  }
}

async function startTestBuildUpdate() {
  const select = document.getElementById('select-test-build');
  const idx = parseInt(select.value, 10);
  if (isNaN(idx) || !testBuildsList[idx]) {
    alert('Please select a valid test build.');
    return;
  }
  const tb = testBuildsList[idx];
  if (!confirm('Install test build "' + (tb.name || tb.version) + '"? Device will reboot upon completion.')) return;

  const pArea = document.getElementById('ota-progress-area');
  const pBar = document.getElementById('ota-progress-bar');
  const pText = document.getElementById('ota-status-text');
  const pPct = document.getElementById('ota-percent-text');

  pArea.classList.remove('hidden');
  pBar.style.width = '0%';
  pPct.textContent = '0%';
  pText.textContent = 'Starting test build update...';

  try {
    const res = await fetch('/api/ota/update', {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({
        custom_bin_url: tb.bin_url,
        version: tb.version,
        hash: tb.hash
      })
    });
    const data = await res.json();
    if (!data.success) {
      alert('Test update failed: ' + data.message);
      pArea.classList.add('hidden');
      return;
    }
    pollOtaStatus();
  } catch (e) {
    alert('Failed to initiate test build OTA: ' + e);
    pArea.classList.add('hidden');
  }
}

function pollOtaStatus() {
  const pBar = document.getElementById('ota-progress-bar');
  const pText = document.getElementById('ota-status-text');
  const pPct = document.getElementById('ota-percent-text');

  const timer = setInterval(async () => {
    try {
      const res = await fetch('/api/ota/status', { cache: 'no-store' });
      if (!res.ok) return;
      const data = await res.json();

      pBar.style.width = data.progress + '%';
      pPct.textContent = data.progress + '%';
      pText.textContent = data.message || ('Downloading: ' + data.progress + '%');

      if (data.state === 'SUCCESS') {
        clearInterval(timer);
        pText.textContent = 'Update installed! Device is restarting...';
        setTimeout(() => location.reload(), 8000);
      } else if (data.state === 'FAILED') {
        clearInterval(timer);
        alert('OTA Update Failed: ' + data.message);
      }
    } catch (e) {
      console.log('Poll waiting for reboot/network...');
    }
  }, 1000);
}

function uploadFirmwareManual() {
  const fileInput = document.getElementById('firmware-file');
  const file = fileInput.files[0];
  if (!file) {
    alert('Please select a .bin file first.');
    return;
  }

  const pArea = document.getElementById('manual-progress-area');
  const pBar = document.getElementById('manual-progress-bar');
  const pText = document.getElementById('manual-status-text');
  const pPct = document.getElementById('manual-percent-text');
  const btn = document.getElementById('btn-upload-file');

  btn.disabled = true;
  pArea.classList.remove('hidden');

  const xhr = new XMLHttpRequest();
  xhr.open('POST', '/api/ota/upload', true);

  xhr.upload.onprogress = (e) => {
    if (e.lengthComputable) {
      const pct = Math.round((e.loaded / e.total) * 100);
      pBar.style.width = pct + '%';
      pPct.textContent = pct + '%';
      pText.textContent = 'Uploading: ' + pct + '%';
    }
  };

  xhr.onload = () => {
    if (xhr.status === 200) {
      pText.textContent = 'Upload complete! Rebooting device...';
      setTimeout(() => location.reload(), 7000);
    } else {
      alert('Upload failed: ' + xhr.responseText);
      btn.disabled = false;
    }
  };

  xhr.onerror = () => {
    alert('Network error during upload');
    btn.disabled = false;
  };

  xhr.send(file);
}

async function restartDevice() {
  if (!confirm('Reboot the ESP32 now?')) return;
  await fetch('/api/restart', { method: 'POST' });
  alert('Device is rebooting. Page will reload in 7 seconds.');
  setTimeout(() => location.reload(), 7000);
}

document.addEventListener('DOMContentLoaded', () => {
  document.querySelectorAll('.tab-btn').forEach(btn => {
    btn.addEventListener('click', function() {
      const onclickAttr = this.getAttribute('onclick') || '';
      const match = onclickAttr.match(/switchTab\('([^']+)'/);
      if (match && match[1]) {
        switchTab(match[1], this);
      }
    });
  });
  fetchStatus();
  loadConfig();
  if (!pollTimer) {
    pollTimer = setInterval(fetchStatus, 5000);
  }
});

fetchStatus();
loadConfig();
if (!pollTimer) {
  pollTimer = setInterval(fetchStatus, 5000);
}
</script>
</body>
</html>
)rawliteral";
