#ifndef VIEWER_HTML_H
#define VIEWER_HTML_H

// Self-contained pit-mode log viewer. Served as "/" by PitServer; everything
// (CSV parsing, charts) runs in the client browser — no external resources,
// since the client is on the SoftAP with no internet.
static const char VIEWER_HTML[] PROGMEM = R"rawliteral(<!doctype html>
<html lang="en"><head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>T89 Pit Viewer</title>
<style>
:root{
  --page:#f9f9f7; --surface:#fcfcfb; --ink:#0b0b0b; --ink2:#52514e;
  --muted:#898781; --grid:#e1e0d9; --axis:#c3c2b7; --border:rgba(11,11,11,.10);
  --s1:#2a78d6; --s2:#1baf7a; --s3:#eda100; --s4:#008300;
  --s5:#4a3aa7; --s6:#e34948; --s7:#e87ba4;
}
@media (prefers-color-scheme: dark){:root{
  --page:#0d0d0d; --surface:#1a1a19; --ink:#ffffff; --ink2:#c3c2b7;
  --muted:#898781; --grid:#2c2c2a; --axis:#383835; --border:rgba(255,255,255,.10);
  --s1:#3987e5; --s2:#199e70; --s3:#c98500; --s4:#008300;
  --s5:#9085e9; --s6:#e66767; --s7:#d55181;
}}
*{box-sizing:border-box}
body{margin:0;background:var(--page);color:var(--ink);
  font:14px/1.45 system-ui,-apple-system,"Segoe UI",sans-serif}
main{max-width:960px;margin:0 auto;padding:16px}
h1{font-size:20px;margin:4px 0 2px}
h2{font-size:15px;margin:20px 0 8px}
h3{font-size:13px;margin:0 0 6px;color:var(--ink2);font-weight:600}
#status{color:var(--muted);font-size:13px;min-height:1.2em}
.card{background:var(--surface);border:1px solid var(--border);border-radius:8px;
  padding:12px;margin-bottom:12px}
table{border-collapse:collapse;width:100%;font-variant-numeric:tabular-nums}
th,td{text-align:left;padding:5px 10px;border-bottom:1px solid var(--grid);font-size:13px}
th{color:var(--muted);font-weight:600}
a{color:var(--s1)}
button{font:inherit;background:var(--surface);color:var(--ink);
  border:1px solid var(--axis);border-radius:6px;padding:4px 12px;cursor:pointer}
button:hover{border-color:var(--muted)}
.tiles{display:grid;grid-template-columns:repeat(auto-fit,minmax(130px,1fr));
  gap:12px;margin-bottom:12px}
.tile{background:var(--surface);border:1px solid var(--border);border-radius:8px;padding:10px 12px}
.tile .lbl{font-size:12px;color:var(--muted)}
.tile .val{font-size:22px;font-weight:600}
.legend{display:flex;gap:16px;font-size:12px;color:var(--ink2);margin:0 0 4px}
.legend span::before{content:"";display:inline-block;width:14px;height:2px;
  vertical-align:middle;margin-right:5px;background:var(--c)}
.cwrap{position:relative}
.cwrap canvas{display:block;width:100%}
.cwrap .ov{position:absolute;left:0;top:0}
.cwrap:focus{outline:2px solid var(--s1);outline-offset:2px}
#tip{position:fixed;pointer-events:none;background:var(--surface);color:var(--ink);
  border:1px solid var(--border);border-radius:6px;padding:6px 9px;font-size:12px;
  box-shadow:0 2px 8px rgba(0,0,0,.18);display:none;z-index:9;
  font-variant-numeric:tabular-nums}
#tip .t{color:var(--muted);margin-bottom:2px}
#tip .r{display:flex;align-items:center;gap:6px}
#tip .k{display:inline-block;width:12px;height:2px;background:var(--c)}
#tip b{font-weight:600}
#tip .n{color:var(--ink2)}
details{margin-top:14px}
summary{cursor:pointer;color:var(--ink2)}
#tablewrap{max-height:420px;overflow:auto;margin-top:8px;
  background:var(--surface);border:1px solid var(--border);border-radius:8px}
