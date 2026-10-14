# gen_results_md.py [MODE]  (MODE = 16-cekirdek | tek-cekirdek)
# Writes results/AMD_Ryzen_7_7800X3D/<MODE>/SONUCLAR.md from that folder's ham.tsv.
import os, sys
MODE = sys.argv[1] if len(sys.argv) > 1 else '16-cekirdek'
ROOT = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', '..'))
D = f'{ROOT}/results/AMD_Ryzen_7_7800X3D/{MODE}'
t = {}
for l in open(f'{D}/ham.tsv', encoding='utf-8').read().splitlines()[1:]:
    p = l.split('\t'); t[p[0]] = p
files = sorted(f[:-4] for f in os.listdir(f'{ROOT}/bench') if f.endswith('.txt'))
done = [f for f in files if f in t]; skip = [f for f in files if f not in t]
prov = sum(1 for f in done if t[f][3] == 'ispatli')
if MODE == '16-cekirdek':
    title = '16 çekirdek (16 iş parçacığı)'
    build = '`gcc -O2 -march=native -Wa,-muse-unaligned-vector-move -fopenmp -static final/parcalar.c`'
    run = '`OMP_NUM_THREADS=16`'
else:
    title = 'tek çekirdek'
    build = '`gcc -O2 -march=native -Wa,-muse-unaligned-vector-move -static final/parcalar.c` (OpenMP yok)'
    run = 'tek iş parçacığı'
L = [f'# ParçalaR sonuçları — AMD Ryzen 7 7800X3D, {title}\n',
     '- **İşlemci:** AMD Ryzen 7 7800X3D 8-Core Processor (8 fiziksel çekirdek, 16 mantıksal), Windows 10 Pro',
     '- **Derleyici:** gcc 16.2.0 (WinLibs, MinGW-w64 UCRT)',
     f'- **Derleme:** {build}',
     "  (`-Wa,-muse-unaligned-vector-move`: MinGW'de `-march=native` ile oluşan AVX stack hizalama çökmesini önler, GCC bug 54412)",
     f'- **Koşu:** {run}, `-t 600` (dosya başına 600 sn sınır), dosyalar tek tek, sırayla',
     f'- **Durum:** {len(done)}/{len(files)} dosya koşuldu, {prov}/{len(done)} ispatlı. {len(skip)} dosya bu turda koşulmadı.',
     '- Doğrulama (tools/verify.py) bu dosyada yer almıyor; ayrıca yapılacak.\n',
     'Hücre: Z/X (süre, sn). `—` = bu turda koşulmadı. `*` = süre sınırında bitmedi (ispatsız).\n',
     '| n | %20 | %30 | %40 | %50 | %60 | %70 | %80 | %90 |', '|' + '---|' * 9]
for n in range(100, 201, 10):
    row = []
    for p in range(20, 100, 10):
        k = f'{n}{p}'
        if k in t:
            r = t[k]; row.append(f'{r[1]}/{r[2]} ({float(r[5]):.2f})' + ('' if r[3] == 'ispatli' else '*'))
        else:
            row.append('—')
    L.append(f'| {n} | ' + ' | '.join(row) + ' |')
if skip:
    L.append('\n## Koşulmayanlar\n')
    L.append('Bu turda bilerek atlandı (uzun sürmesi beklenen seyrek graflar):\n')
    for p in range(20, 100, 10):
        s = [f for f in skip if f.endswith(str(p))]
        if s: L.append(f'- %{p}: ' + ', '.join(s))
L.append('\n## Ayrıntı\n')
L.append('`süre` = programın ölçtüğü süre (dosya okuma + arama + çıktı yazma + öz-kontrol). '
         '`duvar` = dışarıdan ölçülen toplam süre (süreç başlatma dahil).\n')
L.append('| dosya | Z | X | durum | süre (sn) | duvar (sn) |'); L.append('|---|---|---|---|---|---|')
for f in done:
    r = t[f]; L.append(f'| {f}.txt | {r[1]} | {r[2]} | {r[3]} | {r[4]} | {r[5]} |')
L.append(f'\nToplam süre (koşulan {len(done)} dosya): {sum(float(t[f][5]) for f in done):.1f} sn.')
L.append('\nÇıktılar (`Z/X` ve `S:` listesi, stderr öz-kontrol): `ciktilar/<dosya>.out`, `ciktilar/<dosya>.err`. Ham ölçümler: `ham.tsv`.')
open(f'{D}/SONUCLAR.md', 'w', encoding='utf-8').write('\n'.join(L) + '\n')
print(len(done), prov, len(skip))
