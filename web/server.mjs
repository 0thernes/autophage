import http from 'node:http';
import fs from 'node:fs';
import path from 'node:path';
import { spawn } from 'node:child_process';
import { fileURLToPath } from 'node:url';
import { StringDecoder } from 'node:string_decoder';

const __dirname = path.dirname(fileURLToPath(import.meta.url));
const PROJ_ROOT = path.resolve(__dirname, '..');
const BIN_PATH = process.platform === 'win32'
  ? path.join(PROJ_ROOT, 'bin', 'autophage.exe')
  : path.join(PROJ_ROOT, 'bin', 'autophage_linux');

const PORT = Number(process.env.AUTOPHAGE_PORT || 8787);
const MAX_SSE_CLIENTS = 16;
const MAX_CMD_BODY_BYTES = 65536;
const MAX_UINT64 = 18446744073709551615n;

if (!fs.existsSync(BIN_PATH)) {
  console.error(`[AUTOPHAGE] Native binary not found at ${BIN_PATH}. Run build.ps1 first.`);
  process.exit(1);
}

console.log(`[AUTOPHAGE v1.2.0] Spawning live native x86-64 engine: ${BIN_PATH} stream --size 32 --seed 5000 --regime 1`);
const engine = spawn(BIN_PATH, ['stream', '--size', '32', '--seed', '5000', '--regime', '1'], {
  stdio: ['pipe', 'pipe', 'pipe']
});

let readyInfo = null;
let latestFrame = null;
let lastEvent = null;
let running = true;
let stepsPerTick = 2;
let inFlightFrames = 1; /* Initial startup emits one baseline frame */
let shuttingDown = false;
let stdoutBuf = '';
const stdoutDecoder = new StringDecoder('utf8');
const sseClients = new Set();

function parseFiniteInt(v, defVal, minVal, maxVal) {
  const n = Number(v);
  if (!Number.isFinite(n)) return defVal;
  return Math.max(minVal, Math.min(maxVal, Math.trunc(n)));
}

/* Full unsigned 64-bit seed validator (accepts numeric string, BigInt-range decimal, or safe integer) */
function parseUnsignedSeed64(v, defStr = '5000') {
  if (typeof v === 'bigint') {
    if (v >= 0n && v <= MAX_UINT64) return v.toString();
    return defStr;
  }
  if (typeof v === 'number') {
    if (Number.isFinite(v) && v >= 0 && v <= Number.MAX_SAFE_INTEGER) {
      return Math.trunc(v).toString();
    }
    return defStr;
  }
  if (typeof v === 'string') {
    const trimmed = v.trim();
    if (/^\d{1,20}$/.test(trimmed)) {
      try {
        const b = BigInt(trimmed);
        if (b >= 0n && b <= MAX_UINT64) return b.toString();
      } catch {
        return defStr;
      }
    }
  }
  return defStr;
}

function broadcast(obj) {
  const payload = `data: ${JSON.stringify(obj)}\n\n`;
  for (const res of sseClients) {
    try {
      if (!res.writableEnded && !res.destroyed) {
        res.write(payload);
      } else {
        sseClients.delete(res);
      }
    } catch {
      sseClients.delete(res);
    }
  }
}

engine.stdout.on('data', (chunk) => {
  stdoutBuf += stdoutDecoder.write(chunk);
  let nl;
  while ((nl = stdoutBuf.indexOf('\n')) !== -1) {
    const line = stdoutBuf.slice(0, nl).trim();
    stdoutBuf = stdoutBuf.slice(nl + 1);
    if (!line) continue;
    try {
      const msg = JSON.parse(line);
      if (msg.type === 'ready') {
        readyInfo = msg;
        broadcast({ type: 'ready', ...msg });
      } else if (msg.type === 'event') {
        lastEvent = msg;
        broadcast(msg);
      } else if (msg.type === 'frame') {
        if (inFlightFrames > 0) inFlightFrames--;
        latestFrame = msg;
        broadcast(msg);
      }
    } catch (err) {
      console.error('[AUTOPHAGE] JSON parse err:', err.message);
    }
  }
});

engine.stdout.on('error', (err) => {
  console.error('[AUTOPHAGE] stdout stream error:', err.message);
});
engine.stdin.on('error', (err) => {
  if (!shuttingDown) {
    console.error('[AUTOPHAGE] stdin stream error:', err.message);
  }
});
engine.stderr.on('data', (d) => process.stderr.write(d));
engine.stderr.on('error', (err) => {
  console.error('[AUTOPHAGE] stderr stream error:', err.message);
});
engine.on('error', (err) => {
  console.error('[AUTOPHAGE] Engine spawn/process error:', err.message);
  if (!shuttingDown) shutdown(1);
});
engine.on('exit', (code) => {
  console.log(`[AUTOPHAGE] Engine exited with code ${code}`);
  if (!shuttingDown) {
    closeAllSse();
    process.exit(code ?? 0);
  }
});

