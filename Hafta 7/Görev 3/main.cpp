// Hafta 7 - Görev 3: Gürültülü veriyi Gauss ve Medyan filtrelerle temizleme
//
// Görev 2'deki gürültülü sinüs vektörü sırasıyla şu filtrelerden geçirilir:
//   a) Gauss filtresi  : her eleman, komşularının Gauss ağırlıklı ortalaması olur
//                        (merkeze yakın komşu daha çok, uzak komşu daha az etkiler)
//   b) Medyan filtresi : her eleman, penceredeki değerlerin ortancası olur
//   c) Gauss + Medyan  : önce Gauss, sonra Medyan (iki filtre art arda)
// Her filtre için temiz sinyale göre hata (ortalama mutlak hata, MSE) hesaplanır,
// sonuçlar grafiklerle gösterilir.
//
// Kenarlarda pencere vektör dışına taşarsa en yakın kenar elemanı kullanılır.

#include <opencv2/opencv.hpp>
#include <algorithm>
#include <cmath>
#include <iostream>
#include <random>
#include <vector>

const int PENCERE = 5;        // filtre penceresi (tek sayı): her elemanın 2 sol + 2 sağ komşusu
const double SIGMA = 1.0;     // Gauss filtresinin standart sapması

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

// ---- Görev 3: filtreler ----

// Kenarda pencere dışına taşan indisi en yakın kenara çeker
int sinirla(int i, int N) {
    if (i < 0) return 0;
    if (i > N - 1) return N - 1;
    return i;
}

// 1 boyutlu Gauss filtresi
std::vector<double> gaussFiltre(const std::vector<double>& v, int pencere, double sigma) {
    int N = (int)v.size(), yaricap = pencere / 2;

    // Çekirdek: k[i] = exp(-i^2 / (2*sigma^2)), toplamı 1 olacak şekilde normalize
    std::vector<double> cekirdek(pencere);
    double toplam = 0;
    for (int i = -yaricap; i <= yaricap; i++) {
        cekirdek[i + yaricap] = std::exp(-(i * i) / (2 * sigma * sigma));
        toplam += cekirdek[i + yaricap];
    }
    for (int i = 0; i < pencere; i++) cekirdek[i] /= toplam;

    // Konvolüsyon: her eleman = komşuların ağırlıklı toplamı
    std::vector<double> sonuc(N);
    for (int n = 0; n < N; n++) {
        double s = 0;
        for (int i = -yaricap; i <= yaricap; i++) {
            s += cekirdek[i + yaricap] * v[sinirla(n + i, N)];
        }
        sonuc[n] = s;
    }
    return sonuc;
}

// 1 boyutlu medyan filtresi
std::vector<double> medyanFiltre(const std::vector<double>& v, int pencere) {
    int N = (int)v.size(), yaricap = pencere / 2;
    std::vector<double> sonuc(N);
    for (int n = 0; n < N; n++) {
        std::vector<double> komsular;
        for (int i = -yaricap; i <= yaricap; i++) {
            komsular.push_back(v[sinirla(n + i, N)]);
        }
        std::sort(komsular.begin(), komsular.end());
        sonuc[n] = komsular[yaricap];   // ortanca eleman
    }
    return sonuc;
}

// Temiz sinyale göre hata: ortalama mutlak hata ve ortalama kare hata (MSE)
void hataHesapla(const std::vector<double>& temiz, const std::vector<double>& v, double& mae, double& mse) {
    mae = 0;
    mse = 0;
    for (size_t n = 0; n < v.size(); n++) {
        double fark = v[n] - temiz[n];
        mae += std::abs(fark);
        mse += fark * fark;
    }
    mae /= v.size();
    mse /= v.size();
}

