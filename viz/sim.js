// Simplified trace of the ParcalaR exact search on the 7-node example (small/example.txt).
// Produces serial and parallel event lists for the animation page.
const N = 7;
const EDGES = [[1,2],[1,6],[1,7],[2,6],[2,7],[3,4],[3,5],[4,5],[5,7]];
const adj = Array.from({length: N + 1}, () => new Set());
for (const [a, b] of EDGES) { adj[a].add(b); adj[b].add(a); }
const V = [1,2,3,4,5,6,7];

function comps(S) {               // components of G[S]
  const left = new Set(S), out = [];
  while (left.size) {
    const s = left.values().next().value, c = [s]; left.delete(s);
    for (let i = 0; i < c.length; i++) for (const w of adj[c[i]]) if (left.has(w)) { left.delete(w); c.push(w); }
    out.push(c);
  }
  return out;
}
const maxcomp = S => comps(S).reduce((m, c) => Math.max(m, c.length), 0);
function closedN(S) { const r = new Set(S); for (const v of S) for (const w of adj[v]) r.add(w); return r; }
const region = A => { const cn = closedN(A); return V.filter(v => !cn.has(v)); };
const Lof = R => maxcomp(region(R));   // largest component of G - N[R]
function destroyed(R) {           // S = V \ (R + largest component of G - N[R])
  const cs = comps(region(R)); let L = [];
  for (const c of cs) if (c.length > L.length) L = c;
  const K = new Set([...R, ...L]); return V.filter(v => !K.has(v));
}

// best R in P with every component <= k, maximising (|R|, L(R))
function bestR(P, k) {
  let best = null;
  for (let m = 0; m < (1 << P.length); m++) {
    const R = P.filter((_, i) => m >> i & 1);
    if (maxcomp(R) > k) continue;
    const f = R.length, y = Lof(R);
    if (!best || f > best.f || (f === best.f && y > best.y)) best = {R, f, y};
  }
  return best;
}

// independent-set start: F = alpha - 1, Y = best L(I \ {v})
function misStart() {
  let I = [];
  for (let m = 0; m < 128; m++) {
    const S = V.filter((_, i) => m >> i & 1);
    if (S.every(a => S.every(b => a === b || !adj[a].has(b))) && S.length > I.length) I = S;
  }
  let Y = 1, R = I.slice(1);
  for (const v of I) { const r = I.filter(x => x !== v), y = Lof(r); if (y > Y) { Y = y; R = r; } }
  return {F: I.length - 1, Y, I, R};
}

// run one anchor with local (F, Y); returns events and whether (F, Y) improved
function runAnchor(c, st, nodeId, parentId) {
  const ev = [];
  let improved = false;
  function dfs(A, Fr, Allow, parent) {
    const id = nodeId(); const P = region(A);
    ev.push({t: 'visit', id, parent, A: A.slice(), P, line: A.length === 1 ? 7 : 7});
    ev.push({t: 'region', id, A: A.slice(), P, line: 8});
    if (P.length < st.F) { ev.push({t: 'prune', id, A: A.slice(), P, line: 9, F: st.F}); return false; }
    if (A.length >= 2) {
      const b = bestR(P, A.length);
      ev.push({t: 'check', id, A: A.slice(), P, R: b.R, f: b.f, y: b.y, line: 10});
      if (b.f > st.F || (b.f === st.F && b.y > st.Y)) {
        st.F = b.f; st.Y = b.y; st.R = b.R;
        ev.push({t: 'improve', id, A: A.slice(), P, R: b.R, F: st.F, Y: st.Y, line: 12});
        ev.push({t: 'restart', id, line: 13});
        improved = true; return true;
      }
    }
    Fr = Fr.slice(); Allow = Allow.slice();
    while (Fr.length) {
      const u = Fr[0], A2 = [...A, u].sort((a, b) => a - b);
      const Fr2 = [...new Set([...Fr, ...[...adj[u]].filter(w => Allow.includes(w))])].filter(w => !A2.includes(w)).sort((a, b) => a - b);
      if (dfs(A2, Fr2, Allow, id)) return true;
      Fr = Fr.filter(w => w !== u); Allow = Allow.filter(w => w !== u);
    }
    return false;
  }
  const above = V.filter(v => v > c);
  dfs([c], [...adj[c]].filter(w => w > c).sort((a, b) => a - b), above, parentId);
  return {ev, improved};
}