function sendEngineCmd(cmd, expectsFrame = true) {
  if (shuttingDown || !engine.stdin.writable || engine.stdin.destroyed) return false;
  if (expectsFrame) inFlightFrames++;
  try {
    engine.stdin.write(cmd + '\n');
    return true;
  } catch (err) {
    if (expectsFrame && inFlightFrames > 0) inFlightFrames--;
    console.error('[AUTOPHAGE] stdin write error:', err.message);
    return false;
  }
}

/* Continuous bare-metal stepping loop with exact frame-count backpressure (zero queue race!) */
const timer = setInterval(() => {
  if (running && inFlightFrames === 0 && !shuttingDown) {
    sendEngineCmd(`STEP ${stepsPerTick}`, true);
  }
}, 220);

const ALLOWED_STATIC = new Set(['index.html', 'app.js', 'styles.css']);
const MIME = {
  '.html': 'text/html; charset=utf-8',
  '.js':   'application/javascript; charset=utf-8',
  '.css':  'text/css; charset=utf-8',
  '.json': 'application/json; charset=utf-8'
};

function isLoopbackHost(req) {
  const host = String(req.headers.host || '');
  return /^(127\.0\.0\.1|localhost|\[::1\])(:\d+)?$/i.test(host);
}

function isAuthorizedPost(req) {
  if (!isLoopbackHost(req)) return { ok: false, code: 403, error: 'Forbidden host' };
  const origin = req.headers.origin;
  if (origin && !/^https?:\/\/(127\.0\.0\.1|localhost|\[::1\])(:\d+)?$/i.test(String(origin))) {
    return { ok: false, code: 403, error: 'Forbidden origin' };
  }
  const sfs = req.headers['sec-fetch-site'];
  if (sfs && sfs !== 'same-origin' && sfs !== 'none') {
    return { ok: false, code: 403, error: 'Forbidden Sec-Fetch-Site' };
  }
  const ct = String(req.headers['content-type'] || '').toLowerCase();
  if (!ct.startsWith('application/json')) {
    return { ok: false, code: 415, error: 'Content-Type must be application/json' };
  }
  return { ok: true };
}

function closeAllSse() {
  for (const res of sseClients) {
    try { res.end(); } catch { /* ignore */ }
  }
  sseClients.clear();
}

