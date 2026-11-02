# Hafta 3 – Görev 2: Histogram Eşitleme

> **Görev:** Düşük kontrastlı, tek kanallı görüntünün 256 değerli histogramını çıkarmak, histogramdan kümülatif dağılımı (CDF) hesaplamak ve CDF ile histogram eşitleme yapmak. Hazır fonksiyon kullanmadan yapılacak.

---

## 1. Fikir
Kontrast germe (Görev 1) histogramı sadece **genişletir**. Histogram eşitleme bir adım daha ileri gider ve pikselleri tüm aralığa **mümkün olduğunca eşit** dağıtmaya çalışır.

- **Çok piksel olan** tonlar birbirinden **uzaklaştırılır**, böylece detay artar.
- **Az piksel olan** tonlar birbirine yaklaştırılır.

## 2. Adımlar

### Adım 1: Histogram
`hist[k]` = değeri **k** olan piksel sayısı (Hafta 2'de yapmıştık).

### Adım 2: CDF (Kümülatif dağılım)
`cdf[k]` = değeri **k veya daha küçük** olan piksel sayısı. Yani histogramın birikimli toplamı:

```
hist:  [ 2,  5,  3,  0,  4 ]
cdf :  [ 2,  7, 10, 10, 14 ]      cdf[k] = cdf[k-1] + hist[k]
```
CDF hep artar ve son elemanı **toplam piksel sayısına (N)** eşittir.

### Adım 3: Dönüşüm formülü
```
yeni(k) = (cdf[k] - cdf_min) * 255 / (N - cdf_min)
```
- `cdf[k] / N` → "pikselllerin yüzde kaçı k'dan koyu?" sorusunun cevabı
- Bu oran 255 ile çarpılır. Örneğin piksellerin %50'si k'dan koyuysa, k yeni görüntüde **127** olur.
- `cdf_min` → sıfır olmayan ilk CDF değeri. Bunu çıkarınca en koyu piksel tam **0**'a gelir.

### Adım 4: Tablo ile uygulama
Formül her piksel için ayrı ayrı hesaplanmaz. Önce 256 değerlik bir **tablo** hazırlanır (`tablo[eski] = yeni`), sonra her piksel bu tablodan okunur. 262.144 piksel yerine sadece 256 kez hesap yapılmış olur.

---

## 3. Kod – parça parça

```cpp
int hist[256] = {0};
hist[img.at<uchar>(y, x)]++;                  // 1) histogram
```
```cpp
cdf[0] = hist[0];
for (int k = 1; k < 256; k++)
    cdf[k] = cdf[k - 1] + hist[k];            // 2) CDF
```
```cpp
for (int k = 0; k < 256; k++)
    if (cdf[k] > 0) { cdfMin = cdf[k]; break; }   // ilk sıfır olmayan
```
`break` → döngüyü hemen bitirir. İlk değeri bulduk, devam etmeye gerek yok.

```cpp
double oran = (double)(cdf[k] - cdfMin) / (N - cdfMin);
tablo[k] = cv::saturate_cast<uchar>(oran * 255);   // 3) tablo
```
`(double)` → tam sayıyı ondalıklıya çevirir. Bunu yazmasak tam sayı bölmesi sonucu hep 0 veya 1 olurdu.

```cpp
yeni.at<uchar>(y, x) = tablo[img.at<uchar>(y, x)];   // 4) uygula
```

`histogramCiz` fonksiyonu sadece görselleştirme için. Hafta 2 Görev 4'teki çizimle aynı.

---

## 4. Çalıştırma
```bash
cd "Görev 2"
```
```bash
cmake --build build && ./build/gorev2
```

**Çıktı dosyaları:**
- `karsilastirma.png` → sol orijinal, sağ eşitlenmiş
- `histogramlar.png` → sol eşitlemeden önceki histogram, sağ sonraki

## 5. Yorum
- **Önce:** histogram 80–169 arasına sıkışmış, dar bir tepe şeklinde.
- **Sonra:** histogram **0–255'in tamamına** yayılmış. Çubuklar arasında **boşluklar** var. Bunun sebebi 90 civarı farklı tonun 256'lık aralığa dağıtılması. Ton sayısı artmaz, sadece aralarındaki mesafe açılır.
- Görüntü çok daha canlı ve kontrastlı görünüyor.

## 6. Sınav notları
- **CDF** = histogramın kümülatif toplamı. Son değeri N'dir.
- Eşitleme **doğrusal değildir**, çünkü dönüşüm CDF'in şekline bağlıdır.
- Eşitleme **global** bir işlemdir: tüm görüntüye tek bir tablo uygulanır. Bu yüzden bazı bölgelerde gürültüyü fazla artırabilir. Çözümü CLAHE (Görev 3).
- Çıkan histogram tam düz olmaz, sadece "daha düz" olur.
