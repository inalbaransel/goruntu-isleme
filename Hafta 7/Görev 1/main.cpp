// Hafta 7 - Görev 1: Sinüs vektörü oluşturma ve görselleştirme
//
// sinusDoldur fonksiyonu genlik, frekans ve faz bilgisini alır ve parametre
// olarak verilen vektörü sinüs değerleri ile doldurur:
//
//     v[n] = genlik * sin(2 * pi * frekans * n / N + faz)      n = 0, 1, ..., N-1
//
//   N       : vektörün boyutu (2'nin kuvveti olmalı: 8, 16, 32, 64, ...)
//   frekans : vektör boyunca kaç tam dalga (periyot) olduğu
//   periyot : bir dalganın kaç örnek sürdüğü = N / frekans
//   faz     : dalganın başlangıç kayması (radyan)
//
// Sonra vektör iki şekilde görselleştirilir:
//   a) Vektörün kendisi: her eleman bir kutu, değeri gri tonu (siyah = -genlik, beyaz = +genlik)
//   b) Sinüzoidal dalga: örnekler (noktalar) ve gerçek sürekli sinüs eğrisi

#include <opencv2/opencv.hpp>
#include <cmath>
#include <iostream>
#include <vector>

// N'nin 2'nin kuvveti olup olmadığını kontrol eder (1, 2, 4, 8, ...)
bool ikininKuvvetiMi(int N) {
    if (N < 1) return false;
    while (N % 2 == 0) N = N / 2;
    return N == 1;
}

// Verilen vektörü sinüs değerleriyle doldurur. Vektörün boyutu 2'nin kuvveti değilse false döner.
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

int main() {
    // ================= PARAMETRELER =================
    int N = 64;                  // vektör boyutu (2^6)
    double genlik = 1.0;
    double frekans = 3.0;        // vektör boyunca 3 tam dalga
    double faz = CV_PI / 4;      // 45 derece
    double periyot = N / frekans;

    // ================= VEKTÖRÜ DOLDUR =================
    std::vector<double> v(N);
    if (!sinusDoldur(v, genlik, frekans, faz)) return 1;

    // 2'nin kuvveti olmayan boyutla deneme (hata vermeli)
    std::vector<double> yanlis(50);
    sinusDoldur(yanlis, genlik, frekans, faz);

    std::cout << "\nN = " << N << ", genlik = " << genlik << ", frekans = " << frekans
              << " (periyot = " << periyot << " ornek), faz = " << faz << " rad" << std::endl;
    std::cout << "Vektor:" << std::endl;
    for (int n = 0; n < N; n++) {
        printf("  v[%2d] = %+.4f\n", n, v[n]);
    }

    // ================= a) VEKTÖRÜN GÖRÜNTÜSÜ =================
    // Her eleman 12 piksel genişliğinde bir kutu, değeri -genlik..+genlik -> 0..255 gri ton
    int kutu = 12;
    int genislik = N * kutu;
    cv::Mat vektorResmi(60, genislik, CV_8UC3, cv::Scalar(255, 255, 255));
    for (int n = 0; n < N; n++) {
        int gri = (int)((v[n] + genlik) / (2 * genlik) * 255);
        cv::rectangle(vektorResmi, cv::Point(n * kutu, 10), cv::Point((n + 1) * kutu - 1, 50),
                      cv::Scalar(gri, gri, gri), cv::FILLED);
    }
    cv::rectangle(vektorResmi, cv::Point(0, 10), cv::Point(genislik - 1, 50), cv::Scalar(0, 0, 0), 1);

    // ================= b) SİNÜZOİDAL DALGA =================
    int yukseklik = 330;
    int orta = 30 + (yukseklik - 30) / 2;   // y = 0 çizgisi (üstte 30 piksel başlık payı)
    double olcek = ((yukseklik - 30) / 2 - 15) / genlik;
    cv::Mat grafik(yukseklik, genislik, CV_8UC3, cv::Scalar(255, 255, 255));

    // Eksenler: yatay 0 çizgisi ve her periyodun başında dikey çizgi
    cv::line(grafik, cv::Point(0, orta), cv::Point(genislik, orta), cv::Scalar(180, 180, 180), 1);
    for (double p = 0; p < N; p += periyot) {
        int x = (int)(p * kutu + kutu / 2);
        cv::line(grafik, cv::Point(x, 0), cv::Point(x, yukseklik), cv::Scalar(220, 220, 220), 1);
    }

    // Gerçek (sürekli) sinüs eğrisi: örnekler arasında da hesaplanmış ince mavi çizgi
    cv::Point onceki;
    for (int px = 0; px < genislik; px++) {
        double n = (px - kutu / 2.0) / kutu;   // piksel -> örnek indisi (ondalıklı)
        double deger = genlik * std::sin(2 * CV_PI * frekans * n / N + faz);
        cv::Point nokta(px, (int)(orta - deger * olcek));
        if (px > 0) cv::line(grafik, onceki, nokta, cv::Scalar(255, 150, 0), 1);
        onceki = nokta;
    }

    // Vektördeki örnekler: 0 çizgisinden değere dikey çizgi + kırmızı nokta
    for (int n = 0; n < N; n++) {
        int x = n * kutu + kutu / 2;
        int y = (int)(orta - v[n] * olcek);
        cv::line(grafik, cv::Point(x, orta), cv::Point(x, y), cv::Scalar(0, 0, 200), 1);
        cv::circle(grafik, cv::Point(x, y), 3, cv::Scalar(0, 0, 255), cv::FILLED);
    }

    // Başlık
    char baslik[200];
    snprintf(baslik, sizeof(baslik), "N=%d  genlik=%.1f  frekans=%.1f  periyot=%.1f  faz=%.2f rad", N, genlik,
             frekans, periyot, faz);
    cv::putText(grafik, baslik, cv::Point(10, 20), cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(0, 0, 0), 1);

    // ================= KAYDET =================
    cv::Mat sonuc;
    cv::vconcat(grafik, vektorResmi, sonuc);
    cv::imwrite("sinus_dalgasi.png", grafik);
    cv::imwrite("sinus_vektoru.png", vektorResmi);
    cv::imwrite("sinus.png", sonuc);
    std::cout << "\nKaydedildi: sinus_dalgasi.png, sinus_vektoru.png, sinus.png" << std::endl;
    std::cout << "(sinus.png: ustte dalga (kirmizi = vektordeki ornekler, mavi = surekli sinus),"
                 " altta vektor (gri ton))" << std::endl;
    return 0;
}