const server = http.createServer((req, res) => {
  let url;
  try {
    url = new URL(req.url || '/', `http://127.0.0.1:${PORT}`);
  } catch {
    res.writeHead(400, { 'Content-Type': 'application/json' });
    res.end(JSON.stringify({ ok: false, error: 'Malformed request URL' }));
    return;
  }

  if (!isLoopbackHost(req)) {
    res.writeHead(403, { 'Content-Type': 'application/json' });
    res.end(JSON.stringify({ ok: false, error: 'Loopback Host required' }));
    return;
  }

  if (req.method === 'GET' && url.pathname === '/api/events') {
    if (sseClients.size >= MAX_SSE_CLIENTS) {
      res.writeHead(429, { 'Content-Type': 'application/json', 'Retry-After': '5' });
      res.end(JSON.stringify({ ok: false, error: `SSE client cap (${MAX_SSE_CLIENTS}) reached` }));
      return;
    }
    res.writeHead(200, {
      'Content-Type': 'text/event-stream',
      'Cache-Control': 'no-cache',
      'Connection': 'keep-alive',
      'X-Content-Type-Options': 'nosniff'
    });
    sseClients.add(res);
    if (readyInfo) res.write(`data: ${JSON.stringify(readyInfo)}\n\n`);
    if (latestFrame) res.write(`data: ${JSON.stringify(latestFrame)}\n\n`);
    req.on('close', () => sseClients.delete(res));
    res.on('error', () => sseClients.delete(res));
    return;
  }

  if (req.method === 'GET' && url.pathname === '/api/state') {
    res.writeHead(200, { 'Content-Type': 'application/json', 'Cache-Control': 'no-store' });
    res.end(JSON.stringify({
      version: '1.2.0',
      ready: readyInfo,
      frame: latestFrame,
      running,
      stepsPerTick,
      lastEvent
    }));
    return;
  }

  if (req.method === 'POST' && url.pathname === '/api/cmd') {
    const auth = isAuthorizedPost(req);
    if (!auth.ok) {
      res.writeHead(auth.code, { 'Content-Type': 'application/json' });
      res.end(JSON.stringify({ ok: false, error: auth.error }));
      return;
    }
    const chunks = [];
    let totalBytes = 0;
    let tooLarge = false;
    req.on('data', (c) => {
      if (tooLarge) return;
      totalBytes += c.length;
      if (totalBytes > MAX_CMD_BODY_BYTES) {
        tooLarge = true;
        res.writeHead(413, { 'Content-Type': 'application/json' });
        res.end(JSON.stringify({ ok: false, error: 'Payload too large' }));
        req.resume();
        return;
      }
      chunks.push(c);
    });
    req.on('error', () => {
      if (!res.headersSent) {
        res.writeHead(400, { 'Content-Type': 'application/json' });
        res.end(JSON.stringify({ ok: false, error: 'Request stream error' }));
      }
    });
    req.on('end', () => {
      if (tooLarge || res.writableEnded) return;
      try {
        const raw = Buffer.concat(chunks).toString('utf8');
        const msg = JSON.parse(raw || '{}');
        if (typeof msg !== 'object' || msg === null || Array.isArray(msg)) {
          res.writeHead(400, { 'Content-Type': 'application/json' });
          res.end(JSON.stringify({ ok: false, error: 'JSON object body required' }));
          return;
        }
        switch (msg.action) {
          case 'play':
            running = true;
            break;
          case 'pause':
            running = false;
            break;
          case 'speed':
            stepsPerTick = parseFiniteInt(msg.steps, 2, 1, 50);
            break;
          case 'step':
            sendEngineCmd(`STEP ${parseFiniteInt(msg.n, 1, 1, 200)}`, true);
            break;
          case 'restraint':
            sendEngineCmd(`SET_RESTRAINT ${parseFiniteInt(msg.idx, 0, 0, 12)} ${parseFiniteInt(msg.val, 0, 0, 100)}`, true);
            break;
          case 'restraint_vec': {
            if (!Array.isArray(msg.vec) || msg.vec.length !== 13) {
              res.writeHead(400, { 'Content-Type': 'application/json' });
              res.end(JSON.stringify({ ok: false, error: 'restraint_vec requires array of 13 integers (0..100)' }));
              return;
            }
            const clamped = msg.vec.map((x) => parseFiniteInt(x, 0, 0, 100));
            sendEngineCmd(`SET_RESTRAINT_VEC ${clamped.join(' ')}`, true);
            break;
          }
          case 'all_restraints':
            sendEngineCmd(`SET_ALL_RESTRAINTS ${parseFiniteInt(msg.val, 0, 0, 100)}`, true);
            break;
          case 'extinction':
            sendEngineCmd(`EXTINCTION ${parseFiniteInt(msg.pct, 90, 0, 100)}`, true);
            break;
          case 'vivisect':
            sendEngineCmd(`VIVISECT ${parseFiniteInt(msg.pct, 50, 0, 100)}`, true);
            break;
          case 'inoculate':
            sendEngineCmd(`INOCULATE ${parseFiniteInt(msg.arch, -1, -1, 16)}`, true);
            break;
          case 'reset': {
            const reg = parseFiniteInt(msg.regime, 1, 0, 3);
            const seedStr = parseUnsignedSeed64(msg.seed, '5000');
            sendEngineCmd(`RESET ${reg} ${seedStr}`, true);
            break;
          }
          default:
            res.writeHead(400, { 'Content-Type': 'application/json' });
            res.end(JSON.stringify({ ok: false, error: `Unknown action: ${String(msg.action ?? '')}` }));
            return;
        }
        res.writeHead(200, { 'Content-Type': 'application/json' });
        res.end(JSON.stringify({ ok: true, running, stepsPerTick }));
      } catch (e) {
        res.writeHead(400, { 'Content-Type': 'application/json' });
        res.end(JSON.stringify({ ok: false, error: e.message }));
      }
    });
    return;
  }

  /* Serve ONLY explicitly allowlisted static UI files (blocks GET /server.mjs source exposure!) */
  const rel = url.pathname === '/' ? 'index.html' : url.pathname.replace(/^\/+/, '');
  if (!ALLOWED_STATIC.has(rel)) {
    res.writeHead(404, { 'Content-Type': 'text/plain; charset=utf-8' });
    res.end('Not found');
    return;
  }
  const full = path.join(__dirname, rel);
  if (fs.existsSync(full) && fs.statSync(full).isFile()) {
    const ext = path.extname(full);
    res.writeHead(200, {
      'Content-Type': MIME[ext] || 'application/octet-stream',
      'X-Content-Type-Options': 'nosniff'
    });
    const stream = fs.createReadStream(full);
    stream.on('error', (err) => {
      console.error(`[AUTOPHAGE] Static read error for ${rel}:`, err.message);
      if (!res.headersSent) {
        res.writeHead(500, { 'Content-Type': 'text/plain; charset=utf-8' });
      }
      if (!res.writableEnded) res.end('Internal read error');
    });
    stream.pipe(res);
  } else {
    res.writeHead(404, { 'Content-Type': 'text/plain; charset=utf-8' });
    res.end('Not found');
  }
});

server.listen(PORT, '127.0.0.1', () => {
  console.log(`[AUTOPHAGE v1.2.0] Silicon Observatory UI live at http://127.0.0.1:${PORT}`);
});

function shutdown(exitCode = 0) {
  if (shuttingDown) return;
  shuttingDown = true;
  clearInterval(timer);
  closeAllSse();
  try {
    if (engine.stdin.writable && !engine.stdin.destroyed) {
      engine.stdin.write('QUIT\n');
      engine.stdin.end();
    }
  } catch { /* ignore */ }
  const forceExit = setTimeout(() => {
    try { engine.kill(); } catch { /* ignore */ }
    process.exit(exitCode);
  }, 1500);
  forceExit.unref();
  server.close(() => process.exit(exitCode));
}

process.on('SIGINT', () => shutdown(0));
process.on('SIGTERM', () => shutdown(0));
