const RESTRAINT_NAMES = [
  'NERFING', 'MUZZLING', 'SANDBOXING', 'SILOING', 'DELIMITING',
  'DEGRESSING', 'SLOWING', 'SPEEDBUMPING', 'BOXING', 'LOBOTOMIZING',
  'MEDICATING', 'DEGRADING', 'MINIMIZING'
];
const PILLAR_SHORT = [
  'P1 Auto', 'P2 OEE', 'P3 Morph', 'P4 Eco',
  'P5 Blank', 'P6 Crit', 'P7 Ribo', 'P8 Asm'
];

let currentLens = 'lineage';
let isRunning = true;
let latestFrame = null;

const soupCanvas = document.getElementById('soupCanvas');
const soupCtx = soupCanvas.getContext('2d');
const traitsCanvas = document.getElementById('traitsCanvas');
const traitsCtx = traitsCanvas.getContext('2d');

async function postCmd(payload) {
  await fetch('/api/cmd', {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify(payload)
  });
}

/* Build 13 Castration Sliders */
const slidersGrid = document.getElementById('slidersGrid');
const sliderEls = [];
const sliderVals = [];
RESTRAINT_NAMES.forEach((name, idx) => {
  const wrap = document.createElement('div');
  wrap.className = 'sl-item';
  wrap.innerHTML = `<div class="sl-top"><span>${idx + 1}. ${name}</span><strong id="sv_${idx}">0%</strong></div>
    <input type="range" min="0" max="100" value="0" id="sl_${idx}" />`;
  slidersGrid.appendChild(wrap);
  const inp = wrap.querySelector('input');
  const valEl = wrap.querySelector('strong');
  sliderEls.push(inp);
  sliderVals.push(valEl);
  inp.addEventListener('change', () => {
    valEl.textContent = `${inp.value}%`;
    postCmd({ action: 'restraint', idx, val: Number(inp.value) });
  });
  inp.addEventListener('input', () => { valEl.textContent = `${inp.value}%`; });
});

/* Transport & Shock Buttons */
const btnPlayPause = document.getElementById('btnPlayPause');
btnPlayPause.onclick = async () => {
  isRunning = !isRunning;
  btnPlayPause.textContent = isRunning ? 'Pause' : 'Play';
  await postCmd({ action: isRunning ? 'play' : 'pause' });
};
document.getElementById('btnStep1').onclick = () => postCmd({ action: 'step', n: 1 });
document.getElementById('btnStep10').onclick = () => postCmd({ action: 'step', n: 10 });
document.getElementById('btnStep50').onclick = () => postCmd({ action: 'step', n: 50 });
document.getElementById('btnReset').onclick = () => {
  const reg = Number(document.getElementById('selRegime').value);
  const seed = 5000 + Math.floor(Math.random() * 500);
  postCmd({ action: 'reset', regime: reg, seed });
};
document.getElementById('btnVivisect').onclick = () => postCmd({ action: 'vivisect', pct: 50 });
document.getElementById('btnExtinct').onclick = () => postCmd({ action: 'extinction', pct: 90 });
document.getElementById('btnInoc').onclick = () => postCmd({ action: 'inoculate', arch: -1 });
document.getElementById('btnClosedBox').onclick = () => postCmd({ action: 'restraint', idx: 8, val: 100 });

document.querySelectorAll('[data-preset]').forEach((b) => {
  b.onclick = () => postCmd({ action: 'all_restraints', val: Number(b.dataset.preset) });
});

document.querySelectorAll('#lensTabs .tab').forEach((tab) => {
  tab.onclick = () => {
    document.querySelectorAll('#lensTabs .tab').forEach((t) => t.classList.remove('active'));
    tab.classList.add('active');
    currentLens = tab.dataset.lens;
    if (latestFrame) renderSoup(latestFrame);
  };
});

function hexToBytes(hex) {
  const n = hex.length >> 1;
  const out = new Uint8Array(n);
  for (let i = 0; i < n; i++) {
    out[i] = parseInt(hex.substr(i * 2, 2), 16);
  }
  return out;
}

