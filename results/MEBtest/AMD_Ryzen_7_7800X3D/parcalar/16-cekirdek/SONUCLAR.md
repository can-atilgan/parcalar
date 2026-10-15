# ParçalaR sonuçları — MEBtest grafları, AMD Ryzen 7 7800X3D, 16 çekirdek

- **Graflar:** `data/MEBtest/` (hocanın test grafları, 2014). Aynı adlı `data/bench/` graflarından farklıdır ve iç içedir:
  her yoğunluk, bir altındakinin bütün kenarlarını içerir.
- **Kapsam:** 88 dosyanın hepsi.
- **İşlemci:** AMD Ryzen 7 7800X3D 8-Core Processor (8 fiziksel çekirdek, 16 mantıksal), Windows 10 Pro
- **Derleyici:** gcc 16.2.0 (WinLibs, MinGW-w64 UCRT)
- **Derleme:** `gcc -O2 -march=native -Wa,-muse-unaligned-vector-move -fopenmp -static final/parcalar.c`
- **Koşu:** `OMP_NUM_THREADS=16`, dosyalar tek tek, sırayla. İspatlanan 74 dosya `-t 600` ile koşuldu (en uzunu 240 sn sürdü):
  `BENCH=data/MEBtest SKIP='^(1[2-9]|20)020$|^(1[6-9]|20)030$' tools/windows/run_bench.sh <exe> results/MEBtest/AMD_Ryzen_7_7800X3D/parcalar/16-cekirdek 16`
  En seyrek 14 dosya (12020–20020 ve 16030–20030) ardından `-t 300` ile, aralarında işlemci soğusun diye 10 sn beklenerek koşuldu:
  `TLIM=300 PAUSE=10 BENCH=data/MEBtest tools/windows/run_bench.sh <exe> results/MEBtest/AMD_Ryzen_7_7800X3D/parcalar/16-cekirdek 16`
- **Durum:** 88/88 dosya koşuldu, 74/88 ispatlı.
- **Doğrulama:** her çıktı `tools/verify.py` ile kontrol edildi, hepsi UYGUN (|S| = X ve X + Y = Z).

Hücre: Z/X (süre, sn). `—` = bu turda koşulmadı. `?` = süre sınırında bitmedi (ispatsız).

| n | %20 | %30 | %40 | %50 | %60 | %70 | %80 | %90 |
|---|---|---|---|---|---|---|---|---|
| 100 | 75/71 (35.79) | 83/78 (1.80) | 87/82 (0.33) | 91/85 (0.20) | 92/90 (0.05) | 94/91 (0.06) | 96/92 (0.06) | 97/94 (0.04) |
| 110 | 85/82 (240.17) | 93/86 (4.21) | 97/93 (0.43) | 100/96 (0.16) | 102/100 (0.05) | 104/100 (0.05) | 106/101 (0.05) | 107/104 (0.04) |
| 120 | 94/88 (299.80) ? | 102/97 (14.29) | 106/101 (0.60) | 110/105 (0.21) | 112/108 (0.07) | 114/108 (0.07) | 115/111 (0.06) | 117/113 (0.06) |
| 130 | 103/96 (299.99) ? | 112/106 (46.83) | 116/112 (1.10) | 120/115 (0.29) | 122/118 (0.07) | 124/118 (0.06) | 125/123 (0.05) | 127/124 (0.05) |
| 140 | 112/105 (299.70) ? | 121/115 (77.78) | 126/121 (1.93) | 129/127 (0.23) | 132/128 (0.07) | 134/128 (0.06) | 135/133 (0.05) | 137/133 (0.05) |
| 150 | 123/117 (299.70) ? | 131/126 (227.32) | 136/131 (8.05) | 139/135 (0.35) | 142/137 (0.10) | 143/140 (0.06) | 145/140 (0.05) | 147/143 (0.06) |
| 160 | 131/124 (299.69) ? | 141/134 (299.85) ? | 145/141 (11.87) | 149/145 (0.78) | 151/146 (0.08) | 154/147 (0.08) | 155/151 (0.05) | 157/153 (0.07) |
| 170 | 142/135 (299.73) ? | 150/144 (299.82) ? | 155/151 (13.30) | 158/154 (0.47) | 161/157 (0.11) | 163/160 (0.06) | 165/160 (0.07) | 166/164 (0.05) |
| 180 | 149/145 (299.71) ? | 159/155 (299.85) ? | 165/161 (58.59) | 169/163 (2.10) | 171/166 (0.15) | 173/169 (0.07) | 175/170 (0.06) | 176/174 (0.04) |
| 190 | 160/151 (299.71) ? | 170/163 (299.85) ? | 175/170 (122.78) | 178/175 (1.12) | 181/177 (0.18) | 183/180 (0.06) | 185/180 (0.06) | 187/182 (0.07) |
| 200 | 170/161 (299.71) ? | 179/174 (299.82) ? | 184/179 (54.47) | 188/183 (4.43) | 191/187 (0.28) | 193/190 (0.07) | 195/190 (0.06) | 197/192 (0.07) |

