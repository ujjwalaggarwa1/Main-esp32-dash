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
    const el = document.querySelector(`[onclick*="${tabId}"]`);
    if (el) el.classList.add('active');
  }
}

async function fetchStatus() {
  try {
    const res = await fetch('/api/status');
    if (!res.ok) return;
    const data = await res.json();

    document.getElementById('stat-name').textContent = data.device_name || 'MasterHelper';
    document.getElementById('stat-ver').textContent = 'v' + data.version;
    document.getElementById('stat-hash').textContent = data.hash || 'unknown';
    document.getElementById('stat-partition').textContent = data.partition || 'ota_0';
    document.getElementById('stat-ip').textContent = data.ip;
    document.getElementById('stat-rssi').textContent = (data.mode || 'STA') + (data.rssi ? ` (${data.rssi} dBm)` : '');
    document.getElementById('stat-heap').textContent = (data.free_heap / 1024).toFixed(1) + ' KB';

    const sec = data.uptime || 0;
    const h = Math.floor(sec / 3600);
    const m = Math.floor((sec % 3600) / 60);
    const s = sec % 60;
    document.getElementById('stat-uptime').textContent = `${h}h ${m}m ${s}s`;

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
  }
}

async function loadConfig() {
  try {
    const res = await fetch('/api/config');
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
      alertEl.innerHTML = `<strong>Update Available!</strong> New Version: v${data.remote_version} | Hash: <span class="code-inline">${data.remote_hash}</span><br>Changelog: ${data.notes || 'No release notes.'}`;
      runBtn.disabled = false;
    } else {
      lastManifest = null;
      runBtn.disabled = true;
      alertEl.className = 'alert alert-info';
      alertEl.textContent = data.message || 'Firmware is already up to date.';
    }

    // Handle available test builds
    const testSection = document.getElementById('test-builds-section');
    const testSelect = document.getElementById('select-test-build');
    testBuildsList = data.test_builds || [];

    if (testBuildsList.length > 0) {
      testSection.classList.remove('hidden');
      testSelect.innerHTML = testBuildsList.map((tb, idx) => 
        `<option value="${idx}">${tb.name || tb.version} (v${tb.version})</option>`
      ).join('');
      onTestBuildSelected();
    } else {
      testSection.classList.add('hidden');
    }
  } catch (e) {
    btn.disabled = false;
    btn.innerHTML = '&#128269; Check for Updates';
    alertEl.className = 'alert alert-danger';
    alertEl.textContent = 'Error checking update: ' + e;
  }
}

async function startOtaUpdate() {
  if (!confirm('Start flashing the new firmware version from GitHub? The device will reboot automatically once finished.')) return;

  const runBtn = document.getElementById('btn-run-ota');
  const checkBtn = document.getElementById('btn-check-ota');
  runBtn.disabled = true;
  checkBtn.disabled = true;

  const progressArea = document.getElementById('ota-progress-area');
  progressArea.classList.remove('hidden');

  try {
    const res = await fetch('/api/ota/update', {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({
        bin_url: lastManifest ? lastManifest.bin_url : '',
        new_version: lastManifest ? lastManifest.remote_version : '',
        new_hash: lastManifest ? lastManifest.remote_hash : ''
      })
    });

    if (res.ok) {
      pollOtaProgress();
    } else {
      alert('Failed to trigger update on device.');
      runBtn.disabled = false;
      checkBtn.disabled = false;
    }
  } catch (e) {
    alert('Error triggering OTA: ' + e);
    runBtn.disabled = false;
    checkBtn.disabled = false;
  }
}

function onTestBuildSelected() {
  const sel = document.getElementById('select-test-build');
  const infoEl = document.getElementById('test-build-info');
  const idx = parseInt(sel.value, 10);
  if (isNaN(idx) || !testBuildsList[idx]) {
    infoEl.textContent = 'No test build selected';
    return;
  }
  const tb = testBuildsList[idx];
  infoEl.innerHTML = `<strong>Version:</strong> v${tb.version} | <strong>Hash:</strong> <span class="code-inline">${tb.hash}</span><br><strong>URL:</strong> <span class="code-inline">${tb.bin_url}</span><br><strong>Notes:</strong> ${tb.notes || 'None'}`;
}