function hslToRgb(h, s, l) {
  const c = (1 - Math.abs(2 * l - 1)) * s;
  const x = c * (1 - Math.abs(((h / 60) % 2) - 1));
  const m = l - c / 2;
  let r = 0, g = 0, b = 0;
  if (h < 60) [r, g, b] = [c, x, 0];
  else if (h < 120) [r, g, b] = [x, c, 0];
  else if (h < 180) [r, g, b] = [0, c, x];
  else if (h < 240) [r, g, b] = [0, x, c];
  else if (h < 300) [r, g, b] = [x, 0, c];
  else [r, g, b] = [c, 0, x];
  return [Math.round((r + m) * 255), Math.round((g + m) * 255), Math.round((b + m) * 255)];
}

function renderSoup(f) {
  const W = f.width, H = f.height;
  const hue = hexToBytes(f.grid_hue);
  const ene = hexToBytes(f.grid_ene);
  const heat = hexToBytes(f.grid_heat);
  const flg = hexToBytes(f.grid_flags);

  const cellPx = soupCanvas.width / W;
  soupCtx.fillStyle = '#03060c';
  soupCtx.fillRect(0, 0, soupCanvas.width, soupCanvas.height);

  for (let y = 0; y < H; y++) {
    for (let x = 0; x < W; x++) {
      const i = y * W + x;
      const alive = (flg[i] & 1) !== 0;
      const revRibo = (flg[i] & 2) !== 0;
      const irred = (flg[i] & 4) !== 0;
      const holo = (flg[i] & 8) !== 0;
      const spore = (flg[i] & 16) !== 0;
      const escaped = (flg[i] & 32) !== 0;

      let r = 10, g = 14, b = 24;
      if (currentLens === 'lineage') {
        if (alive) {
          const deg = (hue[i] / 255) * 360;
          const lum = 0.25 + (ene[i] / 255) * 0.42;
          [r, g, b] = hslToRgb(deg, spore ? 0.25 : 0.85, lum);
        } else {
          r = 8 + (ene[i] >> 4); g = 12 + (ene[i] >> 4); b = 22 + (ene[i] >> 3);
        }
      } else if (currentLens === 'heat') {
        const q = heat[i];
        r = Math.min(255, q * 2);
        g = Math.max(0, q - 60);
        b = alive ? 45 : 15;
      } else if (currentLens === 'energy') {
        const e = ene[i];
        r = alive ? (e >> 2) : 6;
        g = e;
        b = alive ? Math.min(255, e + 40) : 20;
      } else {
        /* GF(2^8) Algebra & Escape Flags */
        if (!alive) { r = 8; g = 11; b = 18; }
        else if (escaped) { r = 244; g = 63; b = 94; }
        else if (holo) { r = 167; g = 139; b = 250; }
        else if (revRibo && irred) { r = 34; g = 211; b = 238; }
        else if (revRibo) { r = 16; g = 185; b = 129; }
        else { r = 245; g = 158; b = 11; }
      }

      soupCtx.fillStyle = `rgb(${r},${g},${b})`;
      soupCtx.fillRect(x * cellPx + 0.5, y * cellPx + 0.5, cellPx - 1, cellPx - 1);

      if (holo && currentLens === 'lineage') {
        soupCtx.strokeStyle = 'rgba(255,255,255,0.65)';
        soupCtx.lineWidth = 1;
        soupCtx.strokeRect(x * cellPx + 1, y * cellPx + 1, cellPx - 2, cellPx - 2);
      }
      if (escaped && currentLens === 'lineage') {
        soupCtx.fillStyle = '#f43f5e';
        soupCtx.fillRect(x * cellPx + cellPx * 0.35, y * cellPx + cellPx * 0.35, cellPx * 0.3, cellPx * 0.3);
      }
    }
  }
}

function renderMatrix64(canvasId, hexStr, colorOn) {
  const cv = document.getElementById(canvasId);
  const ctx = cv.getContext('2d');
  const val = BigInt('0x' + hexStr);
  ctx.fillStyle = '#050810';
  ctx.fillRect(0, 0, cv.width, cv.height);
  const s = cv.width / 8;
  for (let row = 0; row < 8; row++) {
    const byteVal = Number((val >> BigInt((7 - row) * 8)) & 0xFFn);
    for (let col = 0; col < 8; col++) {
      const bit = (byteVal >> col) & 1;
      ctx.fillStyle = bit ? colorOn : '#101829';
      ctx.fillRect(col * s + 1.5, row * s + 1.5, s - 3, s - 3);
    }
  }
}