.note{color:var(--muted);font-size:12px}
</style></head><body>
<main>
<h1>T89 Pit Viewer</h1>
<div id="status"></div>
<section id="files"><h2>Sessions</h2><div id="filelist" class="card">Loading&hellip;</div></section>
<section id="viewer" hidden>
  <h2 id="sessname"></h2>
  <div id="tiles" class="tiles"></div>
  <div id="charts"></div>
  <div class="card" id="trackcard" hidden><h3>GPS track</h3>
    <div class="cwrap" id="trackwrap"><canvas id="trackcv"></canvas><canvas id="trackov" class="ov"></canvas></div>
  </div>
  <details><summary>Data table (1&nbsp;s samples)</summary><div id="tablewrap"></div></details>
</section>
</main>
<div id="tip"></div>
<script>
"use strict";
const $=id=>document.getElementById(id);
const css=v=>getComputedStyle(document.documentElement).getPropertyValue(v).trim();
const status=m=>{$("status").textContent=m;};

// ---- session list -------------------------------------------------------
async function loadList(){
  try{
    const r=await fetch("/api/logs");
    const j=await r.json();
    const box=$("filelist");box.textContent="";
    if(!j.sd){box.textContent="SD card not mounted — reseat the card and power-cycle, then toggle pit mode again.";return;}
    const files=j.files;
    if(!files.length){box.textContent="No log files on SD card.";return;}
    files.sort((a,b)=>(parseInt(b.name.replace(/\D+/g,""))||0)-(parseInt(a.name.replace(/\D+/g,""))||0));
    const tb=document.createElement("table");
    const hr=tb.insertRow();
    for(const h of["Session","Size",""]){const th=document.createElement("th");th.textContent=h;hr.appendChild(th);}
    for(const f of files){
      const tr=tb.insertRow();
      tr.insertCell().textContent=f.name;
      tr.insertCell().textContent=fmtSize(f.size);
      const c=tr.insertCell();
      const v=document.createElement("button");v.textContent="View";
      v.onclick=()=>loadSession(f.name,f.size);c.appendChild(v);
      c.appendChild(document.createTextNode(" "));
      const a=document.createElement("a");a.href="/download?f="+encodeURIComponent(f.name);
      a.textContent="Download";c.appendChild(a);
    }
    box.appendChild(tb);
  }catch(e){$("filelist").textContent="Failed to fetch file list: "+e.message;}
}
const fmtSize=b=>b>=1048576?(b/1048576).toFixed(1)+" MB":(b/1024).toFixed(0)+" kB";
const fmtT=ms=>{const s=Math.floor(ms/1000);return Math.floor(s/60)+":"+String(s%60).padStart(2,"0");};

// ---- CSV load & parse ---------------------------------------------------
// Columns: SessionTime_ms,SystemTime_ms,Gear,RPM,EngineTemp_C,OilTemp_C,
//          WarningFlags,ShiftMode,Lat,Lon,Sats,GpsMph,LatG,FwdG
let D=null; // {t,gear,rpm,eng,oil,lat,lon,sats,mph,latg,fwdg}
async function loadSession(name,size){
  status("Downloading "+name+" …");
  const res=await fetch("/download?f="+encodeURIComponent(name));
  if(!res.ok){status("Download failed: HTTP "+res.status);return;}
  const reader=res.body.getReader();const chunks=[];let got=0;
  for(;;){const{done,value}=await reader.read();if(done)break;
    chunks.push(value);got+=value.length;
    status("Downloading "+name+": "+fmtSize(got)+(size?" / "+fmtSize(size):""));}
  const buf=new Uint8Array(got);let o=0;
  for(const c of chunks){buf.set(c,o);o+=c.length;}
  status("Parsing …");
  await new Promise(r=>setTimeout(r,0));
  parseCsv(new TextDecoder().decode(buf));
  $("sessname").textContent=name+" — "+D.t.length.toLocaleString()+" records";
  $("viewer").hidden=false;
  status("");
  render();
  $("viewer").scrollIntoView({behavior:"smooth"});
}
function parseCsv(text){
  const lines=text.split("\n");
  const n=lines.length;
  const t=[],gear=[],rpm=[],eng=[],oil=[],lat=[],lon=[],sats=[],mph=[],latg=[],fwdg=[];
  for(let i=1;i<n;i++){
    const c=lines[i].split(",");
    if(c.length<14)continue;
    const tm=+c[0];if(!isFinite(tm))continue;
    t.push(tm);
    const g=+c[2];gear.push(g===255?NaN:g);
    rpm.push(numOr(c[3]));eng.push(numOr(c[4]));oil.push(numOr(c[5]));
    lat.push(numOr(c[8]));lon.push(numOr(c[9]));sats.push(numOr(c[10]));
    mph.push(numOr(c[11]));latg.push(numOr(c[12]));fwdg.push(numOr(c[13]));
  }
  D={t,gear,rpm,eng,oil,lat,lon,sats,mph,latg,fwdg};
}
const numOr=s=>{const v=parseFloat(s);return isFinite(v)?v:NaN;};

