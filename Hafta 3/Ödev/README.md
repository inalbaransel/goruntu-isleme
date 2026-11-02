# Hafta 3 – Ödev: Görev 1–5'in Çoklu Thread ve CUDA Sürümleri

> **Ödev:** Görev 1–5'te yapılan işlemleri (kontrast germe, histogram eşitleme, CLAHE, 3×3 ortalama, 5×5 medyan) hem **çoklu thread'li CPU** hem de **CUDA (GPU)** ile gerçeklemek ve süreleri karşılaştırmak.

> ℹ️ Bu klasördeki kod Görev 1–5'teki sade kodlardan **daha ileri seviye** yazılmış. Mantık aynı, sadece paralel hale getirilmiş. Önce Görev 1–5'in README'lerini oku, sonra burayı.

---

## 1. Dosyalar

| Dosya | Ne var |
|---|---|
| `islemler.hpp` | 5 işlemin **çoklu thread'li** CPU hali. İki program da bu dosyayı kullanıyor. |
| `main_mt.cpp` | CPU programı: her işlemi 1, 4 ve 10 thread ile çalıştırıp süreleri ölçer |
| `main_cuda.cu` | GPU programı: 5 işlemin CUDA kernel'leri. Sadece Colab'da çalışır (Mac'te NVIDIA yok). |
| `odev_colab.ipynb` | Colab'da her şeyi sırayla kuran, derleyen ve çalıştıran defter |

---

## 2. Çoklu thread fikri (Hafta 2'nin devamı)
Görüntü **yatay bantlara** bölünür ve her bandı bir thread işler. Hafta 2'deki `rowRange` mantığıyla aynı:

```
thread 0 → satır    0 –  511
thread 1 → satır  512 – 1023
thread 2 → satır 1024 – 1535
thread 3 → satır 1536 – 2047
```

`islemler.hpp`'deki `paralel(n, threadSayisi, fonk)` fonksiyonu bu bölme işini her görev için tekrar yazmamak için yapılmış. "0..n satırını thread'lere böl, her biri kendi aralığında `fonk`'u çalıştırsın" demek.

### Hangi işlem nasıl paralelleşiyor?

| İşlem | Paralel kısım | Sonradan birleştirme gerekiyor mu? |
|---|---|---|
| 1) Kontrast germe | Her thread **kendi bandının** min/max'ını bulur | ✔ 4 min'in en küçüğü, 4 max'ın en büyüğü alınır |
| 2) Histogram eşitleme | Her thread kendi bandının histogramını çıkarır | ✔ Histogramlar toplanır (Hafta 2 Görev 4'teki gibi), sonra CDF alınır |
| 3) CLAHE | Karolar thread'lere dağıtılır (her karonun tablosu bağımsız) | ✘ |
| 4) Ortalama 3×3 | Her thread kendi satırlarını filtreler | ✘ |
| 5) Medyan 5×5 | Her thread kendi satırlarını filtreler | ✘ |

**Neden kilit (mutex) yok?** Her thread **farklı satırlara** yazıyor. Ortak bir şey toplanması gereken yerlerde (min/max, histogram) de her thread **kendi kopyasına** yazıyor ve sonuçlar en sonda birleştiriliyor. Yani race condition oluşmuyor (bkz. Hafta 2 Görev 4).

---

## 3. CPU sonuçları (bu Mac, 10 çekirdek)

Süre ölçümü, farkın net görülmesi için görüntüler **2048×2048**'e büyütülerek yapıldı.

```bash
cd Ödev
```
```bash
cmake --build build && ./build/odev_mt
```

| İşlem | 1 thread | 4 thread | 10 thread | Hızlanma |
|---|---|---|---|---|
| Kontrast germe | 2.25 ms | 0.70 ms | 0.61 ms | 3.7× |
| Histogram eşitleme | 5.50 ms | 2.07 ms | 1.33 ms | 4.1× |
| CLAHE | 11.50 ms | 3.99 ms | 2.71 ms | 4.2× |
| Ortalama 3×3 | 8.57 ms | 2.17 ms | 1.79 ms | 4.8× |
| Medyan 5×5 | 753 ms | 223 ms | 94 ms | **8.1×** |

