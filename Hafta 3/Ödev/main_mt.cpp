// Hafta 3 - Ödev (CPU, multi-threaded)
// Görev 1-5'in çoklu iş parçacıklı sürümleri: her işlem 1, 4 ve tüm
// çekirdek sayısı kadar thread ile çalıştırılıp süreleri karşılaştırılır.
// Tüm thread sayılarında sonucun 1 thread ile birebir aynı olduğu ve
// OpenCV'nin hazır fonksiyonlarına göre farkı kontrol edilir.
//
// Süre ölçümü, farkı belirgin görmek için görüntülerin 2048x2048'e
// büyütülmüş hali üzerinde yapılır. Çıktı resimleri orijinal boyuttadır.
//
// Kullanım: ./odev_mt [goruntu_dizini]   (varsayılan: ../goruntuler)

#include "islemler.hpp"

#include <chrono>
#include <functional>
#include <iostream>
#include <string>

using Saat = std::chrono::steady_clock;
using Islem = std::function<cv::Mat(const cv::Mat&, int)>;

double olcMs(const Islem& f, const cv::Mat& img, int thread, int tekrar) {
    f(img, thread);  // ısınma
    auto bas = Saat::now();
    for (int i = 0; i < tekrar; ++i) f(img, thread);
    return std::chrono::duration<double, std::milli>(Saat::now() - bas).count() / tekrar;
}

int main(int argc, char** argv) {
    std::string dizin = argc >= 2 ? argv[1] : "../goruntuler";
    cv::Mat dusuk = cv::imread(dizin + "/dusuk_kontrast.png", cv::IMREAD_GRAYSCALE);
    cv::Mat gurultulu = cv::imread(dizin + "/gurultulu.png", cv::IMREAD_GRAYSCALE);
    cv::Mat tuzBiber = cv::imread(dizin + "/tuz_biber.png", cv::IMREAD_GRAYSCALE);
    if (dusuk.empty() || gurultulu.empty() || tuzBiber.empty()) {
        std::cerr << "Goruntuler okunamadi (" << dizin << "/dusuk_kontrast.png, gurultulu.png, tuz_biber.png)\n";
        return 1;
    }
    cv::setNumThreads(1);  // OpenCV referans fonksiyonları kendi thread'lerini açmasın

    const int cekirdek = std::max(1u, std::thread::hardware_concurrency());
    std::vector<int> threadSayilari = {1, 4};
    if (cekirdek != 1 && cekirdek != 4) threadSayilari.push_back(cekirdek);

    cv::Ptr<cv::CLAHE> opencvClahe = cv::createCLAHE(2.0, cv::Size(8, 8));
    struct Gorev {
        std::string ad, dosya;
        cv::Mat giris;
        Islem islem;
        std::function<cv::Mat(const cv::Mat&)> opencv;  // karşılaştırma için hazır fonksiyon
    };
    std::vector<Gorev> gorevler = {
        {"1) Kontrast germe", "1_kontrast_gerilmis.png", dusuk, islem::kontrastGer,
         [](const cv::Mat& g) { cv::Mat c; cv::normalize(g, c, 0, 255, cv::NORM_MINMAX); return c; }},
        {"2) Histogram esitleme", "2_esitlenmis.png", dusuk, islem::histogramEsitle,
         [](const cv::Mat& g) { cv::Mat c; cv::equalizeHist(g, c); return c; }},
        {"3) CLAHE", "3_clahe.png", dusuk,
         [](const cv::Mat& g, int t) { return islem::clahe(g, 2.0, 8, 8, t); },
         [&](const cv::Mat& g) { cv::Mat c; opencvClahe->apply(g, c); return c; }},
        {"4) Ortalama 3x3", "4_ortalama.png", gurultulu, islem::ortalama3x3,
         [](const cv::Mat& g) { cv::Mat c; cv::blur(g, c, {3, 3}, {-1, -1}, cv::BORDER_REPLICATE); return c; }},
        {"5) Medyan 5x5", "5_medyan.png", tuzBiber, islem::medyan5x5,
         [](const cv::Mat& g) { cv::Mat c; cv::medianBlur(g, c, 5); return c; }},
    };

    std::cout << "Cekirdek sayisi: " << cekirdek << ", sure olcumu 2048x2048 goruntude\n\n";
    printf("%-24s", "Islem");
    for (int t : threadSayilari) printf(" | %2d thread (ms)", t);
    printf(" | hizlanma | ayni | OpenCV farki\n");

    for (auto& g : gorevler) {
        cv::Mat buyuk;
        cv::resize(g.giris, buyuk, cv::Size(2048, 2048));

        cv::Mat referans = g.islem(buyuk, 1);
        bool hepsiAyni = true;
        for (int t : threadSayilari) hepsiAyni &= cv::norm(referans, g.islem(buyuk, t), cv::NORM_INF) == 0;

        int tekrar = g.ad[0] == '5' ? 2 : 10;
        std::vector<double> sureler;
        for (int t : threadSayilari) sureler.push_back(olcMs(g.islem, buyuk, t, tekrar));

        cv::Mat sonuc = g.islem(g.giris, cekirdek);
        double opencvFark = cv::norm(sonuc, g.opencv(g.giris), cv::NORM_INF);
        cv::imwrite(g.dosya, sonuc);

        printf("%-24s", g.ad.c_str());
        for (double s : sureler) printf(" | %14.2f", s);
        printf(" | %7.2fx | %-4s | %.0f\n", sureler.front() / sureler.back(), hepsiAyni ? "evet" : "HAYIR",
               opencvFark);
    }
    std::cout << "\nhizlanma = 1 thread suresi / en cok thread suresi\n"
                 "ayni     = tum thread sayilarinda sonuc 1 thread ile birebir ayni mi\n"
                 "OpenCV farki = hazir OpenCV fonksiyonuyla en buyuk piksel farki (0 = birebir ayni)\n"
                 "Cikti resimleri bu dizine kaydedildi (1_... 5_...png)\n";
    return 0;
}
