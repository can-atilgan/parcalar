# gen_meb_results_md.py [ROOT]
# Writes results/MEBtest/AMD_Ryzen_7_7800X3D/parcalar/16-cekirdek/SONUCLAR.md from that folder's ham.tsv,
# with the same-named data/bench results next to it.
import os, sys
ROOT = sys.argv[1] if len(sys.argv) > 1 else os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', '..'))
D = f'{ROOT}/results/MEBtest/AMD_Ryzen_7_7800X3D/parcalar/16-cekirdek'
t = {}
for l in open(f'{D}/ham.tsv', encoding='utf-8').read().splitlines()[1:]:
    p = l.split('\t'); t[p[0]] = p
b = {}
for l in open(f'{ROOT}/results/bench/AMD_Ryzen_7_7800X3D/parcalar/16-cekirdek/ham.tsv', encoding='utf-8').read().splitlines()[1:]:
    p = l.split('\t'); b[p[0]] = p
files = sorted(f[:-4] for f in os.listdir(f'{ROOT}/data/MEBtest') if f.endswith('.txt'))
done = [f for f in files if f in t]; skip = [f for f in files if f not in t]
prov = sum(1 for f in done if t[f][3] == 'ispatli')
L = ['# ParçalaR sonuçları — MEBtest grafları, AMD Ryzen 7 7800X3D, 16 çekirdek\n',
     "- **Graflar:** `data/MEBtest/` (hocanın test grafları, 2014). Aynı adlı `data/bench/` graflarından farklıdır ve iç içedir:",
     '  her yoğunluk, bir altındakinin bütün kenarlarını içerir.',
     '- **Kapsam:** 88 dosyanın hepsi.',
     '- **İşlemci:** AMD Ryzen 7 7800X3D 8-Core Processor (8 fiziksel çekirdek, 16 mantıksal), Windows 10 Pro',
     '- **Derleyici:** gcc 16.2.0 (WinLibs, MinGW-w64 UCRT)',
     '- **Derleme:** `gcc -O2 -march=native -Wa,-muse-unaligned-vector-move -fopenmp -static final/parcalar.c`',
     "- **Koşu:** `OMP_NUM_THREADS=16`, dosyalar tek tek, sırayla. İspatlanan 74 dosya `-t 600` ile koşuldu (en uzunu 240 sn sürdü):",
     "  `BENCH=data/MEBtest SKIP='^(1[2-9]|20)020$|^(1[6-9]|20)030$' tools/windows/run_bench.sh <exe> results/MEBtest/AMD_Ryzen_7_7800X3D/parcalar/16-cekirdek 16`",
     "  En seyrek 14 dosya (12020–20020 ve 16030–20030) ardından `-t 300` ile, aralarında işlemci soğusun diye 10 sn beklenerek koşuldu:",
     "  `TLIM=300 PAUSE=10 BENCH=data/MEBtest tools/windows/run_bench.sh <exe> results/MEBtest/AMD_Ryzen_7_7800X3D/parcalar/16-cekirdek 16`",
     f'- **Durum:** {len(done)}/{len(files)} dosya koşuldu, {prov}/{len(done)} ispatlı.',
     '- **Doğrulama:** her çıktı `tools/verify.py` ile kontrol edildi, hepsi UYGUN (|S| = X ve X + Y = Z).\n',
     'Hücre: Z/X (süre, sn). `—` = bu turda koşulmadı. `?` = süre sınırında bitmedi (ispatsız).\n',
     '| n | %20 | %30 | %40 | %50 | %60 | %70 | %80 | %90 |', '|' + '---|' * 9]
for n in range(100, 201, 10):
    row = []
    for p in range(20, 100, 10):
        k = f'{n}{p}'
        if k in t:
            r = t[k]; row.append(f'{r[1]}/{r[2]} ({float(r[5]):.2f})' + ('' if r[3] == 'ispatli' else ' ?'))
        else:
            row.append('—')
    L.append(f'| {n} | ' + ' | '.join(row) + ' |')
