// Hafta 6 - Görev 2: Hough dönüşümü ile doğru bulma (tamamen kendi kodumuz)
//
// Bir doğru (rho, theta) ile gösterilir:   rho = x * cos(theta) + y * sin(theta)
//   theta : doğrunun normalinin açısı (0..179 derece)
//   rho   : doğrunun orijine (sol üst köşe) uzaklığı (-D..+D, D = resmin köşegeni)
//
// Adımlar:
//   1) Canny ile kenar pikselleri bulunur (Hough için sadece bu kısım hazır)
//   2) 180 açı için sin ve cos değerleri BAŞTAN hesaplanıp tabloda tutulur
//   3) Oylama matrisi oy[rho][theta] oluşturulur (başta hepsi 0)
//   4) OYLAMA: her kenar pikseli (x, y), her theta için kendi rho'sunu hesaplar
//      ve oy[rho][theta] hücresine 1 oy verir. Aynı doğru üzerindeki tüm
//      pikseller aynı (rho, theta) hücresine oy verdiği için orada tepe oluşur.
//   5) Oylama matrisi resim olarak kaydedilir
//   6) Eşik değerini geçen ve çevresinin en büyüğü olan hücreler doğru kabul
//      edilir, orijinal resim üzerine çizilir

#include <opencv2/opencv.hpp>
#include <cmath>
#include <iostream>

const int ACI_SAYISI = 180;   // theta: 0, 1, 2, ..., 179 derece
const int ESIK = 50;          // bir doğru için gereken en az oy
const int KOMSU = 7;          // tepe ararken bakılan komşuluk yarıçapı (rho ve theta yönünde)