Tüm thread sayılarında sonuç 1 thread'le **birebir aynı** ve OpenCV'nin hazır fonksiyonlarıyla da **farkı 0**.

### Yorum
- **Medyan en çok hızlandı (8×).** Her pikselde 25 sayı sıralandığı için iş çok **ağır**. Ağır işte thread açma maliyeti önemsizleşir ve neredeyse çekirdek sayısı kadar hız kazanılır.
- **Kontrast germe en az hızlandı.** İş çok **hafif** (piksel başına bir tablo okuma). Bu durumda bellek hızı sınırlayıcı oluyor ve 4'ten 10 thread'e geçmek neredeyse hiç fark yaratmıyor.
- **Kural:** piksel başına iş ne kadar fazlaysa paralelleştirme o kadar kazandırır.

---

## 4. CUDA (GPU) sürümü – mantık

GPU'da binlerce küçük çekirdek var. Bu yüzden genelde **her piksele bir GPU thread'i** atanır.

### Temel CUDA kavramları
```cpp
__global__ void ortalamaKernel(...) {          // __global__ = GPU'da çalışan fonksiyon (kernel)
    int x = blockIdx.x * blockDim.x + threadIdx.x;   // bu thread hangi piksel?
    int y = blockIdx.y * blockDim.y + threadIdx.y;
    ...
}
ortalamaKernel<<<blokSayisi, blokBoyutu>>>(...);  // kernel'i binlerce thread ile başlat
```
- **Thread'ler bloklar halinde** gruplanır (ör. 16×16 = 256 thread'lik bloklar).
- `blockIdx` (kaçıncı blok) ve `threadIdx` (blok içinde kaçıncı thread) kullanılarak her thread kendi pikselini hesaplar.
- Veri akışı Hafta 1'deki gibi: `cudaMemcpy` ile **CPU → GPU**, kernel çalışır, sonra **GPU → CPU**.

### İşlemler GPU'da nasıl yapılıyor?
| İşlem | GPU'da |
|---|---|
| Kontrast germe | Paralel **indirgeme (reduction)** ile min/max bulunur, sonra her thread bir pikseli dönüştürür |
| Histogram | Her blok kendi histogramını hızlı **paylaşımlı bellekte** (shared memory) tutar ve `atomicAdd` ile sayar, sonra bloklar birleştirilir |
| CLAHE | Her karoya bir blok (histogram + kırpma + CDF), sonra her piksele bir thread (interpolasyon) |
| Ortalama / Medyan | Her piksele bir thread |

> `atomicAdd` → GPU'da binlerce thread aynı kutuyu artırabileceği için race condition'ı önleyen **bölünemez toplama** işlemi. CPU'da ayrı diziyle çözdüğümüz sorunun GPU'daki çözümü bu.

### Colab'da çalıştırma
`odev_colab.ipynb` dosyasını Colab'a yükle, **T4 GPU** seç ve hücreleri sırayla çalıştır. Program her işlem için iki süre verir:
- **kernel** süresi → sadece GPU'daki hesap
- **toplam** süre → CPU↔GPU kopyalama dahil

Hafif işlemlerde (kontrast germe) kopyalama süresi hesaptan uzun sürebilir. Bu yüzden GPU'nun **toplamda** CPU'dan yavaş çıkması normaldir. Ağır işlemlerde (medyan) ise GPU açık ara öne geçer.

> Not: CUDA sürümü bu Mac'te çalıştırılamadı (NVIDIA ekran kartı yok). Sonuçları Colab'da alman gerekiyor.

---

## 5. Sınav notları
- Paralelleştirme yaparken **bağımsız** parçalar (satırlar, karolar) seçilir.
- Ortak sonuç gerekiyorsa (histogram, min/max): CPU'da **her thread'e ayrı kopya + sonda birleştirme**, GPU'da **atomic işlemler + shared memory** kullanılır.
- Hızlanma iş yüküne bağlıdır: **ağır iş = iyi hızlanma**, hafif işte bellek ve ek maliyet baskın gelir.
- GPU'da **veri kopyalama maliyeti** unutulmamalı. Küçük/hafif işlerde GPU kazandırmayabilir.