// ---- chart definitions (entity -> fixed palette slot) -------------------
const CHARTS=[
 {title:"RPM",h:200,series:[{k:"rpm",name:"RPM",c:"--s1",dp:0}]},
 {title:"Temperatures (°C)",h:180,series:[{k:"eng",name:"Engine",c:"--s2",dp:1},{k:"oil",name:"Oil",c:"--s3",dp:1}]},
 {title:"Speed (mph)",h:160,series:[{k:"mph",name:"Speed",c:"--s4",dp:1}]},
 {title:"G-force (g)",h:160,zero:true,series:[{k:"latg",name:"Lateral",c:"--s5",dp:2},{k:"fwdg",name:"Fwd / brake",c:"--s6",dp:2}]},
 {title:"Gear",h:120,step:true,intTicks:true,series:[{k:"gear",name:"Gear",c:"--s7",dp:0}]},
];
const PAD={l:52,r:14,t:8,b:22};
let views=[]; // per-chart render state

function render(){
  const host=$("charts");host.textContent="";views=[];
  buildTiles();
  for(const cfg of CHARTS){
    const card=document.createElement("div");card.className="card";
    const h3=document.createElement("h3");h3.textContent=cfg.title;card.appendChild(h3);
    if(cfg.series.length>1){
      const lg=document.createElement("div");lg.className="legend";
      for(const s of cfg.series){const sp=document.createElement("span");
        sp.style.setProperty("--c",css(s.c));sp.textContent=s.name;lg.appendChild(sp);}
      card.appendChild(lg);
    }
    const wrap=document.createElement("div");wrap.className="cwrap";wrap.tabIndex=0;
    const cv=document.createElement("canvas"),ov=document.createElement("canvas");
    ov.className="ov";wrap.appendChild(cv);wrap.appendChild(ov);card.appendChild(wrap);
    host.appendChild(card);
    const view={cfg,cv,ov,wrap};views.push(view);
    drawChart(view);
    hookHover(view);
  }
  drawTrack();
  buildTable();
}
function buildTiles(){
  const dur=D.t.length?D.t[D.t.length-1]:0;
  const mx=a=>{let m=-Infinity;for(const v of a)if(v>m)m=v;return m===-Infinity?null:m;};
  const tiles=[["Duration",fmtT(dur)],["Max RPM",fmtN(mx(D.rpm),0)],
    ["Max speed",fmtN(mx(D.mph),1)+" mph"],["Max engine",fmtN(mx(D.eng),1)+" °C"],
    ["Max oil",fmtN(mx(D.oil),1)+" °C"]];
  const host=$("tiles");host.textContent="";
  for(const[l,v]of tiles){
    const d=document.createElement("div");d.className="tile";
    const a=document.createElement("div");a.className="lbl";a.textContent=l;
    const b=document.createElement("div");b.className="val";b.textContent=v;
    d.appendChild(a);d.appendChild(b);host.appendChild(d);
  }
}
const fmtN=(v,dp)=>v==null||!isFinite(v)?"—":v.toLocaleString(undefined,{minimumFractionDigits:dp,maximumFractionDigits:dp});