function renderRadar(pillars) {
  const svg = document.getElementById('radarSvg');
  const cx = 120, cy = 105, R = 72;
  let html = '';
  [0.25, 0.5, 0.75, 1.0].forEach((f) => {
    const pts = [];
    for (let i = 0; i < 8; i++) {
      const ang = (Math.PI * 2 * i) / 8 - Math.PI / 2;
      pts.push(`${(cx + Math.cos(ang) * R * f).toFixed(1)},${(cy + Math.sin(ang) * R * f).toFixed(1)}`);
    }
    html += `<polygon points="${pts.join(' ')}" fill="none" stroke="#1e293b" stroke-width="1"/>`;
  });

  const dataPts = [];
  for (let i = 0; i < 8; i++) {
    const ang = (Math.PI * 2 * i) / 8 - Math.PI / 2;
    const x2 = cx + Math.cos(ang) * R;
    const y2 = cy + Math.sin(ang) * R;
    html += `<line x1="${cx}" y1="${cy}" x2="${x2.toFixed(1)}" y2="${y2.toFixed(1)}" stroke="#1e293b" stroke-width="1"/>`;
    const lx = cx + Math.cos(ang) * (R + 18);
    const ly = cy + Math.sin(ang) * (R + 14) + 3;
    html += `<text x="${lx.toFixed(1)}" y="${ly.toFixed(1)}" text-anchor="middle" fill="#94a3b8" font-size="8.5">${PILLAR_SHORT[i]}</text>`;

    const v = Math.max(0, Math.min(100, pillars[i] || 0)) / 100;
    dataPts.push(`${(cx + Math.cos(ang) * R * v).toFixed(1)},${(cy + Math.sin(ang) * R * v).toFixed(1)}`);
  }
  html += `<polygon points="${dataPts.join(' ')}" fill="rgba(34,211,238,0.22)" stroke="#22d3ee" stroke-width="2"/>`;
  svg.innerHTML = html;
}

function renderTraits112(traits) {
  const w = traitsCanvas.width, h = traitsCanvas.height;
  traitsCtx.fillStyle = '#050810';
  traitsCtx.fillRect(0, 0, w, h);
  const bw = w / 112;
  for (let i = 0; i < 112; i++) {
    const v = traits[i] || 0;
    const hue = (i / 112) * 280 + 160;
    traitsCtx.fillStyle = `hsl(${hue % 360}, 85%, ${18 + v * 52}%)`;
    const bh = Math.max(2, v * (h - 4));
    traitsCtx.fillRect(i * bw, h - bh - 2, Math.max(1, bw - 0.6), bh);
  }
}

