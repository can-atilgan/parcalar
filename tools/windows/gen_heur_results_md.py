# gen_heur_results_md.py HEUR_DIR EXACT_DIR GRAPHS_DIR TITLE
# Writes HEUR_DIR/SONUCLAR.md: the heuristic's Z/X per file, checked with tools/verify.py,
# next to the exact solver's result (EXACT_DIR/ham.tsv) where it exists.
import os, subprocess, sys
H, E, G, TITLE = sys.argv[1:5]
ROOT = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', '..'))
VERIFY = os.path.join(ROOT, 'tools', 'verify.py')

def tsv(p):
    rows = {}
    if os.path.exists(p):
        for l in open(p, encoding='utf-8').read().splitlines()[1:]:
            f = l.split('\t'); rows[f[0]] = f
    return rows

h, e = tsv(os.path.join(H, 'ham.tsv')), tsv(os.path.join(E, 'ham.tsv'))
files = sorted(h)
ok = {}
for f in files:
    r = subprocess.run([sys.executable, '-I', VERIFY, os.path.join(G, f + '.txt'), os.path.join(H, 'ciktilar', f + '.out')],
                       capture_output=True, text=True)
    ok[f] = r.returncode == 0

def cmp(f):
    z, x = int(h[f][1]), int(h[f][2])
    if f not in e: return 'kesin sonuç yok'
    ez, ex, est = int(e[f][1]), int(e[f][2]), e[f][3]
    if est == 'ispatli':
        if (z, x) == (ez, ex): return 'optimal'
        if z == ez: return 'Z optimal, X fazla'
        return 'Z fazla'
    if z < ez or (z == ez and x < ex): return 'kesin çözücüden iyi (o ispatsız)'
    if (z, x) == (ez, ex): return 'kesin çözücüyle aynı (o ispatsız)'
    return 'kesin çözücüden kötü (o ispatsız)'

c = {f: cmp(f) for f in files}
n_opt = sum(1 for f in files if c[f] == 'optimal')
n_proven = sum(1 for f in files if f in e and e[f][3] == 'ispatli')
L = [f'# ParçalaR sonuçları — {TITLE}\n',
     f'- **Doğrulama:** {sum(ok.values())}/{len(files)} çıktı `tools/verify.py` ile UYGUN.',
     f'- **Kesin çözücüyle karşılaştırma:** kesin çözücünün ispatlı olduğu {n_proven} dosyanın {n_opt} tanesinde '
     'sezgisel çözücü tam olarak optimal Z ve X\'i buldu.',
     '- Sezgisel çözücü optimal olduğunu **ispatlayamaz**; "optimal" etiketi, sonucun kesin çözücünün ispatlı sonucuyla aynı olduğunu söyler.\n',
     'Hücre: Z/X (en iyi çözüme ulaşma süresi, sn). `✓` = kesin çözücünün ispatlı optimumuyla aynı. '
     '`≠` = farklı. `?` = karşılaştırılacak ispatlı sonuç yok.\n',
     '| n | %20 | %30 | %40 | %50 | %60 | %70 | %80 | %90 |', '|' + '---|' * 9]
mark = {'optimal': ' ✓'}
for n in range(100, 201, 10):
    row = []
    for p in range(20, 100, 10):
        k = f'{n}{p}'
        if k in h:
            r = h[k]; m = mark.get(c[k], ' ?' if c[k].startswith('kesin') else ' ≠')
            row.append(f'{r[1]}/{r[2]} ({float(r[5]):.2f}){m}')
        else:
            row.append('—')
    L.append(f'| {n} | ' + ' | '.join(row) + ' |')
L.append('\n## Ayrıntı\n')
L.append('| dosya | Z | X | Y | en iyiye (sn) | doğrulama | kesin çözücü Z/X | karşılaştırma |')
L.append('|---|---|---|---|---|---|---|---|')
for f in files:
    r = h[f]; q = e.get(f)
    ex = f'{q[1]}/{q[2]}' + ('' if q[3] == 'ispatli' else ' (ispatsız)') if q else '—'
    L.append(f'| {f}.txt | {r[1]} | {r[2]} | {r[4]} | {r[5]} | {"uygun" if ok[f] else "UYGUN DEĞİL"} | {ex} | {c[f]} |')
L.append('\n`en iyiye (sn)` = sezgisel çözücünün en iyi çözümünü bulduğu an (işlemci süresi). '
         'Çıktılar: `ciktilar/<dosya>.out` (Z/X ve `S:` listesi), `ciktilar/<dosya>.err` (`-v` ayrıntısı). Ham ölçümler: `ham.tsv`.')
open(os.path.join(H, 'SONUCLAR.md'), 'w', encoding='utf-8').write('\n'.join(L) + '\n')
from collections import Counter
print(Counter(c.values()), 'verify ok', sum(ok.values()), '/', len(files))
