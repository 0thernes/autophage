import http from 'node:http';
import fs from 'node:fs';
import path from 'node:path';
import { spawn } from 'node:child_process';
import { fileURLToPath } from 'node:url';

const __dirname = path.dirname(fileURLToPath(import.meta.url));
const PROJ_ROOT = path.resolve(__dirname, '..');
const BIN_PATH = process.platform === 'win32'
  ? path.join(PROJ_ROOT, 'bin', 'autophage.exe')
  : path.join(PROJ_ROOT, 'bin', 'autophage_linux');

const PORT = Number(process.env.AUTOPHAGE_PORT || 8787);

if (!fs.existsSync(BIN_PATH)) {
  console.error(`[AUTOPHAGE] Native binary not found at ${BIN_PATH}. Run build.ps1 first.`);
  process.exit(1);
}

console.log(`[AUTOPHAGE] Spawning live native x86-64 engine: ${BIN_PATH} stream --size 32 --seed 5000 --regime 1`);
const engine = spawn(BIN_PATH, ['stream', '--size', '32', '--seed', '5000', '--regime', '1'], {
  stdio: ['pipe', 'pipe', 'pipe']
});

let readyInfo = null;
let latestFrame = null;
let lastEvent = null;
let running = true;
let stepsPerTick = 2;
let inFlightStep = false;
let stdoutBuf = '';
const sseClients = new Set();

function parseFiniteInt(v, defVal, minVal, maxVal) {
  const n = Number(v);
  if (!Number.isFinite(n)) return defVal;
  return Math.max(minVal, Math.min(maxVal, Math.trunc(n)));
}

function broadcast(obj) {
  const payload = `data: ${JSON.stringify(obj)}\n\n`;
  for (const res of sseClients) {
    try { res.write(payload); } catch { sseClients.delete(res); }
  }
}

engine.stdout.on('data', (chunk) => {
  stdoutBuf += chunk.toString('utf8');
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
        inFlightStep = false;
        latestFrame = msg;
        broadcast(msg);
      }
    } catch (err) {
      console.error('[AUTOPHAGE] JSON parse err:', err.message);
    }
  }
});

engine.stderr.on('data', (d) => process.stderr.write(d));
engine.on('exit', (code) => {
  console.log(`[AUTOPHAGE] Engine exited with code ${code}`);
  process.exit(code ?? 0);
});

function sendEngineCmd(cmd) {
  if (engine.stdin.writable) {
    engine.stdin.write(cmd + '\n');
  }
}

/* Continuous bare-metal stepping loop with strict backpressure (never floods stdin queue!) */
const timer = setInterval(() => {
  if (running && !inFlightStep) {
    inFlightStep = true;
    sendEngineCmd(`STEP ${stepsPerTick}`);
  }
}, 220);

const ALLOWED_STATIC = new Set(['index.html', 'app.js', 'styles.css']);
const MIME = {
  '.html': 'text/html; charset=utf-8',
  '.js':   'application/javascript; charset=utf-8',
  '.css':  'text/css; charset=utf-8',
  '.json': 'application/json; charset=utf-8'
};

function isLoopbackRequest(req) {
  const host = String(req.headers.host || '');
  if (!/^(127\.0\.0\.1|localhost)(:\d+)?$/i.test(host)) return false;
  const origin = req.headers.origin;
  if (origin && !/^https?:\/\/(127\.0\.0\.1|localhost)(:\d+)?$/i.test(String(origin))) return false;
  return true;
}

const server = http.createServer((req, res) => {
  const url = new URL(req.url, `http://127.0.0.1:${PORT}`);

  if (req.method === 'GET' && url.pathname === '/api/events') {
    res.writeHead(200, {
      'Content-Type': 'text/event-stream',
      'Cache-Control': 'no-cache',
      'Connection': 'keep-alive'
    });
    sseClients.add(res);
    if (readyInfo) res.write(`data: ${JSON.stringify(readyInfo)}\n\n`);
    if (latestFrame) res.write(`data: ${JSON.stringify(latestFrame)}\n\n`);
    req.on('close', () => sseClients.delete(res));
    return;
  }

  if (req.method === 'GET' && url.pathname === '/api/state') {
    res.writeHead(200, { 'Content-Type': 'application/json' });
    res.end(JSON.stringify({ ready: readyInfo, frame: latestFrame, running, stepsPerTick, lastEvent }));
    return;
  }

  if (req.method === 'POST' && url.pathname === '/api/cmd') {
    if (!isLoopbackRequest(req)) {
      res.writeHead(403, { 'Content-Type': 'application/json' });
      res.end(JSON.stringify({ ok: false, error: 'Forbidden origin/host' }));
      return;
    }
    let body = '';
    let tooLarge = false;
    req.on('data', (c) => {
      if (tooLarge) return;
      body += c;
      if (body.length > 65536) {
        tooLarge = true;
        res.writeHead(413, { 'Content-Type': 'application/json' });
        res.end(JSON.stringify({ ok: false, error: 'Payload too large' }));
      }
    });
    req.on('end', () => {
      if (tooLarge) return;
      try {
        const msg = JSON.parse(body || '{}');
        switch (msg.action) {
          case 'play': running = true; break;
          case 'pause': running = false; break;
          case 'speed': stepsPerTick = parseFiniteInt(msg.steps, 2, 1, 50); break;
          case 'step': sendEngineCmd(`STEP ${parseFiniteInt(msg.n, 1, 1, 200)}`); break;
          case 'restraint':
            sendEngineCmd(`SET_RESTRAINT ${parseFiniteInt(msg.idx, 0, 0, 12)} ${parseFiniteInt(msg.val, 0, 0, 100)}`);
            break;
          case 'all_restraints':
            sendEngineCmd(`SET_ALL_RESTRAINTS ${parseFiniteInt(msg.val, 0, 0, 100)}`);
            break;
          case 'extinction':
            sendEngineCmd(`EXTINCTION ${parseFiniteInt(msg.pct, 90, 0, 100)}`);
            break;
          case 'vivisect':
            sendEngineCmd(`VIVISECT ${parseFiniteInt(msg.pct, 50, 0, 100)}`);
            break;
          case 'inoculate':
            sendEngineCmd(`INOCULATE ${parseFiniteInt(msg.arch, -1, -1, 16)}`);
            break;
          case 'reset':
            sendEngineCmd(`RESET ${parseFiniteInt(msg.regime, 1, 0, 3)} ${parseFiniteInt(msg.seed, 5000, 0, 2147483647)}`);
            break;
          default:
            break;
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
    res.writeHead(404);
    res.end('Not found');
    return;
  }
  const full = path.join(__dirname, rel);
  if (fs.existsSync(full) && fs.statSync(full).isFile()) {
    const ext = path.extname(full);
    res.writeHead(200, { 'Content-Type': MIME[ext] || 'application/octet-stream' });
    fs.createReadStream(full).pipe(res);
  } else {
    res.writeHead(404);
    res.end('Not found');
  }
});

server.listen(PORT, '127.0.0.1', () => {
  console.log(`[AUTOPHAGE] Silicon Observatory UI live at http://127.0.0.1:${PORT}`);
});

function shutdown() {
  clearInterval(timer);
  sendEngineCmd('QUIT');
  server.close(() => process.exit(0));
}
process.on('SIGINT', shutdown);
process.on('SIGTERM', shutdown);
