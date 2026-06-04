#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <DHT.h>
#include <time.h>
#include <ESPmDNS.h>
#include <Preferences.h>

// ===== WIFI =====
const char* WIFI_SSID = "SINGTEL-A6EK";
const char* WIFI_PASS = "hkcb5h674n";
#define HOSTNAME "voiddeck"

// ===== DHT =====
#define DHTPIN   26
#define DHTTYPE  DHT11
DHT dht(DHTPIN, DHTTYPE);

// ===== Web server =====
WebServer server(80);
Preferences prefs;

// ===== Reading cadence =====
const unsigned long READ_INTERVAL_MS = 2000;
unsigned long lastReadMs = 0;

// ===== Latest reading =====
volatile float curT = NAN, curH = NAN;
volatile bool  curOK = false;
volatile uint8_t curTries = 0;
volatile uint32_t curUpdatedMs = 0;

// ===== Hourly accumulation =====
double sumT = 0.0, sumH = 0.0;
uint32_t sampleCount = 0;
time_t currentHourStart = 0;

// ===== Daily accumulation =====
double daySumT = 0.0, daySumH = 0.0;
uint32_t daySampleCount = 0;
time_t currentDayStart = 0;

// ===== Hourly history (24h) =====
static const int HIST_H = 24;
time_t histHour[HIST_H];
float  histAvgT[HIST_H];
float  histAvgH[HIST_H];
int    histHead = -1;

// ===== Daily history (7d) =====
static const int HIST_D = 7;
time_t histDay[HIST_D];
float  histDayAvgT[HIST_D];
float  histDayAvgH[HIST_D];
int    histDayHead = -1;

// ===== NVS persistence =====
void saveHourly() {
  prefs.begin("dht_hist", false);
  prefs.putInt("head", histHead);
  prefs.putBytes("hours", histHour, sizeof(histHour));
  prefs.putBytes("avgT",  histAvgT, sizeof(histAvgT));
  prefs.putBytes("avgH",  histAvgH, sizeof(histAvgH));
  prefs.end();
}

void loadHourly() {
  prefs.begin("dht_hist", true);
  histHead = prefs.getInt("head", -1);
  prefs.getBytes("hours", histHour, sizeof(histHour));
  prefs.getBytes("avgT",  histAvgT, sizeof(histAvgT));
  prefs.getBytes("avgH",  histAvgH, sizeof(histAvgH));
  prefs.end();
  Serial.printf("Loaded hourly history, head=%d\n", histHead);
}

void saveDaily() {
  prefs.begin("dht_day", false);
  prefs.putInt("head", histDayHead);
  prefs.putBytes("days",  histDay,     sizeof(histDay));
  prefs.putBytes("avgT",  histDayAvgT, sizeof(histDayAvgT));
  prefs.putBytes("avgH",  histDayAvgH, sizeof(histDayAvgH));
  prefs.end();
}

void loadDaily() {
  prefs.begin("dht_day", true);
  histDayHead = prefs.getInt("head", -1);
  prefs.getBytes("days",  histDay,     sizeof(histDay));
  prefs.getBytes("avgT",  histDayAvgT, sizeof(histDayAvgT));
  prefs.getBytes("avgH",  histDayAvgH, sizeof(histDayAvgH));
  prefs.end();
  Serial.printf("Loaded daily history, head=%d\n", histDayHead);
}

// ===== Helpers =====
static inline time_t nowEpoch() { time_t t = time(nullptr); return t > 100000 ? t : 0; }
static inline time_t hourStart(time_t t) { return t - (t % 3600); }
static inline time_t dayStart(time_t t)  { return t - (t % 86400); } // UTC day; fine since we offset NTP

static void pushHourly(time_t hour, float aT, float aH) {
  histHead = (histHead + 1) % HIST_H;
  histHour[histHead] = hour;
  histAvgT[histHead] = aT;
  histAvgH[histHead] = aH;
  saveHourly();
}

static void pushDaily(time_t day, float aT, float aH) {
  histDayHead = (histDayHead + 1) % HIST_D;
  histDay[histDayHead]     = day;
  histDayAvgT[histDayHead] = aT;
  histDayAvgH[histDayHead] = aH;
  saveDaily();
}

static void finalizeHour(time_t hour) {
  if (sampleCount == 0) return;
  float aT = (float)(sumT / sampleCount);
  float aH = (float)(sumH / sampleCount);
  pushHourly(hour, aT, aH);
  // also accumulate into daily
  daySumT += aT; daySumH += aH; daySampleCount++;
  sumT = sumH = 0.0; sampleCount = 0;
}

