# Hafta 2 – Görev 2: 4 Thread ile Parlaklık

> **Görev:** Görüntünün 4 parçaya bölünerek 4 ayrı iş parçacığı (thread) ile farklı parlaklık değerlerine ayarlanması.

---

## 1. Thread nedir?
İşlemcinin birden fazla çekirdeği vardır. Normalde program **tek bir iş kolunda** çalışır ve işleri sırayla yapar.

**Thread** = aynı anda çalışan ayrı bir iş kolu.

> Benzetme: 4 sayfalık ödevi 1 kişi yazarsa 4 saat sürer. 4 kişiye birer sayfa verirsen yaklaşık 1 saatte biter.

## 2. Plan
Görüntü **4 yatay şeride** bölünür ve her şeridi bir thread işler:

```
┌────────────────┐
│ parça 0 :  -60 │ ← thread 0 (koyulaştır)
├────────────────┤
│ parça 1 :  -20 │ ← thread 1
├────────────────┤
│ parça 2 :  +20 │ ← thread 2
├────────────────┤
│ parça 3 :  +60 │ ← thread 3 (açıklaştır)
└────────────────┘
```

---

## 3. Kod – parça parça

### Parlaklık fonksiyonu
```cpp
void parlaklik(cv::Mat parca, int deger) {
    for (int y = 0; y < parca.rows; y++) {
        for (int x = 0; x < parca.cols; x++) {
            int yeniDeger = parca.at<uchar>(y, x) + deger;
            parca.at<uchar>(y, x) = cv::saturate_cast<uchar>(yeniDeger);
        }
    }
}
```
- Verilen parçanın **her pikseline** `deger` ekler.
- `void` = fonksiyon geriye bir şey döndürmez.
- **`saturate_cast` neden var?** 250 + 60 = 310 olur ama piksel en fazla 255 tutabilir. Bu fonksiyon olmasa sayı **taşar** (310 → 54) ve beyaz yer siyaha döner. `saturate_cast` sonucu 0–255 aralığına **kırpar**: 310 → 255, −30 → 0.

### Parçalara bölme
```cpp
int R = img.rows;
cv::Mat parca = img.rowRange(i * R / 4, (i + 1) * R / 4);
```
- `rowRange(bas, son)` → görüntünün `bas` ile `son` arasındaki satırları.
- Önemli: bu bir **kopya değil**, orijinal görüntünün o bölgesine açılan bir **pencere**. Parçayı değiştirmek orijinal görüntüyü de değiştirir.
- R = 512 için parçalar: 0–128, 128–256, 256–384, 384–512.

### Thread başlatma ve bekleme
```cpp
std::thread t[4];
for (int i = 0; i < 4; i++)
    t[i] = std::thread(parlaklik, parca, degerler[i]);   // başlat

for (int i = 0; i < 4; i++)
    t[i].join();                                          // bitmesini bekle
```
- `std::thread(fonksiyon, arg1, arg2)` → yeni bir thread açar ve fonksiyonu o argümanlarla çalıştırır.
- `join()` → "bu thread bitene kadar bekle". Bunu yazmazsak program thread'ler bitmeden kaydetmeye geçer (hatta çöker).

**Thread'ler neden çakışmıyor?** Her thread **farklı satırlarda** çalışıyor, yani aynı piksele iki thread hiç dokunmuyor. Bu yüzden ek bir önlem gerekmez.

---

## 4. Çalıştırma
```bash
cd "Görev 2"
```
```bash
cmake --build build && ./build/gorev2
```

**Çıktı:** `parlaklik_4_thread.png`. Görüntü yukarıdan aşağı 4 bant halinde koyudan açığa doğru gider.

## 5. Sınav notları
- Thread oluşturmak için `std::thread`, bitmesini beklemek için `join()` kullanılır.
- Parçalar çakışmazsa thread'ler aynı görüntüye güvenle yazabilir.
- `saturate_cast` taşmayı (overflow) önler.
- Mac/Linux'ta derlerken `-pthread` gerekir. CMake'te `Threads::Threads` bunu yapıyor.
