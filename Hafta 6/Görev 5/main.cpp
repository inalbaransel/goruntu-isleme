// Hafta 6 - Görev 5: Hough dönüşümü ile EŞ MERKEZLİ çemberleri bulma (kendi kodumuz)
//
// Fikir: Bir çemberin her noktasındaki normal (gradyan yönü) çemberin
// merkezinden geçer. Eş merkezli çemberlerin merkezi ortak olduğu için,
// TÜM halkaların kenar pikselleri normalleri boyunca AYNI (a, b) noktasına
// oy verir. Yani eş merkezlilik oylamanın içinde ortaya çıkar:
//   - 3 boyutlu oylama matrisi oy[r][b][a]: merkez (a, b), yarıçap r
//   - Her kenar pikseli, her r için sadece normali üzerindeki 2 noktaya oy verir:
//         a = x -/+ r * cos(aci)      b = y -/+ r * sin(aci)
//     (aci = pikselin gradyan açısı; parlak halka da koyu halka da olabilir, iki yön)
//   - Oylama sırasında aynı oy merkez matrisine de eklenir: merkezOy[b][a].
//     Bu matris, oy[r][b][a]'nın tüm r'ler boyunca toplamıdır. Eş merkezli
//     halkalar aynı hücreye yığıldığı için merkezleri burada en yüksek tepeleri verir.
//   - Bir merkezin oylama matrisindeki r ekseni boyunca (oy[*][b][a]) birden
//     fazla tepe varsa o merkezde birden fazla çember vardır: EŞ MERKEZLİ.
//
// Yani önce ayrı ayrı çember bulup sonra merkezleri karşılaştırmıyoruz
// (post processing değil); ortak merkez doğrudan oylamadan çıkıyor.
//
// Adımlar:
//   1) Canny ile kenarlar, Sobel ile her kenar pikselinin gradyan açısı
//   2) 360 açı için sin/cos BAŞTAN hesaplanıp tabloda tutulur
//   3) oy[r][b][a] (3 boyutlu) ve merkezOy[b][a] oluşturulur
//   4) OYLAMA
//   5) Merkezler: merkezOy'daki eşiği geçen tepeler
//   6) Her merkez için r profili (oy[*][b][a]) incelenir, tepeler = halkalar.
//      En az 2 halka varsa eş merkezli grup olarak çizilir.

#include <opencv2/opencv.hpp>
#include <cmath>
#include <iostream>

const int R_MIN = 15, R_MAX = 100;     // aranan yarıçap aralığı
const int ACI_SAYISI = 360;            // 0..359 derece
const int MERKEZ_ESIK = 300;           // merkez için gereken en az oy (tüm r'ler toplamı)
const int MERKEZ_KOMSU = 15;           // iki merkez arası en az uzaklık
const double HALKA_ESIK = 0.4;         // halka için: oy / (2*pi*r) en az bu kadar olmalı
const int HALKA_KOMSU = 6;             // iki halka arası en az yarıçap farkı

