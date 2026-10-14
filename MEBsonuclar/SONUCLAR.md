# ParçalaR sonuçları — MEBtest grafları, AMD Ryzen 7 7800X3D, 16 çekirdek

- **Graflar:** `MEBtest/` (hocanın test grafları, 2014). Aynı adlı `bench/` graflarından farklıdır ve iç içedir:
  her yoğunluk, bir altındakinin bütün kenarlarını içerir.
- **Kapsam:** `bench/` 16 çekirdek turunda koşulan dosyaların aynısı (n ≥ 120 için %20, n ≥ 160 için %30 koşulmadı), artı 12020.
- **İşlemci:** AMD Ryzen 7 7800X3D 8-Core Processor (8 fiziksel çekirdek, 16 mantıksal), Windows 10 Pro
- **Derleyici:** gcc 16.2.0 (WinLibs, MinGW-w64 UCRT)
- **Derleme:** `gcc -O2 -march=native -Wa,-muse-unaligned-vector-move -fopenmp -static final/parcalar.c`
- **Koşu:** `OMP_NUM_THREADS=16`, `-t 600`, dosyalar tek tek, sırayla:
  `BENCH=MEBtest SKIP='^(1[2-9]|20)020$|^(1[6-9]|20)030$' tools/windows/run_bench.sh <exe> MEBsonuclar 16`
- **Durum:** 75/88 dosya koşuldu, 74/75 ispatlı.
- **Doğrulama:** her çıktı `tools/verify.py` ile kontrol edildi, hepsi UYGUN (|S| = X ve X + Y = Z).

Hücre: Z/X (süre, sn). `—` = bu turda koşulmadı. `*` = süre sınırında bitmedi (ispatsız).

| n | %20 | %30 | %40 | %50 | %60 | %70 | %80 | %90 |
|---|---|---|---|---|---|---|---|---|
| 100 | 75/71 (35.79) | 83/78 (1.80) | 87/82 (0.33) | 91/85 (0.20) | 92/90 (0.05) | 94/91 (0.06) | 96/92 (0.06) | 97/94 (0.04) |
| 110 | 85/82 (240.17) | 93/86 (4.21) | 97/93 (0.43) | 100/96 (0.16) | 102/100 (0.05) | 104/100 (0.05) | 106/101 (0.05) | 107/104 (0.04) |
| 120 | 94/88 (599.55)* | 102/97 (14.29) | 106/101 (0.60) | 110/105 (0.21) | 112/108 (0.07) | 114/108 (0.07) | 115/111 (0.06) | 117/113 (0.06) |
| 130 | — | 112/106 (46.83) | 116/112 (1.10) | 120/115 (0.29) | 122/118 (0.07) | 124/118 (0.06) | 125/123 (0.05) | 127/124 (0.05) |
| 140 | — | 121/115 (77.78) | 126/121 (1.93) | 129/127 (0.23) | 132/128 (0.07) | 134/128 (0.06) | 135/133 (0.05) | 137/133 (0.05) |
| 150 | — | 131/126 (227.32) | 136/131 (8.05) | 139/135 (0.35) | 142/137 (0.10) | 143/140 (0.06) | 145/140 (0.05) | 147/143 (0.06) |
| 160 | — | — | 145/141 (11.87) | 149/145 (0.78) | 151/146 (0.08) | 154/147 (0.08) | 155/151 (0.05) | 157/153 (0.07) |
| 170 | — | — | 155/151 (13.30) | 158/154 (0.47) | 161/157 (0.11) | 163/160 (0.06) | 165/160 (0.07) | 166/164 (0.05) |
| 180 | — | — | 165/161 (58.59) | 169/163 (2.10) | 171/166 (0.15) | 173/169 (0.07) | 175/170 (0.06) | 176/174 (0.04) |
| 190 | — | — | 175/170 (122.78) | 178/175 (1.12) | 181/177 (0.18) | 183/180 (0.06) | 185/180 (0.06) | 187/182 (0.07) |
| 200 | — | — | 184/179 (54.47) | 188/183 (4.43) | 191/187 (0.28) | 193/190 (0.07) | 195/190 (0.06) | 197/192 (0.07) |

## Ayrıntı ve `bench/` ile karşılaştırma

`süre` = programın ölçtüğü süre (dosya okuma + arama + çıktı yazma + öz-kontrol). Son iki sütun aynı adlı `bench/` grafının sonucu (farklı graf).

