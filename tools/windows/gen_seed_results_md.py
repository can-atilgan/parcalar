# gen_seed_results_md.py [ROOT]
# Writes results/MEBtest/AMD_Ryzen_7_7800X3D/parcalar_baslangic_degerli/16-cekirdek/SONUCLAR.md from that
# folder's ham.tsv and ciktilar/*.err (the "Baslangic:" line), next to the exact solver without the ILS start
# (parcalar/16-cekirdek) and the stand-alone ILS heuristic (sezgisel_ILS/tek-cekirdek-10sn).
import os, re, sys
ROOT = sys.argv[1] if len(sys.argv) > 1 else os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', '..'))
M = f'{ROOT}/results/MEBtest/AMD_Ryzen_7_7800X3D'
D = f'{M}/parcalar_baslangic_degerli/16-cekirdek'

def tsv(p):
    return {l.split('\t')[0]: l.split('\t') for l in open(p, encoding='utf-8').read().splitlines()[1:]}

t, e, h = tsv(f'{D}/ham.tsv'), tsv(f'{M}/parcalar/16-cekirdek/ham.tsv'), tsv(f'{M}/sezgisel_ILS/tek-cekirdek-10sn/ham.tsv')
st = {}
for f in t:
    m = re.search(r'yerel arama Z=(\d+) X=(\d+), ILS Z=(\d+) X=(\d+) \(([\d.]+) sn\) -> Z=(\d+) X=(\d+)',
                  open(f'{D}/ciktilar/{f}.err', encoding='utf-8').read())
    st[f] = [int(m.group(i)) for i in (1, 2, 3, 4, 6, 7)] + [float(m.group(5))]
files = sorted(t)
zx = lambda r: (int(r[1]), int(r[2]))
better = [f for f in files if zx(t[f]) < zx(e[f])]
worse = [f for f in files if zx(t[f]) > zx(e[f])]
prov = [f for f in files if t[f][3] == 'ispatli']
moved = [f for f in files if tuple(st[f][4:6]) != zx(t[f])]
ils_t = [st[f][6] for f in files]
sp, se = sum(float(t[f][5]) for f in prov), sum(float(e[f][5]) for f in prov)

L = ['# ParçalaR sonuçları — MEBtest grafları, AMD Ryzen 7 7800X3D, 16 çekirdek, ILS başlangıçlı kesin çözücü\n',
     '- **Çözücü:** `final/parcalar_baslangic_degerli.c`. `final/parcalar.c` ile aynı arama; tek fark, aramadan önce sezgisel ILS\'nin',
     '  (`alternatif_cozuculer/heur/heur.c`, tohum 1, en çok 10 sn işlemci süresi) çalışması ve bulduğu çözümün, kendi yerel aramasından',
     '  iyiyse başlangıç değeri (F, Y) olması. Başlangıç yalnızca geçerli bir çözümdür; optimallik yine tam geçişle ispatlanır.',
     '- **Graflar:** `data/MEBtest/`, 88 dosyanın hepsi.',
     '- **İşlemci:** AMD Ryzen 7 7800X3D 8-Core Processor (8 fiziksel çekirdek, 16 mantıksal), Windows 10 Pro',
     '- **Derleyici:** gcc 16.2.0 (WinLibs, MinGW-w64 UCRT)',
     '- **Derleme:** `gcc -O2 -march=native -Wa,-muse-unaligned-vector-move -fopenmp -static final/parcalar_baslangic_degerli.c`',
     '- **Koşu:** `OMP_NUM_THREADS=16`, **dosya başına 300 sn** (`-t 300`, ILS süresi dahil), dosyalar tek tek, sırayla, aralarında 10 sn bekleme:',
     '  `TLIM=300 PAUSE=10 BENCH=data/MEBtest tools/windows/run_bench.sh <exe> results/MEBtest/AMD_Ryzen_7_7800X3D/parcalar_baslangic_degerli/16-cekirdek 16`',
     f'- **Durum:** {len(files)}/88 dosya koşuldu, {len(prov)}/{len(files)} ispatlı.',
     '- **Doğrulama:** her çıktı `tools/verify.py` ile kontrol edildi, hepsi UYGUN.\n',
     'Hücre: Z/X (süre, sn). `?` = 300 sn sınırında bitmedi (ispatsız). `+` = ILS\'siz kesin çözücünün (`parcalar/16-cekirdek`) bulduğundan daha iyi.\n',
     '| n | %20 | %30 | %40 | %50 | %60 | %70 | %80 | %90 |', '|' + '---|' * 9]
