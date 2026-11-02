// Hafta 6 - Görev 4: Hough dönüşümü ile çember bulma (tamamen kendi kodumuz)
//
// Merkezi (a, b), yarıçapı r olan çember üzerindeki her (x, y) noktası için:
//     x = a + r * cos(aci)      y = b + r * sin(aci)
// Tersinden: (x, y) kenar pikseli, merkez için şu noktalara oy verir:
//     a = x - r * cos(aci)      b = y - r * sin(aci)      (aci = 0..359)
// Yani her kenar pikseli, her r için kendi etrafında r yarıçaplı bir çember
// çizer ve üzerindeki hücrelere oy verir. Gerçek çemberin tüm pikselleri aynı
// (a, b, r) hücresine oy verdiği için orada tepe oluşur.
//
// Adımlar:
//   1) Canny ile kenarlar bulunur
//   2) 360 açı için sin ve cos BAŞTAN hesaplanıp tabloda tutulur
//   3) 3 boyutlu oylama matrisi oy[r][b][a] oluşturulur (başta hepsi 0)
//   4) OYLAMA: her kenar pikseli, her r ve her açı için bir (a, b) hücresine oy verir
//   5) Puan = oy / (2 * pi * r): çemberin çevresinin ne kadarının kenar olduğu.
//      (Büyük çember doğal olarak daha çok oy alır, bu yüzden çevreye bölüyoruz.)
//      Puanı eşikten büyük ve komşularının en büyüğü olan hücreler çember kabul edilir.

#include <opencv2/opencv.hpp>
#include <cmath>
#include <iostream>

const int R_MIN = 20, R_MAX = 100;   // aranan yarıçap aralığı
const int ACI_SAYISI = 360;          // 0, 1, ..., 359 derece
const double ESIK = 0.5;             // çevrenin en az %50'si kenar olmalı
const int KOMSU = 10;                // tepe ararken a, b ve r yönünde bakılan komşuluk