## Ayrıntı ve `data/bench/` ile karşılaştırma

`süre` = programın ölçtüğü süre (dosya okuma + arama + çıktı yazma + öz-kontrol). Son iki sütun aynı adlı `data/bench/` grafının sonucu (farklı graf).

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
| 12020.txt | 94 | 88 | ISPATSIZ | 299.77 | — | — |
| 12030.txt | 102 | 97 | ispatli | 14.25 | 102/99 | 11.11 |
| 12040.txt | 106 | 101 | ispatli | 0.56 | 106/102 | 0.53 |
| 12050.txt | 110 | 105 | ispatli | 0.17 | 110/105 | 0.19 |
| 12060.txt | 112 | 108 | ispatli | 0.03 | 112/108 | 0.07 |
| 12070.txt | 114 | 108 | ispatli | 0.03 | 114/109 | 0.05 |
| 12080.txt | 115 | 111 | ispatli | 0.02 | 116/111 | 0.05 |
| 12090.txt | 117 | 113 | ispatli | 0.02 | 117/114 | 0.05 |
| 13020.txt | 103 | 96 | ISPATSIZ | 299.96 | — | — |
| 13030.txt | 112 | 106 | ispatli | 46.79 | 110/106 | 14.55 |
| 13040.txt | 116 | 112 | ispatli | 1.07 | 116/113 | 1.06 |
| 13050.txt | 120 | 115 | ispatli | 0.26 | 119/115 | 0.19 |
| 13060.txt | 122 | 118 | ispatli | 0.04 | 122/117 | 0.07 |
| 13070.txt | 124 | 118 | ispatli | 0.04 | 123/121 | 0.05 |
| 13080.txt | 125 | 123 | ispatli | 0.02 | 125/123 | 0.05 |
| 13090.txt | 127 | 124 | ispatli | 0.02 | 127/123 | 0.05 |
| 14020.txt | 112 | 105 | ISPATSIZ | 299.66 | — | — |
| 14030.txt | 121 | 115 | ispatli | 77.75 | 121/116 | 151.90 |
| 14040.txt | 126 | 121 | ispatli | 1.90 | 126/121 | 1.95 |
| 14050.txt | 129 | 127 | ispatli | 0.20 | 129/125 | 0.43 |
| 14060.txt | 132 | 128 | ispatli | 0.04 | 132/127 | 0.07 |
| 14070.txt | 134 | 128 | ispatli | 0.03 | 134/128 | 0.06 |
| 14080.txt | 135 | 133 | ispatli | 0.02 | 135/132 | 0.05 |
| 14090.txt | 137 | 133 | ispatli | 0.02 | 137/133 | 0.05 |
| 15020.txt | 123 | 117 | ISPATSIZ | 299.67 | — | — |
| 15030.txt | 131 | 126 | ispatli | 227.28 | 130/126 | 141.27 |
| 15040.txt | 136 | 131 | ispatli | 8.01 | 136/130 | 4.04 |
| 15050.txt | 139 | 135 | ispatli | 0.32 | 139/135 | 0.35 |
| 15060.txt | 142 | 137 | ispatli | 0.07 | 142/137 | 0.10 |
| 15070.txt | 143 | 140 | ispatli | 0.03 | 144/138 | 0.07 |
| 15080.txt | 145 | 140 | ispatli | 0.02 | 145/143 | 0.05 |
| 15090.txt | 147 | 143 | ispatli | 0.03 | 147/142 | 0.07 |
| 16020.txt | 131 | 124 | ISPATSIZ | 299.66 | — | — |
| 16030.txt | 141 | 134 | ISPATSIZ | 299.82 | — | — |
| 16040.txt | 145 | 141 | ispatli | 11.84 | 145/140 | 12.93 |
| 16050.txt | 149 | 145 | ispatli | 0.75 | 149/145 | 0.82 |
| 16060.txt | 151 | 146 | ispatli | 0.05 | 151/148 | 0.08 |
| 16070.txt | 154 | 147 | ispatli | 0.05 | 153/150 | 0.06 |
| 16080.txt | 155 | 151 | ispatli | 0.02 | 155/151 | 0.06 |
| 16090.txt | 157 | 153 | ispatli | 0.03 | 157/153 | 0.06 |
| 17020.txt | 142 | 135 | ISPATSIZ | 299.69 | — | — |
| 17030.txt | 150 | 144 | ISPATSIZ | 299.78 | — | — |
| 17040.txt | 155 | 151 | ispatli | 13.26 | 155/151 | 13.31 |
| 17050.txt | 158 | 154 | ispatli | 0.44 | 159/152 | 0.79 |
| 17060.txt | 161 | 157 | ispatli | 0.08 | 161/159 | 0.15 |
| 17070.txt | 163 | 160 | ispatli | 0.03 | 163/160 | 0.06 |
| 17080.txt | 165 | 160 | ispatli | 0.03 | 165/160 | 0.06 |
| 17090.txt | 166 | 164 | ispatli | 0.02 | 167/163 | 0.06 |
| 18020.txt | 149 | 145 | ISPATSIZ | 299.68 | — | — |
| 18030.txt | 159 | 155 | ISPATSIZ | 299.82 | — | — |
| 18040.txt | 165 | 161 | ispatli | 58.55 | 165/159 | 48.99 |
| 18050.txt | 169 | 163 | ispatli | 2.07 | 169/163 | 1.21 |
| 18060.txt | 171 | 166 | ispatli | 0.12 | 171/167 | 0.11 |
| 18070.txt | 173 | 169 | ispatli | 0.03 | 173/169 | 0.06 |
| 18080.txt | 175 | 170 | ispatli | 0.03 | 175/170 | 0.06 |
| 18090.txt | 176 | 174 | ispatli | 0.02 | 176/173 | 0.06 |
| 19020.txt | 160 | 151 | ISPATSIZ | 299.68 | — | — |
| 19030.txt | 170 | 163 | ISPATSIZ | 299.81 | — | — |
| 19040.txt | 175 | 170 | ispatli | 122.74 | 175/168 | 88.43 |
| 19050.txt | 178 | 175 | ispatli | 1.09 | 178/173 | 2.23 |
| 19060.txt | 181 | 177 | ispatli | 0.16 | 181/177 | 0.12 |
| 19070.txt | 183 | 180 | ispatli | 0.03 | 183/179 | 0.07 |
| 19080.txt | 185 | 180 | ispatli | 0.03 | 185/180 | 0.06 |
| 19090.txt | 187 | 182 | ispatli | 0.05 | 187/183 | 0.06 |
| 20020.txt | 170 | 161 | ISPATSIZ | 299.67 | — | — |
| 20030.txt | 179 | 174 | ISPATSIZ | 299.79 | — | — |
| 20040.txt | 184 | 179 | ispatli | 54.43 | 184/180 | 54.74 |
| 20050.txt | 188 | 183 | ispatli | 4.40 | 188/184 | 1.96 |
| 20060.txt | 191 | 187 | ispatli | 0.25 | 190/188 | 0.12 |
| 20070.txt | 193 | 190 | ispatli | 0.04 | 193/190 | 0.06 |
| 20080.txt | 195 | 190 | ispatli | 0.03 | 195/190 | 0.06 |
| 20090.txt | 197 | 192 | ispatli | 0.04 | 196/195 | 0.05 |

