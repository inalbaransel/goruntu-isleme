# Hafta 3 – Görev 5: Medyan Filtre (Tuz-Biber Gürültüsü)

> **Görev:** Salt-and-Pepper (tuz-biber) gürültülü görüntüdeki gürültüyü medyan filtre ile bastırmak:
> a) OpenCV'nin hazır medyan filtre fonksiyonu ile
> b) Hazır fonksiyon kullanmadan 5×5 medyan filtre yazarak

---

## 1. Tuz-biber gürültüsü nedir?
Rastgele piksellerin tamamen **beyaz (255 = tuz)** ya da tamamen **siyah (0 = biber)** olması. Görüntü üzerine tuz ve karabiber serpilmiş gibi görünür. Bizim görüntümüzde piksellerin %5'i tuz, %5'i biber.

## 2. Neden ortalama filtresi işe yaramaz?
```
Komşular: 100 102 98 101 255 99 97 103 100
Ortalama : 1055 / 9 ≈ 117     ← 255 ortalamayı yukarı çekti, gri bir leke kaldı
Medyan   : 100                ← 255'in hiç etkisi yok
```
Ortalama, uç değerlerden çok etkilenir. **Medyan (ortanca)** ise etkilenmez.

## 3. Medyan filtre nasıl çalışır?
Her piksel için:
1. Etrafındaki **5×5 = 25** pikseli al.
2. Küçükten büyüğe **sırala**.
3. **Ortadakini** (13. değeri) al → bu pikselin yeni değeri.

```
Sıralı 25 değer:
[0, 0, 95, 96, 97, 98, 98, 99, 99, 100, 100, 100, |101|, 101, 102, 102, 103, 103, 104, 105, 106, 107, 108, 255, 255]
                                                     ↑
                                              13. eleman = 101
```
0'lar en başa, 255'ler en sona gider. Ortaya neredeyse hiç gelemezler.

---

## 4. Kod – parça parça

### a) Hazır medyan (1 satır)
```cpp
cv::medianBlur(img, hazir, 5);    // 5 = pencere boyutu (5x5)
```

### b) Kendi medyan filtremiz
```cpp
for (int y = 2; y < img.rows - 2; y++)
    for (int x = 2; x < img.cols - 2; x++)
```
5×5 pencerenin yarıçapı 2 olduğu için kenardaki **2 piksellik** şerit atlanıyor. Yoksa pencere görüntü dışına taşardı.

```cpp
int pencere[25];
int n = 0;
for (int i = -2; i <= 2; i++)
    for (int j = -2; j <= 2; j++) {
        pencere[n] = img.at<uchar>(y + i, x + j);
        n++;
    }
```
25 komşu sırayla diziye konuluyor. `n` bir sonraki boş yerin numarasını tutuyor.

```cpp
std::sort(pencere, pencere + 25);    // küçükten büyüğe sırala
kendi.at<uchar>(y, x) = pencere[12]; // ortanca (indis 0'dan başladığı için 13. eleman = [12])
```
- `std::sort(baş, son)` C++'ın standart sıralama fonksiyonu. OpenCV'ye ait değil, ama hoca "sıralamayı da kendin yaz" derse bunun yerine kabarcık sıralaması (bubble sort) yazılabilir.
- `pencere + 25` → dizinin bittiği yer.

### Karşılaştırma
```cpp
if (hazir.at<uchar>(y, x) != kendi.at<uchar>(y, x)) farkli++;
```
Kenarlar hariç (orada iki yöntem kenarı farklı işliyor) kaç pikselin farklı olduğunu sayıyor. `!=` "eşit değil" demek.

---

## 5. Çalıştırma
```bash
cd "Görev 5"
```
```bash
cmake --build build && ./build/gorev5
```

**Çıktı:**
```
Kenarlar haric farkli piksel sayisi: 0
```
Kendi filtremiz OpenCV ile **birebir aynı** sonucu veriyor ✔

`karsilastirma.png`: gürültülü | OpenCV medyan | kendi medyan

## 6. Yorum
- Tuz-biber noktaları **neredeyse tamamen** kayboldu.
- Ortalama filtresinden farklı olarak **kenarlar keskin kalır**, çünkü medyan her zaman komşulardan **gerçek bir değer** seçer ve yeni ara değer üretmez.
- Dezavantajı: sıralama yüzünden **yavaştır** (her pikselde 25 sayı sıralanıyor). 5×5 pencere ince detayları (ör. tüy uçları) biraz yok edebilir.

## 7. Sınav notları
- Medyan filtre **doğrusal olmayan (non-linear)** bir filtredir. Konvolüsyonla ifade edilemez.
- **Tuz-biber** gürültüsü için en iyi seçimdir. Gauss gürültüsü için ortalama/Gauss filtresi daha uygundur.
- Kenarları korur (edge-preserving).
- Pencere boyutu tek sayı olmalı (3, 5, 7) ki tek bir merkez ve tek bir ortanca olsun.