static void finalizeDay(time_t day) {
  if (daySampleCount == 0) return;
  pushDaily(day, (float)(daySumT / daySampleCount), (float)(daySumH / daySampleCount));
  daySumT = daySumH = 0.0; daySampleCount = 0;
}

// ===== Comfort score (0-100) =====
// Based on temperature and humidity. Returns score and label index.
// Label indices: 0=Ideal 1=Comfortable 2=Warm 3=Humid 4=Hot
static int comfortScore(float t, float h, int &labelIdx) {
  if (isnan(t) || isnan(h)) { labelIdx = 0; return -1; }
  // Penalise deviation from ideal (24°C, 55% RH)
  float tPenalty = abs(t - 24.0f) * 4.0f;   // -4 pts per °C from ideal
  float hPenalty = abs(h - 55.0f) * 0.6f;   // -0.6 pts per % RH from ideal
  // Extra penalty for hot+humid combo (feels worse than either alone)
  float combo = (t > 28.0f && h > 70.0f) ? (t - 28.0f) * (h - 70.0f) * 0.15f : 0;
  int score = (int)constrain(100.0f - tPenalty - hPenalty - combo, 0.0f, 100.0f);
  if      (score >= 80) labelIdx = 0; // Ideal
  else if (score >= 60) labelIdx = 1; // Comfortable
  else if (h > 75)      labelIdx = 3; // Humid
  else if (t > 30)      labelIdx = 4; // Hot
  else                  labelIdx = 2; // Warm
  return score;
}

// ===== Rolling averages from hourly buffer =====
// n = number of most recent completed hours to average
static bool rollingAvg(int n, float &outT, float &outH) {
  if (histHead < 0) return false;
  double st = 0, sh = 0; int cnt = 0;
  for (int i = 0; i < HIST_H && cnt < n; i++) {
    int idx = (histHead - i + HIST_H) % HIST_H;
    if (histHour[idx] == 0) break;
    st += histAvgT[idx]; sh += histAvgH[idx]; cnt++;
  }
  if (cnt == 0) return false;
  outT = (float)(st / cnt); outH = (float)(sh / cnt);
  return true;
}

