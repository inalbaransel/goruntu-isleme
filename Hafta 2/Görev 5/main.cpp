// Hafta 2 - Görev 5
// Histogram hesaplamayı 1 thread ve 4 thread ile yapıp
// sürelerini karşılaştır.

#include <opencv2/opencv.hpp>
#include <chrono>
#include <iostream>
#include <thread>

// Bir parçanın histogramını "hist" dizisine yazar
void histogramHesapla(cv::Mat parca, int* hist) {
    for (int y = 0; y < parca.rows; y++) {
        for (int x = 0; x < parca.cols; x++) {
            hist[parca.at<uchar>(y, x)]++;
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
    int R = img.rows;

    // ---------------- 1 THREAD ----------------
    int hist1[256] = {0};
    auto bas = std::chrono::steady_clock::now();

    histogramHesapla(img, hist1);                // tüm görüntü tek seferde

    auto son = std::chrono::steady_clock::now();
    double sure1 = std::chrono::duration<double, std::milli>(son - bas).count();

    // ---------------- 4 THREAD ----------------
    int parcaHist[4][256] = {0};
    int hist4[256] = {0};
    bas = std::chrono::steady_clock::now();

    std::thread t[4];
    for (int i = 0; i < 4; i++) {
        t[i] = std::thread(histogramHesapla, img.rowRange(i * R / 4, (i + 1) * R / 4), parcaHist[i]);
    }
    for (int i = 0; i < 4; i++) {
        t[i].join();
    }
    for (int i = 0; i < 4; i++) {                // birleştirme de süreye dahil
        for (int k = 0; k < 256; k++) {
            hist4[k] += parcaHist[i][k];
        }
    }

    son = std::chrono::steady_clock::now();
    double sure4 = std::chrono::duration<double, std::milli>(son - bas).count();

    // ---------------- SONUÇ ----------------
    std::cout << "1 thread : " << sure1 << " ms" << std::endl;
    std::cout << "4 thread : " << sure4 << " ms" << std::endl;
    std::cout << "Hizlanma : " << sure1 / sure4 << " kat" << std::endl;

    // İki yöntem aynı histogramı vermeli
    bool ayni = true;
    for (int k = 0; k < 256; k++) {
        if (hist1[k] != hist4[k]) ayni = false;
    }
    std::cout << "Histogramlar ayni mi? " << (ayni ? "evet" : "hayir") << std::endl;
    return 0;
}
