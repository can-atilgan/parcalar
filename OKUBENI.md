# ParçalaR — paket içeriği

- `final/parcalar.c` — teslim edilecek kesin (exact) çözücü (tek C dosyası). Kullanım: `final/README.md`.
  Derleme: `gcc -O2 parcalar.c -o parcalar` (tek çekirdek) veya `gcc -O2 -fopenmp parcalar.c -o parcalar`.
- `results/SONUCLAR.md` — 88 test dosyası için Z/X tablosu, ispatlı/ispatsız durumu, süreler.
- `results/ciktilar/*.out` — her dosya için programın çıktısı (Z, X ve `S:` listesi).
- `results/*.tsv` — ham ölçümler (120 sn tek çekirdek turu; 900 sn 4 çekirdek uzun turu).
- `bench/` — test için üretilen rastgele graflar (`{n}{yüzde}.txt`, hocanın dosyaları değil).
- `small/example.txt` — sorudaki 7 düğümlü örnek (cevap Z=4 X=1, S: 7).
- `tools/verify.py` — bağımsız uygunluk kontrolü: `python3 tools/verify.py GRAF.txt CIKTI.out`
- `tools/brute.c` — küçük graflar (n ≤ 24) için kaba kuvvet doğrulayıcı; `tools/check_small.py` ile
  rastgele karşılaştırma; `tools/gen.py` — graf üretici; `tools/cpsat.py` — OR-Tools ile bağımsız
  model (yalnızca küçük graflar).
- `alternatif_cozuculer/` — geliştirme sırasında yazılan diğer kesin çözücüler (karşılaştırma ve
  çapraz doğrulama için) ve sezgisel başlangıç modülü, her biri NOTES ile.
