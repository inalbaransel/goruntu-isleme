// Hafta 3 - Görev 3: CLAHE
// a) OpenCV'nin hazır CLAHE fonksiyonu
// b) CLAHE'yi kendimiz yazıyoruz
// sonra iki sonucu karşılaştırıyoruz.
//
// CLAHE adımları:
//   1) Görüntüyü 8x8 = 64 karoya böl
//   2) Her karonun histogramını çıkar
//   3) Histogramı limitte kırp, taşan kısmı tüm kutulara eşit dağıt
//   4) Kırpılmış histogramın CDF'inden karo için bir tablo yap
//   5) Her pikseli en yakın 4 karonun tablosuyla hesapla ve ağırlıklı ortala
//      (bilineer interpolasyon -> karo sınırlarında kare kare görüntü oluşmaz)

#include <opencv2/opencv.hpp>
#include <cmath>
#include <iostream>

const int KARO = 8;          // 8x8 karo
const double LIMIT = 2.0;    // kırpma limiti (clipLimit)

uchar tablo[KARO][KARO][256];  // her karo için bir dönüşüm tablosu

int main() {
    cv::Mat img = cv::imread("../goruntuler/dusuk_kontrast.png", cv::IMREAD_GRAYSCALE);
    if (img.empty()) {
        std::cout << "Goruntu okunamadi!" << std::endl;
        return 1;
    }

    // ================= a) HAZIR CLAHE =================
    cv::Ptr<cv::CLAHE> clahe = cv::createCLAHE(LIMIT, cv::Size(KARO, KARO));
    cv::Mat hazir;
    clahe->apply(img, hazir);

    // ================= b) KENDİ CLAHE'MİZ =================
    int karoW = img.cols / KARO;            // bir karonun genişliği (512/8 = 64)
    int karoH = img.rows / KARO;            // bir karonun yüksekliği
    int alan = karoW * karoH;               // bir karodaki piksel sayısı
    int limit = (int)(LIMIT * alan / 256);  // bir kutuya izin verilen en fazla piksel

    // ---- Adım 1-4: her karo için tablo hazırla ----
    for (int ky = 0; ky < KARO; ky++) {
        for (int kx = 0; kx < KARO; kx++) {

            // 2) Bu karonun histogramı
            int hist[256] = {0};
            for (int y = ky * karoH; y < (ky + 1) * karoH; y++) {
                for (int x = kx * karoW; x < (kx + 1) * karoW; x++) {
                    hist[img.at<uchar>(y, x)]++;
                }
            }

            // 3) Kırp ve taşanı eşit dağıt
            int tasan = 0;
            for (int k = 0; k < 256; k++) {
                if (hist[k] > limit) {
                    tasan += hist[k] - limit;
                    hist[k] = limit;
                }
            }
            for (int k = 0; k < 256; k++) {
                hist[k] += tasan / 256;              // herkese eşit pay
            }
            int kalan = tasan % 256;                 // eşit bölünemeyen artık
            if (kalan > 0) {
                int adim = 256 / kalan;              // artığı aralıklı kutulara 1'er 1'er ver
                for (int k = 0; kalan > 0; k += adim) {
                    hist[k]++;
                    kalan--;
                }
            }

            // 4) CDF -> tablo (normal histogram eşitlemedeki gibi)
            int toplam = 0;
            for (int k = 0; k < 256; k++) {
                toplam += hist[k];
                tablo[ky][kx][k] = cv::saturate_cast<uchar>(toplam * 255.0 / alan);
            }
        }
    }

    // ---- Adım 5: her pikseli 4 komşu karonun tablosuyla hesapla ----
    cv::Mat kendi = img.clone();
    for (int y = 0; y < img.rows; y++) {
        for (int x = 0; x < img.cols; x++) {
            int p = img.at<uchar>(y, x);

            // Piksel, karo merkezlerine göre nerede? (ör. 2.3 = 2. ve 3. karo arası)
            double fy = (double)y / karoH - 0.5;
            double fx = (double)x / karoW - 0.5;

            int y1 = (int)std::floor(fy), y2 = y1 + 1;   // üst ve alt karo
            int x1 = (int)std::floor(fx), x2 = x1 + 1;   // sol ve sağ karo
            double a = fy - y1;                          // alttakine yakınlık (0..1)
            double b = fx - x1;                          // sağdakine yakınlık (0..1)

            // Kenarlarda karo dışına çıkmasın
            if (y1 < 0) y1 = 0;
            if (x1 < 0) x1 = 0;
            if (y2 > KARO - 1) y2 = KARO - 1;
            if (x2 > KARO - 1) x2 = KARO - 1;

            // 4 karonun tablosundan gelen değerlerin ağırlıklı ortalaması
            double ust = tablo[y1][x1][p] * (1 - b) + tablo[y1][x2][p] * b;
            double alt = tablo[y2][x1][p] * (1 - b) + tablo[y2][x2][p] * b;
            kendi.at<uchar>(y, x) = cv::saturate_cast<uchar>(ust * (1 - a) + alt * a);
        }
    }

    // ================= KARŞILAŞTIRMA =================
    // İki sonuç arasındaki ortalama ve en büyük piksel farkı
    double toplamFark = 0;
    int enBuyukFark = 0;
    for (int y = 0; y < img.rows; y++) {
        for (int x = 0; x < img.cols; x++) {
            int fark = std::abs(hazir.at<uchar>(y, x) - kendi.at<uchar>(y, x));
            toplamFark += fark;
            if (fark > enBuyukFark) enBuyukFark = fark;
        }
    }
    std::cout << "Ortalama fark : " << toplamFark / (img.rows * img.cols) << std::endl;
    std::cout << "En buyuk fark : " << enBuyukFark << std::endl;

    // Kaydet: orijinal | hazır CLAHE | kendi CLAHE
    cv::Mat ikili, yanYana;
    cv::hconcat(img, hazir, ikili);
    cv::hconcat(ikili, kendi, yanYana);
    cv::imwrite("clahe_opencv.png", hazir);
    cv::imwrite("clahe_kendi.png", kendi);
    cv::imwrite("karsilastirma.png", yanYana);
    std::cout << "Kaydedildi: clahe_opencv.png, clahe_kendi.png, karsilastirma.png" << std::endl;
    return 0;
}