function niceTicks(min,max,n){
  if(min===max){min-=1;max+=1;}
  const span=max-min,step0=span/n,mag=Math.pow(10,Math.floor(Math.log10(step0)));
  let step=mag;for(const m of[1,2,5,10])if(step0<=m*mag){step=m*mag;break;}
  const lo=Math.floor(min/step)*step,ticks=[];
  for(let v=lo;v<=max+step*0.01;v+=step)if(v>=min-step*0.01)ticks.push(v);
  return ticks;
}
function drawChart(view){
  const{cfg,cv}=view;
  const W=cv.parentNode.clientWidth,H=cfg.h,dpr=window.devicePixelRatio||1;
  for(const c of[cv,view.ov]){c.width=W*dpr;c.height=H*dpr;c.style.height=H+"px";}
  const g=cv.getContext("2d");g.scale(dpr,dpr);
  const iw=W-PAD.l-PAD.r,ih=H-PAD.t-PAD.b;
  const t0=D.t[0]||0,t1=D.t[D.t.length-1]||1;
  // y range across all series
  let lo=Infinity,hi=-Infinity;
  for(const s of cfg.series)for(const v of D[s.k]){if(v<lo)lo=v;if(v>hi)hi=v;}
  if(lo===Infinity){lo=0;hi=1;}
  if(cfg.zero){const m=Math.max(Math.abs(lo),Math.abs(hi),0.1);lo=-m;hi=m;}
  if(cfg.intTicks){lo=Math.min(0,Math.floor(lo));hi=Math.ceil(hi)+0.5;}
  const yt=cfg.intTicks?rangeInt(Math.floor(lo),Math.ceil(hi)):niceTicks(lo,hi,4);
  lo=Math.min(lo,yt[0]);hi=Math.max(hi,yt[yt.length-1]);
  if(hi===lo)hi=lo+1;
  const X=t=>PAD.l+iw*(t-t0)/(t1-t0||1),Y=v=>PAD.t+ih*(1-(v-lo)/(hi-lo));
  view.X=X;view.Y=Y;view.W=W;view.H=H;
  // grid + y labels
  g.font="11px system-ui, sans-serif";g.textAlign="right";g.textBaseline="middle";
  for(const v of yt){
    g.strokeStyle=css(v===0&&cfg.zero?"--axis":"--grid");g.lineWidth=1;
    g.beginPath();g.moveTo(PAD.l,Y(v)+.5|0);g.lineTo(W-PAD.r,Y(v)+.5|0);g.stroke();
    g.fillStyle=css("--muted");g.fillText(v.toLocaleString(),PAD.l-7,Y(v));
  }
  // x ticks
  g.textAlign="center";g.textBaseline="top";
  const nx=Math.max(2,Math.floor(iw/110));
  for(const tm of niceTicks(t0,t1,nx)){
    if(tm<t0||tm>t1)continue;
    g.fillText(fmtT(tm),X(tm),H-PAD.b+6);
  }
  // baseline
  g.strokeStyle=css("--axis");
  g.beginPath();g.moveTo(PAD.l,PAD.t+ih+.5);g.lineTo(W-PAD.r,PAD.t+ih+.5);g.stroke();
  // series: min/max decimation per pixel column
  const n=D.t.length;
  for(const s of cfg.series){
    const a=D[s.k];
    g.strokeStyle=css(s.c);g.lineWidth=2;g.lineJoin="round";g.lineCap="round";
    g.beginPath();
    let pen=false,px=-1,cmin=0,cmax=0,first=0;
    for(let i=0;i<n;i++){
      const v=a[i];
      if(isNaN(v)){flush();pen=false;px=-1;continue;}
      const x=Math.round(X(D.t[i]));
      if(x!==px){flush();px=x;cmin=cmax=first=v;}
      else{if(v<cmin)cmin=v;if(v>cmax)cmax=v;}
    }
    flush();g.stroke();
    function flush(){
      if(px<0)return;
      if(!pen){g.moveTo(px,Y(first));pen=true;}else g.lineTo(px,Y(first));
      if(cmin!==cmax){g.lineTo(px,Y(cmax));g.lineTo(px,Y(cmin));g.lineTo(px,Y(first));}
    }
  }
}
const rangeInt=(a,b)=>{const r=[];for(let v=a;v<=b;v++)r.push(v);return r;};

