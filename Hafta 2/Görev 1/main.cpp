// Hafta 2 - Görev 1
// Gri görüntüyü 8-bit'ten (256 seviye) 6-bit'e (64 seviye) nicemleme.
// Formül: yeni = (eski / 4) * 4   -> değerler 0, 4, 8, ..., 252 olur

#include <opencv2/opencv.hpp>
#include <iostream>

int main() {
    // 1) Görüntüyü GRİ (tek kanal) olarak oku
    cv::Mat img = cv::imread("../goruntu.jpg", cv::IMREAD_GRAYSCALE);
    if (img.empty()) {
        std::cout << "Goruntu okunamadi!" << std::endl;
        return 1;
    }

    // 2) Sonuç için orijinalin kopyasını al
    cv::Mat yeni = img.clone();

    // 3) Her pikseli gez ve nicemle
    for (int y = 0; y < img.rows; y++) {          // her satır
        for (int x = 0; x < img.cols; x++) {      // her sütun
            int eski = img.at<uchar>(y, x);       // pikselin değeri (0-255)
            yeni.at<uchar>(y, x) = (eski / 4) * 4; // tam sayı bölmesi küsuratı atar
        }
    }

    // 4) Kaydet (PNG: JPG sıkıştırması ara değerleri geri getirirdi)
    cv::imwrite("gri_8bit.png", img);
    cv::imwrite("gri_6bit.png", yeni);

    std::cout << "Ornek: 203 -> " << (203 / 4) * 4 << std::endl;
    std::cout << "Kaydedildi: gri_8bit.png, gri_6bit.png" << std::endl;
    return 0;
}