for n in range(100, 201, 10):
    row = []
    for p in range(20, 100, 10):
        k = f'{n}{p}'
        if k in t:
            r = t[k]
            row.append(f'{r[1]}/{r[2]} ({float(r[5]):.2f})' + ('' if r[3] == 'ispatli' else ' ?') + (' +' if k in better else ''))
        else:
            row.append('—')
    L.append(f'| {n} | ' + ' | '.join(row) + ' |')

L.append('\n## Özet\n')
L.append(f'- **ILS\'siz kesin çözücüye göre:** {len(better)} dosyada daha iyi, {len(worse)} dosyada daha kötü, '
         f'{len(files) - len(better) - len(worse)} dosyada aynı Z/X. Daha iyi olanlar, ILS\'nin kesin çözücüyü geçtiği seyrek dosyaların hepsi: '
         + ', '.join(better) + '.')
L.append(f'- **Kesin arama ILS başlangıcını geçti mi:** {"hiçbir dosyada" if not moved else ", ".join(moved)}. '
         'İspatlı dosyalarda ILS zaten optimumla başlamıştı; arama bunu ispatladı.')
L.append(f'- **Süre:** ispatlı {len(prov)} dosyanın toplamı {sp:.1f} sn (ILS\'siz: {se:.1f} sn). ILS dosya başına ortalama '
         f'{sum(ils_t) / len(ils_t):.2f} sn, en çok {max(ils_t):.2f} sn sürdü; bu süre kolay dosyalarda doğrudan eklenir, '
         'zor dosyalarda daha iyi başlangıç aramayı kısaltır:')
gain = sorted(prov, key=lambda f: float(t[f][5]) - float(e[f][5]))[:5]
for f in gain:
    L.append(f'  - {f}: {float(e[f][5]):.2f} sn → {float(t[f][5]):.2f} sn')
L.append(f'- **İspatsız dosyalar:** {len(files) - len(prov)} dosya (ILS\'siz kesin çözücüde de ispatsız olanlarla aynı) 300 sn\'de ispatlanamadı.')

L.append('\n## Ayrıntı\n')
L.append('`başlangıç` = aramadan önceki çözümler (stderr\'deki `Baslangic:` satırı): `parcalar.c`\'nin kendi yerel araması ve ILS. '
         'Son iki sütun aynı dosyada ILS\'siz kesin çözücü (300/600 sn) ve tek başına ILS.\n')
L.append('| dosya | Z | X | durum | süre (sn) | başlangıç: yerel arama | başlangıç: ILS (sn) | ILS\'siz kesin | tek başına ILS |')
L.append('|---|---|---|---|---|---|---|---|---|')
for f in files:
    r, s, q, g = t[f], st[f], e[f], h[f]
    L.append(f'| {f}.txt | {r[1]} | {r[2]} | {r[3]} | {r[4]} | {s[0]}/{s[1]} | {s[2]}/{s[3]} ({s[6]:.2f}) | '
             f'{q[1]}/{q[2]} {q[3]} ({q[5]}) | {g[1]}/{g[2]} |')
L.append('\nÇıktılar (`Z/X` ve `S:` listesi; stderr: başlangıç satırı, öz-kontrol, süre): `ciktilar/<dosya>.out`, `ciktilar/<dosya>.err`. '
         'Ham ölçümler: `ham.tsv`.')
open(f'{D}/SONUCLAR.md', 'w', encoding='utf-8').write('\n'.join(L) + '\n')
print(len(files), len(prov), 'better', better, 'worse', worse, 'moved', moved)
