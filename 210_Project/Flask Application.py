from flask import Flask, jsonify, render_template_string, request
import sqlite3, json
from datetime import datetime
import paho.mqtt.client as mqtt

app = Flask(__name__)
DB = "swings.db"
TOPIC = "golf/swing"

CLUB_OFFSETS = {
    "Driver": 0.21, "3 Wood": 0.16, "4 Iron": 0.038, "5 Iron": 0.025,
    "6 Iron": 0.013, "7 Iron": 0.0, "8 Iron": -0.013,
    "9 Iron": -0.025, "Pitching Wedge": -0.038
}

HTML = """
<!DOCTYPE html>
<html>
<head>
<title>Golf Swing Dashboard</title>
<meta name="viewport" content="width=device-width, initial-scale=1">
<style>
body{font-family:Arial;background:#f6f8fa;color:#111;margin:0;padding:20px}
h1{margin-top:0}.card{background:white;border:1px solid #ddd;border-radius:12px;padding:16px;margin-bottom:16px}
.grid{display:grid;grid-template-columns:1fr 1fr 1fr;gap:12px}.box{background:#fff;border:1px solid #ddd;border-radius:10px;padding:14px}
.label{font-size:13px;color:#666}.value{font-size:22px;font-weight:bold;margin-top:6px}
input,select{padding:8px;margin:5px 8px 5px 0}table{width:100%;border-collapse:collapse;background:white}
th,td{border:1px solid #ccc;padding:8px;text-align:center}th{background:#e8e8e8}
@media(max-width:750px){.grid{grid-template-columns:1fr}}
</style>
</head>
<body>
<h1>Golf Swing Dashboard</h1>

<div class="card">
  <h2>User Settings</h2>
  <label>Height in metres:</label>
  <input id="height" type="number" step="0.01" value="1.80">

  <label>Club:</label>
  <select id="club">
    <option>Driver</option><option>3 Wood</option><option>4 Iron</option>
    <option>5 Iron</option><option>6 Iron</option><option>7 Iron</option>
    <option>8 Iron</option><option>9 Iron</option><option>Pitching Wedge</option>
  </select>
</div>

<div class="card">
  <h2>Latest Swing</h2>
  <div class="grid">
    <div class="box"><div class="label">Swing Type</div><div class="value" id="swingType">N/A</div></div>
    <div class="box"><div class="label">Club</div><div class="value" id="latestClub">N/A</div></div>
    <div class="box"><div class="label">Calculated Speed</div><div class="value" id="calcSpeed">N/A</div></div>
    <div class="box"><div class="label">Tempo</div><div class="value" id="tempo">N/A</div></div>
    <div class="box"><div class="label">Face Angle</div><div class="value" id="faceAngle">N/A</div></div>
    <div class="box"><div class="label">Backswing</div><div class="value" id="backswing">N/A</div></div>
    <div class="box"><div class="label">Strike Factor</div><div class="value" id="strikeFactor">N/A</div></div>
  </div>
</div>

<div class="card">
<h2>Past Swings</h2>
<table>
<thead>
<tr>
<th>Time</th><th>Type</th><th>Club</th><th>Calculated Speed</th>
<th>Tempo</th><th>Face</th><th>Backswing</th><th>Total Time</th><th>Strike Factor</th>
</tr>
</thead>
<tbody id="rows"></tbody>
</table>
</div>

<script>
async function saveSettings(){
  const height = parseFloat(document.getElementById("height").value);
  const club = document.getElementById("club").value;

  await fetch("/api/settings", {
    method:"POST",
    headers:{"Content-Type":"application/json"},
    body:JSON.stringify({height:height, club:club})
  });
}

async function loadSettings(){
  const s = await (await fetch("/api/settings")).json();
  document.getElementById("height").value = s.height;
  document.getElementById("club").value = s.club;
}

async function loadSwings(){
  const swings = await (await fetch("/api/swings")).json();
  const rows = document.getElementById("rows");
  rows.innerHTML = "";

  if(swings.length === 0) return;

  const s = swings[0];
  document.getElementById("swingType").innerText = s.swingType ?? "N/A";
  document.getElementById("latestClub").innerText = s.club ?? "N/A";
  document.getElementById("calcSpeed").innerText = s.calculatedSpeed != null ? s.calculatedSpeed + " km/h" : "N/A";
  document.getElementById("tempo").innerText = s.tempo ?? "N/A";
  document.getElementById("faceAngle").innerText = s.faceAngle ?? "N/A";
  document.getElementById("backswing").innerText = s.backswingTime != null ? s.backswingTime + " ms" : "N/A";
  document.getElementById("strikeFactor").innerText = s.strikeFactor ?? "N/A";

  swings.forEach(x => {
    rows.innerHTML += `
      <tr>
        <td>${x.timestamp}</td>
        <td>${x.swingType ?? "N/A"}</td>
        <td>${x.club ?? "N/A"}</td>
        <td>${x.calculatedSpeed != null ? x.calculatedSpeed + " km/h" : "N/A"}</td>
        <td>${x.tempo ?? "N/A"}</td>
        <td>${x.faceAngle ?? "N/A"}</td>
        <td>${x.backswingTime ?? "N/A"} ms</td>
        <td>${x.totalSwingTime ?? "N/A"} ms</td>
        <td>${x.strikeFactor ?? "N/A"}</td>
      </tr>`;
  });
}

document.getElementById("height").addEventListener("input", saveSettings);
document.getElementById("club").addEventListener("change", saveSettings);

loadSettings().then(loadSwings);
setInterval(loadSwings, 2000);
</script>
</body>
</html>
"""

