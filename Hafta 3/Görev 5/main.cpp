// Hafta 3 - Görev 5: Medyan filtre (tuz-biber gürültüsü)
// a) OpenCV'nin hazır medyan filtresi (cv::medianBlur)
// b) Kendi yazdığımız 5x5 medyan filtre
//
// Medyan filtre: her pikselin etrafındaki 5x5 = 25 pikseli sırala,
// ortadakini (13. değer) al. Tuz (255) ve biber (0) uç değer oldukları için
// sıralamada en başa / en sona gider, ortaya neredeyse hiç gelemez.

#include <opencv2/opencv.hpp>
#include <algorithm>
#include <iostream>

int main() {
    cv::Mat img = cv::imread("../goruntuler/tuz_biber.png", cv::IMREAD_GRAYSCALE);
    if (img.empty()) {
        std::cout << "Goruntu okunamadi!" << std::endl;
        return 1;
    }

    // ================= a) HAZIR MEDYAN =================
    cv::Mat hazir;
    cv::medianBlur(img, hazir, 5);

    // ================= b) KENDİ MEDYAN FİLTREMİZ =================
    cv::Mat kendi = img.clone();

    // Kenardaki 2 piksellik şeridi atlıyoruz (5x5 pencere görüntü dışına taşar)
    for (int y = 2; y < img.rows - 2; y++) {
        for (int x = 2; x < img.cols - 2; x++) {

            // 1) 5x5 penceredeki 25 değeri bir diziye topla
            int pencere[25];
            int n = 0;
            for (int i = -2; i <= 2; i++) {
                for (int j = -2; j <= 2; j++) {
                    pencere[n] = img.at<uchar>(y + i, x + j);
                    n++;
                }
            }

            // 2) Küçükten büyüğe sırala
            std::sort(pencere, pencere + 25);

            // 3) Ortanca değer = 13. eleman (indis 12)
            kendi.at<uchar>(y, x) = pencere[12];
        }
    }

    // ================= KARŞILAŞTIRMA =================
    // Kenarlar hariç iki sonuç kaç pikselde farklı?
    int farkli = 0;
    for (int y = 2; y < img.rows - 2; y++) {
        for (int x = 2; x < img.cols - 2; x++) {
            if (hazir.at<uchar>(y, x) != kendi.at<uchar>(y, x)) farkli++;
        }
    }
    std::cout << "Kenarlar haric farkli piksel sayisi: " << farkli << std::endl;

    // Kaydet: gürültülü | hazır medyan | kendi medyan
    cv::Mat ikili, yanYana;
    cv::hconcat(img, hazir, ikili);
    cv::hconcat(ikili, kendi, yanYana);
    cv::imwrite("medyan_opencv.png", hazir);
    cv::imwrite("medyan_kendi.png", kendi);
    cv::imwrite("karsilastirma.png", yanYana);
    std::cout << "Kaydedildi: medyan_opencv.png, medyan_kendi.png, karsilastirma.png" << std::endl;
    return 0;
}