int main() {
    cv::Mat img = cv::imread("../goruntuler/cizgiler.png", cv::IMREAD_GRAYSCALE);
    if (img.empty()) {
        std::cout << "Goruntu okunamadi!" << std::endl;
        return 1;
    }

    // ================= 1) KENARLAR =================
    cv::Mat yumusak, kenar;
    cv::GaussianBlur(img, yumusak, cv::Size(5, 5), 1.5);
    cv::Canny(yumusak, kenar, 50, 150);

    // ================= 2) SIN / COS TABLOSU =================
    // Oylamada her piksel için 180 kez sin/cos gerekir; her seferinde
    // hesaplamak yerine bir kere hesaplayıp tablodan okuyoruz.
    double cosTablo[ACI_SAYISI], sinTablo[ACI_SAYISI];
    for (int t = 0; t < ACI_SAYISI; t++) {
        double radyan = t * CV_PI / 180.0;
        cosTablo[t] = std::cos(radyan);
        sinTablo[t] = std::sin(radyan);
    }

    // ================= 3) OYLAMA MATRİSİ =================
    // rho negatif olabilir, matris indisi negatif olamaz -> satır = rho + D
    int D = (int)std::ceil(std::sqrt(img.cols * img.cols + img.rows * img.rows));
    int rhoSayisi = 2 * D + 1;
    cv::Mat oy = cv::Mat::zeros(rhoSayisi, ACI_SAYISI, CV_32S);   // oy[rho + D][theta]

    // ================= 4) OYLAMA =================
    int kenarSayisi = 0;
    bool ornekYazildi = false;
    for (int y = 0; y < kenar.rows; y++) {
        for (int x = 0; x < kenar.cols; x++) {
            if (kenar.at<uchar>(y, x) == 0) continue;   // kenar değilse oy vermez
            kenarSayisi++;

            for (int t = 0; t < ACI_SAYISI; t++) {
                int rho = (int)std::round(x * cosTablo[t] + y * sinTablo[t]);
                oy.at<int>(rho + D, t)++;               // (rho, theta) hücresine 1 oy
            }

            // Oylamayı göstermek için ilk kenar pikselinin birkaç oyunu yazdır
            if (!ornekYazildi) {
                std::cout << "Ornek: (" << x << ", " << y << ") pikselinin oylari:" << std::endl;
                for (int t = 0; t < ACI_SAYISI; t += 45) {
                    int rho = (int)std::round(x * cosTablo[t] + y * sinTablo[t]);
                    std::cout << "  theta = " << t << " derece -> rho = " << rho
                              << "  => oy[rho=" << rho << "][theta=" << t << "]++" << std::endl;
                }
                ornekYazildi = true;
            }
        }
    }

    int enBuyukOy = 0;
    for (int r = 0; r < rhoSayisi; r++)
        for (int t = 0; t < ACI_SAYISI; t++)
            if (oy.at<int>(r, t) > enBuyukOy) enBuyukOy = oy.at<int>(r, t);

    std::cout << "\nKenar piksel sayisi : " << kenarSayisi << std::endl;
    std::cout << "Oylama matrisi      : " << rhoSayisi << " (rho) x " << ACI_SAYISI << " (theta)" << std::endl;
    std::cout << "Toplam oy           : " << kenarSayisi * ACI_SAYISI << std::endl;
    std::cout << "En buyuk oy         : " << enBuyukOy << std::endl;

    // ================= 5) OYLAMA MATRİSİNİ RESİM OLARAK KAYDET =================
    // Oyları 0..255 aralığına ölçekle (en çok oy alan hücre beyaz olur).
    // Karekök alıyoruz: az oylu hücreler de görünsün (sinüs eğrileri belli olsun).
    cv::Mat oyResmi(rhoSayisi, ACI_SAYISI, CV_8UC1);
    for (int r = 0; r < rhoSayisi; r++)
        for (int t = 0; t < ACI_SAYISI; t++)
            oyResmi.at<uchar>(r, t) = (uchar)(255 * std::sqrt((double)oy.at<int>(r, t) / enBuyukOy));

    // ================= 6) EŞİK + TEPE BULMA =================
    // Bir hücre doğru sayılır: oyu eşikten büyükse VE komşuluğundaki en büyük oysa
    // (aynı doğrunun yakın hücrelerini tekrar tekrar bulmamak için)
    std::vector<cv::Point> tepeler;   // x = theta, y = rho + D
    for (int r = 0; r < rhoSayisi; r++) {
        for (int t = 0; t < ACI_SAYISI; t++) {
            int v = oy.at<int>(r, t);
            if (v < ESIK) continue;

            bool enBuyuk = true;
            for (int dr = -KOMSU; dr <= KOMSU && enBuyuk; dr++) {
                for (int dt = -KOMSU; dt <= KOMSU; dt++) {
                    int rr = r + dr, tt = t + dt;
                    if (rr < 0 || rr >= rhoSayisi) continue;
                    // theta 0 ile 179 komşudur (doğru 180 derecede kendine döner, rho işaret değiştirir)
                    if (tt < 0) { tt += ACI_SAYISI; rr = rhoSayisi - 1 - rr; }
                    if (tt >= ACI_SAYISI) { tt -= ACI_SAYISI; rr = rhoSayisi - 1 - rr; }
                    int w = oy.at<int>(rr, tt);
                    // eşit oylarda sadece ilkini tut
                    if (w > v || (w == v && (dr < 0 || (dr == 0 && dt < 0)))) {
                        enBuyuk = false;
                        break;
                    }
                }
            }
            if (enBuyuk) tepeler.push_back(cv::Point(t, r));
        }
    }

    // Doğruları orijinal resim üzerine çiz
    cv::Mat sonuc;
    cv::cvtColor(img, sonuc, cv::COLOR_GRAY2BGR);
    std::cout << "\nEsik = " << ESIK << " oy, bulunan dogrular:" << std::endl;
    for (size_t i = 0; i < tepeler.size(); i++) {
        int t = tepeler[i].x;
        int rho = tepeler[i].y - D;
        std::cout << "  rho = " << rho << ", theta = " << t << " derece, oy = "
                  << oy.at<int>(tepeler[i].y, t) << std::endl;

        double a = cosTablo[t], b = sinTablo[t];
        double x0 = a * rho, y0 = b * rho;   // doğrunun orijine en yakın noktası
        cv::Point p1(cvRound(x0 - 1000 * b), cvRound(y0 + 1000 * a));
        cv::Point p2(cvRound(x0 + 1000 * b), cvRound(y0 - 1000 * a));
        cv::line(sonuc, p1, p2, cv::Scalar(0, 0, 255), 1);
    }
    std::cout << "Toplam " << tepeler.size() << " dogru" << std::endl;

    // Oylama matrisinin renkli ve büyütülmüş hali, bulunan tepeler işaretli
    cv::Mat oyRenkli;
    cv::applyColorMap(oyResmi, oyRenkli, cv::COLORMAP_JET);
    for (size_t i = 0; i < tepeler.size(); i++)
        cv::circle(oyRenkli, tepeler[i], 6, cv::Scalar(255, 255, 255), 1);
    cv::resize(oyRenkli, oyRenkli, cv::Size(ACI_SAYISI * 3, rhoSayisi), 0, 0, cv::INTER_NEAREST);

    cv::imwrite("kenar.png", kenar);
    cv::imwrite("oylama_matrisi.png", oyResmi);
    cv::imwrite("oylama_matrisi_renkli.png", oyRenkli);
    cv::imwrite("dogrular.png", sonuc);
    std::cout << "\nKaydedildi: kenar.png, oylama_matrisi.png, oylama_matrisi_renkli.png, dogrular.png" << std::endl;
    std::cout << "(oylama matrisi: yatay eksen theta 0..179, dikey eksen rho -D..+D)" << std::endl;
    return 0;
}