// ===== HTTP handlers =====
const char* HTML = R"HTML(
<!doctype html><html>
<head>
<meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1">
<title>Voiddeck ENV</title>
<script src="https://cdn.jsdelivr.net/npm/chart.js@4.4.0/dist/chart.umd.min.js"></script>
<style>
  body { font-family: system-ui, sans-serif; margin: 0; padding: 20px; background: #fff; color: #111; }
  @media (prefers-color-scheme: dark) { body { background: #111; color: #eee; } }
  .wrap { max-width: 860px; }
  h1 { font-size: 1rem; font-weight: 500; margin: 0 0 4px; }
  h2 { font-size: 0.8rem; font-weight: 500; margin: 24px 0 8px; color: #888; text-transform: uppercase; letter-spacing: 0.05em; }
  .sub { font-size: 0.8rem; color: #888; margin-bottom: 20px; }
  .cards { display: grid; grid-template-columns: repeat(auto-fit, minmax(130px, 1fr)); gap: 10px; margin-bottom: 12px; }
  .card { background: #f5f5f5; border-radius: 8px; padding: 12px 14px; }
  @media (prefers-color-scheme: dark) { .card { background: #1e1e1e; } }
  .card-label { font-size: 11px; color: #888; margin-bottom: 4px; text-transform: uppercase; letter-spacing: 0.05em; }
  .big { font-size: 28px; font-weight: 500; line-height: 1.1; }
  .unit { font-size: 11px; color: #888; }
  .status { font-size: 11px; color: #888; margin-top: 6px; }
  .t-color { color: #c0392b; }
  .h-color { color: #16a085; }
  .comfort-ideal       { color: #27ae60; }
  .comfort-comfortable { color: #2980b9; }
  .comfort-warm        { color: #e67e22; }
  .comfort-humid       { color: #8e44ad; }
  .comfort-hot         { color: #c0392b; }

  /* Rolling averages table */
  .avg-table { width: 100%; border-collapse: collapse; font-size: 13px; margin-bottom: 20px; }
  .avg-table th { font-size: 11px; color: #888; font-weight: 400; text-transform: uppercase;
                  padding: 5px 6px; border-bottom: 1px solid #eee; text-align: center; }
  .avg-table td { padding: 7px 6px; text-align: center; border-bottom: 1px solid #eee; }
  .avg-table td:first-child { text-align: left; color: #888; font-size: 12px; }
  @media (prefers-color-scheme: dark) { .avg-table th, .avg-table td { border-color: #333; } }

  /* Chart */
  .chart-wrap { margin-bottom: 20px; }
  .legend { display: flex; gap: 16px; font-size: 12px; color: #888; margin-bottom: 8px; }
  .leg-line { width: 24px; height: 3px; display: inline-block; border-radius: 2px; vertical-align: middle; margin-right: 4px; }

  /* History tables */
  table.hist { width: 100%; border-collapse: collapse; font-size: 13px; margin-bottom: 20px; }
  table.hist th { text-align: left; padding: 6px 4px; border-bottom: 1px solid #ddd;
                  font-weight: 400; color: #888; font-size: 11px; text-transform: uppercase; }
  table.hist td { padding: 7px 4px; border-bottom: 1px solid #eee; vertical-align: top; }
  @media (prefers-color-scheme: dark) { table.hist th, table.hist td { border-color: #333; } }
  tr.active-row td { background: #fff8e1; font-weight: 500; }
  @media (prefers-color-scheme: dark) { tr.active-row td { background: #2a2200; } }
  tr.active-row td:first-child { border-left: 3px solid #e67e22; padding-left: 1px; }

  /* Guide tables */
  .bar-cell { width: 90px; }
  .bar-bg { background: #eee; border-radius: 4px; height: 7px; overflow: hidden; margin-top: 5px; }
  @media (prefers-color-scheme: dark) { .bar-bg { background: #333; } }
  .bar-fill { height: 7px; border-radius: 4px; }
  details { margin-top: 16px; }
  summary { cursor: pointer; font-size: 0.85rem; color: #888; user-select: none; }
  summary:hover { color: #555; }
</style>
</head>
<body>
<div class="wrap">
  <h1>Voiddeck ENV Monitor</h1>
  <div class="sub" id="date">--</div>

  <!-- Live readings + comfort -->
  <div class="cards">
    <div class="card">
      <div class="card-label">Temperature</div>
      <div id="t" class="big t-color">--.-</div>
      <div class="unit">°C</div>
      <div class="status" id="msg">starting…</div>
    </div>
    <div class="card">
      <div class="card-label">Humidity</div>
      <div id="h" class="big h-color">--.-</div>
      <div class="unit">%</div>
    </div>
    <div class="card">
      <div class="card-label">Comfort</div>
      <div id="comfortScore" class="big">--</div>
      <div class="unit" id="comfortLabel">waiting…</div>
    </div>
    <div class="card">
      <div class="card-label">Clock (SGT)</div>
      <div id="clk" class="big">--:--:--</div>
      <div class="unit" id="date2">--</div>
    </div>
    <div class="card">
      <div class="card-label">Last hr avg</div>
      <div id="avgT" class="big t-color">--.-</div>
      <div class="unit t-color">°C</div>
      <div id="avgH" class="big h-color" style="margin-top:4px;">--.-</div>
      <div class="unit h-color">%</div>
      <div class="status" id="avgMeta">waiting…</div>
    </div>
  </div>

  <!-- Rolling averages -->
  <h2>Rolling averages</h2>
  <table class="avg-table">
    <thead>
      <tr>
        <th></th>
        <th>3 h</th>
        <th>12 h</th>
        <th>24 h</th>
        <th>7 d</th>
      </tr>
    </thead>
    <tbody>
      <tr>
        <td>Temp °C</td>
        <td id="r3T" class="t-color">--</td>
        <td id="r12T" class="t-color">--</td>
        <td id="r24T" class="t-color">--</td>
        <td id="r7dT" class="t-color">--</td>
      </tr>
      <tr>
        <td>Humidity %</td>
        <td id="r3H" class="h-color">--</td>
        <td id="r12H" class="h-color">--</td>
        <td id="r24H" class="h-color">--</td>
        <td id="r7dH" class="h-color">--</td>
      </tr>
    </tbody>
  </table>

  <!-- Hourly chart -->
  <h2>Last 24 hours</h2>
  <div class="chart-wrap">
    <div class="legend">
      <span><span class="leg-line" style="background:#c0392b;"></span>Temp °C (left)</span>
      <span><span class="leg-line" style="background:#16a085; border-top:2px dashed #16a085; height:0; width:24px; display:inline-block; vertical-align:middle;"></span>Humidity % (right)</span>
    </div>
    <div style="position:relative; width:100%; height:220px;">
      <canvas id="hourChart" role="img" aria-label="Hourly temperature and humidity line chart">No data yet.</canvas>
    </div>
  </div>

  <!-- Hourly history table -->
  <table class="hist">
    <thead><tr><th>Hour (SGT)</th><th class="t-color">Avg Temp °C</th><th class="h-color">Avg Hum %</th></tr></thead>
    <tbody id="hourTbody"></tbody>
  </table>

  <!-- Daily chart -->
  <h2>Last 7 days</h2>
  <div class="chart-wrap">
    <div class="legend">
      <span><span class="leg-line" style="background:#c0392b;"></span>Temp °C (left)</span>
      <span><span class="leg-line" style="background:#16a085; border-top:2px dashed #16a085; height:0; width:24px; display:inline-block; vertical-align:middle;"></span>Humidity % (right)</span>
    </div>
    <div style="position:relative; width:100%; height:180px;">
      <canvas id="dayChart" role="img" aria-label="Daily average temperature and humidity bar chart">No data yet.</canvas>
    </div>
  </div>

  <!-- Daily history table -->
  <table class="hist">
    <thead><tr><th>Day</th><th class="t-color">Avg Temp °C</th><th class="h-color">Avg Hum %</th><th>Comfort</th></tr></thead>
    <tbody id="dayTbody"></tbody>
  </table>

  <!-- Current status cards (from guide) -->
  <h2>Current status</h2>
  <div class="cards" style="margin-bottom:20px;">
    <div class="card">
      <div class="card-label">Temperature Status</div>
      <div id="tempHighlight" style="font-size:13px;">Waiting…</div>
    </div>
    <div class="card">
      <div class="card-label">Humidity Status</div>
      <div id="humHighlight" style="font-size:13px;">Waiting…</div>
    </div>
  </div>

  <!-- Guides (collapsed) -->
  <details>
    <summary>Temperature guide — bedroom context</summary>
    <table class="hist" id="tempGuide" style="margin-top:10px;">
      <thead><tr><th>Range</th><th>Feel</th><th>Indoor context</th><th>vs. outside</th></tr></thead>
      <tbody>
        <tr data-tmin="0" data-tmax="22">
          <td>Below 22°C</td><td>Cool</td>
          <td>Air-con running cold, or early morning before sunrise</td>
          <td>Singapore rarely dips below 23°C even at 3am — room is colder than outdoors</td>
        </tr>
        <tr data-tmin="22" data-tmax="25">
          <td>22 – 25°C</td><td>Comfortable</td>
          <td>Ideal sleep temperature. Air-con at 23–25°C achieves this</td>
          <td>Outside at this temp = light rain just happened or 5–6am pre-dawn</td>
        </tr>
        <tr data-tmin="25" data-tmax="28">
          <td>25 – 28°C</td><td>Warm, neutral</td>
          <td>Air-con off but fan on. Feels neutral if humidity is low</td>
          <td>Overcast afternoon or post-rain outside. Room roughly matching outdoors</td>
        </tr>
        <tr data-tmin="28" data-tmax="31">
          <td>28 – 31°C</td><td>Warm, stuffy</td>
          <td>No airflow, fan not enough. Sticky if humidity above 70%</td>
          <td>Typical sunny afternoon outside. Room at outdoor daytime level</td>
        </tr>
        <tr data-tmin="31" data-tmax="99">
          <td>Above 31°C</td><td>Hot</td>
          <td>Unventilated room mid-day. Air feels thick</td>
          <td>Hotter than shaded outdoor spot — heat is trapped. Turn on air-con</td>
        </tr>
      </tbody>
    </table>
  </details>

  <details style="margin-top:10px;">
    <summary>Humidity guide — bedroom context</summary>
    <table class="hist" id="humGuide" style="margin-top:10px;">
      <thead><tr><th>Range</th><th>Feel</th><th>Indoor context</th><th>vs. outside</th><th class="bar-cell">Level</th></tr></thead>
      <tbody>
        <tr data-hmin="0" data-hmax="40">
          <td>Below 40%</td><td>Dry</td>
          <td>Air-con long running. Throat and skin may feel dry. Unusual for SG</td>
          <td>Almost never this dry outdoors in Singapore</td>
          <td class="bar-cell"><div class="bar-bg"><div class="bar-fill" style="width:20%;background:#3498db;"></div></div></td>
        </tr>
        <tr data-hmin="40" data-hmax="60">
          <td>40 – 60%</td><td>Comfortable</td>
          <td>Ideal sleep range. Air-con achieves this. Feels fresh</td>
          <td>Only during dry northeast monsoon (Dec–Feb) on clear days</td>
          <td class="bar-cell"><div class="bar-bg"><div class="bar-fill" style="width:50%;background:#2ecc71;"></div></div></td>
        </tr>
        <tr data-hmin="60" data-hmax="70">
          <td>60 – 70%</td><td>Slightly humid</td>
          <td>Fan mode or light air-con. OK if temperature is below 27°C</td>
          <td>Cloudy morning or just after rain outside</td>
          <td class="bar-cell"><div class="bar-bg"><div class="bar-fill" style="width:65%;background:#f1c40f;"></div></div></td>
        </tr>
        <tr data-hmin="70" data-hmax="80">
          <td>70 – 80%</td><td>Humid</td>
          <td>Windows open daytime. Sweat does not evaporate well above 28°C</td>
          <td>Typical Singapore outdoor daytime humidity. Room equalised with outside</td>
          <td class="bar-cell"><div class="bar-bg"><div class="bar-fill" style="width:75%;background:#e67e22;"></div></div></td>
        </tr>
        <tr data-hmin="80" data-hmax="999">
          <td>Above 80%</td><td>Very humid</td>
          <td>Heavy rain with windows open. Mould risk if sustained</td>
          <td>Outdoor air during active rainfall. Close windows</td>
          <td class="bar-cell"><div class="bar-bg"><div class="bar-fill" style="width:90%;background:#e74c3c;"></div></div></td>
        </tr>
      </tbody>
    </table>
  </details>

</div>
<script>
let serverEpochMs = 0, localNowAtFetch = 0;
let liveT = NaN, liveH = NaN;
function fmt(n, d=1) { return (n==null||isNaN(n)) ? "--.-" : (+n).toFixed(d); }
function setClockFromServer(ms) { serverEpochMs = ms; localNowAtFetch = Date.now(); }

const COMFORT_LABELS = ['Ideal','Comfortable','Warm','Humid','Hot'];
const COMFORT_CLASSES = ['comfort-ideal','comfort-comfortable','comfort-warm','comfort-humid','comfort-hot'];

function makeChart(id, height) {
  return new Chart(document.getElementById(id), {
    type: 'line',
    data: { labels: [], datasets: [
      { label:'Temp °C', data:[], yAxisID:'yT', borderColor:'#c0392b',
        backgroundColor:'rgba(192,57,43,0.06)', tension:0.3, pointRadius:4, fill:true, borderDash:[] },
      { label:'Humidity %', data:[], yAxisID:'yH', borderColor:'#16a085',
        backgroundColor:'rgba(22,160,133,0.06)', tension:0.3, pointRadius:4, fill:true, borderDash:[5,3] }
    ]},
    options: {
      responsive:true, maintainAspectRatio:false,
      interaction:{ mode:'index', intersect:false },
      plugins:{ legend:{ display:false } },
      scales:{
        x:{ ticks:{ color:'#888', font:{size:11}, autoSkip:true, maxRotation:45 }, grid:{ color:'rgba(128,128,128,0.15)' } },
        yT:{ type:'linear', position:'left', min:20, max:40,
             ticks:{ color:'#c0392b', font:{size:11}, callback: v=>v.toFixed(0)+'°' },
             grid:{ color:'rgba(128,128,128,0.15)' },
             title:{ display:true, text:'°C', color:'#c0392b', font:{size:11} } },
        yH:{ type:'linear', position:'right', min:40, max:100,
             ticks:{ color:'#16a085', font:{size:11}, callback: v=>v.toFixed(0)+'%' },
             grid:{ drawOnChartArea:false },
             title:{ display:true, text:'%', color:'#16a085', font:{size:11} } }
      }
    }
  });
}

const hourChart = makeChart('hourChart');
const dayChart  = makeChart('dayChart');

function setChartData(chart, items) {
  if (!items.length) return;
  const temps = items.map(it=>it.avgT);
  const hums  = items.map(it=>it.avgH);
  chart.options.scales.yT.min = Math.floor(Math.min(...temps)-2);
  chart.options.scales.yT.max = Math.ceil(Math.max(...temps)+2);
  chart.options.scales.yH.min = Math.floor(Math.min(...hums)-5);
  chart.options.scales.yH.max = Math.ceil(Math.max(...hums)+5);
  chart.data.datasets[0].data = temps;
  chart.data.datasets[1].data = hums;
  chart.update();
}

function tickClock() {
  if (!serverEpochMs) return;
  const nowMs = serverEpochMs + (Date.now() - localNowAtFetch);
  const d = new Date(nowMs);
  const pad = n => String(n).padStart(2,'0');
  document.getElementById('clk').textContent = `${pad(d.getHours())}:${pad(d.getMinutes())}:${pad(d.getSeconds())}`;
  const ds = d.toLocaleDateString(undefined,{weekday:'short',year:'numeric',month:'short',day:'numeric'});
  document.getElementById('date').textContent = ds;
  document.getElementById('date2').textContent = ds;
}
setInterval(tickClock, 250);

function highlightGuide(tableId, val, minAttr, maxAttr, highlightId) {
  if (isNaN(val)) return;
  let activeText = '';
  document.querySelectorAll(`#${tableId} tbody tr`).forEach(tr => {
    const lo = parseFloat(tr.dataset[minAttr]);
    const hi = parseFloat(tr.dataset[maxAttr]);
    const active = val >= lo && val < hi;
    tr.classList.toggle('active-row', active);
    if (active) {
      const cells = tr.querySelectorAll('td');
      activeText = `<strong>${cells[0].textContent}</strong> — ${cells[1].textContent}<br>
        <span style="color:#888;font-size:12px;">${cells[2].textContent}</span>`;
    }
  });
  document.getElementById(highlightId).innerHTML = activeText || 'No match';
}

async function refreshLive() {
  try {
    const j = await fetch('/diag?x='+Date.now()).then(r=>r.json());
    if (j.ok) {
      liveT = j.temp; liveH = j.humidity;
      document.getElementById('t').textContent = fmt(j.temp);
      document.getElementById('h').textContent = fmt(j.humidity);
      document.getElementById('msg').textContent = `tries=${j.tries} · ${(j.ms/1000).toFixed(1)}s ago`;
      highlightGuide('tempGuide', liveT, 'tmin', 'tmax', 'tempHighlight');
      highlightGuide('humGuide',  liveH, 'hmin', 'hmax', 'humHighlight');
    } else {
      document.getElementById('msg').textContent = `read failed (tries=${j.tries})`;
    }
  } catch(e) { document.getElementById('msg').textContent='request failed'; }
}
setInterval(refreshLive, 2000);

async function refreshClock() {
  try {
    const j = await fetch('/time?x='+Date.now()).then(r=>r.json());
    if (j.ok) setClockFromServer(j.epoch_ms);
  } catch(e) {}
}
setInterval(refreshClock, 10000);

async function refreshStats() {
  try {
    const j = await fetch('/stats?x='+Date.now()).then(r=>r.json());
    if (!j.ok) return;

    // Comfort score
    const scoreEl = document.getElementById('comfortScore');
    const labelEl = document.getElementById('comfortLabel');
    if (j.score >= 0) {
      scoreEl.textContent = j.score;
      scoreEl.className = 'big ' + (COMFORT_CLASSES[j.labelIdx] || '');
      labelEl.textContent = COMFORT_LABELS[j.labelIdx] || '--';
    }

    // Rolling averages
    const set = (id, val) => { const el=document.getElementById(id); if(el) el.textContent=fmt(val); };
    set('r3T',  j.r3T);  set('r3H',  j.r3H);
    set('r12T', j.r12T); set('r12H', j.r12H);
    set('r24T', j.r24T); set('r24H', j.r24H);
    set('r7dT', j.r7dT); set('r7dH', j.r7dH);
  } catch(e) {}
}
setInterval(refreshStats, 30000);

async function refreshHistory() {
  try {
    const j = await fetch('/history?x='+Date.now()).then(r=>r.json());
    const tb = document.getElementById('hourTbody');
    tb.innerHTML = '';
    if (j.ok && j.items && j.items.length) {
      const first = j.items[0];
      document.getElementById('avgT').textContent = fmt(first.avgT);
      document.getElementById('avgH').textContent = fmt(first.avgH);
      const d = new Date(first.hour_start_ms);
      document.getElementById('avgMeta').textContent = 'hr @ ' + d.toLocaleTimeString([],{hour:'2-digit',minute:'2-digit'});

      const ordered = [...j.items].reverse();
      hourChart.data.labels = ordered.map(it => new Date(it.hour_start_ms).toLocaleTimeString([],{hour:'2-digit',minute:'2-digit'}));
      setChartData(hourChart, ordered);

      j.items.forEach(it => {
        const tr = document.createElement('tr');
        const dt = new Date(it.hour_start_ms);
        tr.innerHTML = `<td>${dt.toLocaleString()}</td><td class="t-color">${fmt(it.avgT,2)}</td><td class="h-color">${fmt(it.avgH,2)}</td>`;
        tb.appendChild(tr);
      });
    } else {
      document.getElementById('avgMeta').textContent = 'no completed hour yet';
    }
  } catch(e) {}
}
setInterval(refreshHistory, 10000);

async function refreshDaily() {
  try {
    const j = await fetch('/daily?x='+Date.now()).then(r=>r.json());
    const tb = document.getElementById('dayTbody');
    tb.innerHTML = '';
    if (j.ok && j.items && j.items.length) {
      const ordered = [...j.items].reverse();
      dayChart.data.labels = ordered.map(it => new Date(it.day_start_ms).toLocaleDateString([],{weekday:'short',month:'short',day:'numeric'}));
      setChartData(dayChart, ordered);

      j.items.forEach(it => {
        const tr = document.createElement('tr');
        const dt = new Date(it.day_start_ms);
        const score = it.score >= 0 ? it.score : '--';
        const lbl   = COMFORT_LABELS[it.labelIdx] || '--';
        const cls   = COMFORT_CLASSES[it.labelIdx] || '';
        tr.innerHTML = `
          <td>${dt.toLocaleDateString([],{weekday:'short',month:'short',day:'numeric'})}</td>
          <td class="t-color">${fmt(it.avgT,2)}</td>
          <td class="h-color">${fmt(it.avgH,2)}</td>
          <td class="${cls}">${score} — ${lbl}</td>`;
        tb.appendChild(tr);
      });
    }
  } catch(e) {}
}
setInterval(refreshDaily, 30000);

// initial load
refreshClock(); refreshLive(); refreshStats(); refreshHistory(); refreshDaily();
</script>
</body></html>
)HTML";

void handleRoot() { server.send(200, "text/html; charset=utf-8", HTML); }

void handleDiag() {
  String json = "{";
  json += "\"ok\":";        json += curOK ? "true" : "false";
  json += ",\"temp\":";     json += isnan(curT) ? "null" : String(curT, 2);
  json += ",\"humidity\":"; json += isnan(curH) ? "null" : String(curH, 2);
  json += ",\"tries\":";    json += String(curTries);
  uint32_t ageMs = millis() - curUpdatedMs;

  json += ",\"ms\":";
  json += String(ageMs);  json += "}";
  server.sendHeader("Cache-Control", "no-cache");
  server.send(200, "application/json", json);
}

void handleTime() {
  time_t t = nowEpoch();
  if (t == 0) { server.send(200, "application/json", "{\"ok\":false}"); return; }
  String json = "{\"ok\":true,\"epoch_ms\":";
  json += String((unsigned long long)t * 1000ULL);
  json += "}";
  server.sendHeader("Cache-Control", "no-cache");
  server.send(200, "application/json", json);
}

void handleStats() {
  int labelIdx = 0;
  int score = comfortScore(curT, curH, labelIdx);

  float r3T, r3H, r12T, r12H, r24T, r24H;
  bool has3  = rollingAvg(3,  r3T,  r3H);
  bool has12 = rollingAvg(12, r12T, r12H);
  bool has24 = rollingAvg(24, r24T, r24H);

  // 7d from daily buffer
  float r7dT = NAN, r7dH = NAN;
  bool has7d = false;
  if (histDayHead >= 0) {
    double st = 0, sh = 0; int cnt = 0;
    for (int i = 0; i < HIST_D; i++) {
      int idx = (histDayHead - i + HIST_D) % HIST_D;
      if (histDay[idx] == 0) break;
      st += histDayAvgT[idx]; sh += histDayAvgH[idx]; cnt++;
    }
    if (cnt > 0) { r7dT = (float)(st/cnt); r7dH = (float)(sh/cnt); has7d = true; }
  }

  auto fv = [](float v) -> String { return isnan(v) ? "null" : String(v, 2); };

  String json = "{\"ok\":true";
  json += ",\"score\":";    json += String(score);
  json += ",\"labelIdx\":"; json += String(labelIdx);
  json += ",\"r3T\":";      json += has3  ? fv(r3T)  : "null";
  json += ",\"r3H\":";      json += has3  ? fv(r3H)  : "null";
  json += ",\"r12T\":";     json += has12 ? fv(r12T) : "null";
  json += ",\"r12H\":";     json += has12 ? fv(r12H) : "null";
  json += ",\"r24T\":";     json += has24 ? fv(r24T) : "null";
  json += ",\"r24H\":";     json += has24 ? fv(r24H) : "null";
  json += ",\"r7dT\":";     json += has7d ? fv(r7dT) : "null";
  json += ",\"r7dH\":";     json += has7d ? fv(r7dH) : "null";
  json += "}";
  server.sendHeader("Cache-Control", "no-cache");
  server.send(200, "application/json", json);
}

void handleHistory() {
  String out = "{\"ok\":true,\"items\":[";
  bool first = true;
  if (histHead >= 0) {
    for (int i = 0; i < HIST_H; i++) {
      int idx = (histHead - i + HIST_H) % HIST_H;
      if (histHour[idx] == 0) break;
      if (!first) out += ",";
      first = false;
      out += "{\"hour_start_ms\":";
      out += String((unsigned long long)histHour[idx] * 1000ULL);
      out += ",\"avgT\":"; out += String(histAvgT[idx], 3);
      out += ",\"avgH\":"; out += String(histAvgH[idx], 3);
      out += "}";
    }
  }
  out += "]}";
  server.sendHeader("Cache-Control", "no-cache");
  server.send(200, "application/json", out);
}

void handleDaily() {
  String out = "{\"ok\":true,\"items\":[";
  bool first = true;
  if (histDayHead >= 0) {
    for (int i = 0; i < HIST_D; i++) {
      int idx = (histDayHead - i + HIST_D) % HIST_D;
      if (histDay[idx] == 0) break;
      if (!first) out += ",";
      first = false;
      int li = 0;
      int sc = comfortScore(histDayAvgT[idx], histDayAvgH[idx], li);
      out += "{\"day_start_ms\":";
      out += String((unsigned long long)histDay[idx] * 1000ULL);
      out += ",\"avgT\":";    out += String(histDayAvgT[idx], 3);
      out += ",\"avgH\":";    out += String(histDayAvgH[idx], 3);
      out += ",\"score\":";   out += String(sc);
      out += ",\"labelIdx\":";out += String(li);
      out += "}";
    }
  }
  out += "]}";
  server.sendHeader("Cache-Control", "no-cache");
  server.send(200, "application/json", out);
}

void handleFavicon() { server.send(204); }
void handleNotFound() { server.send(404, "text/plain", "Not found"); }

void setupTime() {
  setenv("TZ", "SGT-8", 1); tzset();
  configTime(8*3600, 0, "pool.ntp.org", "time.google.com", "time.cloudflare.com");
  Serial.print("Syncing time");
  for (int i = 0; i < 40; ++i) {
    if (nowEpoch() != 0) { Serial.println(" OK"); return; }
    Serial.print("."); delay(250);
  }
  Serial.println(" (timed out)");
}

void readDHTOnce() {
  curOK = false; curTries = 0;
  for (int i = 0; i < 4 && !curOK; i++) {
    curTries = i + 1;
    float h = dht.readHumidity(), t = dht.readTemperature();
    if (!isnan(h) && !isnan(t)) { curH = h; curT = t; curOK = true; }
    else delay(120);
  }
  curUpdatedMs = millis();

  time_t te = nowEpoch();
  if (te != 0) {
    time_t hour = hourStart(te);
    time_t day  = dayStart(te);

    // init on first reading
    if (currentHourStart == 0) currentHourStart = hour;
    if (currentDayStart  == 0) currentDayStart  = day;

    // hour rollover
    if (hour != currentHourStart) {
      finalizeHour(currentHourStart);
      currentHourStart = hour;
    }
    // day rollover
    if (day != currentDayStart) {
      finalizeDay(currentDayStart);
      currentDayStart = day;
    }

    if (curOK) { sumT += curT; sumH += curH; sampleCount++; }
  }
}

void setup() {
  Serial.begin(115200); delay(200);
  Serial.println("\nBooting…");
  pinMode(DHTPIN, INPUT_PULLUP); dht.begin();

  loadHourly();
  loadDaily();

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  Serial.printf("Connecting to %s", WIFI_SSID);
  for (int i = 0; i < 60 && WiFi.status() != WL_CONNECTED; i++) { delay(500); Serial.print("."); }
  if (WiFi.status() != WL_CONNECTED) { Serial.println("\nWiFi failed; rebooting"); delay(1500); ESP.restart(); }
  Serial.printf("\nConnected! IP: %s\n", WiFi.localIP().toString().c_str());

  if (MDNS.begin(HOSTNAME)) {
    Serial.printf("mDNS: http://%s.local\n", HOSTNAME);
    MDNS.addService("http", "tcp", 80);
  }

  setupTime();

  server.on("/",        HTTP_GET, handleRoot);
  server.on("/diag",    HTTP_GET, handleDiag);
  server.on("/time",    HTTP_GET, handleTime);
  server.on("/stats",   HTTP_GET, handleStats);
  server.on("/history", HTTP_GET, handleHistory);
  server.on("/daily",   HTTP_GET, handleDaily);
  server.on("/favicon.ico", HTTP_GET, handleFavicon);
  server.onNotFound(handleNotFound);
  server.begin();
  Serial.println("HTTP server started");
  lastReadMs = 0;
}

void loop() {
  server.handleClient();
  unsigned long nowMs = millis();
  if (nowMs - lastReadMs >= READ_INTERVAL_MS) {
    static bool warmed = false;
    if (!warmed) { delay(1500); warmed = true; }
    readDHTOnce();
    lastReadMs = nowMs;
  }
  delay(1);
}