async function startTestBuildUpdate() {
  const sel = document.getElementById('select-test-build');
  const idx = parseInt(sel.value, 10);
  if (isNaN(idx) || !testBuildsList[idx]) {
    alert('Please select a valid test build first.');
    return;
  }
  const tb = testBuildsList[idx];

  if (!confirm(`Flash test build "${tb.name || tb.version}" (v${tb.version}) to the ESP32? The device will reboot after completion.`)) {
    return;
  }

  const runBtn = document.getElementById('btn-run-ota');
  const checkBtn = document.getElementById('btn-check-ota');
  const flashTestBtn = document.getElementById('btn-flash-test');
  runBtn.disabled = true;
  checkBtn.disabled = true;
  flashTestBtn.disabled = true;

  const progressArea = document.getElementById('ota-progress-area');
  progressArea.classList.remove('hidden');

  try {
    const res = await fetch('/api/ota/update', {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({
        bin_url: tb.bin_url,
        new_version: tb.version,
        new_hash: tb.hash
      })
    });

    if (res.ok) {
      pollOtaProgress();
    } else {
      alert('Failed to trigger test update on device.');
      runBtn.disabled = false;
      checkBtn.disabled = false;
      flashTestBtn.disabled = false;
    }
  } catch (e) {
    alert('Error triggering test OTA: ' + e);
    runBtn.disabled = false;
    checkBtn.disabled = false;
    flashTestBtn.disabled = false;
  }
}

function pollOtaProgress() {
  if (pollTimer) clearInterval(pollTimer);
  pollTimer = setInterval(async () => {
    try {
      const res = await fetch('/api/ota/status');
      if (!res.ok) return;
      const data = await res.json();

      const pct = data.progress || 0;
      document.getElementById('ota-progress-bar').style.width = pct + '%';
      document.getElementById('ota-percent-text').textContent = pct + '%';
      document.getElementById('ota-status-text').textContent = data.message || 'Updating...';

      if (data.state === 'SUCCESS' || pct >= 100) {
        clearInterval(pollTimer);
        document.getElementById('ota-status-text').textContent = 'Update installed! Device is rebooting...';
        setTimeout(() => {
          alert('Device rebooting. Page will reload in 10 seconds.');
          location.reload();
        }, 3000);
      } else if (data.state === 'FAILED') {
        clearInterval(pollTimer);
        alert('OTA Update Failed: ' + (data.error || 'Unknown error'));
        document.getElementById('btn-run-ota').disabled = false;
        document.getElementById('btn-check-ota').disabled = false;
      }
    } catch (e) {
      // Possible device rebooting
    }
  }, 1000);
}

async function uploadManualBinary() {
  const fileInput = document.getElementById('manual-file');
  if (!fileInput.files || fileInput.files.length === 0) {
    alert('Please select a .bin firmware file first.');
    return;
  }
  const file = fileInput.files[0];
  const btn = document.getElementById('btn-manual-upload');
  btn.disabled = true;

  const pArea = document.getElementById('manual-progress-area');
  const pBar = document.getElementById('manual-progress-bar');
  const pText = document.getElementById('manual-status-text');
  const pPct = document.getElementById('manual-percent-text');
  pArea.classList.remove('hidden');

  const xhr = new XMLHttpRequest();
  xhr.open('POST', '/api/ota/upload', true);

  xhr.upload.onprogress = (e) => {
    if (e.lengthComputable) {
      const pct = Math.round((e.loaded / e.total) * 100);
      pBar.style.width = pct + '%';
      pPct.textContent = pct + '%';
      pText.textContent = `Uploading: ${pct}%`;
    }
  };

  xhr.onload = () => {
    if (xhr.status === 200) {
      pText.textContent = 'Upload complete! Rebooting device...';
      setTimeout(() => { location.reload(); }, 6000);
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

// Initial loads
fetchStatus();
loadConfig();
setInterval(fetchStatus, 5000);
