# Hafta 2 – Görev 6: `0.75·p + 20` ile `0.75·p` Karşılaştırması

> **Görev:** Görüntü piksel değerlerini 0.75 ile çarpıp 20 ekleme işlemi; elde edilen sonucun sadece 0.75 ile çarpılan durumla görsel/sayısal olarak karşılaştırılması.

---

## 1. Mantık
Genel formül:
```
yeni = a · eski + b
```
| Parametre | Etkisi |
|---|---|
| **a** (çarpan) | **Kontrast**. a < 1 ise görüntü sönükleşir ve kararır, a > 1 ise kontrast artar. |
| **b** (toplanan) | **Parlaklık**. Bütün görüntüyü yukarı (+) veya aşağı (−) kaydırır. |

Bu görevde:
- **A = 0.75 · p** → sadece kontrast azaltma
- **B = 0.75 · p + 20** → kontrast azaltma + parlaklık artırma

### Örnek değerler
| Orijinal | A = 0.75·p | B = 0.75·p + 20 |
|---|---|---|
| 0 | 0 | 20 |
| 100 | 75 | 95 |
| 200 | 150 | 170 |
| 255 | 191 | 211 |

---

## 2. Kod – parça parça

```cpp
for (int y = 0; y < img.rows; y++)
    for (int x = 0; x < img.cols; x++) {
        int p = img.at<uchar>(y, x);
        a.at<uchar>(y, x) = cv::saturate_cast<uchar>(0.75 * p);
        b.at<uchar>(y, x) = cv::saturate_cast<uchar>(0.75 * p + 20);
    }
```
- Her piksele iki formül de uygulanır.
- `0.75 * p` ondalıklı sonuç verir (ör. 75.75). `saturate_cast<uchar>` bunu en yakın tam sayıya yuvarlar ve 0–255 aralığında tutar.

> Kısa yolu: OpenCV'de `img.convertTo(a, -1, 0.75, 0);` ve `img.convertTo(b, -1, 0.75, 20);` aynı işi tek satırda yapar.

### Sayısal karşılaştırma
```cpp
cv::mean(img)[0]                          // ortalama parlaklık
cv::minMaxLoc(img, &enKucuk, &enBuyuk);   // en küçük ve en büyük piksel değeri
```
`&enKucuk` → "sonucu bu değişkene yaz" demek. `minMaxLoc` iki değeri birden döndürmek için bu yolu kullanır.

### Görsel karşılaştırma
```cpp
cv::hconcat(img, a, ikili);       // yan yana: orijinal | A
cv::hconcat(ikili, b, yanYana);   // yan yana: orijinal | A | B
```
`hconcat` = görüntüleri yatayda yan yana yapıştırır.

---

## 3. Çalıştırma
```bash
cd "Görev 6"
```
```bash
cmake --build build && ./build/gorev6
```

**Çıktı:**
```
Orijinal    -> ortalama: 109.43  min: 0   max: 255
0.75*p      -> ortalama: 82.05   min: 0   max: 191
0.75*p + 20 -> ortalama: 102.05  min: 20  max: 211
```
Görsel: `karsilastirma.png` (soldan sağa: orijinal | 0.75·p | 0.75·p+20)

---

## 4. Yorum

| | Ortalama | Aralık (max − min) | Görünüm |
|---|---|---|---|
| Orijinal | 109 | 255 | Normal |
| 0.75·p | 82 (↓) | 191 | **Karanlık ve sönük**. Beyazlar artık beyaz değil, gri. |
| 0.75·p + 20 | 102 (↓ ama daha az) | 191 | Orijinale daha yakın parlaklıkta ama **soluk/puslu**. Siyahlar artık siyah değil, koyu gri. |

- **Kontrast:** İki sonuçta da aralık 191 ve **aynı**. Çünkü kontrastı sadece çarpan (0.75) belirler.
- **Parlaklık:** +20 sayesinde B'nin ortalaması A'dan tam **20 fazla** (102 − 82 = 20). B'nin histogramı A'nınkinin 20 birim **sağa kaydırılmış** hali.
- **Sonuç:** 0.75 ile çarpmak görüntüyü hem karartır hem kontrastını düşürür. +20 eklemek kararmayı kısmen telafi eder ama kaybolan kontrastı geri getirmez.

## 5. Sınav notları
- `g = a·f + b` → a **kontrast (gain)**, b **parlaklık (bias)**.
- a < 1 → kontrast azalır. a > 1 → kontrast artar.
- b sadece kaydırır, kontrastı değiştirmez.
- Taşmayı engellemek için `saturate_cast` kullanılır.