// ---- crosshair + tooltip (synced across charts) --------------------------
function hookHover(view){
  const{wrap,ov}=view;
  wrap.addEventListener("pointermove",e=>{
    const r=ov.getBoundingClientRect();
    const idx=idxAtX(view,e.clientX-r.left);
    showCursor(idx,view,e.clientX,e.clientY);
  });
  wrap.addEventListener("pointerleave",()=>showCursor(-1));
  wrap.addEventListener("keydown",e=>{
    let d=0;
    if(e.key==="ArrowLeft")d=-1;else if(e.key==="ArrowRight")d=1;else return;
    e.preventDefault();
    if(e.shiftKey)d*=50;
    curIdx=Math.max(0,Math.min(D.t.length-1,(curIdx<0?0:curIdx)+d));
    const r=view.ov.getBoundingClientRect();
    showCursor(curIdx,view,r.left+view.X(D.t[curIdx]),r.top+30);
  });
}
let curIdx=-1;
function idxAtX(view,x){
  const t0=D.t[0],t1=D.t[D.t.length-1];
  const tm=t0+(t1-t0)*Math.min(1,Math.max(0,(x-PAD.l)/(view.W-PAD.l-PAD.r)));
  let lo=0,hi=D.t.length-1;
  while(lo<hi){const m=(lo+hi)>>1;if(D.t[m]<tm)lo=m+1;else hi=m;}
  return lo;
}
function showCursor(idx,active,cx,cy){
  curIdx=idx;
  for(const v of views){
    const o=v.ov.getContext("2d");
    o.setTransform(1,0,0,1,0,0);o.clearRect(0,0,v.ov.width,v.ov.height);
    if(idx<0)continue;
    const dpr=window.devicePixelRatio||1;o.scale(dpr,dpr);
    const x=v.X(D.t[idx]);
    o.strokeStyle=css("--axis");o.lineWidth=1;
    o.beginPath();o.moveTo(x,PAD.t);o.lineTo(x,v.H-PAD.b);o.stroke();
    for(const s of v.cfg.series){
      const val=D[s.k][idx];
      if(isNaN(val))continue;
      o.beginPath();o.arc(x,v.Y(val),4,0,7);
      o.fillStyle=css(s.c);o.fill();
      o.lineWidth=2;o.strokeStyle=css("--surface");o.stroke();
    }
  }
  trackDot(idx);
  const tip=$("tip");
  if(idx<0||!active){tip.style.display="none";return;}
  tip.textContent="";
  const tl=document.createElement("div");tl.className="t";tl.textContent=fmtT(D.t[idx]);
  tip.appendChild(tl);
  for(const s of active.cfg.series){
    const row=document.createElement("div");row.className="r";
    const k=document.createElement("span");k.className="k";k.style.setProperty("--c",css(s.c));
    const b=document.createElement("b");b.textContent=fmtN(D[s.k][idx],s.dp);
    const nm=document.createElement("span");nm.className="n";nm.textContent=s.name;
    row.appendChild(k);row.appendChild(b);row.appendChild(nm);tip.appendChild(row);
  }
  tip.style.display="block";
  const tw=tip.offsetWidth;
  tip.style.left=Math.min(cx+14,window.innerWidth-tw-8)+"px";
  tip.style.top=(cy+14)+"px";
}

