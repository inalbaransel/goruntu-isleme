# Hafta 3 – Görev 3: CLAHE

> **Görev:** Düşük kontrastlı görüntüyü önce OpenCV'nin hazır CLAHE fonksiyonu ile düzeltmek, sonra CLAHE'yi hazır fonksiyon kullanmadan yazıp iki sonucu karşılaştırmak.

---

## 1. CLAHE nedir?
**C**ontrast **L**imited **A**daptive **H**istogram **E**qualization = Kontrast Sınırlı Uyarlamalı Histogram Eşitleme.

Normal histogram eşitlemenin (Görev 2) iki sorunu var. CLAHE ikisini de çözüyor:

| Sorun | CLAHE'nin çözümü |
|---|---|
| Tüm görüntüye **tek tablo** uygulanır. Bir köşe karanlık, bir köşe aydınlıksa ikisine birden uygun olmaz. | **Adaptive:** görüntü karolara bölünür, **her karoya kendi tablosu** hesaplanır. |
| Düz bölgelerde (gökyüzü, duvar) aynı tonda çok piksel olduğu için o bölgedeki **gürültü aşırı büyür**. | **Contrast Limited:** histogramdaki çok yüksek tepeler **kırpılır**, böylece kontrast artışı sınırlanır. |

## 2. Adımlar

### Adım 1: Karolara bölme
512×512 görüntü **8×8 = 64 karoya** bölünür. Her karo 64×64 pikseldir.

```
┌──┬──┬──┬──┬──┬──┬──┬──┐
│  │  │  │  │  │  │  │  │   her kutu = 1 karo (64x64)
├──┼──┼──┼──┼──┼──┼──┼──┤
│  │  │  │  │  │  │  │  │
...
```

### Adım 2: Her karonun histogramı
Hafta 2'deki histogram hesabıyla aynı, sadece o karonun pikselleri sayılır.

### Adım 3: Kırpma (Clip)
```
limit = clipLimit * karo_alanı / 256 = 2.0 * 4096 / 256 = 32
```
Bir kutuda 32'den fazla piksel varsa fazlası kesilir ve **tüm kutulara eşit dağıtılır**:

```
Önce:      █                     Sonra:
           █                     ─────── limit (32)
           █   ← fazlası kesilir  █
   ▂  ▃    █  ▂                  ▃ ▄  █ ▃     + kesilen kısım
  ▂█▂▂█▂▂▂▂█▂▂█▂               ▃█▃▃█▃▃█▃█▃   herkese eşit eklenir
```
Böylece CDF'te ani sıçrama olmaz, dolayısıyla kontrast aşırı artmaz.

Kesilen toplam 256'ya tam bölünmezse **artık** kalır. Artık da aralıklı kutulara 1'er 1'er eklenir. OpenCV bunu böyle yapıyor ve bu adım olmazsa sonuçta 10 küsur seviyelik fark oluşuyor.

### Adım 4: CDF → tablo
Görev 2'deki gibi. Kırpılmış histogramın CDF'i alınır ve her karo için 256'lık bir tablo çıkarılır:
```
tablo[k] = CDF[k] * 255 / karo_alanı
```

### Adım 5: Bilineer interpolasyon (en önemli kısım)
Her pikseli **sadece kendi karosunun** tablosuyla dönüştürseydik karo sınırlarında **kare kare** keskin geçişler görünürdü.

Çözüm: her piksel, **en yakın 4 karonun** tablosuyla ayrı ayrı dönüştürülür ve sonuçlar **mesafeye göre ağırlıklı ortalanır**. Bu, Hafta 1'deki resize'da gördüğümüz bilineer interpolasyonun aynısı.

```
  karo(y1,x1) ●───────────● karo(y1,x2)
              │     b     │
              │  ←───→ ■  │  ■ = piksel
              │       ↕ a │
  karo(y2,x1) ●───────────● karo(y2,x2)

ust  = T[y1][x1]·(1−b) + T[y1][x2]·b     (üstteki 2 karo arası)
alt  = T[y2][x1]·(1−b) + T[y2][x2]·b     (alttaki 2 karo arası)
sonuc = ust·(1−a) + alt·a                (üst ile alt arası)
```
- `a`, `b` → pikselin alttaki/sağdaki karo merkezine ne kadar yakın olduğu (0–1 arası)
- Piksel bir karonun tam merkezindeyse `a = b = 0` olur ve sadece o karonun tablosu kullanılır.

---

## 3. Kod – parça parça

### a) Hazır CLAHE (2 satır)
```cpp
cv::Ptr<cv::CLAHE> clahe = cv::createCLAHE(LIMIT, cv::Size(KARO, KARO));
clahe->apply(img, hazir);
```
- `createCLAHE(clipLimit, karo_sayısı)` → bir CLAHE "nesnesi" oluşturur.
- `cv::Ptr<...>` → OpenCV'nin akıllı işaretçisi. Belleği kendisi yönetir.
- `->apply(giriş, çıkış)` → nesnenin `apply` fonksiyonunu çağırır. İşaretçide `.` yerine `->` kullanılır.

### b) Kendi CLAHE'miz
```cpp
uchar tablo[KARO][KARO][256];   // 8x8 karo, her birinde 256'lık tablo
```
3 boyutlu dizi: `tablo[ky][kx][k]` = (ky, kx) karosunda **k** değerinin yeni hali.

```cpp
double fy = (double)y / karoH - 0.5;
int y1 = (int)std::floor(fy), y2 = y1 + 1;
double a = fy - y1;
```
- `y / karoH` → piksel kaçıncı karoda (ondalıklı). `- 0.5` ise karo **merkezine** göre konumu verir.
- `floor` → aşağı yuvarlar (−0.3 → −1). `(int)` −0.3'ü 0 yapardı, o yüzden `floor` kullanılıyor.
- Görüntünün kenarlarında y1 = −1 veya y2 = 8 çıkabilir. `if` ile 0–7 aralığına çekilir.

### Karşılaştırma
```cpp
int fark = std::abs(hazir.at<uchar>(y, x) - kendi.at<uchar>(y, x));
```
`std::abs` → mutlak değer. Her pikselde iki sonucun farkı alınır, ortalaması ve en büyüğü yazdırılır.

---

## 4. Çalıştırma
```bash
cd "Görev 3"
```
```bash
cmake --build build && ./build/gorev3
```

**Çıktı:**
```
Ortalama fark : 0
En buyuk fark : 0
```
Kendi yazdığımız CLAHE, OpenCV'ninkiyle **birebir aynı** sonucu veriyor ✔

`karsilastirma.png`: orijinal | OpenCV CLAHE | kendi CLAHE

## 5. Yorum
- CLAHE sonucu orijinalden çok daha canlı. Tüylerdeki **yerel detaylar** belirginleşmiş.
- Normal eşitlemeye (Görev 2) göre daha **doğal** görünüyor ve arka plandaki düz bölgelerde gürültü abartılmıyor.

## 6. Sınav notları
- **CL** (Contrast Limited) = histogram **kırpma**. **A** (Adaptive) = **karolara** bölme.
- `clipLimit` büyüdükçe sonuç normal histogram eşitlemeye yaklaşır. Küçüldükçe görüntü orijinale yaklaşır.
- Karo sınırlarındaki blok görüntüsü **bilineer interpolasyon** ile önlenir.
- Tipik değerler: clipLimit = 2.0, karo = 8×8.