| dosya | Z | X | durum | süre (sn) | bench Z/X | bench süre (sn) |
|---|---|---|---|---|---|---|
| 10020.txt | 75 | 71 | ispatli | 35.74 | 76/70 | 51.27 |
| 10030.txt | 83 | 78 | ispatli | 1.77 | 83/76 | 1.55 |
| 10040.txt | 87 | 82 | ispatli | 0.30 | 87/82 | 0.33 |
| 10050.txt | 91 | 85 | ispatli | 0.17 | 90/85 | 0.15 |
| 10060.txt | 92 | 90 | ispatli | 0.02 | 92/88 | 0.05 |
| 10070.txt | 94 | 91 | ispatli | 0.02 | 94/91 | 0.05 |
| 10080.txt | 96 | 92 | ispatli | 0.02 | 96/91 | 0.05 |
| 10090.txt | 97 | 94 | ispatli | 0.01 | 97/94 | 0.04 |
| 11020.txt | 85 | 82 | ispatli | 240.13 | 85/78 | 238.73 |
| 11030.txt | 93 | 86 | ispatli | 4.18 | 92/87 | 3.51 |
| 11040.txt | 97 | 93 | ispatli | 0.40 | 97/91 | 0.47 |
| 11050.txt | 100 | 96 | ispatli | 0.13 | 100/95 | 0.18 |
| 11060.txt | 102 | 100 | ispatli | 0.02 | 102/100 | 0.05 |
| 11070.txt | 104 | 100 | ispatli | 0.02 | 104/100 | 0.05 |
| 11080.txt | 106 | 101 | ispatli | 0.02 | 105/102 | 0.04 |
| 11090.txt | 107 | 104 | ispatli | 0.01 | 107/103 | 0.05 |
| 12020.txt | 94 | 88 | ISPATSIZ | 599.51 | — | — |
| 12030.txt | 102 | 97 | ispatli | 14.25 | 102/99 | 11.11 |
| 12040.txt | 106 | 101 | ispatli | 0.56 | 106/102 | 0.53 |
| 12050.txt | 110 | 105 | ispatli | 0.17 | 110/105 | 0.19 |
| 12060.txt | 112 | 108 | ispatli | 0.03 | 112/108 | 0.07 |
| 12070.txt | 114 | 108 | ispatli | 0.03 | 114/109 | 0.05 |
| 12080.txt | 115 | 111 | ispatli | 0.02 | 116/111 | 0.05 |
| 12090.txt | 117 | 113 | ispatli | 0.02 | 117/114 | 0.05 |
| 13030.txt | 112 | 106 | ispatli | 46.79 | 110/106 | 14.55 |
| 13040.txt | 116 | 112 | ispatli | 1.07 | 116/113 | 1.06 |
| 13050.txt | 120 | 115 | ispatli | 0.26 | 119/115 | 0.19 |
| 13060.txt | 122 | 118 | ispatli | 0.04 | 122/117 | 0.07 |
| 13070.txt | 124 | 118 | ispatli | 0.04 | 123/121 | 0.05 |
| 13080.txt | 125 | 123 | ispatli | 0.02 | 125/123 | 0.05 |
| 13090.txt | 127 | 124 | ispatli | 0.02 | 127/123 | 0.05 |
| 14030.txt | 121 | 115 | ispatli | 77.75 | 121/116 | 151.90 |
| 14040.txt | 126 | 121 | ispatli | 1.90 | 126/121 | 1.95 |
| 14050.txt | 129 | 127 | ispatli | 0.20 | 129/125 | 0.43 |
| 14060.txt | 132 | 128 | ispatli | 0.04 | 132/127 | 0.07 |
| 14070.txt | 134 | 128 | ispatli | 0.03 | 134/128 | 0.06 |
| 14080.txt | 135 | 133 | ispatli | 0.02 | 135/132 | 0.05 |
| 14090.txt | 137 | 133 | ispatli | 0.02 | 137/133 | 0.05 |
| 15030.txt | 131 | 126 | ispatli | 227.28 | 130/126 | 141.27 |
| 15040.txt | 136 | 131 | ispatli | 8.01 | 136/130 | 4.04 |
| 15050.txt | 139 | 135 | ispatli | 0.32 | 139/135 | 0.35 |
| 15060.txt | 142 | 137 | ispatli | 0.07 | 142/137 | 0.10 |
| 15070.txt | 143 | 140 | ispatli | 0.03 | 144/138 | 0.07 |
| 15080.txt | 145 | 140 | ispatli | 0.02 | 145/143 | 0.05 |
| 15090.txt | 147 | 143 | ispatli | 0.03 | 147/142 | 0.07 |
| 16040.txt | 145 | 141 | ispatli | 11.84 | 145/140 | 12.93 |
| 16050.txt | 149 | 145 | ispatli | 0.75 | 149/145 | 0.82 |
| 16060.txt | 151 | 146 | ispatli | 0.05 | 151/148 | 0.08 |
| 16070.txt | 154 | 147 | ispatli | 0.05 | 153/150 | 0.06 |
| 16080.txt | 155 | 151 | ispatli | 0.02 | 155/151 | 0.06 |
| 16090.txt | 157 | 153 | ispatli | 0.03 | 157/153 | 0.06 |
| 17040.txt | 155 | 151 | ispatli | 13.26 | 155/151 | 13.31 |
| 17050.txt | 158 | 154 | ispatli | 0.44 | 159/152 | 0.79 |
| 17060.txt | 161 | 157 | ispatli | 0.08 | 161/159 | 0.15 |
| 17070.txt | 163 | 160 | ispatli | 0.03 | 163/160 | 0.06 |
| 17080.txt | 165 | 160 | ispatli | 0.03 | 165/160 | 0.06 |
| 17090.txt | 166 | 164 | ispatli | 0.02 | 167/163 | 0.06 |
| 18040.txt | 165 | 161 | ispatli | 58.55 | 165/159 | 48.99 |
| 18050.txt | 169 | 163 | ispatli | 2.07 | 169/163 | 1.21 |
| 18060.txt | 171 | 166 | ispatli | 0.12 | 171/167 | 0.11 |
| 18070.txt | 173 | 169 | ispatli | 0.03 | 173/169 | 0.06 |
| 18080.txt | 175 | 170 | ispatli | 0.03 | 175/170 | 0.06 |
| 18090.txt | 176 | 174 | ispatli | 0.02 | 176/173 | 0.06 |
| 19040.txt | 175 | 170 | ispatli | 122.74 | 175/168 | 88.43 |
| 19050.txt | 178 | 175 | ispatli | 1.09 | 178/173 | 2.23 |
| 19060.txt | 181 | 177 | ispatli | 0.16 | 181/177 | 0.12 |
| 19070.txt | 183 | 180 | ispatli | 0.03 | 183/179 | 0.07 |
| 19080.txt | 185 | 180 | ispatli | 0.03 | 185/180 | 0.06 |
| 19090.txt | 187 | 182 | ispatli | 0.05 | 187/183 | 0.06 |
| 20040.txt | 184 | 179 | ispatli | 54.43 | 184/180 | 54.74 |
| 20050.txt | 188 | 183 | ispatli | 4.40 | 188/184 | 1.96 |
| 20060.txt | 191 | 187 | ispatli | 0.25 | 190/188 | 0.12 |
| 20070.txt | 193 | 190 | ispatli | 0.04 | 193/190 | 0.06 |
| 20080.txt | 195 | 190 | ispatli | 0.03 | 195/190 | 0.06 |
| 20090.txt | 197 | 192 | ispatli | 0.04 | 196/195 | 0.05 |

