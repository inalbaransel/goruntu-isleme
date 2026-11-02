# Hafta 2 – Görev 1: Nicemleme (8-bit → 6-bit)

> **Görev:** Tek kanallı (gri seviye) görüntü üzerinde 8-bit'ten 6-bit'e parlaklık nicemleme (quantization) işlemi.

---

## 1. Mantık

| | Bit | Seviye sayısı | Değerler |
|---|---|---|---|
| Önce | 8 | 2⁸ = **256** | 0, 1, 2, ..., 255 |
| Sonra | 6 | 2⁶ = **64** | 0, 4, 8, ..., 252 |

256 / 64 = **4**. Yani yan yana her 4 değer tek bir değerde birleşir:

```
0, 1, 2, 3        → 0
4, 5, 6, 7        → 4
...
252, 253, 254, 255 → 252
```

### Formül
```
yeni = (eski / 4) * 4
```
C++'ta tam sayı bölmesi **küsuratı atar**:
```
203 / 4 = 50     (50.75 değil)
50  * 4 = 200
```
Böylece 200, 201, 202 ve 203'ün hepsi 200 olur. **Nicemleme** = değerleri daha az basamağa yuvarlamak.

---

## 2. Kod – parça parça

```cpp
cv::Mat img = cv::imread("../goruntu.jpg", cv::IMREAD_GRAYSCALE);
```
Görüntüyü **gri (tek kanal)** olarak okur. Görev "tek kanallı" dediği için `IMREAD_GRAYSCALE` şart.

```cpp
cv::Mat yeni = img.clone();
```
`clone()` görüntünün **gerçek bir kopyasını** alır. Sonucu buna yazacağız, orijinal bozulmayacak.

```cpp
for (int y = 0; y < img.rows; y++) {          // her satır (yukarıdan aşağı)
    for (int x = 0; x < img.cols; x++) {      // her sütun (soldan sağa)
        int eski = img.at<uchar>(y, x);
        yeni.at<uchar>(y, x) = (eski / 4) * 4;
    }
}
```
- İç içe iki döngü **tüm pikselleri tek tek** gezer. Kitap okur gibi: satır satır, soldan sağa.
- `img.at<uchar>(y, x)` → y. satır, x. sütundaki piksel. `uchar` = 0–255 arası sayı.

```cpp
cv::imwrite("gri_6bit.png", yeni);
```
Neden **PNG**? JPG kayıplı sıkıştırma yaptığı için kaydederken ara değerleri geri getirir ve nicemleme bozulur. PNG kayıpsızdır.

---

## 3. Çalıştırma

```bash
cd "Görev 1"
```
```bash
cmake --build build && ./build/gorev1
```

(`build` klasörü yoksa önce `cmake -S . -B build` çalıştır.)

**Çıktı:**
```
Ornek: 203 -> 200
Kaydedildi: gri_8bit.png, gri_6bit.png
```

---

## 4. Sonuç / yorum
- `gri_8bit.png` ile `gri_6bit.png` arasındaki fark **gözle neredeyse görülmez**, çünkü insan gözü 64 gri tonu ile 256 gri tonunu zor ayırır.
- Bit sayısı çok düşürülürse fark belirginleşir. Örneğin 2-bit için formül `(eski / 64) * 64` olur ve sadece 4 ton kalır. O zaman yumuşak geçişlerde **basamaklar** oluşur. Bunun adı **false contouring (sahte kontur)**.
- Avantajı: daha az bit = **daha az bellek**. 8 → 6 bit %25 tasarruf sağlar.

## 5. Sınav notları
- n bit → 2ⁿ seviye.
- 8 → 6 bit: bölme katsayısı 2^(8−6) = 4.
- Alternatif yazım: `(p >> 2) << 2`. Sağa 2 bit kaydırmak 4'e bölmek demek, sola 2 bit kaydırmak 4 ile çarpmak.
- Aşırı nicemleme → sahte kontur.