int main() {
    cv::Mat img = cv::imread("../goruntuler/es_merkezli.png", cv::IMREAD_GRAYSCALE);
    if (img.empty()) {
        std::cout << "Goruntu okunamadi!" << std::endl;
        return 1;
    }
    int W = img.cols, H = img.rows;
    int R_SAYISI = R_MAX - R_MIN + 1;

    // ================= 1) KENARLAR VE GRADYAN AÇISI =================
    cv::Mat yumusak, kenar, gx, gy;
    cv::GaussianBlur(img, yumusak, cv::Size(5, 5), 1.5);
    cv::Canny(yumusak, kenar, 50, 150);
    cv::Sobel(yumusak, gx, CV_32F, 1, 0);   // yatay türev
    cv::Sobel(yumusak, gy, CV_32F, 0, 1);   // dikey türev

    // ================= 2) SIN / COS TABLOSU =================
    double cosTablo[ACI_SAYISI], sinTablo[ACI_SAYISI];
    for (int t = 0; t < ACI_SAYISI; t++) {
        double radyan = t * CV_PI / 180.0;
        cosTablo[t] = std::cos(radyan);
        sinTablo[t] = std::sin(radyan);
    }

    // ================= 3) OYLAMA MATRİSLERİ =================
    int boyutlar[3] = {R_SAYISI, H, W};
    cv::Mat oy(3, boyutlar, CV_32S, cv::Scalar(0));        // oy[r - R_MIN][b][a]
    cv::Mat merkezOy = cv::Mat::zeros(H, W, CV_32S);       // merkezOy[b][a] = tüm r'lerin toplamı
    std::cout << "Oylama matrisi: " << R_SAYISI << " (r) x " << H << " (b) x " << W << " (a)" << std::endl;

    // ================= 4) OYLAMA =================
    int kenarSayisi = 0;
    bool ornekYazildi = false;
    for (int y = 0; y < H; y++) {
        for (int x = 0; x < W; x++) {
            if (kenar.at<uchar>(y, x) == 0) continue;
            kenarSayisi++;

            // Gradyan açısı (derece) -> tablodaki indis
            double aciDerece = std::atan2(gy.at<float>(y, x), gx.at<float>(y, x)) * 180.0 / CV_PI;
            int t = ((int)std::round(aciDerece) + ACI_SAYISI) % ACI_SAYISI;

            for (int r = R_MIN; r <= R_MAX; r++) {
                for (int yon = -1; yon <= 1; yon += 2) {             // normalin iki yönü
                    int a = (int)std::round(x - yon * r * cosTablo[t]);
                    int b = (int)std::round(y - yon * r * sinTablo[t]);
                    if (a < 0 || a >= W || b < 0 || b >= H) continue;
                    oy.at<int>(r - R_MIN, b, a)++;   // (a, b, r) hücresine 1 oy
                    merkezOy.at<int>(b, a)++;        // ortak merkez birikimi
                }
            }

            if (!ornekYazildi) {
                std::cout << "\nOrnek: (" << x << ", " << y << ") pikseli, gradyan acisi = " << t
                          << " derece. r = 30 ve r = 55 icin oylar:" << std::endl;
                for (int r : {30, 55}) {
                    for (int yon = -1; yon <= 1; yon += 2) {
                        int a = (int)std::round(x - yon * r * cosTablo[t]);
                        int b = (int)std::round(y - yon * r * sinTablo[t]);
                        std::cout << "  r = " << r << ", yon = " << (yon > 0 ? "+" : "-") << " -> a = " << a
                                  << ", b = " << b;
                        if (a < 0 || a >= W || b < 0 || b >= H)
                            std::cout << "  => resim disinda, oy verilmez" << std::endl;
                        else
                            std::cout << "  => oy[r=" << r << "][b=" << b << "][a=" << a
                                      << "]++ ve merkezOy[" << b << "][" << a << "]++" << std::endl;
                    }
                }
                ornekYazildi = true;
            }
        }
    }
    std::cout << "\nKenar piksel sayisi: " << kenarSayisi << std::endl;

    // ================= 5) MERKEZLER =================
    // Gradyan açısı birkaç derece sapabildiği için oylar merkezin 1-2 piksel
    // çevresine dağılır; o yüzden her hücreyi 5x5 komşuluğunun toplamıyla değerlendiriyoruz.
    cv::Mat merkezToplam = cv::Mat::zeros(H, W, CV_32S);
    for (int b = 2; b < H - 2; b++)
        for (int a = 2; a < W - 2; a++) {
            int toplam = 0;
            for (int i = -2; i <= 2; i++)
                for (int j = -2; j <= 2; j++) toplam += merkezOy.at<int>(b + i, a + j);
            merkezToplam.at<int>(b, a) = toplam;
        }

    std::vector<cv::Point> merkezler;
    for (int b = 0; b < H; b++) {
        for (int a = 0; a < W; a++) {
            int v = merkezToplam.at<int>(b, a);
            if (v < MERKEZ_ESIK) continue;
            bool enBuyuk = true;
            for (int i = -MERKEZ_KOMSU; i <= MERKEZ_KOMSU && enBuyuk; i++)
                for (int j = -MERKEZ_KOMSU; j <= MERKEZ_KOMSU; j++) {
                    int bb = b + i, aa = a + j;
                    if (bb < 0 || bb >= H || aa < 0 || aa >= W) continue;
                    int w = merkezToplam.at<int>(bb, aa);
                    if (w > v || (w == v && (i < 0 || (i == 0 && j < 0)))) {
                        enBuyuk = false;
                        break;
                    }
                }
            if (enBuyuk) merkezler.push_back(cv::Point(a, b));
        }
    }

    // ================= 6) HER MERKEZİN r PROFİLİ =================
    cv::Mat sonuc;
    cv::cvtColor(img, sonuc, cv::COLOR_GRAY2BGR);
    cv::Scalar renkler[] = {cv::Scalar(0, 0, 255), cv::Scalar(0, 255, 0), cv::Scalar(255, 0, 0),
                            cv::Scalar(0, 255, 255), cv::Scalar(255, 0, 255)};
    int grupSayisi = 0;

    for (size_t m = 0; m < merkezler.size(); m++) {
        int a0 = merkezler[m].x, b0 = merkezler[m].y;

        // r profili: her r için merkezin 5x5 çevresindeki oyların toplamı, çevreye bölünmüş.
        // Kalın bir halkanın iç ve dış kenarı (yaklaşık r-3 ve r+3) ayrı ayrı oy aldığı için
        // r yönünde de +-3'lük pencere topluyoruz; böylece ikisi halkanın ortasında birleşir.
        std::vector<double> profil(R_SAYISI);
        for (int ri = 0; ri < R_SAYISI; ri++) {
            int toplam = 0;
            for (int dr = -3; dr <= 3; dr++) {
                int rr = ri + dr;
                if (rr < 0 || rr >= R_SAYISI) continue;
                for (int i = -2; i <= 2; i++)
                    for (int j = -2; j <= 2; j++) {
                        int bb = b0 + i, aa = a0 + j;
                        if (bb >= 0 && bb < H && aa >= 0 && aa < W) toplam += oy.at<int>(rr, bb, aa);
                    }
            }
            profil[ri] = toplam / (2 * CV_PI * (ri + R_MIN));
        }

        // Profildeki tepeler = bu merkezdeki halkalar
        std::vector<int> halkalar;
        for (int ri = 0; ri < R_SAYISI; ri++) {
            if (profil[ri] < HALKA_ESIK) continue;
            bool enBuyuk = true;
            for (int d = -HALKA_KOMSU; d <= HALKA_KOMSU; d++) {
                int rr = ri + d;
                if (rr < 0 || rr >= R_SAYISI || d == 0) continue;
                if (profil[rr] > profil[ri] || (profil[rr] == profil[ri] && d < 0)) {
                    enBuyuk = false;
                    break;
                }
            }
            if (enBuyuk) halkalar.push_back(ri + R_MIN);
        }

        std::cout << "\nMerkez (" << a0 << ", " << b0 << "), merkez oyu = " << merkezToplam.at<int>(b0, a0)
                  << ", halka sayisi = " << halkalar.size() << ", yaricaplar:";
        for (int r : halkalar) std::cout << " " << r;
        std::cout << std::endl;

        if (halkalar.size() >= 2) {
            cv::Scalar renk = renkler[grupSayisi % 5];
            for (int r : halkalar) cv::circle(sonuc, cv::Point(a0, b0), r, renk, 2);
            cv::circle(sonuc, cv::Point(a0, b0), 3, renk, -1);
            std::cout << "  -> ES MERKEZLI grup " << grupSayisi + 1 << std::endl;
            grupSayisi++;
        } else {
            std::cout << "  -> tek cember, es merkezli degil (cizilmedi)" << std::endl;
        }
    }
    std::cout << "\nToplam " << grupSayisi << " es merkezli grup bulundu" << std::endl;

    // merkezOy matrisini resim olarak kaydet (eş merkezli merkezler en parlak noktalar)
    int enBuyukOy = 1;
    for (int b = 0; b < H; b++)
        for (int a = 0; a < W; a++)
            if (merkezOy.at<int>(b, a) > enBuyukOy) enBuyukOy = merkezOy.at<int>(b, a);
    cv::Mat merkezResmi(H, W, CV_8UC1);
    for (int b = 0; b < H; b++)
        for (int a = 0; a < W; a++)
            merkezResmi.at<uchar>(b, a) = (uchar)(255 * std::sqrt((double)merkezOy.at<int>(b, a) / enBuyukOy));
    cv::applyColorMap(merkezResmi, merkezResmi, cv::COLORMAP_JET);

    cv::imwrite("kenar.png", kenar);
    cv::imwrite("merkez_oylari.png", merkezResmi);
    cv::imwrite("es_merkezli_sonuc.png", sonuc);
    std::cout << "\nKaydedildi: kenar.png, merkez_oylari.png, es_merkezli_sonuc.png" << std::endl;
    return 0;
}
