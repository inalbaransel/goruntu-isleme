# Hafta 3 – Görev 1: Kontrast Germe

> **Görev:** Düşük kontrastlı, tek kanallı görüntünün minimum ve maksimum değerlerini bulmak, bu değerlerle parlaklığı [0, 255] aralığına doğrusal olarak ölçeklemek. Hazır fonksiyon (`normalize`, `minMaxLoc` vb.) kullanmadan yapılacak.

---

## 1. Kontrast nedir?
Kontrast = görüntüdeki **en koyu ile en açık** ton arasındaki fark.

Bizim görüntümüzde pikseller sadece **80 ile 169** arasında. 0–255 aralığının yaklaşık üçte birini kullanıyor. Bu yüzden görüntü **gri ve sönük** görünür: gerçek siyah da yok, gerçek beyaz da.

```
Önce:   0 ......... [80 ███████ 169] ......... 255     (dar aralık)
Sonra:  [0 ████████████████████████████████████ 255]   (tam aralık)
```

## 2. Formül
```
yeni = (p - min) * 255 / (max - min)
```
- `p - min` → en koyu piksel 0'a gelir
- `/ (max - min)` → değer 0 ile 1 arasına iner
- `* 255` → 0 ile 255 arasına yayılır

| p | hesap | yeni |
|---|---|---|
| 80 (min) | (80−80)·255/89 | **0** |
| 124 | (124−80)·255/89 | **126** |
| 169 (max) | (169−80)·255/89 | **255** |

> ⚠️ Kodda **önce çarpma, sonra bölme** yapılıyor. Tam sayı bölmesinde `(p−min)/(max−min)` hep 0 çıkardı (ör. 44/89 = 0), bu yüzden çarpma önce gelmeli.

---

## 3. Kod – parça parça

### 1) Min ve max bulmak
```cpp
int enKucuk = 255;   // başlangıçta en büyük olası değer
int enBuyuk = 0;     // başlangıçta en küçük olası değer
...
if (p < enKucuk) enKucuk = p;
if (p > enBuyuk) enBuyuk = p;
```
Bu klasik "en küçüğü bul" yöntemi. Başlangıç değeri olarak **ters uç** veriliyor (min için 255). Böylece ilk gelen piksel mutlaka onu günceller. Sonra tüm pikseller gezilir ve daha küçüğü (büyüğü) görülünce değer güncellenir.

### 2) Formülü uygulamak
```cpp
yeni.at<uchar>(y, x) = (p - enKucuk) * 255 / (enBuyuk - enKucuk);
```

### 3) Kontrol
Aynı min/max döngüsü yeni görüntüde tekrar çalıştırılır. Sonuç **0 ve 255** çıkmalı.

---

## 4. Çalıştırma
```bash
cd "Görev 1"
```
```bash
cmake --build build && ./build/gorev1
```

**Çıktı:**
```
Once  -> min: 80  max: 169
Sonra -> min: 0  max: 255
```
`karsilastirma.png`: sol orijinal (sönük), sağ gerilmiş (canlı).

## 5. Sınav notları
- Kontrast germe = **doğrusal** dönüşüm. Histogramın şekli değişmez, sadece **genişler**.
- Görüntüde tek bir 0 ve tek bir 255 piksel varsa (ör. gürültü) germe **hiç işe yaramaz**, çünkü min=0 ve max=255 olur. Bu durumda histogram eşitleme (Görev 2) daha etkilidir.
- max = min ise (tek renk görüntü) sıfıra bölme olur.
