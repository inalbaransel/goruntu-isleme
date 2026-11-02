// Hafta 7 - Görev 5: İki gürültülü sinyalin DFT analizi ve toplamları
//
//   Sinyal 1: frekans 50, genlik 12, gürültü eklenmiş
//   Sinyal 2: frekans 30, genlik  5, gürültü eklenmiş
//
// Adımlar:
//   1) İki sinyal ayrı ayrı oluşturulur, gürültü eklenir ve ayrı ayrı DFT'si alınır
//   2) İki sinyal toplanır: toplam[n] = sinyal1[n] + sinyal2[n]
//   3) Toplam sinyal ve onun DFT'si (harmonik vektörü) görselleştirilir
//
// Beklenen: DFT doğrusaldır, DFT(a + b) = DFT(a) + DFT(b). Toplam sinyalin harmonik
// vektöründe iki tepe görülür: k = 50'de 12/2 = 6 ve k = 30'da 5/2 = 2.5
// (Görev 4'teki gibi harmonik vektöründe |X[k]|/N gösterilir, genliğin yarısı).
//
// Görev 1, 2 ve 4'teki fonksiyonlar (sinusDoldur, gurultuEkle, dft) ve çizim
// yardımcıları aynen kullanılmıştır.

#include <opencv2/opencv.hpp>
#include <algorithm>
#include <cmath>
#include <complex>
#include <iostream>
#include <random>
#include <vector>

// ---- Görev 1 ve 2'deki fonksiyonlar ----
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

std::mt19937 rastgele(42);

bool gurultuEkle(std::vector<double>& v, double oran) {
    if (oran < 0 || oran > 1) {
        std::cout << "Hata: gurultu orani [0, 1] araliginda olmali (verilen: " << oran << ")" << std::endl;
        return false;
    }
    double genlik = 0;
    for (size_t n = 0; n < v.size(); n++) {
        if (std::abs(v[n]) > genlik) genlik = std::abs(v[n]);
    }
    double gurultuGenligi = oran * genlik;
    std::uniform_real_distribution<double> dagilim(-gurultuGenligi, gurultuGenligi);
    for (size_t n = 0; n < v.size(); n++) {
        v[n] += dagilim(rastgele);
    }
    return true;
}

// ---- Görev 4: DFT ----
// Harmonik vektörünü döndürür: sonuc[k] = X[k], k = 0..N-1.
// Vektör boyutu 2'nin kuvveti değilse boş vektör döner.
std::vector<std::complex<double>> dft(const std::vector<double>& x) {
    int N = (int)x.size();
    if (!ikininKuvvetiMi(N)) {
        std::cout << "Hata: DFT icin vektor boyutu (" << N << ") 2'nin kuvveti olmali!" << std::endl;
        return {};
    }

    // cos ve sin değerlerini baştan hesapla: cos(2*pi*m/N), m = 0..N-1.
    // k*n çarpımı ne kadar büyük olursa olsun açı (k*n) mod N ile aynıdır (periyodik).
    std::vector<double> cosTablo(N), sinTablo(N);
    for (int m = 0; m < N; m++) {
        cosTablo[m] = std::cos(2 * CV_PI * m / N);
        sinTablo[m] = std::sin(2 * CV_PI * m / N);
    }

    std::vector<std::complex<double>> X(N);
    for (int k = 0; k < N; k++) {             // her harmonik için
        double gercel = 0, sanal = 0;
        for (int n = 0; n < N; n++) {         // sinyalin tüm elemanlarıyla çarpıp topla
            int m = (k * n) % N;
            gercel += x[n] * cosTablo[m];
            sanal -= x[n] * sinTablo[m];
        }
        X[k] = std::complex<double>(gercel, sanal);
    }
    return X;
}

// Harmonik vektöründeki büyüklük: |X[k]| / N
double harmonikGenlik(const std::vector<std::complex<double>>& X, int k) {
    return std::abs(X[k]) / X.size();
}

