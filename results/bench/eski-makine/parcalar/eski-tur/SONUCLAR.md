# ParçalaR sonuçları (data/bench/*.txt, rastgele G(n,M) test grafları; eski makine)

İspatlı: 67/88.  (Arka planda uzun koşular sürüyordu; bu dosya o anki durumdur.)
Hücre: Z/X.  `*` = süre sınırında bitmedi: geçerli çözüm, optimalliği ISPATLANMADI.
İşaretsiz = kesin (ispatlı) optimum.  Tüm S listeleri tools/verify.py ile 'uygun' doğrulandı.

| n | %20 | %30 | %40 | %50 | %60 | %70 | %80 | %90 |
|---|---|---|---|---|---|---|---|---|
| 100 | 76/70* | 83/76 | 87/82 | 90/85 | 92/88 | 94/91 | 96/91 | 97/94 |
| 110 | 85/78* | 92/87 | 97/91 | 100/95 | 102/100 | 104/100 | 105/102 | 107/103 |
| 120 | 93/87* | 102/99* | 106/102 | 110/105 | 112/108 | 114/109 | 116/111 | 117/114 |
| 130 | 103/95* | 110/106* | 116/113 | 119/115 | 122/117 | 123/121 | 125/123 | 127/123 |
| 140 | 113/105* | 121/117* | 126/121 | 129/125 | 132/127 | 134/128 | 135/132 | 137/133 |
| 150 | 123/116* | 130/126* | 136/130 | 139/135 | 142/137 | 144/138 | 145/143 | 147/142 |
| 160 | 132/124* | 140/134* | 145/140 | 149/145 | 151/148 | 153/150 | 155/151 | 157/153 |
| 170 | 140/133* | 149/145* | 155/151 | 159/152 | 161/159 | 163/160 | 165/160 | 167/163 |
| 180 | 150/145* | 159/155* | 165/159 | 169/163 | 171/167 | 173/169 | 175/170 | 176/173 |
| 190 | 159/153* | 169/163* | 175/168 | 178/173 | 181/177 | 183/179 | 185/180 | 187/183 |
| 200 | 168/163* | 178/174* | 184/180* | 188/184 | 190/188 | 193/190 | 195/190 | 196/195 |

## Ayrıntı (dosya, Z, X, durum, süre sn, koşu, doğrulama)