// Temiz sinyal (mavi) ve verilen vektör (renkli) tek grafikte
cv::Mat grafikCiz(const std::vector<double>& temiz, const std::vector<double>& v, const std::string& baslik,
                  cv::Scalar renk) {
    int N = (int)v.size();
    int kutu = 12, genislik = N * kutu, yukseklik = 240;
    int orta = 30 + (yukseklik - 30) / 2;
    double olcek = ((yukseklik - 30) / 2 - 10) / 1.8;   // tüm grafikler aynı ölçekte (+-1.8)
    cv::Mat grafik(yukseklik, genislik, CV_8UC3, cv::Scalar(255, 255, 255));
    cv::line(grafik, cv::Point(0, orta), cv::Point(genislik, orta), cv::Scalar(200, 200, 200), 1);

    for (int n = 1; n < N; n++) {
        int x1 = (n - 1) * kutu + kutu / 2, x2 = n * kutu + kutu / 2;
        cv::line(grafik, cv::Point(x1, (int)(orta - temiz[n - 1] * olcek)),
                 cv::Point(x2, (int)(orta - temiz[n] * olcek)), cv::Scalar(255, 170, 0), 2);   // temiz: MAVİ
        cv::line(grafik, cv::Point(x1, (int)(orta - v[n - 1] * olcek)),
                 cv::Point(x2, (int)(orta - v[n] * olcek)), renk, 1);
    }
    for (int n = 0; n < N; n++) {
        cv::circle(grafik, cv::Point(n * kutu + kutu / 2, (int)(orta - v[n] * olcek)), 3, renk, cv::FILLED);
    }
    cv::putText(grafik, baslik, cv::Point(10, 20), cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(0, 0, 0), 1);
    cv::line(grafik, cv::Point(0, yukseklik - 1), cv::Point(genislik, yukseklik - 1), cv::Scalar(0, 0, 0), 1);
    return grafik;
}