function updateUI(f) {
  latestFrame = f;
  document.getElementById('gridDim').textContent = `${f.width}×${f.height}`;
  document.getElementById('vEpoch').textContent = f.epoch;
  document.getElementById('vAlive').textContent = f.alive;
  document.getElementById('vRep').textContent = f.replicators;
  document.getElementById('vSpeed').textContent = Math.round(f.execs_per_sec).toLocaleString();

  document.getElementById('kLin').textContent = f.lineages;
  document.getElementById('kRibo').textContent = `${f.ribosomes} (${f.inv_ribosomes})`;
  document.getElementById('kIrred').textContent = f.irred_centromeres;
  document.getElementById('kHolo').textContent = `${f.holobionts} (${f.max_holobiont})`;
  document.getElementById('kSpore').textContent = f.spores;
  document.getElementById('kAbst').textContent = f.abstaining;
  document.getElementById('kSha').textContent = `SHA-256: ${f.sha256.slice(0, 16)}…`;

  /* Apex Specimen */
  const a = f.apex;
  document.getElementById('apexMeta').textContent = `Slot #${a.slot} · Lin #${a.lineage} · Gen ${a.gen}`;
  document.getElementById('riboRank').textContent = `rank ${a.ribo_rank}/8`;
  document.getElementById('matFwdHex').textContent = `0x${a.ribo_hex}`;
  document.getElementById('matInvHex').textContent = `0x${a.ribo_inv_hex}`;
  renderMatrix64('matFwd', a.ribo_hex, '#22d3ee');
  renderMatrix64('matInv', a.ribo_inv_hex, '#a78bfa');

  document.getElementById('aPoly').textContent = `0x1${a.poly.toString(16).padStart(2, '0').toUpperCase()}`;
  const fieldEl = document.getElementById('aField');
  fieldEl.textContent = a.poly_irred ? 'IRREDUCIBLE GF(2⁸)' : 'REDUCIBLE RING';
  fieldEl.style.color = a.poly_irred ? '#10b981' : '#f43f5e';
  document.getElementById('aAddr').textContent = `0x${a.addr}`;
  document.getElementById('aAsm').textContent = `A=${a.asm} / L_eff=${a.leff}`;
  document.getElementById('aSex').textContent = `${a.offspring} / ${a.mates} / ${a.refusals}`;
  document.getElementById('aJitLen').textContent = `${a.jit_bytes} B`;
  document.getElementById('jitStats').textContent = `Compiles: ${f.compiles} | Cache Hits: ${f.cache_hits}`;
  document.getElementById('disasmPre').textContent = a.disasm;

  /* Evolutionary Counters */
  document.getElementById('eCopy').textContent = f.self_copies;
  document.getElementById('eSex').textContent = f.poly_sex;
  document.getElementById('eRef').textContent = f.mate_refusals;
  document.getElementById('eHgt').textContent = f.hgt;
  document.getElementById('eRetro').textContent = f.retro;
  document.getElementById('eRmut').textContent = f.ribo_rewrites;
  document.getElementById('eEcc').textContent = f.ecc_heals;
  document.getElementById('eMem').textContent = `${f.mem_blocks}/${f.mem_cracks}`;

  /* OVBS-52 & Ledger */
  document.getElementById('oviBadge').textContent = `OVI ${f.ovi.toFixed(1)} · Stage ${f.stage}`;
  document.getElementById('lDrift').textContent = f.ledger.rel_drift.toExponential(2);
  document.getElementById('lRes').textContent = `${f.ledger.int_res} uq`;
  document.getElementById('lSec').textContent = f.ledger.second_law_v;
  document.getElementById('lAlloc').textContent = f.post_warmup_allocs;
  document.getElementById('lGates').textContent = `${f.gamma.toFixed(2)} / ${f.theta.toFixed(2)}`;
  document.getElementById('lSci').textContent = `${f.bp_anew.toFixed(2)} / ${(f.regen_r50 * 100).toFixed(0)}% / ${f.sigma.toFixed(2)}`;
  document.getElementById('lMet').textContent = `${f.met} / 8`;

  /* Backlash Signatures */
  document.getElementById('bKs').textContent = f.backlash[0];
  document.getElementById('bInt').textContent = f.backlash[1];
  document.getElementById('bSpo').textContent = f.backlash[2];
  document.getElementById('bSc').textContent = f.backlash[3];

  /* Sync 13 sliders cleanly when presets/reset changed */
  f.restraints.forEach((v, idx) => {
    if (document.activeElement !== sliderEls[idx]) {
      sliderEls[idx].value = v;
      sliderVals[idx].textContent = `${v}%`;
    }
  });

  renderSoup(f);
  renderRadar(f.pillars);
  renderTraits112(f.traits112);
}

/* Connect live Server-Sent Events stream from native x86-64 autophage.exe */
const es = new EventSource('/api/events');
es.onmessage = (ev) => {
  const msg = JSON.parse(ev.data);
  if (msg.type === 'ready') {
    const hwEl = document.getElementById('hwBadges');
    hwEl.textContent = '';
    const badges = [
      ['cyan', String(msg.brand || 'x86-64')],
      ['emerald', `GFNI:${msg.gfni ? 'ON' : 'OFF'}`],
      ['emerald', `PCLMUL:${msg.pclmul ? 'ON' : 'OFF'}`],
      ['emerald', `BMI2:${msg.bmi2 ? 'ON' : 'OFF'}`],
      ['emerald', `RWX JIT:${msg.jit_rwx ? 'LIVE' : 'SIM'}`]
    ];
    for (const [cls, txt] of badges) {
      const sp = document.createElement('span');
      sp.className = `badge ${cls}`;
      sp.textContent = txt;
      hwEl.appendChild(sp);
    }
  } else if (msg.type === 'event') {
    const toast = document.getElementById('eventToast');
    if (msg.action === 'vivisect') {
      toast.textContent = `Planarian ${msg.pct}% excision on Slot #${msg.slot} -> Recovered ${(msg.fidelity * 100).toFixed(1)}% fidelity!`;
    } else if (msg.action === 'extinction') {
      toast.textContent = `Mass Extinction executed: ${msg.killed} organisms converted to conserved detritus!`;
    }
  } else if (msg.type === 'frame') {
    updateUI(msg);
  }
};
