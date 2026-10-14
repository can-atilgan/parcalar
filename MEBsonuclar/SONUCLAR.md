# ParçalaR sonuçları — MEBtest grafları (n = 120), AMD Ryzen 7 7800X3D, 16 çekirdek

- **Graflar:** `MEBtest/` klasöründeki dosyalar (hocanın test grafları, 2014). Bu dosyalar repoda yok;
  `bench/` altındaki graflardan farklıdır. Yoğun graflar, daha seyrek olanların bütün kenarlarını içerir (iç içe).
- **Kapsam:** şimdilik yalnızca n = 120 satırı, 12020 hariç (en seyrek graf; bu turda koşulmadı).
- **İşlemci:** AMD Ryzen 7 7800X3D 8-Core Processor (8 fiziksel çekirdek, 16 mantıksal), Windows 10 Pro
- **Derleyici:** gcc 16.2.0 (WinLibs, MinGW-w64 UCRT)
- **Derleme:** `gcc -O2 -march=native -Wa,-muse-unaligned-vector-move -fopenmp -static final/parcalar.c`
- **Koşu:** `OMP_NUM_THREADS=16`, `-t 600`, dosyalar tek tek, sırayla
  (`BENCH=MEBtest SKIP='^(1[013-9]|20)|^12020$' tools/windows/run_bench.sh <exe> MEBsonuclar 16`)
- **Doğrulama:** her çıktı `tools/verify.py` ile kontrol edildi, hepsi UYGUN (|S| = X ve X + Y = Z).

| dosya | Z | X | Y | durum | süre (sn) | doğrulama |
|---|---|---|---|---|---|---|
| 12020.txt | — | — | — | koşulmadı | — | — |
| 12030.txt | 102 | 97 | 5 | ispatlı | 14.25 | uygun |
| 12040.txt | 106 | 101 | 5 | ispatlı | 0.56 | uygun |
| 12050.txt | 110 | 105 | 5 | ispatlı | 0.17 | uygun |
| 12060.txt | 112 | 108 | 4 | ispatlı | 0.03 | uygun |
| 12070.txt | 114 | 108 | 6 | ispatlı | 0.03 | uygun |
| 12080.txt | 115 | 111 | 4 | ispatlı | 0.02 | uygun |
| 12090.txt | 117 | 113 | 4 | ispatlı | 0.02 | uygun |

`süre` = programın ölçtüğü süre (dosya okuma + arama + çıktı yazma + öz-kontrol).
Çıktılar (`Z/X` ve `S:` listesi, stderr öz-kontrol): `ciktilar/<dosya>.out`, `ciktilar/<dosya>.err`. Ham ölçümler: `ham.tsv`.

## Not: 12080 için başka bir programın cevabı

Başka bir programın 120 düğümlü bir graf için verdiği `Z=115 X=114` (Y = 1, 6 bağımsız bilgisayar kalıyor)
cevabı MEBtest'te 12020–12080 graflarında geçerli, 12090'da geçersiz. Z = 115 yalnızca 12080'in optimumuyla
tutuyor. Ancak aynı Z, 111 bilgisayar parçalanarak da elde ediliyor (Y = 4). Ödevin eşitlik kuralı en az
bilgisayar parçalayan çözümü istediği için 12080'in doğru cevabı **Z = 115, X = 111**.