// ---- Çizim yardımcıları (eksenli grafik) ----
struct Grafik {
    cv::Mat resim;
    double xMin, xMax, yMin, yMax;
    int sol = 70, sag = 20, ust = 40, alt = 50;

    // Veri koordinatı -> piksel
    cv::Point nokta(double x, double y) const {
        int w = resim.cols - sol - sag, h = resim.rows - ust - alt;
        return cv::Point(sol + (int)std::round((x - xMin) / (xMax - xMin) * w),
                         ust + (int)std::round((yMax - y) / (yMax - yMin) * h));
    }
};

// Eksen aralığına göre okunaklı bir çizgi aralığı seç (1, 2, 5, 10, 20, 50, ...)
double adimSec(double aralik) {
    double adim = 1;
    while (aralik / adim > 8) {
        if (aralik / (adim * 2) <= 8) return adim * 2;
        if (aralik / (adim * 5) <= 8) return adim * 5;
        adim *= 10;
    }
    return adim;
}

Grafik eksenCiz(const std::string& baslik, double xMin, double xMax, double yMin, double yMax,
                const std::string& xEtiket, const std::string& yEtiket) {
    Grafik g;
    g.resim = cv::Mat(440, 680, CV_8UC3, cv::Scalar(255, 255, 255));
    g.xMin = xMin; g.xMax = xMax; g.yMin = yMin; g.yMax = yMax;
    cv::Scalar siyah(0, 0, 0), gri(90, 90, 90);
    auto yazi = [&](const std::string& s, cv::Point p, double boy, cv::Scalar renk) {
        cv::putText(g.resim, s, p, cv::FONT_HERSHEY_SIMPLEX, boy, renk, 1, cv::LINE_AA);
    };

    // Eksen işaretleri ve sayılar
    double xAdim = adimSec(xMax - xMin), yAdim = adimSec(yMax - yMin);
    for (double x = std::ceil(xMin / xAdim) * xAdim; x <= xMax; x += xAdim) {
        cv::Point p = g.nokta(x, yMin);
        cv::line(g.resim, p, p + cv::Point(0, 5), siyah, 1);
        std::string s = std::to_string((int)std::round(x));
        yazi(s, p + cv::Point(-4 * (int)s.size(), 20), 0.42, gri);
    }
    for (double y = std::ceil(yMin / yAdim) * yAdim; y <= yMax; y += yAdim) {
        cv::Point p = g.nokta(xMin, y);
        cv::line(g.resim, p, p - cv::Point(5, 0), siyah, 1);
        std::string s = std::to_string((int)std::round(y));
        yazi(s, p + cv::Point(-10 - 9 * (int)s.size(), 5), 0.42, gri);
    }
    // Çerçeve
    cv::rectangle(g.resim, g.nokta(xMin, yMax), g.nokta(xMax, yMin), siyah, 1);

    // Başlık ve eksen adları
    int tabanCizgi = 0;
    cv::Size bs = cv::getTextSize(baslik, cv::FONT_HERSHEY_SIMPLEX, 0.55, 1, &tabanCizgi);
    yazi(baslik, cv::Point((g.resim.cols - bs.width) / 2, 25), 0.55, siyah);
    if (!xEtiket.empty()) {
        cv::Size xs = cv::getTextSize(xEtiket, cv::FONT_HERSHEY_SIMPLEX, 0.45, 1, &tabanCizgi);
        yazi(xEtiket, cv::Point(g.sol + (g.resim.cols - g.sol - g.sag - xs.width) / 2, g.resim.rows - 12), 0.45,
             siyah);
    }
    if (!yEtiket.empty()) yazi(yEtiket, cv::Point(5, g.ust - 8), 0.42, siyah);
    return g;
}

