# ParçalaR — kesin (exact) çözücü

## Derleme
    gcc -O2 parcalar.c -o parcalar              # tek çekirdek
    gcc -O2 -fopenmp parcalar.c -o parcalar     # tüm çekirdekler (sonuç aynı, daha hızlı)

## Çalıştırma
    ./parcalar                 # N ve M sorulur -> "<N><M>.txt" okunur (N=100, M=20 -> 10020.txt)
    ./parcalar 100 20          # aynı şey, argümanla
    ./parcalar 10020.txt       # doğrudan dosya adı
    seçenekler: -t SN  (zaman sınırı; dolarsa bulunan en iyi GEÇERLİ çözüm yazılır ve
                        "optimalliği ISPATLANMADI" uyarısı verilir — varsayılan: sınırsız/kesin)
                -v     (arama profili, stderr)   -q (S satırını yazma)

## Çıktı
    Z=4  X=1          <- sorudaki format
    S: 7              <- diski parçalanan bilgisayarlar (1'den başlayan numaralar)
stderr: öz-kontrol (S çıkarılınca kalan en büyük bileşen yeniden hesaplanır), "Optimallik
ispatlandı." ve süre.

X birden fazla optimal çözümde farklı olabilir; program, optimal Z'yi veren çözümler arasında
EN AZ bilgisayarı parçalayanı (en büyük Y) seçer.

## Bağımsız doğrulama
    ./parcalar 10020.txt > cikti.txt
    python3 ../tools/verify.py 10020.txt cikti.txt     # UYGUN (feasible) - tutarli

## Algoritma (özet)
K = ayakta kalan küme, f(K) = |K| − (en büyük bileşen). Z = n − max f.
K'nin en büyük bileşeni C1 "çapa" olarak sayılır (bağlantılı kümeler, en küçük köşeden kanonik);
geri kalan R, V∖N[C1] içinde bileşenleri ≤ |C1| olan en büyük küme. Truncation lemması
(en büyük bileşeni ikinci en büyüğün boyutuna kırpmak f'yi değiştirmez) sayesinde iyileşme için
aynı boyutta ikinci bir bileşen C2 aranır; |V∖N[C1]| < F olan çapalar budanır, bu da k-döngüsünü
kendiliğinden ve kesin olarak bitirir. Başlangıç değeri için MIS + yerel arama; sonuç her zaman
dal-sınır aramasıyla ispatlanır. Çapalar OpenMP ile paralel işlenir; (F,Y) hiç değişmeden biten
bir tam geçiş optimallik ispatıdır.