int main() {
    cv::Mat img = cv::imread("../goruntuler/cemberler.png", cv::IMREAD_GRAYSCALE);
    if (img.empty()) {
        std::cout << "Goruntu okunamadi!" << std::endl;
        return 1;
    }
    int W = img.cols, H = img.rows;
    int R_SAYISI = R_MAX - R_MIN + 1;

    // ================= 1) KENARLAR =================
    cv::Mat yumusak, kenar;
    cv::GaussianBlur(img, yumusak, cv::Size(5, 5), 1.5);
    cv::Canny(yumusak, kenar, 50, 150);

    // ================= 2) SIN / COS TABLOSU =================
    double cosTablo[ACI_SAYISI], sinTablo[ACI_SAYISI];
    for (int t = 0; t < ACI_SAYISI; t++) {
        double radyan = t * CV_PI / 180.0;
        cosTablo[t] = std::cos(radyan);
        sinTablo[t] = std::sin(radyan);
    }

    // ================= 3) 3 BOYUTLU OYLAMA MATRİSİ =================
    // oy.at<int>(r - R_MIN, b, a) : merkezi (a, b), yarıçapı r olan çemberin oyu
    int boyutlar[3] = {R_SAYISI, H, W};
    cv::Mat oy(3, boyutlar, CV_32S, cv::Scalar(0));
    std::cout << "Oylama matrisi: " << R_SAYISI << " (r) x " << H << " (b) x " << W << " (a)" << std::endl;

    // ================= 4) OYLAMA =================
    int kenarSayisi = 0;
    bool ornekYazildi = false;
    for (int y = 0; y < H; y++) {
        for (int x = 0; x < W; x++) {
            if (kenar.at<uchar>(y, x) == 0) continue;
            kenarSayisi++;

            for (int r = R_MIN; r <= R_MAX; r++) {
                int oncekiA = -1, oncekiB = -1;
                for (int t = 0; t < ACI_SAYISI; t++) {
                    int a = (int)std::round(x - r * cosTablo[t]);
                    int b = (int)std::round(y - r * sinTablo[t]);
                    if (a < 0 || a >= W || b < 0 || b >= H) continue;   // merkez resim dışında
                    // Küçük r'de art arda açılar aynı hücreye düşer; bir piksel bir
                    // hücreye aynı r için sadece 1 kez oy versin
                    if (a == oncekiA && b == oncekiB) continue;
                    oy.at<int>(r - R_MIN, b, a)++;                       // (a, b, r) hücresine 1 oy
                    oncekiA = a;
                    oncekiB = b;
                }
            }

            // Oylamayı göstermek için ilk kenar pikselinin r = 50 için birkaç oyu
            if (!ornekYazildi) {
                std::cout << "\nOrnek: (" << x << ", " << y << ") pikseli, r = 50 icin oylar:" << std::endl;
                for (int t = 0; t < ACI_SAYISI; t += 90) {
                    int a = (int)std::round(x - 50 * cosTablo[t]);
                    int b = (int)std::round(y - 50 * sinTablo[t]);
                    std::cout << "  aci = " << t << " -> a = " << a << ", b = " << b;
                    if (a < 0 || a >= W || b < 0 || b >= H)
                        std::cout << "  => resim disinda, oy verilmez" << std::endl;
                    else
                        std::cout << "  => oy[r=50][b=" << b << "][a=" << a << "]++" << std::endl;
                }
                ornekYazildi = true;
            }
        }
    }
    std::cout << "\nKenar piksel sayisi: " << kenarSayisi << std::endl;

    // ================= 5) EŞİK + TEPE BULMA =================
    // Önce her hücrenin puanını hesapla: oy / çevre
    cv::Mat puan(3, boyutlar, CV_32F);
    for (int ri = 0; ri < R_SAYISI; ri++) {
        double cevre = 2 * CV_PI * (ri + R_MIN);
        for (int b = 0; b < H; b++)
            for (int a = 0; a < W; a++)
                puan.at<float>(ri, b, a) = (float)(oy.at<int>(ri, b, a) / cevre);
    }

    std::vector<cv::Vec3i> bulunan;   // (a, b, r)
    for (int ri = 0; ri < R_SAYISI; ri++) {
        for (int b = 0; b < H; b++) {
            for (int a = 0; a < W; a++) {
                float p = puan.at<float>(ri, b, a);
                if (p < ESIK) continue;

                // 3 boyutlu komşulukta daha yüksek puanlı hücre var mı?
                bool enBuyuk = true;
                for (int dr = -KOMSU; dr <= KOMSU && enBuyuk; dr++) {
                    int rr = ri + dr;
                    if (rr < 0 || rr >= R_SAYISI) continue;
                    for (int db = -KOMSU; db <= KOMSU && enBuyuk; db++) {
                        int bb = b + db;
                        if (bb < 0 || bb >= H) continue;
                        for (int da = -KOMSU; da <= KOMSU; da++) {
                            int aa = a + da;
                            if (aa < 0 || aa >= W) continue;
                            float q = puan.at<float>(rr, bb, aa);
                            if (q > p || (q == p && (dr < 0 || (dr == 0 && (db < 0 || (db == 0 && da < 0)))))) {
                                enBuyuk = false;
                                break;
                            }
                        }
                    }
                }
                if (enBuyuk) bulunan.push_back(cv::Vec3i(a, b, ri + R_MIN));
            }
        }
    }

    // ================= ÇİZ =================
    cv::Mat sonuc;
    cv::cvtColor(img, sonuc, cv::COLOR_GRAY2BGR);
    std::cout << "\nEsik = " << ESIK << " (cevrenin orani), bulunan cemberler:" << std::endl;
    for (size_t i = 0; i < bulunan.size(); i++) {
        int a = bulunan[i][0], b = bulunan[i][1], r = bulunan[i][2];
        std::cout << "  merkez = (" << a << ", " << b << "), yaricap = " << r << ", oy = "
                  << oy.at<int>(r - R_MIN, b, a) << ", puan = " << puan.at<float>(r - R_MIN, b, a) << std::endl;
        cv::circle(sonuc, cv::Point(a, b), r, cv::Scalar(255, 0, 255), 2);   // çember: MOR
        cv::circle(sonuc, cv::Point(a, b), 3, cv::Scalar(0, 255, 255), -1);  // merkez: SARI
    }
    std::cout << "Toplam " << bulunan.size() << " cember" << std::endl;

    // Oylama matrisinden kesitler: bulunan her çemberin yarıçapındaki (a, b) düzlemi.
    // Gerçek merkezde parlak bir nokta görülür.
    std::vector<cv::Mat> kesitler;
    for (size_t i = 0; i < bulunan.size(); i++) {
        int ri = bulunan[i][2] - R_MIN;
        cv::Mat kesit(H, W, CV_8UC1);
        int enBuyukOy = 1;
        for (int b = 0; b < H; b++)
            for (int a = 0; a < W; a++)
                if (oy.at<int>(ri, b, a) > enBuyukOy) enBuyukOy = oy.at<int>(ri, b, a);
        for (int b = 0; b < H; b++)
            for (int a = 0; a < W; a++)
                kesit.at<uchar>(b, a) = (uchar)(255 * std::sqrt((double)oy.at<int>(ri, b, a) / enBuyukOy));
        cv::applyColorMap(kesit, kesit, cv::COLORMAP_JET);
        cv::putText(kesit, "r = " + std::to_string(bulunan[i][2]), cv::Point(10, 30),
                    cv::FONT_HERSHEY_SIMPLEX, 0.9, cv::Scalar(255, 255, 255), 2);
        kesitler.push_back(kesit);
    }

    cv::imwrite("kenar.png", kenar);
    cv::imwrite("cemberler_sonuc.png", sonuc);
    if (!kesitler.empty()) {
        cv::Mat kesitResmi;
        cv::hconcat(kesitler, kesitResmi);
        cv::imwrite("oylama_kesitleri.png", kesitResmi);
    }
    std::cout << "\nKaydedildi: kenar.png, cemberler_sonuc.png, oylama_kesitleri.png" << std::endl;
    std::cout << "(oylama_kesitleri.png: her bulunan yaricap icin oy[r][b][a] matrisinin (a, b) kesiti)" << std::endl;
    return 0;
}