| dosya | Z | X | durum | süre (sn) | koşu | S doğrulama |
|---|---|---|---|---|---|---|
| 10020.txt | 76 | 70 | ISPATSIZ | 119.6 | 120s/1 cekirdek | feasible |
| 10030.txt | 83 | 76 | ispatlı | 19.6 | 120s/1 cekirdek | feasible |
| 10040.txt | 87 | 82 | ispatlı | 1.4 | 120s/1 cekirdek | feasible |
| 10050.txt | 90 | 85 | ispatlı | 0.5 | 120s/1 cekirdek | feasible |
| 10060.txt | 92 | 88 | ispatlı | 0.1 | 120s/1 cekirdek | feasible |
| 10070.txt | 94 | 91 | ispatlı | 0.0 | 120s/1 cekirdek | feasible |
| 10080.txt | 96 | 91 | ispatlı | 0.1 | 120s/1 cekirdek | feasible |
| 10090.txt | 97 | 94 | ispatlı | 0.1 | 120s/1 cekirdek | feasible |
| 11020.txt | 85 | 78 | ISPATSIZ | 119.1 | 120s/1 cekirdek | feasible |
| 11030.txt | 92 | 87 | ispatlı | 53.0 | 120s/1 cekirdek | feasible |
| 11040.txt | 97 | 91 | ispatlı | 3.1 | 120s/1 cekirdek | feasible |
| 11050.txt | 100 | 95 | ispatlı | 0.6 | 120s/1 cekirdek | feasible |
| 11060.txt | 102 | 100 | ispatlı | 0.1 | 120s/1 cekirdek | feasible |
| 11070.txt | 104 | 100 | ispatlı | 0.1 | 120s/1 cekirdek | feasible |
| 11080.txt | 105 | 102 | ispatlı | 0.1 | 120s/1 cekirdek | feasible |
| 11090.txt | 107 | 103 | ispatlı | 0.1 | 120s/1 cekirdek | feasible |
| 12020.txt | 93 | 87 | ISPATSIZ | 119.8 | 120s/1 cekirdek | feasible |
| 12030.txt | 102 | 99 | ISPATSIZ | 119.9 | 120s/1 cekirdek | feasible |
| 12040.txt | 106 | 102 | ispatlı | 4.6 | 120s/1 cekirdek | feasible |
| 12050.txt | 110 | 105 | ispatlı | 0.7 | 120s/1 cekirdek | feasible |
| 12060.txt | 112 | 108 | ispatlı | 0.1 | 120s/1 cekirdek | feasible |
| 12070.txt | 114 | 109 | ispatlı | 0.1 | 120s/1 cekirdek | feasible |
| 12080.txt | 116 | 111 | ispatlı | 0.1 | 120s/1 cekirdek | feasible |
| 12090.txt | 117 | 114 | ispatlı | 0.1 | 120s/1 cekirdek | feasible |
| 13020.txt | 103 | 95 | ISPATSIZ | 119.1 | 120s/1 cekirdek | feasible |
| 13030.txt | 110 | 106 | ISPATSIZ | 120.0 | 120s/1 cekirdek | feasible |
| 13040.txt | 116 | 113 | ispatlı | 16.1 | 120s/1 cekirdek | feasible |
| 13050.txt | 119 | 115 | ispatlı | 1.2 | 120s/1 cekirdek | feasible |
| 13060.txt | 122 | 117 | ispatlı | 0.2 | 120s/1 cekirdek | feasible |
| 13070.txt | 123 | 121 | ispatlı | 0.1 | 120s/1 cekirdek | feasible |
| 13080.txt | 125 | 123 | ispatlı | 0.0 | 120s/1 cekirdek | feasible |
| 13090.txt | 127 | 123 | ispatlı | 0.1 | 120s/1 cekirdek | feasible |
| 14020.txt | 113 | 105 | ISPATSIZ | 119.1 | 120s/1 cekirdek | feasible |
| 14030.txt | 121 | 117 | ISPATSIZ | 119.8 | 120s/1 cekirdek | feasible |
| 14040.txt | 126 | 121 | ispatlı | 31.5 | 120s/1 cekirdek | feasible |
| 14050.txt | 129 | 125 | ispatlı | 2.6 | 120s/1 cekirdek | feasible |
| 14060.txt | 132 | 127 | ispatlı | 0.3 | 120s/1 cekirdek | feasible |
| 14070.txt | 134 | 128 | ispatlı | 0.1 | 120s/1 cekirdek | feasible |
| 14080.txt | 135 | 132 | ispatlı | 0.1 | 120s/1 cekirdek | feasible |
| 14090.txt | 137 | 133 | ispatlı | 0.1 | 120s/1 cekirdek | feasible |
| 15020.txt | 123 | 116 | ISPATSIZ | 120.0 | 120s/1 cekirdek | feasible |
| 15030.txt | 130 | 126 | ISPATSIZ | 119.9 | 120s/1 cekirdek | feasible |
| 15040.txt | 136 | 130 | ispatlı | 69.4 | 120s/1 cekirdek | feasible |
| 15050.txt | 139 | 135 | ispatlı | 3.9 | 120s/1 cekirdek | feasible |
| 15060.txt | 142 | 137 | ispatlı | 0.5 | 120s/1 cekirdek | feasible |
| 15070.txt | 144 | 138 | ispatlı | 0.2 | 120s/1 cekirdek | feasible |
| 15080.txt | 145 | 143 | ispatlı | 0.1 | 120s/1 cekirdek | feasible |
| 15090.txt | 147 | 142 | ispatlı | 0.1 | 120s/1 cekirdek | feasible |
| 16020.txt | 132 | 124 | ISPATSIZ | 119.4 | 120s/1 cekirdek | feasible |
| 16030.txt | 140 | 134 | ISPATSIZ | 119.9 | 120s/1 cekirdek | feasible |
| 16040.txt | 145 | 140 | ispatlı | 58.5 | 900s/4 cekirdek | feasible |
| 16050.txt | 149 | 145 | ispatlı | 5.9 | 120s/1 cekirdek | feasible |
| 16060.txt | 151 | 148 | ispatlı | 0.4 | 120s/1 cekirdek | feasible |
| 16070.txt | 153 | 150 | ispatlı | 0.1 | 120s/1 cekirdek | feasible |
| 16080.txt | 155 | 151 | ispatlı | 0.1 | 120s/1 cekirdek | feasible |
| 16090.txt | 157 | 153 | ispatlı | 0.1 | 120s/1 cekirdek | feasible |
| 17020.txt | 140 | 133 | ISPATSIZ | 119.0 | 120s/1 cekirdek | feasible |
| 17030.txt | 149 | 145 | ISPATSIZ | 119.9 | 120s/1 cekirdek | feasible |
| 17040.txt | 155 | 151 | ispatlı | 60.5 | 900s/4 cekirdek | feasible |
| 17050.txt | 159 | 152 | ispatlı | 9.4 | 120s/1 cekirdek | feasible |
| 17060.txt | 161 | 159 | ispatlı | 0.8 | 120s/1 cekirdek | feasible |
| 17070.txt | 163 | 160 | ispatlı | 0.1 | 120s/1 cekirdek | feasible |
| 17080.txt | 165 | 160 | ispatlı | 0.1 | 120s/1 cekirdek | feasible |
| 17090.txt | 167 | 163 | ispatlı | 0.1 | 120s/1 cekirdek | feasible |
| 18020.txt | 150 | 145 | ISPATSIZ | 119.1 | 120s/1 cekirdek | feasible |
| 18030.txt | 159 | 155 | ISPATSIZ | 119.9 | 120s/1 cekirdek | feasible |
| 18040.txt | 165 | 159 | ispatlı | 220.6 | 900s/4 cekirdek | feasible |
| 18050.txt | 169 | 163 | ispatlı | 16.4 | 120s/1 cekirdek | feasible |
| 18060.txt | 171 | 167 | ispatlı | 0.7 | 120s/1 cekirdek | feasible |
| 18070.txt | 173 | 169 | ispatlı | 0.1 | 120s/1 cekirdek | feasible |
| 18080.txt | 175 | 170 | ispatlı | 0.1 | 120s/1 cekirdek | feasible |
| 18090.txt | 176 | 173 | ispatlı | 0.1 | 120s/1 cekirdek | feasible |
| 19020.txt | 159 | 153 | ISPATSIZ | 119.7 | 120s/1 cekirdek | feasible |
| 19030.txt | 169 | 163 | ISPATSIZ | 119.4 | 120s/1 cekirdek | feasible |
| 19040.txt | 175 | 168 | ispatlı | 414.7 | 900s/4 cekirdek | feasible |
| 19050.txt | 178 | 173 | ispatlı | 18.9 | 120s/1 cekirdek | feasible |
| 19060.txt | 181 | 177 | ispatlı | 0.9 | 120s/1 cekirdek | feasible |
| 19070.txt | 183 | 179 | ispatlı | 0.2 | 120s/1 cekirdek | feasible |
| 19080.txt | 185 | 180 | ispatlı | 0.1 | 120s/1 cekirdek | feasible |
| 19090.txt | 187 | 183 | ispatlı | 0.2 | 120s/1 cekirdek | feasible |
| 20020.txt | 168 | 163 | ISPATSIZ | 119.2 | 120s/1 cekirdek | feasible |
| 20030.txt | 178 | 174 | ISPATSIZ | 119.9 | 120s/1 cekirdek | feasible |
| 20040.txt | 184 | 180 | ISPATSIZ | 119.9 | 120s/1 cekirdek | feasible |
| 20050.txt | 188 | 184 | ispatlı | 34.0 | 120s/1 cekirdek | feasible |
| 20060.txt | 190 | 188 | ispatlı | 1.0 | 120s/1 cekirdek | feasible |
| 20070.txt | 193 | 190 | ispatlı | 0.2 | 120s/1 cekirdek | feasible |
| 20080.txt | 195 | 190 | ispatlı | 0.1 | 120s/1 cekirdek | feasible |
| 20090.txt | 196 | 195 | ispatlı | 0.1 | 120s/1 cekirdek | feasible |
