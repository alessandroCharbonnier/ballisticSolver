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
.unit-row{display:flex;flex-wrap:wrap;gap:6px 12px;margin:6px 0 10px}
.unit-row label{display:inline;margin:0;font-size:11px;color:#8b949e}
.unit-row select{width:auto;min-width:70px;font-size:12px;padding:3px 6px}
</style>
</head>
<body>
<h1>BALLISTIC SENTINEL</h1>

<!-- ═══════════════ SETTINGS SECTION ═══════════════ -->
<h2 id="hdr-settings" class="open" onclick="toggle('settings')">SETTINGS</h2>
<div id="settings" class="section show">

<label>Display Units</label>
<div class="unit-row">
<div><label>Distance</label><select id="u_dist" onchange="unitChanged('distance')"><option value="0">yd</option><option value="1">m</option></select></div>
<div><label>Velocity</label><select id="u_vel" onchange="unitChanged('velocity')"><option value="0">fps</option><option value="1">m/s</option></select></div>
<div><label>Weight</label><select id="u_wt" onchange="unitChanged('weight')"><option value="0">gr</option><option value="1">g</option></select></div>
<div><label>Length</label><select id="u_len" onchange="unitChanged('length')"><option value="0">in</option><option value="1">mm</option></select></div>
<div><label>Temp</label><select id="u_temp" onchange="unitChanged('temperature')"><option value="0">&deg;F</option><option value="1">&deg;C</option></select></div>
<div><label>Pressure</label><select id="u_press" onchange="unitChanged('pressure')"><option value="0">inHg</option><option value="1">hPa</option></select></div>
</div>

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

<div class="row">
<div><label>Cant Offset (&deg;)</label><input id="cant_offset" type="number" step="0.1" value="0" readonly style="opacity:0.6"></div>
<div><label>Cant Sensitivity (&deg;)</label><input id="cant_sens" type="number" step="0.1" value="1"></div>
</div>
<div style="margin:6px 0 4px">
<button class="btn" onclick="calibrateCant()" id="btn_cant_cal">CALIBRATE CANT</button>
<span id="cant_cal_status" style="color:#8b949e;font-size:11px;margin-left:8px"></span>
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
// ═══ Unit conversion helpers ═══
const CV={
 gr_g:v=>+(v*0.06479891).toFixed(2),g_gr:v=>+(v/0.06479891).toFixed(1),
 in_mm:v=>+(v*25.4).toFixed(2),mm_in:v=>+(v/25.4).toFixed(3),
 fps_mps:v=>+(v*0.3048).toFixed(1),mps_fps:v=>Math.round(v/0.3048),
 yd_m:v=>+(v*0.9144).toFixed(1),m_yd:v=>Math.round(v/0.9144),
 f_c:v=>+((v-32)*5/9).toFixed(1),c_f:v=>+(v*9/5+32).toFixed(1),
 inhg_hpa:v=>+(v*33.8639).toFixed(1),hpa_inhg:v=>+(v/33.8639).toFixed(2)
};
// Unit state: tracks current displayed unit per type
// 0=imperial shown, 1=metric shown
let U={distance:0,velocity:0,weight:0,length:0,temperature:0,pressure:0};
// Map unit types to affected fields: [inputId, to_metric_fn, to_imperial_fn, imp_label, met_label]
const UM={
 distance:[
  {id:'zero',lbl:'lbl_zero',il:'Zero Range (yd)',ml:'Zero Range (m)'},
 ],
 velocity:[
  {id:'mv',lbl:'lbl_mv',il:'Muzzle Vel (fps)',ml:'Muzzle Vel (m/s)'},
  {id:'mbv1'},{id:'mbv2'},{id:'mbv3'},
  {id:'ps_v1',lbl:'lbl_ps_v1',il:'Vel 1 (fps)',ml:'Vel 1 (m/s)'},
  {id:'ps_v2',lbl:'lbl_ps_v2',il:'Vel 2 (fps)',ml:'Vel 2 (m/s)'},
 ],
 weight:[
  {id:'weight',lbl:'lbl_weight',il:'Bullet Weight (gr)',ml:'Bullet Weight (g)'},
 ],
 length:[
  {id:'caliber',lbl:'lbl_caliber',il:'Caliber (in)',ml:'Caliber (mm)'},
  {id:'length',lbl:'lbl_length',il:'Bullet Length (in)',ml:'Bullet Length (mm)'},
  {id:'sh',lbl:'lbl_sh',il:'Sight Height (in)',ml:'Sight Height (mm)'},
  {id:'twist',lbl:'lbl_twist',il:'Twist (in, +RH)',ml:'Twist (mm, +RH)'},
 ],
 temperature:[
  {id:'ps_t1',lbl:'lbl_ps_t1',il:'Temp 1 (\u00b0F)',ml:'Temp 1 (\u00b0C)'},
  {id:'ps_t2',lbl:'lbl_ps_t2',il:'Temp 2 (\u00b0F)',ml:'Temp 2 (\u00b0C)'},
 ],
 pressure:[]
};
const CF={
 distance:['yd_m','m_yd'],velocity:['fps_mps','mps_fps'],
 weight:['gr_g','g_gr'],length:['in_mm','mm_in'],
 temperature:['f_c','c_f'],pressure:['inhg_hpa','hpa_inhg']
};

function $(id){return document.getElementById(id)}
function toggle(id){
 let s=$('hdr-'+id),d=$(id);
 s.classList.toggle('open');d.classList.toggle('show');
}

function unitChanged(type){
 let selMap={distance:'u_dist',velocity:'u_vel',weight:'u_wt',length:'u_len',temperature:'u_temp',pressure:'u_press'};
 let nv=+$(selMap[type]).value;
 let ov=U[type];
 if(nv===ov)return;
 let fns=CF[type];
 let toM=CV[fns[0]],toI=CV[fns[1]];
 let cvt=nv===1?toM:toI;
 (UM[type]||[]).forEach(f=>{
  let el=$(f.id);if(!el)return;
  let v=+el.value;
  if(!isNaN(v)&&v!==0)el.value=cvt(v);
  if(f.lbl)$(f.lbl).textContent=nv===1?f.ml:f.il;
 });
 // Special header labels
 if(type==='velocity')$('th_mbv').textContent=nv===1?'Velocity (m/s)':'Velocity (fps)';
 if(type==='distance'){
  $('th_stg_dist').textContent=nv===1?'Distance (m)':'Distance (yd)';
  stages.forEach(s=>{s.distance=nv===1?+CV.yd_m(s.distance):CV.m_yd(s.distance)});
  renderStages();
 }
 U[type]=nv;
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

// Convert displayed value back to imperial for a given unit type
function toImp(type,v){
 if(U[type]===0)return v;
 return CV[CF[type][1]](v);
}
function gatherConfig(){
 let c={
  bc:+$('bc').value, drag_table:$('drag_table').value,
  weight:toImp('weight',+$('weight').value), caliber:toImp('length',+$('caliber').value),
  length:toImp('length',+$('length').value), mv:toImp('velocity',+$('mv').value),
  sh:toImp('length',+$('sh').value), twist:toImp('length',+$('twist').value),
  zero:toImp('distance',+$('zero').value), lat:+$('lat').value,
  corr_unit:$('corr_unit').value, click_size:+$('click_size').value,
  cant_offset:+$('cant_offset').value, cant_sens:+$('cant_sens').value,
  multi_bc:$('multi_bc').checked,
  use_ps:$('use_ps').checked,
  units:{distance:U.distance,velocity:U.velocity,weight:U.weight,
         length:U.length,temperature:U.temperature,pressure:U.pressure}
 };
 if(c.multi_bc){
  c.bc_points=[];
  for(let i=1;i<=3;i++){
   let b=+$('mbc'+i).value,v=toImp('velocity',+$('mbv'+i).value);
   if(b>0&&v>0)c.bc_points.push({bc:b,vel:v});
  }
 }
 if(c.use_ps){
  c.ps={v1:toImp('velocity',+$('ps_v1').value),t1:toImp('temperature',+$('ps_t1').value),
        v2:toImp('velocity',+$('ps_v2').value),t2:toImp('temperature',+$('ps_t2').value)};
 }
 return c;
}

async function saveAll(){
 try{
  let stg=stages.map(s=>({name:s.name,distance:U.distance?CV.m_yd(s.distance):s.distance}));
  let body=JSON.stringify({config:gatherConfig(),stages:stg});
  let r=await fetch('/api/save',{method:'POST',headers:{'Content-Type':'application/json'},body});
  if(r.ok)toast('Saved!');else toast('Save failed: '+r.status,3000);
 }catch(e){toast('Error: '+e.message,3000)}
}

async function disableWifi(){
 try{await fetch('/api/wifi/off',{method:'POST'});toast('WiFi shutting down...');}
 catch(e){toast('Sent shutdown request')}
}
async function calibrateCant(){
 let btn=$('btn_cant_cal'),st=$('cant_cal_status');
 btn.disabled=true;
 st.textContent='Calibrating... (hold rifle level)';
 try{
  let r=await fetch('/api/cant/calibrate',{method:'POST'});
  if(!r.ok){st.textContent='Failed ('+r.status+')';btn.disabled=false;return;}
  let attempts=0;
  let poll=setInterval(async()=>{
   attempts++;
   try{
    let cr=await fetch('/api/config');
    if(cr.ok){
     let d=await cr.json();
     if(d.config && !d.config.cant_calibrating){
      let off=d.config.cant_offset||0;
      $('cant_offset').value=off.toFixed(1);
      st.textContent='Done! Offset: '+off.toFixed(1)+'\u00b0';
      clearInterval(poll);btn.disabled=false;
     } else {
      st.textContent='Calibrating... ('+attempts+'s)';
     }
    }
   }catch(e){}
   if(attempts>=35){clearInterval(poll);st.textContent='Timeout';btn.disabled=false;}
  },1000);
 }catch(e){st.textContent='Error: '+e.message;btn.disabled=false;}
}
async function loadConfig(){
 try{
  let r=await fetch('/api/config');
  if(!r.ok)return;
  let d=await r.json();
  if(d.config){
   let c=d.config;
   // Load per-unit prefs
   let uu=c.units||{};
   let selMap={distance:'u_dist',velocity:'u_vel',weight:'u_wt',length:'u_len',temperature:'u_temp',pressure:'u_press'};
   for(let t in selMap){
    let v=uu[t]||0;
    $(selMap[t]).value=v;
    U[t]=v;
   }
   // Convert from imperial (server values) to displayed unit
   let dsp=(type,v)=>U[type]?CV[CF[type][0]](v):v;
   $('bc').value=c.bc||.315;$('drag_table').value=c.drag_table||'G7';
   $('weight').value=dsp('weight',c.weight||140);$('caliber').value=dsp('length',c.caliber||.264);
   $('length').value=dsp('length',c.length||1.35);$('mv').value=dsp('velocity',c.mv||2710);
   $('sh').value=dsp('length',c.sh||1.5);$('twist').value=dsp('length',c.twist||8);
   $('zero').value=dsp('distance',c.zero||100);$('lat').value=c.lat||0;
   $('corr_unit').value=c.corr_unit||'MOA';$('click_size').value=c.click_size||.25;
   $('cant_offset').value=c.cant_offset||0;$('cant_sens').value=c.cant_sens||1;
   $('multi_bc').checked=!!c.multi_bc;toggleMultiBC();
   if(c.bc_points){
    c.bc_points.forEach((p,i)=>{
     if(i<3){$('mbc'+(i+1)).value=p.bc;$('mbv'+(i+1)).value=dsp('velocity',p.vel)}
    });
   }
   $('use_ps').checked=!!c.use_ps;togglePS();
   if(c.ps){
    $('ps_v1').value=dsp('velocity',c.ps.v1);$('ps_t1').value=dsp('temperature',c.ps.t1);
    $('ps_v2').value=dsp('velocity',c.ps.v2);$('ps_t2').value=dsp('temperature',c.ps.t2);
   }
   // Apply metric labels where needed
   for(let t in UM){
    if(U[t])(UM[t]||[]).forEach(f=>{if(f.lbl)$(f.lbl).textContent=f.ml});
   }
   if(U.velocity)$('th_mbv').textContent='Velocity (m/s)';
   if(U.distance)$('th_stg_dist').textContent='Distance (m)';
  }
  if(d.stages){
   stages=d.stages.map(s=>({name:s.name,distance:U.distance?+CV.yd_m(s.distance):s.distance}));
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