L.append('\n## Ayrıntı ve `data/bench/` ile karşılaştırma\n')
L.append('`süre` = programın ölçtüğü süre (dosya okuma + arama + çıktı yazma + öz-kontrol). Son iki sütun aynı adlı `data/bench/` grafının sonucu (farklı graf).\n')
L.append('| dosya | Z | X | durum | süre (sn) | bench Z/X | bench süre (sn) |'); L.append('|---|---|---|---|---|---|---|')
for f in done:
    r = t[f]; q = b.get(f)
    L.append(f'| {f}.txt | {r[1]} | {r[2]} | {r[3]} | {r[4]} | ' + (f'{q[1]}/{q[2]} | {q[5]}' if q else '— | —') + ' |')
L.append(f'\nToplam süre (koşulan {len(done)} dosya): {sum(float(t[f][5]) for f in done):.1f} sn '
         f'(`data/bench/` aynı dosyalar: {sum(float(b[f][5]) for f in done if f in b):.1f} sn).')
L.append('\nÇıktılar (`Z/X` ve `S:` listesi, stderr öz-kontrol): `ciktilar/<dosya>.out`, `ciktilar/<dosya>.err`. Ham ölçümler: `ham.tsv`.')
L.append('\n## Notlar\n')
L.append('- **10050:** Z = 91, X = 85. Dört bağımsız kesin çözücü (`alternatif_cozuculer/bnb`, `bnb_b`, `rds`, `rds_b`) ve sezgisel '
         'çözücü (`heur`, 3 farklı başlangıç) aynı sonucu verdi. Notlarda ödevin bilinen değeri olarak geçen "(10/5)" (Z = 90) '
         '`data/bench/10050` ile tutuyor, MEBtest ile değil.')
ILS = f'{ROOT}/results/MEBtest/AMD_Ryzen_7_7800X3D/sezgisel_ILS/tek-cekirdek-10sn/ham.tsv'
h = {l.split('\t')[0]: l.split('\t') for l in open(ILS, encoding='utf-8').read().splitlines()[1:]}
L.append('- **İspatlanamayan 14 dosya (300 sn):** yazılan çözümler geçerli, ama daha iyisinin olmadığı gösterilemedi. '
         'Sezgisel ILS (`results/MEBtest/AMD_Ryzen_7_7800X3D/sezgisel_ILS/tek-cekirdek-10sn`, her çözüme 1.05 sn içinde ulaştı) ile karşılaştırma:\n')
L.append('  | dosya | kesin çözücü (300 sn) | sezgisel ILS | |'); L.append('  |---|---|---|---|')
for f in done:
    if t[f][3] == 'ispatli': continue
    ez, ex, hz, hx = int(t[f][1]), int(t[f][2]), int(h[f][1]), int(h[f][2])
    c = 'aynı' if (ez, ex) == (hz, hx) else ("ILS'nin Z'si daha küçük" if hz < ez else ("ILS'nin X'i daha küçük" if hz == ez and hx < ex else 'kesin çözücü daha iyi'))
    L.append(f'  | {f} | {ez}/{ex} | {hz}/{hx} | {c} |')
L.append('')
L.append('  Bu dosyalar önce 150 ve 600 sn sınırlarıyla koşulmuştu; 300 sn ile yeniden koşuldu. 150 sn ile 300 sn arasında yalnızca '
         '17030 (151/141 → 150/144) ve 19030 (170/164 → 170/163) değişti.')
L.append('- **15050:** ödevin bilinen değeri "(11/4)" ile tutuyor (Z = 139, X = 135).')
L.append('- **12080 için başka bir programın cevabı:** `Z=115 X=114` (Y = 1, 6 bağımsız bilgisayar kalıyor) 12020–12080 '
         'graflarında geçerli, 12090\'da geçersiz. Z = 115 yalnızca 12080\'in optimumuyla tutuyor; ancak aynı Z, 111 bilgisayar '
         'parçalanarak da elde ediliyor (Y = 4). Ödevin eşitlik kuralı en az bilgisayar parçalayan çözümü istediği için doğru '
         'cevap **Z = 115, X = 111**.')
open(f'{D}/SONUCLAR.md', 'w', encoding='utf-8').write('\n'.join(L) + '\n')
print(len(done), prov, len(skip))