function serial() {
  const s0 = misStart(), st = {F: s0.F, Y: s0.Y, R: s0.R}, ev = [];
  let nid = 0; const nodeId = () => nid++;
  ev.push({t: 'start', F: st.F, Y: st.Y, I: s0.I, line: 2});
  for (let pass = 1; ; pass++) {
    const root = nodeId();
    ev.push({t: 'pass', pass, id: root, F: st.F, Y: st.Y, line: 4});
    let changed = false;
    for (let c = 1; c <= N; c++) {
      for (;;) {
        ev.push({t: 'anchor', c, line: 6});
        const r = runAnchor(c, st, nodeId, root);
        ev.push(...r.ev);
        if (!r.improved) break;
        changed = true;
      }
    }
    ev.push({t: 'passend', pass, changed, F: st.F, Y: st.Y, line: 14});
    if (!changed) break;
  }
  ev.push({t: 'done', F: st.F, Y: st.Y, Z: N - st.F, X: N - st.F - st.Y, S: destroyed(st.R), line: 15});
  return ev;
}

// parallel: T workers, dynamic schedule, thread-local (F,Y) copied at pass start
function parallel(T) {
  const s0 = misStart(), g = {F: s0.F, Y: s0.Y, R: s0.R};
  const steps = [];   // each step: list of events happening at the same tick
  let nid = 0; const nodeId = () => nid++;
  steps.push([{t: 'start', F: g.F, Y: g.Y, I: s0.I, line: 2}]);
  for (let pass = 1; ; pass++) {
    const root = nodeId();
    steps.push([{t: 'pass', pass, id: root, F: g.F, Y: g.Y, line: 4}]);
    let changed = false;
    const local = Array.from({length: T}, () => ({F: g.F, Y: g.Y, R: g.R}));
    const queues = Array.from({length: T}, () => []);   // per worker event streams
    const free = Array(T).fill(0);
    for (let c = 1; c <= N; c++) {
      let w = 0; for (let i = 1; i < T; i++) if (free[i] < free[w]) w = i;   // dynamic: earliest free worker
      while (queues[w].length < free[w]) queues[w].push(null);
      for (;;) {
        queues[w].push({t: 'anchor', c, line: 6, w});
        const r = runAnchor(c, local[w], nodeId, root);
        for (const e of r.ev) {
          e.w = w;
          if (e.t === 'improve') {
            if (local[w].F > g.F || (local[w].F === g.F && local[w].Y > g.Y)) { g.F = local[w].F; g.Y = local[w].Y; g.R = local[w].R; }
            changed = true;
          }
          queues[w].push(e);
        }
        if (!r.improved) break;
      }
      free[w] = queues[w].length;
    }
    const len = Math.max(...queues.map(q => q.length));
    for (let i = 0; i < len; i++) {
      const s = []; for (let w = 0; w < T; w++) if (queues[w][i]) s.push(queues[w][i]);
      if (s.length) steps.push(s);
    }
    steps.push([{t: 'barrier', pass, line: 14}]);
    steps.push([{t: 'passend', pass, changed, F: g.F, Y: g.Y, line: 14}]);
    if (!changed) break;
  }
  steps.push([{t: 'done', F: g.F, Y: g.Y, Z: N - g.F, X: N - g.F - g.Y, S: destroyed(g.R), line: 15}]);
  return steps;
}

if (typeof module !== 'undefined' && require.main === module) {
  const s = serial();
  const counts = {}; for (const e of s) counts[e.t] = (counts[e.t] || 0) + 1;
  console.log('serial events', s.length, JSON.stringify(counts));
  for (const e of s) if (['start','pass','improve','passend','done'].includes(e.t)) console.log(JSON.stringify(e));
  const p = parallel(3);
  console.log('parallel steps', p.length);
  for (const st of p) for (const e of st) if (['improve','passend','done'].includes(e.t)) console.log('P', JSON.stringify(e));
}
if (typeof module !== 'undefined') module.exports = {serial, parallel, EDGES, N};
