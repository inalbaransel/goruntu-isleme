// Hafta 3 - Görev 1: Kontrast germe
// Düşük kontrastlı görüntünün min ve max değerini bul, sonra değerleri
// [0, 255] aralığına doğrusal olarak yay. (Hazır fonksiyon kullanılmadan)
//
//   yeni = (p - min) * 255 / (max - min)

#include <opencv2/opencv.hpp>
#include <iostream>

int main() {
    cv::Mat img = cv::imread("../goruntuler/dusuk_kontrast.png", cv::IMREAD_GRAYSCALE);
    if (img.empty()) {
        std::cout << "Goruntu okunamadi!" << std::endl;
        return 1;
    }

    // 1) Minimum ve maksimum değeri bul
    int enKucuk = 255;   // başlangıçta en büyük olası değer
    int enBuyuk = 0;     // başlangıçta en küçük olası değer
    for (int y = 0; y < img.rows; y++) {
        for (int x = 0; x < img.cols; x++) {
            int p = img.at<uchar>(y, x);
            if (p < enKucuk) enKucuk = p;
            if (p > enBuyuk) enBuyuk = p;
        }
    }
    std::cout << "Once  -> min: " << enKucuk << "  max: " << enBuyuk << std::endl;

    // 2) Her pikseli [0, 255] aralığına yay
    cv::Mat yeni = img.clone();
    for (int y = 0; y < img.rows; y++) {
        for (int x = 0; x < img.cols; x++) {
            int p = img.at<uchar>(y, x);
            yeni.at<uchar>(y, x) = (p - enKucuk) * 255 / (enBuyuk - enKucuk);
        }
    }

    // 3) Kontrol: yeni min ve max 0 ve 255 olmalı
    int yeniMin = 255, yeniMax = 0;
    for (int y = 0; y < yeni.rows; y++) {
        for (int x = 0; x < yeni.cols; x++) {
            int p = yeni.at<uchar>(y, x);
            if (p < yeniMin) yeniMin = p;
            if (p > yeniMax) yeniMax = p;
        }
    }
    std::cout << "Sonra -> min: " << yeniMin << "  max: " << yeniMax << std::endl;

    // 4) Kaydet (karşılaştırma: sol orijinal, sağ sonuç)
    cv::Mat yanYana;
    cv::hconcat(img, yeni, yanYana);
    cv::imwrite("kontrast_gerilmis.png", yeni);
    cv::imwrite("karsilastirma.png", yanYana);
    std::cout << "Kaydedildi: kontrast_gerilmis.png, karsilastirma.png" << std::endl;
    return 0;
}