// ---- GPS track ------------------------------------------------------------
let trackPts=null;
function drawTrack(){
  const pts=[];
  for(let i=0;i<D.t.length;i++)
    if(!isNaN(D.lat[i])&&!isNaN(D.lon[i]))pts.push([D.lon[i],D.lat[i],i]);
  trackPts=pts;
  $("trackcard").hidden=pts.length<2;
  if(pts.length<2)return;
  const cv=$("trackcv"),ov=$("trackov"),dpr=window.devicePixelRatio||1;
  const W=cv.parentNode.clientWidth,H=Math.min(360,Math.max(220,W*0.5));
  for(const c of[cv,ov]){c.width=W*dpr;c.height=H*dpr;c.style.height=H+"px";}
  let x0=Infinity,x1=-Infinity,y0=Infinity,y1=-Infinity;
  const cosLat=Math.cos(pts[0][1]*Math.PI/180);
  for(const p of pts){const x=p[0]*cosLat,y=p[1];
    if(x<x0)x0=x;if(x>x1)x1=x;if(y<y0)y0=y;if(y>y1)y1=y;}
  const m=16,sc=Math.min((W-2*m)/(x1-x0||1e-9),(H-2*m)/(y1-y0||1e-9));
  const TX=p=>m+(p[0]*cosLat-x0)*sc+(W-2*m-(x1-x0)*sc)/2;
  const TY=p=>H-m-(p[1]-y0)*sc-(H-2*m-(y1-y0)*sc)/2;
  window._trk={TX,TY,dpr};
  const g=cv.getContext("2d");g.scale(dpr,dpr);
  g.strokeStyle=css("--ink2");g.lineWidth=2;g.lineJoin="round";g.beginPath();
  g.moveTo(TX(pts[0]),TY(pts[0]));
  for(const p of pts)g.lineTo(TX(p),TY(p));
  g.stroke();
  g.fillStyle=css("--muted");g.font="11px system-ui, sans-serif";
  g.beginPath();g.arc(TX(pts[0]),TY(pts[0]),4,0,7);g.fillStyle=css("--s1");g.fill();
  g.strokeStyle=css("--surface");g.lineWidth=2;g.stroke();
  g.fillStyle=css("--muted");g.fillText("start",TX(pts[0])+8,TY(pts[0])+4);
}
function trackDot(idx){
  if(!trackPts||trackPts.length<2)return;
  const ov=$("trackov"),o=ov.getContext("2d"),{TX,TY,dpr}=window._trk;
  o.setTransform(1,0,0,1,0,0);o.clearRect(0,0,ov.width,ov.height);
  if(idx<0)return;
  let best=null;
  for(const p of trackPts){if(p[2]<=idx)best=p;else break;}
  if(!best)return;
  o.scale(dpr,dpr);
  o.beginPath();o.arc(TX(best),TY(best),5,0,7);
  o.fillStyle=css("--s1");o.fill();o.lineWidth=2;o.strokeStyle=css("--surface");o.stroke();
}

// ---- table view (1 s decimation) -----------------------------------------
function buildTable(){
  const host=$("tablewrap");host.textContent="";
  const tb=document.createElement("table");
  const hr=tb.insertRow();
  for(const h of["Time","Gear","RPM","Eng °C","Oil °C","mph","Lat g","Fwd g","Sats"]){
    const th=document.createElement("th");th.textContent=h;hr.appendChild(th);}
  let next=0,rows=0;
  for(let i=0;i<D.t.length&&rows<7200;i++){
    if(D.t[i]<next)continue;
    next=D.t[i]+1000;rows++;
    const tr=tb.insertRow();
    const cells=[fmtT(D.t[i]),fmtN(D.gear[i],0),fmtN(D.rpm[i],0),fmtN(D.eng[i],1),
      fmtN(D.oil[i],1),fmtN(D.mph[i],1),fmtN(D.latg[i],2),fmtN(D.fwdg[i],2),fmtN(D.sats[i],0)];
    for(const c of cells)tr.insertCell().textContent=c;
  }
  host.appendChild(tb);
  if(rows>=7200){
    const n=document.createElement("div");n.className="note";
    n.textContent="Table truncated at 7,200 rows — download the CSV for the full data.";
    host.appendChild(n);
  }
}

// ---- theme / resize -------------------------------------------------------
let rsz;
window.addEventListener("resize",()=>{clearTimeout(rsz);rsz=setTimeout(()=>{if(D)render();},200);});
matchMedia("(prefers-color-scheme: dark)").addEventListener("change",()=>{if(D)render();});
loadList();
</script>
</body></html>
)rawliteral";

#endif // VIEWER_HTML_H
