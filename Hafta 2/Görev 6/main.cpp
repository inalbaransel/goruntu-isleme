// Hafta 2 - Görev 6
// A = 0.75 * p        (sadece çarpma)
// B = 0.75 * p + 20   (çarpma + 20 ekleme)
// iki sonucu görsel ve sayısal olarak karşılaştır.

#include <opencv2/opencv.hpp>
#include <iostream>

int main() {
    cv::Mat img = cv::imread("../goruntu.jpg", cv::IMREAD_GRAYSCALE);
    if (img.empty()) {
        std::cout << "Goruntu okunamadi!" << std::endl;
        return 1;
    }

    cv::Mat a = img.clone();
    cv::Mat b = img.clone();

    // Her pikseli gez ve iki formülü uygula
    for (int y = 0; y < img.rows; y++) {
        for (int x = 0; x < img.cols; x++) {
            int p = img.at<uchar>(y, x);
            a.at<uchar>(y, x) = cv::saturate_cast<uchar>(0.75 * p);
            b.at<uchar>(y, x) = cv::saturate_cast<uchar>(0.75 * p + 20);
        }
    }

    // ---- Sayısal karşılaştırma ----
    double enKucuk, enBuyuk;

    cv::minMaxLoc(img, &enKucuk, &enBuyuk);
    std::cout << "Orijinal    -> ortalama: " << cv::mean(img)[0]
              << "  min: " << enKucuk << "  max: " << enBuyuk << std::endl;

    cv::minMaxLoc(a, &enKucuk, &enBuyuk);
    std::cout << "0.75*p      -> ortalama: " << cv::mean(a)[0]
              << "  min: " << enKucuk << "  max: " << enBuyuk << std::endl;

    cv::minMaxLoc(b, &enKucuk, &enBuyuk);
    std::cout << "0.75*p + 20 -> ortalama: " << cv::mean(b)[0]
              << "  min: " << enKucuk << "  max: " << enBuyuk << std::endl;

    // ---- Görsel karşılaştırma: 3 resmi yan yana koy ----
    cv::Mat ikili, yanYana;
    cv::hconcat(img, a, ikili);         // orijinal | A
    cv::hconcat(ikili, b, yanYana);     // orijinal | A | B

    cv::imwrite("carpim_075.png", a);
    cv::imwrite("carpim_075_arti_20.png", b);
    cv::imwrite("karsilastirma.png", yanYana);
    std::cout << "Kaydedildi: carpim_075.png, carpim_075_arti_20.png, karsilastirma.png" << std::endl;
    return 0;
}