int main() {
    int N = 64;
    double oran = 0.6;

    // ================= VERİ (Görev 1 + Görev 2) =================
    std::vector<double> temiz(N);
    if (!sinusDoldur(temiz, 1.0, 3.0, CV_PI / 4)) return 1;
    std::vector<double> gurultulu = temiz;
    gurultuEkle(gurultulu, oran);

    // ================= FİLTRELER =================
    std::vector<double> gauss = gaussFiltre(gurultulu, PENCERE, SIGMA);
    std::vector<double> medyan = medyanFiltre(gurultulu, PENCERE);
    std::vector<double> ikisi = medyanFiltre(gauss, PENCERE);   // önce Gauss, sonra Medyan

    // ================= SONUÇLAR =================
    std::cout << "N = " << N << ", gurultu orani = " << oran << ", pencere = " << PENCERE
              << ", Gauss sigma = " << SIGMA << std::endl;
    std::cout << "\nGauss cekirdegi: ";
    {
        // Çekirdeği göstermek için tek bir 1'i (impuls) filtreden geçir
        std::vector<double> impuls(PENCERE * 3, 0.0);
        impuls[PENCERE * 3 / 2] = 1.0;
        std::vector<double> k = gaussFiltre(impuls, PENCERE, SIGMA);
        for (int i = PENCERE * 3 / 2 - PENCERE / 2; i <= PENCERE * 3 / 2 + PENCERE / 2; i++) printf("%.4f ", k[i]);
    }
    std::cout << std::endl;

    double mae, mse;
    std::cout << "\nVeri               Ort. mutlak hata   MSE" << std::endl;
    hataHesapla(temiz, gurultulu, mae, mse);
    printf("Gurultulu          %16.4f   %.4f\n", mae, mse);
    hataHesapla(temiz, gauss, mae, mse);
    printf("Gauss              %16.4f   %.4f\n", mae, mse);
    hataHesapla(temiz, medyan, mae, mse);
    printf("Medyan             %16.4f   %.4f\n", mae, mse);
    hataHesapla(temiz, ikisi, mae, mse);
    printf("Gauss + Medyan     %16.4f   %.4f\n", mae, mse);

    // Farklı gürültü oranlarında hangi filtre daha iyi? (MSE)
    std::cout << "\nFarkli gurultu oranlarinda MSE:" << std::endl;
    std::cout << "Oran   Gurultulu    Gauss   Medyan   Gauss+Medyan" << std::endl;
    for (double o : {0.1, 0.3, 0.6, 1.0}) {
        std::vector<double> g = temiz;
        gurultuEkle(g, o);
        double m0, m1, m2, m3, a;
        hataHesapla(temiz, g, a, m0);
        hataHesapla(temiz, gaussFiltre(g, PENCERE, SIGMA), a, m1);
        hataHesapla(temiz, medyanFiltre(g, PENCERE), a, m2);
        hataHesapla(temiz, medyanFiltre(gaussFiltre(g, PENCERE, SIGMA), PENCERE), a, m3);
        printf("%.1f   %9.4f   %6.4f   %6.4f   %12.4f\n", o, m0, m1, m2, m3);
    }

    // ================= GRAFİKLER =================
    char b[120];
    std::vector<cv::Mat> grafikler;
    hataHesapla(temiz, gurultulu, mae, mse);
    snprintf(b, sizeof(b), "Gurultulu (oran %.1f)   MSE = %.4f", oran, mse);
    grafikler.push_back(grafikCiz(temiz, gurultulu, b, cv::Scalar(0, 0, 255)));
    hataHesapla(temiz, gauss, mae, mse);
    snprintf(b, sizeof(b), "Gauss filtresi (pencere %d, sigma %.1f)   MSE = %.4f", PENCERE, SIGMA, mse);
    grafikler.push_back(grafikCiz(temiz, gauss, b, cv::Scalar(0, 160, 0)));
    hataHesapla(temiz, medyan, mae, mse);
    snprintf(b, sizeof(b), "Medyan filtresi (pencere %d)   MSE = %.4f", PENCERE, mse);
    grafikler.push_back(grafikCiz(temiz, medyan, b, cv::Scalar(180, 0, 180)));
    hataHesapla(temiz, ikisi, mae, mse);
    snprintf(b, sizeof(b), "Gauss + Medyan   MSE = %.4f", mse);
    grafikler.push_back(grafikCiz(temiz, ikisi, b, cv::Scalar(0, 120, 255)));

    cv::Mat sonuc;
    cv::vconcat(grafikler, sonuc);
    cv::imwrite("filtreler.png", sonuc);
    std::cout << "\nKaydedildi: filtreler.png (mavi kalin cizgi: temiz sinus)" << std::endl;

    std::cout << "\nYorum:\n"
                 "  - Eklenen gurultu duzgun dagilimli: her elemana kucuk-orta buyuklukte, rastgele isaretli\n"
                 "    sapmalar eklendi. Bu tur gurultude Gauss filtresi en iyisi: komsularin agirlikli\n"
                 "    ortalamasini aldigi icin + ve - sapmalar birbirini goturur, egri yumusak kalir.\n"
                 "  - Medyan filtresi de hatayi azaltiyor ama Gauss kadar degil. Ortanca degeri sectigi\n"
                 "    icin degerleri ortalamiyor, mevcut orneklerden birini kopyaliyor: grafikte yer yer\n"
                 "    duz basamaklar olusuyor ve tepeler/cukurlar kirpiliyor. Medyan asil olarak az sayida\n"
                 "    ama cok buyuk sapmalarda (tuz-biber, ani sicrama) gucludur; burada oyle gurultu yok.\n"
                 "  - Gauss + Medyan, Gauss'a cok yakin sonuc veriyor; Gauss zaten gurultuyu temizledigi\n"
                 "    icin arkasindan gelen medyan pek bir sey katmiyor (cok yuksek gurultude biraz yardimci).\n"
                 "  - Gurultu cok azken (oran 0.1) filtreler neredeyse ise yaramiyor, hatta medyan hatayi\n"
                 "    artiriyor: filtre gurultuyle birlikte sinusun kendisini de biraz yumusatir. Yani\n"
                 "    filtrenin bir bedeli var; gurultu buyudukce bu bedel kazanca gore onemsizlesir.\n";
    return 0;
}