// Zaman uzayı: sinyal sürekli çizgi olarak
cv::Mat sinyalCiz(const std::vector<double>& v, const std::string& baslik, cv::Scalar renk) {
    double enKucuk = *std::min_element(v.begin(), v.end());
    double enBuyuk = *std::max_element(v.begin(), v.end());
    double pay = (enBuyuk - enKucuk) * 0.05;
    int N = (int)v.size();
    Grafik g = eksenCiz(baslik, -N * 0.04, (N - 1) + N * 0.04, enKucuk - pay, enBuyuk + pay, "", "");
    for (int n = 1; n < N; n++) {
        cv::line(g.resim, g.nokta(n - 1, v[n - 1]), g.nokta(n, v[n]), renk, 1, cv::LINE_AA);
    }
    return g.resim;
}

// Frekans uzayı: k = 0..N/2-1 için |X[k]|/N, stem (dikey çizgi + nokta) grafiği
cv::Mat spektrumCiz(const std::vector<std::complex<double>>& X, const std::string& baslik, cv::Scalar renk,
                    double yUst) {
    int K = (int)X.size() / 2;
    Grafik g = eksenCiz(baslik, -K * 0.05, (K - 1) + K * 0.05, -yUst * 0.04, yUst, "Frekans (k)", "Genlik |X[k]|/N");
    for (int k = 0; k < K; k++) {
        double a = harmonikGenlik(X, k);
        cv::line(g.resim, g.nokta(k, 0), g.nokta(k, a), renk, 1, cv::LINE_AA);
        cv::circle(g.resim, g.nokta(k, a), 4, renk, cv::FILLED, cv::LINE_AA);
    }
    return g.resim;
}