Toplam süre (koşulan 75 dosya): 1534.7 sn (`bench/` aynı dosyalar: 852.0 sn).

Çıktılar (`Z/X` ve `S:` listesi, stderr öz-kontrol): `ciktilar/<dosya>.out`, `ciktilar/<dosya>.err`. Ham ölçümler: `ham.tsv`.

## Notlar

- **10050:** Z = 91, X = 85. Dört bağımsız kesin çözücü (`alternatif_cozuculer/bnb`, `bnb_b`, `rds`, `rds_b`) ve sezgisel çözücü (`heur`, 3 farklı başlangıç) aynı sonucu verdi. Notlarda ödevin bilinen değeri olarak geçen "(10/5)" (Z = 90) `bench/10050` ile tutuyor, MEBtest ile değil.
- **12020:** 600 sn sınırında ispatlanamadı. Yazılan çözüm (Z = 94, X = 88, Y = 6) geçerli; daha iyisinin olmadığı gösterilemedi. Ayrı koşuldu: `BENCH=MEBtest SKIP='^(1[3-9]|20)020$|^(1[6-9]|20)030$' tools/windows/run_bench.sh <exe> MEBsonuclar 16`.
- **15050:** ödevin bilinen değeri "(11/4)" ile tutuyor (Z = 139, X = 135).
- **12080 için başka bir programın cevabı:** `Z=115 X=114` (Y = 1, 6 bağımsız bilgisayar kalıyor) 12020–12080 graflarında geçerli, 12090'da geçersiz. Z = 115 yalnızca 12080'in optimumuyla tutuyor; ancak aynı Z, 111 bilgisayar parçalanarak da elde ediliyor (Y = 4). Ödevin eşitlik kuralı en az bilgisayar parçalayan çözümü istediği için doğru cevap **Z = 115, X = 111**.
