// Hafta 7 - Görev 2: Vektöre gürültü ekleme ve grafikle gösterme
//
// gurultuEkle fonksiyonu bir vektör ve [0, 1] arasında bir gürültü oranı alır.
// Vektördeki genliğe (en büyük mutlak değer) ve orana göre her elemana
// rastgele gürültü ekler:
//
//     genlik          = max |v[n]|
//     gurultuGenligi  = oran * genlik
//     v[n]            = v[n] + rastgele(-gurultuGenligi, +gurultuGenligi)
//
//   oran = 0   -> hiç gürültü yok
//   oran = 0.5 -> her eleman en fazla genliğin yarısı kadar sapabilir
//   oran = 1   -> gürültü sinyal kadar büyük olabilir
//
// Test için Görev 1'deki sinüs vektörü kullanılır; farklı oranlar için
// orijinal ve gürültülü değerler alt alta grafiklerde gösterilir.

#include <opencv2/opencv.hpp>
#include <cmath>
#include <iostream>
#include <random>
#include <vector>

// ---- Görev 1'deki fonksiyonlar ----
bool ikininKuvvetiMi(int N) {
    if (N < 1) return false;
    while (N % 2 == 0) N = N / 2;
    return N == 1;
}

bool sinusDoldur(std::vector<double>& v, double genlik, double frekans, double faz) {
    int N = (int)v.size();
    if (!ikininKuvvetiMi(N)) {
        std::cout << "Hata: vektor boyutu (" << N << ") 2'nin kuvveti degil!" << std::endl;
        return false;
    }
    for (int n = 0; n < N; n++) {
        v[n] = genlik * std::sin(2 * CV_PI * frekans * n / N + faz);
    }
    return true;
}

// ---- Görev 2: gürültü ekleme ----
std::mt19937 rastgele(42);   // sabit tohum: her çalıştırmada aynı gürültü (karşılaştırması kolay)

bool gurultuEkle(std::vector<double>& v, double oran) {
    if (oran < 0 || oran > 1) {
        std::cout << "Hata: gurultu orani [0, 1] araliginda olmali (verilen: " << oran << ")" << std::endl;
        return false;
    }

    // 1) Vektörün genliği: en büyük mutlak değer
    double genlik = 0;
    for (size_t n = 0; n < v.size(); n++) {
        if (std::abs(v[n]) > genlik) genlik = std::abs(v[n]);
    }

    // 2) Her elemana -oran*genlik ile +oran*genlik arasında rastgele bir değer ekle
    double gurultuGenligi = oran * genlik;
    std::uniform_real_distribution<double> dagilim(-gurultuGenligi, gurultuGenligi);
    for (size_t n = 0; n < v.size(); n++) {
        v[n] += dagilim(rastgele);
    }
    return true;
}

// Orijinal ve gürültülü vektörü tek grafikte çizer
cv::Mat grafikCiz(const std::vector<double>& orijinal, const std::vector<double>& gurultulu, double oran,
                  double eksenSiniri) {
    int N = (int)orijinal.size();
    int kutu = 12;
    int genislik = N * kutu, yukseklik = 260;
    int orta = 30 + (yukseklik - 30) / 2;
    double olcek = ((yukseklik - 30) / 2 - 10) / eksenSiniri;
    cv::Mat grafik(yukseklik, genislik, CV_8UC3, cv::Scalar(255, 255, 255));

    cv::line(grafik, cv::Point(0, orta), cv::Point(genislik, orta), cv::Scalar(180, 180, 180), 1);

    for (int n = 0; n < N; n++) {
        int x = n * kutu + kutu / 2;
        cv::Point o(x, (int)(orta - orijinal[n] * olcek));
        cv::Point g(x, (int)(orta - gurultulu[n] * olcek));
        if (n > 0) {
            int xOnceki = (n - 1) * kutu + kutu / 2;
            cv::line(grafik, cv::Point(xOnceki, (int)(orta - orijinal[n - 1] * olcek)), o,
                     cv::Scalar(255, 150, 0), 2);                                     // orijinal: MAVİ
            cv::line(grafik, cv::Point(xOnceki, (int)(orta - gurultulu[n - 1] * olcek)), g,
                     cv::Scalar(150, 150, 255), 1);                                   // gürültülü: açık kırmızı
        }
        cv::circle(grafik, g, 3, cv::Scalar(0, 0, 255), cv::FILLED);                 // gürültülü örnek: KIRMIZI
    }

    char baslik[100];
    snprintf(baslik, sizeof(baslik), "gurultu orani = %.2f   (mavi: orijinal, kirmizi: gurultulu)", oran);
    cv::putText(grafik, baslik, cv::Point(10, 20), cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(0, 0, 0), 1);
    cv::line(grafik, cv::Point(0, yukseklik - 1), cv::Point(genislik, yukseklik - 1), cv::Scalar(0, 0, 0), 1);
    return grafik;
}

int main() {
    int N = 64;
    std::vector<double> sinus(N);
    if (!sinusDoldur(sinus, 1.0, 3.0, CV_PI / 4)) return 1;

    // Geçersiz oran denemesi (hata vermeli)
    std::vector<double> deneme = sinus;
    gurultuEkle(deneme, 1.5);

    // Farklı oranlar için gürültü ekle ve çiz
    double oranlar[] = {0.0, 0.1, 0.3, 0.6, 1.0};
    std::vector<cv::Mat> grafikler;
    std::cout << "\nOran   Ort. mutlak sapma   En buyuk sapma" << std::endl;
    for (double oran : oranlar) {
        std::vector<double> gurultulu = sinus;   // orijinali bozmamak için kopya
        gurultuEkle(gurultulu, oran);

        double toplamSapma = 0, enBuyukSapma = 0;
        for (int n = 0; n < N; n++) {
            double sapma = std::abs(gurultulu[n] - sinus[n]);
            toplamSapma += sapma;
            if (sapma > enBuyukSapma) enBuyukSapma = sapma;
        }
        printf("%.2f   %17.4f   %14.4f\n", oran, toplamSapma / N, enBuyukSapma);

        // Tüm grafikler aynı ölçekte olsun: en büyük oran 1 -> değerler en fazla +-2
        grafikler.push_back(grafikCiz(sinus, gurultulu, oran, 2.0));
    }

    // İlk birkaç değer (oran = 0.3 için)
    std::vector<double> ornek = sinus;
    rastgele.seed(7);
    gurultuEkle(ornek, 0.3);
    std::cout << "\nOrnek (oran = 0.3):" << std::endl;
    for (int n = 0; n < 6; n++) {
        printf("  v[%d]: %+.4f -> %+.4f\n", n, sinus[n], ornek[n]);
    }

    cv::Mat sonuc;
    cv::vconcat(grafikler, sonuc);
    cv::imwrite("gurultulu_sinus.png", sonuc);
    std::cout << "\nKaydedildi: gurultulu_sinus.png (her satir farkli bir gurultu orani)" << std::endl;
    return 0;
}