def db():
    return sqlite3.connect(DB)

def calculate_speed(x, height, club):
    if x is None:
        return None

    club_offset = CLUB_OFFSETS.get(club, 0.0)

    calculated = float(x) * (float(height) * 0.72 + club_offset)

    return round(calculated, 1)

def init_db():
    with db() as con:
        con.execute("CREATE TABLE IF NOT EXISTS swings(id INTEGER PRIMARY KEY AUTOINCREMENT, timestamp TEXT, data TEXT)")
        con.execute("CREATE TABLE IF NOT EXISTS settings(id INTEGER PRIMARY KEY CHECK(id=1), height REAL, club TEXT)")
        con.execute("INSERT OR IGNORE INTO settings(id,height,club) VALUES(1,1.80,'7 Iron')")

def get_settings():
    with db() as con:
        row = con.execute("SELECT height,club FROM settings WHERE id=1").fetchone()
    return {"height": row[0], "club": row[1]}

def update_settings(height, club):
    with db() as con:
        con.execute("UPDATE settings SET height=?, club=? WHERE id=1", (height, club))

def save_swing(data):
    settings = get_settings()

    data["club"] = settings["club"]
    data["calculatedSpeed"] = calculate_speed(data.get("speed"), settings["height"], settings["club"])

    with db() as con:
        con.execute(
            "INSERT INTO swings(timestamp,data) VALUES(?,?)",
            (datetime.now().strftime("%Y-%m-%d %H:%M:%S"), json.dumps(data))
        )

def get_swings():
    with db() as con:
        rows = con.execute("SELECT timestamp,data FROM swings ORDER BY id DESC LIMIT 30").fetchall()

    output = []
    for timestamp, raw in rows:
        item = json.loads(raw)
        item["timestamp"] = timestamp
        output.append(item)

    return output

def on_message(client, userdata, msg):
    try:
        data = json.loads(msg.payload.decode())
        save_swing(data)
        print("Saved swing:", data)
    except Exception as e:
        print("MQTT error:", e)

@app.route("/")
def index():
    return render_template_string(HTML)

@app.route("/api/swings")
def api_swings():
    return jsonify(get_swings())

@app.route("/api/settings", methods=["GET", "POST"])
def api_settings():
    if request.method == "POST":
        data = request.get_json()
        update_settings(float(data["height"]), data["club"])
        return jsonify({"ok": True})

    return jsonify(get_settings())

if __name__ == "__main__":
    init_db()

    mqtt_client = mqtt.Client()
    mqtt_client.on_message = on_message
    mqtt_client.connect("localhost", 1883, 60)
    mqtt_client.subscribe(TOPIC)
    mqtt_client.loop_start()

    app.run(host="0.0.0.0", port=5000)
