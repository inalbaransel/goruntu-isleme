# Hafta 2 – Görev 5: Histogram – 1 Thread ile 4 Thread Süre Karşılaştırması

> **Görev:** Histogram hesaplama işleminin tek thread ve 4 thread arasındaki süre farkının karşılaştırılması.

---

## 1. Mantık
| | Nasıl |
|---|---|
| **1 thread** | Tüm görüntünün histogramı tek döngüde hesaplanır |
| **4 thread** | Görüntü 4 parçaya bölünür, her parça ayrı thread'de hesaplanır, sonra 4 histogram **toplanır** |

> Birleştirme (toplama) adımı da 4 thread'in süresine **dahil** edilir. Sonucun elde edilmesi için o adım da gerekli.

---

## 2. Kod – parça parça

### Büyütme
```cpp
cv::resize(img, img, cv::Size(4096, 4096));
```
512×512 çok küçük olduğu için fark görünmez. Bu yüzden görüntüyü büyütüyoruz (Görev 3'teki sebeple aynı).

### 1 thread
```cpp
int hist1[256] = {0};
histogramHesapla(img, hist1);           // tüm görüntü bir kerede
```

### 4 thread
```cpp
for (int i = 0; i < 4; i++)
    t[i] = std::thread(histogramHesapla, img.rowRange(i * R / 4, (i + 1) * R / 4), parcaHist[i]);
for (int i = 0; i < 4; i++) t[i].join();

for (int i = 0; i < 4; i++)                       // birleştir
    for (int k = 0; k < 256; k++)
        hist4[k] += parcaHist[i][k];
```

### Süre ölçmek
```cpp
auto bas = std::chrono::steady_clock::now();
// ...
auto son = std::chrono::steady_clock::now();
double sure = std::chrono::duration<double, std::milli>(son - bas).count();
```

### Doğrulama
```cpp
for (int k = 0; k < 256; k++)
    if (hist1[k] != hist4[k]) ayni = false;
```
Hızlı olmak yetmez, sonucun da **doğru** olması gerekir. İki yöntemin 256 kutusu birebir aynı çıkmalı.

---

## 3. Çalıştırma
```bash
cd "Görev 5"
```
```bash
cmake --build build && ./build/gorev5
```

**Örnek çıktı:**
```
1 thread : 10.78 ms
4 thread : 4.76 ms
Hizlanma : 2.26 kat
Histogramlar ayni mi? evet
```

## 4. Yorum
- 4 thread yaklaşık **2.3 kat** hızlı çıktı.
- Görev 3'teki parlaklık işleminden **daha az** hızlandı. Sebepleri:
  - Histogramda her pikselde yapılan iş çok küçük (sadece `++`). Bu durumda bellekten okuma hızı sınırlayıcı oluyor.
  - Sonda ek bir **birleştirme** adımı var.
- İki histogramın aynı çıkması paralel yöntemin doğru olduğunu gösterir.

## 5. Sınav notları
- Paralel histogram = **böl → paralel hesapla → birleştir (reduce)**.
- Birleştirme adımı ek maliyettir, ama 4×256 toplama olduğu için çok küçüktür.
- Hızlanma = süre(1 thread) / süre(4 thread).
