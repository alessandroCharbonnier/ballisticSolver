#pragma once
/// @file web_ui.h
/// @brief Embedded HTML/CSS/JS for the Ballistic Sentinel web interface.
///
/// Single-page app: dark theme (#0d1117) + green accent (#16c79a).
/// Two collapsible sections: Settings / Stage Editor.
/// Monospace font throughout.  All resources self-contained (no CDN).

#include <pgmspace.h>

static const char INDEX_HTML[] PROGMEM = R"rawhtml(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>Ballistic Sentinel</title>
<style>
*{box-sizing:border-box;margin:0;padding:0}
body{background:#0d1117;color:#c9d1d9;font-family:'Courier New',monospace;font-size:14px;padding:12px;max-width:640px;margin:0 auto}
h1{color:#16c79a;font-size:18px;text-align:center;margin:8px 0 16px;letter-spacing:2px}
h2{color:#16c79a;font-size:14px;margin:0;padding:8px 12px;cursor:pointer;user-select:none;border:1px solid #30363d;border-radius:6px 6px 0 0;background:#161b22}
h2:hover{background:#1c2128}
h2::before{content:'▸ ';transition:transform .2s}
h2.open::before{content:'▾ '}
.section{border:1px solid #30363d;border-top:none;border-radius:0 0 6px 6px;padding:12px;display:none;background:#161b22}
.section.show{display:block}
label{display:block;color:#8b949e;font-size:12px;margin:10px 0 2px}
input,select{width:100%;padding:6px 8px;background:#0d1117;color:#c9d1d9;border:1px solid #30363d;border-radius:4px;font-family:inherit;font-size:13px}
input:focus,select:focus{outline:none;border-color:#16c79a}
.row{display:flex;gap:8px}
.row>*{flex:1}
table{width:100%;border-collapse:collapse;margin-top:8px}
th{background:#1c2128;color:#16c79a;font-size:11px;padding:6px;text-align:left;border-bottom:1px solid #30363d}
td{padding:4px 6px;border-bottom:1px solid #21262d}
td input{margin:0}
.del-btn{background:none;border:none;color:#f85149;cursor:pointer;font-size:16px;padding:2px 6px}
.del-btn:hover{color:#ff7b72}
.btn-bar{display:flex;gap:8px;margin-top:16px}
.btn{flex:1;padding:10px;border:1px solid #30363d;border-radius:6px;background:#161b22;color:#c9d1d9;font-family:inherit;font-size:13px;cursor:pointer;text-align:center}
.btn:hover{background:#1c2128}
.btn-primary{background:#16c79a;color:#0d1117;border-color:#16c79a;font-weight:bold}
.btn-primary:hover{background:#1de9b6}
.btn-danger{color:#f85149;border-color:#f85149}
.btn-danger:hover{background:#f8514922}
.toast{position:fixed;bottom:20px;left:50%;transform:translateX(-50%);background:#16c79a;color:#0d1117;padding:8px 20px;border-radius:6px;font-weight:bold;display:none;z-index:99}
.cb-row{display:flex;align-items:center;gap:6px;margin:8px 0}
.cb-row input[type=checkbox]{width:auto}
.mv-table{margin-top:8px}
.mv-table td{padding:2px 4px}
.mv-table input{width:80px}
</style>
</head>
<body>
<h1>BALLISTIC SENTINEL</h1>

<!-- ═══════════════ SETTINGS SECTION ═══════════════ -->
<h2 id="hdr-settings" class="open" onclick="toggle('settings')">SETTINGS</h2>
<div id="settings" class="section show">

<label>Unit System</label>
<select id="unit_sys" onchange="switchUnits()">
<option value="imp">Imperial (yd, fps, gr, in, °F)</option>
<option value="met">Metric (m, m/s, g, mm, °C)</option>
</select>

<div class="row">
<div><label>BC</label><input id="bc" type="number" step="0.001" value="0.315"></div>
<div><label>Drag Table</label>
<select id="drag_table">
<option value="G1">G1</option><option value="G7" selected>G7</option>
<option value="G2">G2</option><option value="G5">G5</option>
<option value="G6">G6</option><option value="G8">G8</option>
<option value="GI">GI</option><option value="GS">GS</option>
<option value="RA4">RA4 (.22LR)</option>
</select></div>
</div>

<div class="cb-row">
<input type="checkbox" id="multi_bc" onchange="toggleMultiBC()">
<label for="multi_bc" style="display:inline;margin:0">Multi-BC</label>
</div>
<div id="multi_bc_section" style="display:none">
<table class="mv-table">
<tr><th>BC</th><th id="th_mbv">Velocity (fps)</th></tr>
<tr><td><input id="mbc1" type="number" step="0.001"></td><td><input id="mbv1" type="number"></td></tr>
<tr><td><input id="mbc2" type="number" step="0.001"></td><td><input id="mbv2" type="number"></td></tr>
<tr><td><input id="mbc3" type="number" step="0.001"></td><td><input id="mbv3" type="number"></td></tr>
</table>
</div>

<div class="row">
<div><label id="lbl_weight">Bullet Weight (gr)</label><input id="weight" type="number" value="140"></div>
<div><label id="lbl_caliber">Caliber (in)</label><input id="caliber" type="number" step="0.001" value="0.264"></div>
</div>

<div class="row">
<div><label id="lbl_length">Bullet Length (in)</label><input id="length" type="number" step="0.001" value="1.350"></div>
<div><label id="lbl_mv">Muzzle Vel (fps)</label><input id="mv" type="number" value="2710"></div>
</div>

<div class="cb-row">
<input type="checkbox" id="use_ps" onchange="togglePS()">
<label for="use_ps" style="display:inline;margin:0">Powder Sensitivity</label>
</div>
<div id="ps_section" style="display:none">
<p style="color:#8b949e;font-size:11px;margin:4px 0">Enter two chrono readings at different temps to auto-calculate</p>
<div class="row">
<div><label id="lbl_ps_v1">Vel 1 (fps)</label><input id="ps_v1" type="number"></div>
<div><label id="lbl_ps_t1">Temp 1 (°F)</label><input id="ps_t1" type="number"></div>
</div>
<div class="row">
<div><label id="lbl_ps_v2">Vel 2 (fps)</label><input id="ps_v2" type="number"></div>
<div><label id="lbl_ps_t2">Temp 2 (°F)</label><input id="ps_t2" type="number"></div>
</div>
</div>

<div class="row">
<div><label id="lbl_sh">Sight Height (in)</label><input id="sh" type="number" step="0.1" value="1.5"></div>
<div><label id="lbl_twist">Twist (in, +RH)</label><input id="twist" type="number" step="0.1" value="8"></div>
</div>

<div class="row">
<div><label id="lbl_zero">Zero Range (yd)</label><input id="zero" type="number" value="100"></div>
<div><label>Latitude (°)</label><input id="lat" type="number" step="0.1" value="0"></div>
</div>

<div class="row">
<div><label>Correction Unit</label>
<select id="corr_unit">
<option value="MOA" selected>MOA</option>
<option value="SMOA">SMOA</option>
<option value="MRAD">MRAD</option>
<option value="cm">cm</option>
<option value="CLICKS">Turret Clicks</option>
</select></div>
<div><label>Click Size (MOA)</label><input id="click_size" type="number" step="0.01" value="0.25"></div>
</div>
</div>

<!-- ═══════════════ STAGE EDITOR SECTION ═══════════════ -->
<h2 id="hdr-stages" onclick="toggle('stages')">STAGE EDITOR</h2>
<div id="stages" class="section">
<table id="stage-table">
<tr><th>#</th><th>Name</th><th id="th_stg_dist">Distance (yd)</th><th></th></tr>
</table>
</div>

<!-- ═══════════════ ACTION BUTTONS ═══════════════ -->
<div class="btn-bar">
<button class="btn btn-primary" onclick="saveAll()">SAVE</button>
<button class="btn btn-danger" onclick="disableWifi()">DISABLE WIFI</button>
</div>

<div class="toast" id="toast"></div>

<script>
// Unit conversion helpers
const CV={
 gr_g:v=>+(v*0.06479891).toFixed(2),g_gr:v=>+(v/0.06479891).toFixed(1),
 in_mm:v=>+(v*25.4).toFixed(2),mm_in:v=>+(v/25.4).toFixed(3),
 fps_mps:v=>+(v*0.3048).toFixed(1),mps_fps:v=>Math.round(v/0.3048),
 yd_m:v=>+(v*0.9144).toFixed(1),m_yd:v=>Math.round(v/0.9144),
 f_c:v=>+((v-32)*5/9).toFixed(1),c_f:v=>+(v*9/5+32).toFixed(1)
};
const UF=[
 ['weight','lbl_weight','Bullet Weight (gr)','Bullet Weight (g)','gr_g','g_gr'],
 ['caliber','lbl_caliber','Caliber (in)','Caliber (mm)','in_mm','mm_in'],
 ['length','lbl_length','Bullet Length (in)','Bullet Length (mm)','in_mm','mm_in'],
 ['mv','lbl_mv','Muzzle Vel (fps)','Muzzle Vel (m/s)','fps_mps','mps_fps'],
 ['sh','lbl_sh','Sight Height (in)','Sight Height (mm)','in_mm','mm_in'],
 ['twist','lbl_twist','Twist (in, +RH)','Twist (mm, +RH)','in_mm','mm_in'],
 ['zero','lbl_zero','Zero Range (yd)','Zero Range (m)','yd_m','m_yd']
];
let isMet=false;

function $(id){return document.getElementById(id)}
function toggle(id){
 let s=$('hdr-'+id),d=$(id);
 s.classList.toggle('open');d.classList.toggle('show');
}
function switchUnits(){
 let m=$('unit_sys').value==='met';
 if(m===isMet)return;
 UF.forEach(f=>{
  let el=$(f[0]),v=+el.value;
  if(!isNaN(v)&&v!==0)el.value=m?CV[f[4]](v):CV[f[5]](v);
  $(f[1]).textContent=m?f[3]:f[2];
 });
 for(let i=1;i<=3;i++){let e=$('mbv'+i);if(e.value)e.value=m?CV.fps_mps(+e.value):CV.mps_fps(+e.value)}
 $('th_mbv').textContent=m?'Velocity (m/s)':'Velocity (fps)';
 ['ps_v1','ps_v2'].forEach(id=>{let e=$(id);if(e.value)e.value=m?CV.fps_mps(+e.value):CV.mps_fps(+e.value)});
 ['ps_t1','ps_t2'].forEach(id=>{let e=$(id);if(e.value)e.value=m?CV.f_c(+e.value):CV.c_f(+e.value)});
 $('lbl_ps_v1').textContent=m?'Vel 1 (m/s)':'Vel 1 (fps)';
 $('lbl_ps_v2').textContent=m?'Vel 2 (m/s)':'Vel 2 (fps)';
 $('lbl_ps_t1').textContent=m?'Temp 1 (°C)':'Temp 1 (°F)';
 $('lbl_ps_t2').textContent=m?'Temp 2 (°C)':'Temp 2 (°F)';
 stages.forEach(s=>{s.distance=m?+CV.yd_m(s.distance):CV.m_yd(s.distance)});
 $('th_stg_dist').textContent=m?'Distance (m)':'Distance (yd)';
 renderStages();
 isMet=m;
}
function toggleMultiBC(){$('multi_bc_section').style.display=$('multi_bc').checked?'block':'none'}
function togglePS(){$('ps_section').style.display=$('use_ps').checked?'block':'none'}

// ═══ Stage Table ═══
let stages=[];
function renderStages(){
 let t=$('stage-table');
 while(t.rows.length>1)t.deleteRow(1);
 stages.forEach((s,i)=>{
  let r=t.insertRow();
  r.innerHTML=`<td>${i+1}</td>
   <td><input value="${s.name||''}" onchange="stages[${i}].name=this.value;checkNewRow()"></td>
   <td><input type="number" value="${s.distance||''}" onchange="stages[${i}].distance=+this.value;checkNewRow()"></td>
   <td><button class="del-btn" onclick="delStage(${i})">✕</button></td>`;
 });
 // Always show an empty row for new entry
 let r=t.insertRow();
 let ni=stages.length;
 r.innerHTML=`<td>${ni+1}</td>
  <td><input placeholder="(optional)" onchange="addStage(this.value,0,'name')"></td>
  <td><input type="number" placeholder="distance" onchange="addStage('',+this.value,'dist')"></td>
  <td></td>`;
}
let pendingName='',pendingDist=0;
function addStage(val,dist,field){
 if(field==='name')pendingName=val;
 if(field==='dist')pendingDist=dist;
 if(pendingDist>0){
  stages.push({name:pendingName,distance:pendingDist});
  pendingName='';pendingDist=0;renderStages();
 }
}
function delStage(i){stages.splice(i,1);renderStages()}
function checkNewRow(){}

// ═══ API ═══
function toast(msg,ms){let t=$('toast');t.textContent=msg;t.style.display='block';setTimeout(()=>t.style.display='none',ms||2000)}

function imp(k,v){return isMet?CV[k](v):v}
function gatherConfig(){
 let c={
  bc:+$('bc').value, drag_table:$('drag_table').value,
  weight:imp('g_gr',+$('weight').value), caliber:imp('mm_in',+$('caliber').value),
  length:imp('mm_in',+$('length').value), mv:imp('mps_fps',+$('mv').value),
  sh:imp('mm_in',+$('sh').value), twist:imp('mm_in',+$('twist').value),
  zero:imp('m_yd',+$('zero').value), lat:+$('lat').value,
  corr_unit:$('corr_unit').value, click_size:+$('click_size').value,
  multi_bc:$('multi_bc').checked,
  use_ps:$('use_ps').checked,
  unit_system:isMet?1:0
 };
 if(c.multi_bc){
  c.bc_points=[];
  for(let i=1;i<=3;i++){
   let b=+$('mbc'+i).value,v=imp('mps_fps',+$('mbv'+i).value);
   if(b>0&&v>0)c.bc_points.push({bc:b,vel:v});
  }
 }
 if(c.use_ps){
  c.ps={v1:imp('mps_fps',+$('ps_v1').value),t1:imp('c_f',+$('ps_t1').value),
        v2:imp('mps_fps',+$('ps_v2').value),t2:imp('c_f',+$('ps_t2').value)};
 }
 return c;
}

async function saveAll(){
 try{
  let stg=stages.map(s=>({name:s.name,distance:isMet?CV.m_yd(s.distance):s.distance}));
  let body=JSON.stringify({config:gatherConfig(),stages:stg});
  let r=await fetch('/api/save',{method:'POST',headers:{'Content-Type':'application/json'},body});
  if(r.ok)toast('Saved!');else toast('Save failed: '+r.status,3000);
 }catch(e){toast('Error: '+e.message,3000)}
}

async function disableWifi(){
 try{await fetch('/api/wifi/off',{method:'POST'});toast('WiFi shutting down...');}
 catch(e){toast('Sent shutdown request')}
}

async function loadConfig(){
 try{
  let r=await fetch('/api/config');
  if(!r.ok)return;
  let d=await r.json();
  if(d.config){
   let c=d.config;
   let m=!!(c.unit_system);
   $('unit_sys').value=m?'met':'imp';isMet=m;
   let dsp=(k,v)=>m?CV[k](v):v;
   $('bc').value=c.bc||.315;$('drag_table').value=c.drag_table||'G7';
   $('weight').value=dsp('gr_g',c.weight||140);$('caliber').value=dsp('in_mm',c.caliber||.264);
   $('length').value=dsp('in_mm',c.length||1.35);$('mv').value=dsp('fps_mps',c.mv||2710);
   $('sh').value=dsp('in_mm',c.sh||1.5);$('twist').value=dsp('in_mm',c.twist||8);
   $('zero').value=dsp('yd_m',c.zero||100);$('lat').value=c.lat||0;
   $('corr_unit').value=c.corr_unit||'MOA';$('click_size').value=c.click_size||.25;
   $('multi_bc').checked=!!c.multi_bc;toggleMultiBC();
   if(c.bc_points){
    c.bc_points.forEach((p,i)=>{
     if(i<3){$('mbc'+(i+1)).value=p.bc;$('mbv'+(i+1)).value=dsp('fps_mps',p.vel)}
    });
   }
   $('use_ps').checked=!!c.use_ps;togglePS();
   if(c.ps){
    $('ps_v1').value=dsp('fps_mps',c.ps.v1);$('ps_t1').value=dsp('f_c',c.ps.t1);
    $('ps_v2').value=dsp('fps_mps',c.ps.v2);$('ps_t2').value=dsp('f_c',c.ps.t2);
   }
   if(m){
    UF.forEach(f=>$(f[1]).textContent=f[3]);
    $('th_mbv').textContent='Velocity (m/s)';
    $('lbl_ps_v1').textContent='Vel 1 (m/s)';$('lbl_ps_v2').textContent='Vel 2 (m/s)';
    $('lbl_ps_t1').textContent='Temp 1 (°C)';$('lbl_ps_t2').textContent='Temp 2 (°C)';
    $('th_stg_dist').textContent='Distance (m)';
   }
  }
  if(d.stages){
   stages=d.stages.map(s=>({name:s.name,distance:isMet?+CV.yd_m(s.distance):s.distance}));
   renderStages();
  }
 }catch(e){console.error(e)}
}

renderStages();
loadConfig();
</script>
</body>
</html>
)rawhtml";
