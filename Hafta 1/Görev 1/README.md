# Hafta 1 – Görev 1

> **Görev:**
> 1. Google Colab üzerinde CUDA destekli OpenCV kurulumu
> 2. C++ ile bir klasördeki görüntülerin okunması ve **1024×768** boyutuna getirilmesi

---

## 1. Temel kavramlar

### Görüntü nedir?
Bilgisayar için görüntü **sayılardan oluşan bir tablodur (matris)**. Tablonun her hücresi bir **piksel**dir.

```
Gri 4x3 görüntü:
[  0,  50, 100, 255 ]     0   = siyah
[ 30,  80, 120, 200 ]     255 = beyaz
[ 10,  60, 140, 250 ]     arası = gri tonlar
```

- Gri görüntüde her piksel **1 sayıdır** (0–255).
- Renkli görüntüde her piksel **3 sayıdır**. OpenCV bunları RGB değil **BGR** (Mavi, Yeşil, Kırmızı) sırasıyla tutar.
- **Çözünürlük** = genişlik × yükseklik. 1024×768 → yatayda 1024, dikeyde 768 piksel.

### "Görüntüyü okumak" ne demek?
Diskteki `.jpg` veya `.png` dosyası **sıkıştırılmış** veridir. Okumak bu sıkıştırmayı çözüp piksel tablosunu RAM'e almaktır:

```
kedi.jpg (diskte, sıkıştırılmış)  --cv::imread-->  cv::Mat (RAM'de piksel tablosu)
cv::Mat (RAM'de)                  --cv::imwrite--> yeni.jpg (diske kaydet)
```

### Yeniden boyutlandırma nasıl olur?
Yeni görüntünün her pikseli için "bu nokta eski görüntüde nereye denk geliyor?" diye bakılır:

```
ölçek = eski_genişlik / yeni_genişlik     (ör: 2000 / 1024 ≈ 1.95)
yeni (10,10) pikseli → eski görüntüde (19.5, 19.5) noktası
```

19.5 diye bir piksel yok. Bu ara değeri tahmin etmeye **interpolasyon** denir:

| Yöntem | Nasıl çalışır | Ne zaman |
|---|---|---|
| `INTER_NEAREST` | En yakın pikseli alır | En hızlısı, kenarlar tırtıklı olur |
| `INTER_LINEAR` (varsayılan) | Çevredeki 4 pikselin ağırlıklı ortalamasını alır | Genel kullanım |
| `INTER_AREA` | Kaynakta düşen alanın ortalamasını alır | Küçültürken en iyisi |
| `INTER_CUBIC` | Çevredeki 16 pikseli kullanır | Büyütürken daha kaliteli |

> ⚠️ 1024×768 oranı **4:3**. 16:9 bir fotoğraf bu boyuta zorlanınca **basık/uzamış** görünür.

---

## 2. Kod (main.cpp) – parça parça

```cpp
#include <filesystem>          // klasör işlemleri
#include <iostream>            // ekrana yazma
#include <opencv2/opencv.hpp>  // OpenCV
namespace fs = std::filesystem; // kısa isim
```
`#include`, Python'daki `import` gibi hazır kod ekler. `::` "içindeki" anlamına gelir: `cv::imread` = "OpenCV'nin içindeki imread".

```cpp
fs::create_directory("output");
```
Çıktı klasörünü oluşturur. Klasör zaten varsa hiçbir şey yapmaz.

```cpp
for (const auto &dosya : fs::directory_iterator("input")) {
```
"`input` klasöründeki **her dosya için** şunları yap" demek. Python'daki `for dosya in os.listdir("input")` ile aynı.

```cpp
cv::Mat img = cv::imread(dosya.path().string());   // 1) OKU
if (img.empty()) continue;                          //    resim değilse atla
```
`cv::Mat` görüntüyü tutan kutudur. `empty()` okuma başarısız olduysa (ör. `.txt` dosyası) `true` döner. `continue` "bu dosyayı bırak, sıradakine geç" demek.

```cpp
cv::Mat yeni;
cv::resize(img, yeni, cv::Size(1024, 768));         // 2) BOYUTLANDIR
```
`resize(kaynak, hedef, boyut)` kullanılır. ⚠️ `cv::Size(genişlik, yükseklik)` sırasına dikkat et.

```cpp
cv::imwrite("output/" + dosya.path().filename().string(), yeni);  // 3) KAYDET
```
`filename()` sadece dosyanın adını verir (`kedi.jpg`). `"output/" + ...` ile yol `output/kedi.jpg` olur.

**Özet:** klasörü gez → her dosyayı **oku → boyutlandır → kaydet**.

---

## 3. Çalıştırma (Mac, CPU)

1. `input` klasörü oluştur ve içine resimleri koy.
2. Derle ve çalıştır:

```bash
cmake -S . -B build && cmake --build build
```
```bash
./build/resize
```

Sonuçlar `output/` klasörüne yazılır.

> Mac'te NVIDIA ekran kartı olmadığı için **CUDA çalışmaz**. CUDA kısmı Colab'da yapılır.

---

## 4. Colab'da CUDA destekli OpenCV

Adım adım hali `hafta1_gorev1_colab.ipynb` dosyasında var. Mantığı şöyle:

1. **Çalışma zamanı → Çalışma zamanı türünü değiştir → T4 GPU** seç.
2. `pip` ile gelen OpenCV CUDA desteği **olmadan** derlenmiş. Ayrıca CUDA modülleri `opencv_contrib` içinde. Bu yüzden OpenCV **kaynak koddan derlenir**:
   - `opencv` ve `opencv_contrib` git ile indirilir.
   - `cmake` ayarlarında en önemli satırlar:
     - `WITH_CUDA=ON` → CUDA desteğini açar
     - `OPENCV_EXTRA_MODULES_PATH=.../opencv_contrib/modules` → CUDA modüllerini ekler
     - `CUDA_ARCH_BIN=7.5` → T4 ekran kartının mimarisi
   - `make` ile derlenir, `make install` ile kurulur.
3. Doğrulama: `cv2.cuda.getCudaEnabledDeviceCount()` → **1** çıkmalı.

### GPU'da resize
```cpp
cv::cuda::GpuMat g1, g2;
g1.upload(img);                               // RAM → GPU
cv::cuda::resize(g1, g2, cv::Size(1024, 768)); // GPU'da işle
g2.download(yeni);                            // GPU → RAM
```
`cv::Mat` RAM'de, `cv::cuda::GpuMat` ekran kartı belleğinde durur. Tek bir küçük resimde GPU, kopyalama maliyeti yüzünden CPU'dan **yavaş** bile olabilir.

---

## 5. Sınav notları
- Görüntü = piksel matrisi. Renkli görüntü 3 kanallıdır ve sırası **BGR**'dir.
- `imread` → oku (decode), `imwrite` → kaydet (encode). Okumadan sonra mutlaka `empty()` kontrolü yapılır.
- `cv::Size(genişlik, yükseklik)` ama `img.at(satır, sütun)`. Sıralar terstir.
- Küçültmede `INTER_AREA`, büyütmede `INTER_CUBIC`/`INTER_LINEAR` tercih edilir.
- CUDA modülleri **opencv_contrib**'de bulunur ve `WITH_CUDA=ON` ile derlenmesi gerekir.