Toplam süre (koşulan 88 dosya): 5132.0 sn (`data/bench/` aynı dosyalar: 852.0 sn).

Çıktılar (`Z/X` ve `S:` listesi, stderr öz-kontrol): `ciktilar/<dosya>.out`, `ciktilar/<dosya>.err`. Ham ölçümler: `ham.tsv`.

## Notlar

- **10050:** Z = 91, X = 85. Dört bağımsız kesin çözücü (`alternatif_cozuculer/bnb`, `bnb_b`, `rds`, `rds_b`) ve sezgisel çözücü (`heur`, 3 farklı başlangıç) aynı sonucu verdi. Notlarda ödevin bilinen değeri olarak geçen "(10/5)" (Z = 90) `data/bench/10050` ile tutuyor, MEBtest ile değil.
- **İspatlanamayan 14 dosya (300 sn):** yazılan çözümler geçerli, ama daha iyisinin olmadığı gösterilemedi. Sezgisel ILS (`results/MEBtest/AMD_Ryzen_7_7800X3D/sezgisel_ILS/tek-cekirdek-10sn`, her çözüme 1.05 sn içinde ulaştı) ile karşılaştırma:

  | dosya | kesin çözücü (300 sn) | sezgisel ILS | |
  |---|---|---|---|
  | 12020 | 94/88 | 94/88 | aynı |
  | 13020 | 103/96 | 103/96 | aynı |
  | 14020 | 112/105 | 112/105 | aynı |
  | 15020 | 123/117 | 123/117 | aynı |
  | 16020 | 131/124 | 131/123 | ILS'nin X'i daha küçük |
  | 16030 | 141/134 | 141/134 | aynı |
  | 17020 | 142/135 | 141/134 | ILS'nin Z'si daha küçük |
  | 17030 | 150/144 | 150/144 | aynı |
  | 18020 | 149/145 | 149/143 | ILS'nin X'i daha küçük |
  | 18030 | 159/155 | 159/153 | ILS'nin X'i daha küçük |
  | 19020 | 160/151 | 160/151 | aynı |
  | 19030 | 170/163 | 169/163 | ILS'nin Z'si daha küçük |
  | 20020 | 170/161 | 168/163 | ILS'nin Z'si daha küçük |
  | 20030 | 179/174 | 179/174 | aynı |

  Bu dosyalar önce 150 ve 600 sn sınırlarıyla koşulmuştu; 300 sn ile yeniden koşuldu. 150 sn ile 300 sn arasında yalnızca 17030 (151/141 → 150/144) ve 19030 (170/164 → 170/163) değişti.
- **15050:** ödevin bilinen değeri "(11/4)" ile tutuyor (Z = 139, X = 135).
- **12080 için başka bir programın cevabı:** `Z=115 X=114` (Y = 1, 6 bağımsız bilgisayar kalıyor) 12020–12080 graflarında geçerli, 12090'da geçersiz. Z = 115 yalnızca 12080'in optimumuyla tutuyor; ancak aynı Z, 111 bilgisayar parçalanarak da elde ediliyor (Y = 4). Ödevin eşitlik kuralı en az bilgisayar parçalayan çözümü istediği için doğru cevap **Z = 115, X = 111**.
