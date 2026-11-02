# Hafta 3 – Görev 4: 3×3 Ortalama Filtresi (Konvolüsyon)

> **Görev:** Gürültülü bir görüntü üzerinde 3×3 ortalama (mean) filtresini konvolüsyon ile gezdirerek gürültüyü bastırmak.

---

## 1. Gürültü nedir?
Görüntüdeki **rastgele parlaklık bozulmaları**. Bizim `gurultulu.png`'de **Gauss gürültüsü** var: her piksele rastgele küçük bir sayı (+/−) eklenmiş. Görüntü **karıncalı** görünür.

## 2. Fikir
Gürültü **rastgele** olduğu için komşu pikseller arasında bazısı yukarı, bazısı aşağı sapar. **Ortalamaları alınırsa birbirini götürür.** Gerçek görüntü ise komşular arasında genelde benzer olduğu için ortalamadan pek etkilenmez.

## 3. Konvolüsyon nedir?
Küçük bir sayı tablosunu (**çekirdek / kernel / maske**) görüntünün üzerinde piksel piksel **kaydırmak**. Her konumda:
1. Çekirdeği pikselin üstüne ortala.
2. Üst üste gelen sayıları **çarp**.
3. Hepsini **topla** → bu toplam, o pikselin yeni değeri olur.

### Ortalama çekirdeği
```
         | 1 1 1 |
K = 1/9  | 1 1 1 |
         | 1 1 1 |
```
9 komşunun her biri 1/9 ile çarpılıp toplanıyor. Bu, 9 sayının **ortalaması** demek.

### Örnek
```
Görüntüde 3x3 bölge:       Hesap:
| 100 102  98 |
| 101 200  99 |   → (100+102+98+101+200+99+97+103+100) / 9 = 1000 / 9 ≈ 111
|  97 103 100 |
```
Ortadaki **200** bir gürültü sıçraması. Filtreden sonra **111** oluyor ve komşularına yaklaşıyor.

---

## 4. Kod – parça parça

### Çekirdek
```cpp
double K[3][3] = {
    {1.0 / 9, 1.0 / 9, 1.0 / 9},
    {1.0 / 9, 1.0 / 9, 1.0 / 9},
    {1.0 / 9, 1.0 / 9, 1.0 / 9},
};
```
⚠️ `1 / 9` yazsaydık tam sayı bölmesi **0** verirdi. `1.0 / 9` ondalıklı bölme yapar (0.111...).

### Görüntüyü gezmek
```cpp
for (int y = 1; y < img.rows - 1; y++)
    for (int x = 1; x < img.cols - 1; x++)
```
Neden 0'dan değil de **1**'den başlıyor? En kenardaki pikselin (ör. y=0) üst komşusu **görüntü dışında** kalır (y=−1). Bu yüzden 1 piksellik kenar şeridi atlanıyor ve orijinal değerinde bırakılıyor.

### Çekirdeği uygulamak
```cpp
double toplam = 0;
for (int i = -1; i <= 1; i++)          // üst (−1), orta (0), alt (+1) satır
    for (int j = -1; j <= 1; j++)      // sol (−1), orta (0), sağ (+1) sütun
        toplam += K[i + 1][j + 1] * img.at<uchar>(y + i, x + j);
```
- `img.at(y + i, x + j)` → (y, x) pikselinin komşusu.
- `K[i + 1][j + 1]` → dizi indisleri 0'dan başladığı için −1..1 aralığını 0..2'ye kaydırıyoruz.

```cpp
yeni.at<uchar>(y, x) = cv::saturate_cast<uchar>(toplam);
```
Sonuç ondalıklı (ör. 111.11). `saturate_cast` bunu yuvarlar (111).

> ⚠️ Sonuç **yeni** görüntüye yazılıyor, okuma ise hep **orijinalden** yapılıyor. Aynı görüntüye yazsaydık bir sonraki piksel, önceden değiştirilmiş komşusunu okurdu ve sonuç bozulurdu.

---

## 5. Çalıştırma
```bash
cd "Görev 4"
```
```bash
cmake --build build && ./build/gorev4
```
`karsilastirma.png`: sol gürültülü, sağ filtrelenmiş.

## 6. Yorum
- Karıncalanma belirgin şekilde **azaldı**.
- **Bedeli:** görüntü biraz **bulanıklaştı**. Ortalama filtresi kenarları ve ince detayları da yumuşatır, çünkü gürültüyle gerçek detayı ayırt edemez.
- Çekirdek büyüdükçe (5×5, 7×7) gürültü daha çok azalır ama bulanıklık da artar.

## 7. Sınav notları
- Ortalama filtresi bir **alçak geçiren (low-pass)** filtredir. Yumuşatır ve bulanıklaştırır.
- Çekirdek elemanlarının toplamı **1** olmalı. Olmazsa görüntünün genel parlaklığı değişir.
- **Gauss gürültüsüne** iyi gelir. **Tuz-biber** gürültüsüne kötü gelir, çünkü 255'lik bir nokta ortalamayı çok yukarı çeker. Tuz-biber için **medyan** filtre kullanılır (Görev 5).
- Kenar işleme yöntemleri: atlamak (bizim yaptığımız), en yakın kenarı tekrarlamak (replicate), yansıtmak (reflect) veya sıfırla doldurmak (zero padding).
- OpenCV'deki hazırı: `cv::blur(img, sonuc, cv::Size(3, 3));`
