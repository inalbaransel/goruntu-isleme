// Hafta 2 - Görev 3
// Parlaklık değiştirme işlemini 1 thread ve 4 thread ile yapıp
// sürelerini karşılaştır.

#include <opencv2/opencv.hpp>
#include <chrono>
#include <iostream>
#include <thread>

// Bir parçadaki her piksele "deger" ekler
void parlaklik(cv::Mat parca, int deger) {
    for (int y = 0; y < parca.rows; y++) {
        for (int x = 0; x < parca.cols; x++) {
            int yeniDeger = parca.at<uchar>(y, x) + deger;
            parca.at<uchar>(y, x) = cv::saturate_cast<uchar>(yeniDeger);
        }
    }
}

int main() {
    cv::Mat img = cv::imread("../goruntu.jpg", cv::IMREAD_GRAYSCALE);
    if (img.empty()) {
        std::cout << "Goruntu okunamadi!" << std::endl;
        return 1;
    }

    // Görüntü küçükse (512x512) fark görünmez, o yüzden büyütüyoruz
    cv::resize(img, img, cv::Size(4096, 4096));

    int degerler[4] = {-60, -20, 20, 60};
    int R = img.rows;

    // ---------------- 1 THREAD ----------------
    cv::Mat tek = img.clone();
    auto bas = std::chrono::steady_clock::now(); // kronometreyi başlat

    for (int i = 0; i < 4; i++) {                // 4 parçayı SIRAYLA işle
        parlaklik(tek.rowRange(i * R / 4, (i + 1) * R / 4), degerler[i]);
    }

    auto son = std::chrono::steady_clock::now(); // kronometreyi durdur
    double sure1 = std::chrono::duration<double, std::milli>(son - bas).count();

    // ---------------- 4 THREAD ----------------
    cv::Mat cok = img.clone();
    bas = std::chrono::steady_clock::now();

    std::thread t[4];
    for (int i = 0; i < 4; i++) {                // 4 parçayı AYNI ANDA işle
        t[i] = std::thread(parlaklik, cok.rowRange(i * R / 4, (i + 1) * R / 4), degerler[i]);
    }
    for (int i = 0; i < 4; i++) {
        t[i].join();
    }

    son = std::chrono::steady_clock::now();
    double sure4 = std::chrono::duration<double, std::milli>(son - bas).count();

    // ---------------- SONUÇ ----------------
    std::cout << "1 thread : " << sure1 << " ms" << std::endl;
    std::cout << "4 thread : " << sure4 << " ms" << std::endl;
    std::cout << "Hizlanma : " << sure1 / sure4 << " kat" << std::endl;
    return 0;
}
