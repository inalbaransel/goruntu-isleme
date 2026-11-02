# Hafta 2 – Görev 4: 4 Thread ile Histogram

> **Görev:** Görüntüyü 4 parçaya bölerek her parçanın histogramının 4 thread ile paralel hesaplanması ve birleştirilerek ana histogramın elde edilmesi.

---

## 1. Histogram nedir?
"Görüntüde **her parlaklık değerinden kaç piksel var?**" sorusunun cevabıdır. 256 kutulu bir sayaç gibi düşün:

```
hist[0]   = 1200   → 1200 tane tam siyah piksel
hist[128] = 5400   → 5400 tane orta gri piksel
hist[255] = 300    → 300 tane tam beyaz piksel
```

**Hesaplama:** her piksele bakılır ve değeri neyse o kutu 1 artırılır:
```cpp
hist[piksel_degeri]++;      // ++ = 1 artır
```

## 2. Plan
1. Görüntüyü 4 parçaya böl.
2. Her thread **kendi parçasının** histogramını **kendi dizisine** yazsın.
3. 4 histogramı **topla** → ana histogram.

### ⚠️ Neden her thread'e ayrı dizi?
4 thread aynı diziye yazarsa, iki thread aynı anda `hist[100]++` yaptığında biri diğerinin artırmasını ezebilir:

```
hist[100] = 50
Thread A okur: 50          Thread B okur: 50
Thread A yazar: 51         Thread B yazar: 51    ← 52 olmalıydı! Bir sayım kayboldu.
```
Buna **race condition (yarış durumu)** denir. Her thread'e ayrı dizi verince sorun tamamen ortadan kalkar.

---

## 3. Kod – parça parça

### Histogram fonksiyonu
```cpp
void histogramHesapla(cv::Mat parca, int* hist) {
    for (int y = 0; y < parca.rows; y++)
        for (int x = 0; x < parca.cols; x++) {
            int deger = parca.at<uchar>(y, x);
            hist[deger]++;
        }
}
```
`int* hist` → "diziyi kopyalama, **kendisini** ver" demek. Fonksiyonun diziye yazdığı her şey dışarıdaki asıl diziye işler.

### Diziler
```cpp
int parcaHist[4][256] = {0};   // 4 satır x 256 sütun, hepsi 0. Her thread bir satır kullanır.
int anaHist[256] = {0};        // birleşmiş histogram
```

### 4 thread
```cpp
for (int i = 0; i < 4; i++) {
    cv::Mat parca = img.rowRange(i * R / 4, (i + 1) * R / 4);
    t[i] = std::thread(histogramHesapla, parca, parcaHist[i]);
}
for (int i = 0; i < 4; i++) t[i].join();
```

### Birleştirme
```cpp
for (int i = 0; i < 4; i++)
    for (int k = 0; k < 256; k++)
        anaHist[k] += parcaHist[i][k];
```
Ana histogramdaki her kutu, 4 parçadaki aynı kutuların toplamıdır.

### Kontrol
Histogramdaki tüm sayıların toplamı **görüntüdeki piksel sayısına** eşit olmalı. Her piksel tam bir kez sayıldığı için.

### Çizim
```cpp
cv::Mat grafik(300, 256, CV_8UC1, cv::Scalar(255));   // 300 yüksek, 256 geniş, beyaz tuval
int boy = anaHist[k] * 300 / enBuyuk;                 // en büyük değer 300 piksel boyunda olsun
cv::line(grafik, cv::Point(k, 300), cv::Point(k, 300 - boy), cv::Scalar(0));  // alttan yukarı siyah çizgi
```
Her k değeri için bir dikey çizgi çizilir. Çizginin boyu o değerdeki piksel sayısıyla orantılıdır.

---

## 4. Çalıştırma
```bash
cd "Görev 4"
```
```bash
cmake --build build && ./build/gorev4
```

**Çıktı:**
```
Histogram toplami : 262144
Piksel sayisi     : 262144      ← 512 x 512, ikisi eşit ✔
Kaydedildi: ana_histogram.png
```

## 5. Yorum
- `ana_histogram.png` grafiğinde solda (koyu tonlarda) bir tepe var, çünkü baykuşun koyu tüyleri baskın.
- En sağdaki (255) uzun çubuk **beyaz arka plan**dan geliyor.
- Toplamın piksel sayısına eşit çıkması, paralel hesabın doğru olduğunu kanıtlar.

## 6. Sınav notları
- Histogram = her parlaklık değerinin frekansı (256 kutu).
- Paralel histogram: **böl → her parçayı ayrı hesapla → topla**.
- Aynı diziye birden fazla thread yazarsa **race condition** oluşur. Çözüm: her thread'e ayrı dizi (veya mutex/atomic kullanmak, ama bunlar yavaştır).
