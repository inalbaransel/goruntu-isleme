# Hafta 2 – Görev 3: Parlaklık – 1 Thread ile 4 Thread Süre Karşılaştırması

> **Görev:** Parlaklık değiştirme işleminin tek iş parçacığı (1 thread) ile çoklu iş parçacığı (4 thread) arasındaki çalışma süresi farkının karşılaştırılması.

---

## 1. Mantık
Aynı iş (Görev 2'deki 4 bantlık parlaklık ayarı) iki şekilde yapılır ve süreler ölçülür:

| | Nasıl |
|---|---|
| **1 thread** | 4 parçayı **sırayla**, tek tek işle |
| **4 thread** | 4 parçayı **aynı anda**, her birini ayrı thread'de işle |

---

## 2. Kod – parça parça

### Görüntüyü büyütmek
```cpp
cv::resize(img, img, cv::Size(4096, 4096));
```
Bizim görüntümüz 512×512 ve çok küçük. İş o kadar kısa sürüyor ki thread açmanın maliyeti kazancı yiyor. Farkı net görmek için görüntüyü 4096×4096'ya (64 kat daha fazla piksel) büyütüyoruz.

### Süre ölçmek (kronometre)
```cpp
auto bas = std::chrono::steady_clock::now();   // başlat
// ... ölçülecek iş ...
auto son = std::chrono::steady_clock::now();   // durdur
double sure = std::chrono::duration<double, std::milli>(son - bas).count();  // ms
```
Kalıp hep aynı, ezberlemek yeterli. `auto` = tipi derleyici kendisi bulur.

### 1 thread
```cpp
for (int i = 0; i < 4; i++)
    parlaklik(tek.rowRange(i * R / 4, (i + 1) * R / 4), degerler[i]);
```
Fonksiyonu kendimiz 4 kere, **sırayla** çağırıyoruz.

### 4 thread
```cpp
for (int i = 0; i < 4; i++)
    t[i] = std::thread(parlaklik, cok.rowRange(i * R / 4, (i + 1) * R / 4), degerler[i]);
for (int i = 0; i < 4; i++)
    t[i].join();
```
Aynı 4 çağrıyı 4 thread'e veriyoruz ve hepsi **aynı anda** çalışıyor.

> `tek` ve `cok` ayrı ayrı `clone()` ile kopyalandı. Böylece iki yöntem de aynı orijinal görüntüden başlıyor ve karşılaştırma adil oluyor.

---

## 3. Çalıştırma
```bash
cd "Görev 3"
```
```bash
cmake --build build && ./build/gorev3
```

**Örnek çıktı (10 çekirdekli Mac):**
```
1 thread : 9.76 ms
4 thread : 3.27 ms
Hizlanma : 2.98 kat
```
Süreler her çalıştırmada biraz değişir. Bu normal, çünkü bilgisayar o sırada başka işler de yapıyor.

## 4. Yorum
- 4 thread yaklaşık **3 kat** hızlı çıktı.
- **Neden tam 4 kat değil?**
  - Thread oluşturmak ve `join` ile beklemek ek zaman alır.
  - 4 thread aynı belleğe erişir ve bellek hızı sınırlayıcı olur.
  - En son biten thread toplam süreyi belirler.
- Küçük görüntüde (512×512) 4 thread **daha yavaş** bile çıkabilir. İş çok kısayken thread açma maliyeti baskın gelir.

## 5. Sınav notları
- Teorik maksimum hızlanma = thread sayısı (4). Pratikte daha düşüktür.
- Paralellik **büyük veride** işe yarar. Küçük veride ek maliyet (overhead) kazancı yer.
- Süre ölçümü için `std::chrono::steady_clock` kullanılır.
