// Hafta 7 - Görev 4: DFT ile frekans uzayına dönüşüm ve harmonik vektörü
//
// Ayrık Fourier Dönüşümü (DFT), N elemanlı bir sinyali N tane harmoniğe ayırır:
//
//     X[k] = Σ (n = 0..N-1)  x[n] * e^(-j*2*pi*k*n/N)
//          = Σ x[n] * cos(2*pi*k*n/N)  -  j * Σ x[n] * sin(2*pi*k*n/N)
//
//   k      : harmonik numarası = vektör boyunca kaç tam dalga (Görev 1'deki "frekans")
//   Genlik : |X[k]| / N  (harmonik vektöründe gösterilen büyüklük)
//            Gerçel bir sinyalde X[k] ile X[N-k] eşleniktir, sinüsün enerjisi bu ikisine
//            yarı yarıya bölünür. Bu yüzden genliği A olan sinüs k'da A/2 olarak görünür
//            (A = 50 -> tepe 25).
//   Faz    : atan2(sanal, gercel)
//
// dft fonksiyonu harmonik vektörünü (her k için karmaşık X[k]) geri döndürür.
// Gürültüsüz ve gürültülü sinüs ayrı ayrı gönderilip harmonik vektörleri karşılaştırılır.

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
        cv::line(g.resim, g.nokta(n - 1, v[n - 1]), g.nokta(n, v[n]), renk, 2, cv::LINE_AA);
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
    // Hocanın örnek çıktısındaki değerler
    int N = 256;
    double genlik = 50, frekans = 4, faz = 0, oran = 0.3;

    // ================= VERİ =================
    std::vector<double> temiz(N);
    if (!sinusDoldur(temiz, genlik, frekans, faz)) return 1;
    std::vector<double> gurultulu = temiz;
    gurultuEkle(gurultulu, oran);

    // ================= DFT =================
    std::vector<std::complex<double>> Xtemiz = dft(temiz);
    std::vector<std::complex<double>> Xgurultulu = dft(gurultulu);

    // Kontrol: 2'nin kuvveti olmayan boyut
    std::vector<double> yanlis(50, 0.0);
    dft(yanlis);

    // Kontrol: OpenCV'nin hazır cv::dft sonucuyla karşılaştır (sadece doğrulama için)
    cv::Mat girdi(1, N, CV_64F, temiz.data()), opencvX;
    cv::dft(girdi, opencvX, cv::DFT_COMPLEX_OUTPUT);
    double enBuyukFark = 0;
    for (int k = 0; k < N; k++) {
        cv::Vec2d o = opencvX.at<cv::Vec2d>(0, k);
        enBuyukFark = std::max(enBuyukFark, std::abs(Xtemiz[k] - std::complex<double>(o[0], o[1])));
    }
    std::cout << "cv::dft ile en buyuk fark (kontrol): " << enBuyukFark << std::endl;

    // ================= HARMONİK VEKTÖRLERİ =================
    std::cout << "\nN = " << N << ", sinus: genlik " << genlik << ", frekans " << frekans << ", faz " << faz
              << "; gurultu orani " << oran << std::endl;
    std::cout << "\nIlk 10 harmonik (|X[k]|/N):" << std::endl;
    std::cout << " k  | Gurultusuz | Gurultulu" << std::endl;
    for (int k = 0; k < 10; k++) {
        printf("%3d | %10.4f | %9.4f\n", k, harmonikGenlik(Xtemiz, k), harmonikGenlik(Xgurultulu, k));
    }

    // Gürültülü sinyalde k = frekans dışındaki en büyük 5 harmonik
    std::vector<std::pair<double, int>> digerleri;
    for (int k = 0; k < N / 2; k++) {
        if (k != (int)frekans) digerleri.push_back({harmonikGenlik(Xgurultulu, k), k});
    }
    std::sort(digerleri.rbegin(), digerleri.rend());
    double ortalama = 0;
    for (auto& d : digerleri) ortalama += d.first;
    ortalama /= digerleri.size();
    std::cout << "\nGurultulu sinyalde k = " << frekans << " disindaki en buyuk 5 harmonik:";
    for (int i = 0; i < 5; i++) printf("  k=%d: %.2f", digerleri[i].second, digerleri[i].first);
    printf("\nk = %g disindaki harmoniklerin ortalamasi: %.3f\n", frekans, ortalama);

    // Enerji: sinyalin harmoniklere dağılımı (Parseval: Σ x[n]^2 = Σ |X[k]|^2 / N)
    double toplamT = 0, toplamG = 0;
    for (int k = 0; k < N; k++) {
        toplamT += std::norm(Xtemiz[k]) / N;
        toplamG += std::norm(Xgurultulu[k]) / N;
    }
    int k0 = (int)frekans;
    double sinyalT = (std::norm(Xtemiz[k0]) + std::norm(Xtemiz[N - k0])) / N;
    double sinyalG = (std::norm(Xgurultulu[k0]) + std::norm(Xgurultulu[N - k0])) / N;
    printf("Enerjinin k = %d harmoniginde olan kismi: gurultusuz %%%.2f, gurultulu %%%.2f\n", k0,
           100 * sinyalT / toplamT, 100 * sinyalG / toplamG);

    // ================= GRAFİKLER (2x2) =================
    double yUst = std::max(harmonikGenlik(Xtemiz, k0), harmonikGenlik(Xgurultulu, k0)) * 1.05;
    cv::Scalar mavi(255, 0, 0), kirmizi(0, 0, 255);
    char baslik[100];
    snprintf(baslik, sizeof(baslik), "Zaman/Uzam Uzayi: Gurultusuz Sinyal (Frekans=%g)", frekans);

    cv::Mat ust, alt, sonuc;
    cv::hconcat(sinyalCiz(temiz, baslik, mavi),
                spektrumCiz(Xtemiz, "Frekans Uzayi: Gurultusuz Harmonik Vektor", mavi, yUst), ust);
    cv::hconcat(sinyalCiz(gurultulu, "Zaman/Uzam Uzayi: Gurultulu Sinyal", kirmizi),
                spektrumCiz(Xgurultulu, "Frekans Uzayi: Gurultulu Harmonik Vektor", kirmizi, yUst), alt);
    cv::vconcat(ust, alt, sonuc);
    cv::imwrite("harmonikler.png", sonuc);
    std::cout << "\nKaydedildi: harmonikler.png" << std::endl;

    std::cout << "\nYorum:\n"
                 "  - Gurultusuz sinus tek bir frekanstan olusuyor: harmonik vektorunde sadece k = 4\n"
                 "    (vektor boyunca 4 tam dalga) dolu, degeri 25 = 50 / 2. Sinusun enerjisi X[k] ve\n"
                 "    X[N-k] arasinda yariya bolundugu icin genligin yarisi gorunur. Diger tum harmonikler\n"
                 "    sifir (1e-14 civari, sadece bilgisayarin yuvarlama hatasi).\n"
                 "  - Gurultulu sinyalde k = 4 hala acik ara en buyuk harmonik ve degeri 25'e cok yakin:\n"
                 "    asil sinyal frekans uzayinda kaybolmuyor.\n"
                 "  - Gurultu ise tek bir yerde toplanmiyor, TUM harmoniklere kucuk kucuk dagiliyor\n"
                 "    (k = 0 dahil). Rastgele gurultunun belirli bir frekansi yok, her frekanstan biraz\n"
                 "    icerir (beyaz gurultu).\n"
                 "  - Sonuc: zaman uzayinda sinyal ile gurultu ic ice, ama frekans uzayinda ayrisiyorlar:\n"
                 "    sinyal tek bir buyuk harmonikte, gurultu tum harmoniklere yayilmis kucuk degerlerde.\n"
                 "    Bu yuzden kucuk harmonikleri sifirlayip ters DFT alarak gurultu temizlenebilir.\n"
                 "  - Gercel bir sinyalde X[k] ile X[N-k] birbirinin eslenigi oldugu icin sadece\n"
                 "    k = 0..N/2 arasi bilgi tasir; grafikte bu yuzden k = 0..127 gosterildi.\n";
    return 0;
}
