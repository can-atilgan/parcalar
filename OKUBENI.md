# ParçalaR — paket içeriği

- `final/parcalar.c` — teslim edilecek kesin (exact) çözücü (tek C dosyası). Kullanım: `final/README.md`.
  Derleme: `gcc -O2 parcalar.c -o parcalar` (tek çekirdek) veya `gcc -O2 -fopenmp parcalar.c -o parcalar`.
- `results/<veri seti>/<bilgisayar>/<yöntem>/<ayar>/` — sonuçlar. Her klasörde `SONUCLAR.md` (Z/X tablosu,
  ispatlı/ispatsız durumu, süreler), ham ölçümler (`ham.tsv` / `*.tsv`) ve `ciktilar/` (her dosyanın çıktısı) var.
  - `results/bench/eski-makine/parcalar/eski-tur/` — ilk tur (120 sn tek çekirdek; 900 sn 4 çekirdek uzun turu).
  - `results/bench/AMD_Ryzen_7_7800X3D/parcalar/16-cekirdek/` — AMD Ryzen 7 7800X3D, 16 iş parçacığı.
  - `results/MEBtest/AMD_Ryzen_7_7800X3D/parcalar/16-cekirdek/` — aynı ayarlarla hocanın MEBtest grafları.
- `data/bench/` — test için üretilen rastgele graflar (`{n}{yüzde}.txt`, hocanın dosyaları değil).
- `data/MEBtest/` — hocanın test grafları (2014), aynı adlarla.
- `data/small/example.txt` — sorudaki 7 düğümlü örnek (cevap Z=4 X=1, S: 7).
- `tools/verify.py` — bağımsız uygunluk kontrolü: `python3 tools/verify.py GRAF.txt CIKTI.out`
- `tools/brute.c` — küçük graflar (n ≤ 24) için kaba kuvvet doğrulayıcı; `tools/check_small.py` ile
  rastgele karşılaştırma; `tools/gen.py` — graf üretici; `tools/cpsat.py` — OR-Tools ile bağımsız
  model (yalnızca küçük graflar).
- `alternatif_cozuculer/` — geliştirme sırasında yazılan diğer kesin çözücüler (karşılaştırma ve
  çapraz doğrulama için) ve sezgisel başlangıç modülü, her biri NOTES ile.