int main() {
    int N = 256;              // 2'nin kuvveti; k = 0..127 gösterilebilir (50 ve 30 sığar)
    double oran = 0.3;        // her sinyale kendi genliğinin %30'u kadar gürültü

    // ================= 1) İKİ SİNYAL + GÜRÜLTÜ =================
    std::vector<double> sinyal1(N), sinyal2(N);
    if (!sinusDoldur(sinyal1, 12, 50, 0)) return 1;   // genlik 12, frekans 50
    if (!sinusDoldur(sinyal2, 5, 30, 0)) return 1;    // genlik 5, frekans 30
    gurultuEkle(sinyal1, oran);
    gurultuEkle(sinyal2, oran);

    // Ayrı ayrı DFT
    std::vector<std::complex<double>> X1 = dft(sinyal1);
    std::vector<std::complex<double>> X2 = dft(sinyal2);

    // ================= 2) TOPLAM SİNYAL =================
    std::vector<double> toplam(N);
    for (int n = 0; n < N; n++) {
        toplam[n] = sinyal1[n] + sinyal2[n];
    }
    std::vector<std::complex<double>> Xtoplam = dft(toplam);

    // ================= SONUÇLAR =================
    std::cout << "N = " << N << ", gurultu orani = " << oran << std::endl;
    std::cout << "\n                         k = 30     k = 50    diger harmoniklerin ortalamasi" << std::endl;
    auto satir = [&](const char* ad, const std::vector<std::complex<double>>& X) {
        double digerToplam = 0;
        for (int k = 0; k < N / 2; k++) {
            if (k != 30 && k != 50) digerToplam += harmonikGenlik(X, k);
        }
        printf("%-22s %8.3f   %8.3f    %8.3f\n", ad, harmonikGenlik(X, 30), harmonikGenlik(X, 50),
               digerToplam / (N / 2 - 2));
    };
    satir("Sinyal 1 (f=50, A=12)", X1);
    satir("Sinyal 2 (f=30, A=5)", X2);
    satir("Toplam sinyal", Xtoplam);
    std::cout << "Beklenen tepe degerleri: k = 50 -> 12/2 = 6,  k = 30 -> 5/2 = 2.5" << std::endl;

    // Doğrusallık kontrolü: DFT(toplam) ile DFT(sinyal1) + DFT(sinyal2) aynı mı?
    double enBuyukFark = 0;
    for (int k = 0; k < N; k++) {
        enBuyukFark = std::max(enBuyukFark, std::abs(Xtoplam[k] - (X1[k] + X2[k])));
    }
    std::cout << "\n|DFT(toplam) - (DFT(s1) + DFT(s2))| en buyuk fark: " << enBuyukFark
              << "  (DFT dogrusal)" << std::endl;

    // Toplam sinyalin harmonik vektöründeki en büyük 4 değer
    std::vector<std::pair<double, int>> sirali;
    for (int k = 0; k < N / 2; k++) sirali.push_back({harmonikGenlik(Xtoplam, k), k});
    std::sort(sirali.rbegin(), sirali.rend());
    std::cout << "Toplam sinyalde en buyuk 4 harmonik:";
    for (int i = 0; i < 4; i++) printf("  k=%d: %.2f", sirali[i].second, sirali[i].first);
    std::cout << std::endl;

    // ================= 3) GÖRSELLEŞTİRME (3 satır x 2 sütun) =================
    // Tüm harmonik grafikleri aynı y ölçeğinde: farkı karşılaştırmak kolay olsun
    double yUst = sirali[0].first * 1.1;
    cv::Scalar mavi(255, 0, 0), yesil(0, 150, 0), mor(160, 0, 160);

    cv::Mat satir1, satir2, satir3, sonuc;
    cv::hconcat(sinyalCiz(sinyal1, "Sinyal 1: Frekans=50, Genlik=12 (gurultulu)", mavi),
                spektrumCiz(X1, "Sinyal 1: Harmonik Vektor", mavi, yUst), satir1);
    cv::hconcat(sinyalCiz(sinyal2, "Sinyal 2: Frekans=30, Genlik=5 (gurultulu)", yesil),
                spektrumCiz(X2, "Sinyal 2: Harmonik Vektor", yesil, yUst), satir2);
    cv::hconcat(sinyalCiz(toplam, "Toplam Sinyal (Sinyal 1 + Sinyal 2)", mor),
                spektrumCiz(Xtoplam, "Toplam Sinyal: Harmonik Vektor", mor, yUst), satir3);
    cv::vconcat(std::vector<cv::Mat>{satir1, satir2, satir3}, sonuc);
    cv::imwrite("iki_sinyal_dft.png", sonuc);

    // Sadece toplam sinyal ve harmonik vektörü (görevin istediği asıl grafik)
    cv::imwrite("toplam_sinyal_dft.png", satir3);
    std::cout << "\nKaydedildi: iki_sinyal_dft.png (3 satir: sinyal 1, sinyal 2, toplam),"
                 " toplam_sinyal_dft.png" << std::endl;

    std::cout << "\nYorum:\n"
                 "  - Her sinyalin harmonik vektorunde tek bir tepe var: sinyal 1'de k = 50 (yaklasik 6),\n"
                 "    sinyal 2'de k = 30 (yaklasik 2.5). Tepe yuksekligi genligin yarisi (12/2, 5/2).\n"
                 "    Gurultu her ikisinde de tum harmoniklere kucuk degerler olarak yayiliyor.\n"
                 "  - Toplam sinyal zaman uzayinda karmasik gorunuyor; iki dalganin ic ice gectigi ve\n"
                 "    hangi frekanslardan olustugu gozle anlasilmiyor.\n"
                 "  - Frekans uzayinda ise iki bilesen net olarak ayrisiyor: k = 30 ve k = 50'de iki ayri\n"
                 "    tepe, yukseklikleri de genliklerle orantili. Yani DFT, karisik bir sinyalin hangi\n"
                 "    frekanslardan ve hangi genliklerle olustugunu ortaya cikariyor.\n"
                 "  - DFT dogrusal: toplam sinyalin DFT'si, iki sinyalin DFT'lerinin toplamina esit\n"
                 "    (fark sadece yuvarlama hatasi). Gurultuler de toplandigi icin toplam sinyalin\n"
                 "    gurultu tabani iki sinyalinkinden biraz yuksek.\n";
    return 0;
}
