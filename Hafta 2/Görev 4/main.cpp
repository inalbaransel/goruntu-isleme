// Hafta 2 - Görev 4
// Görüntüyü 4 parçaya böl, her parçanın histogramını ayrı bir thread ile
// hesapla, sonra 4 histogramı toplayarak ana histogramı elde et.

#include <opencv2/opencv.hpp>
#include <iostream>
#include <thread>

// Bir parçanın histogramını "hist" dizisine yazar
// hist[k] = parlaklığı k olan piksel sayısı
void histogramHesapla(cv::Mat parca, int* hist) {
    for (int y = 0; y < parca.rows; y++) {
        for (int x = 0; x < parca.cols; x++) {
            int deger = parca.at<uchar>(y, x);
            hist[deger]++;                       // o değerin kutusunu 1 artır
        }
    }
}

int main() {
    cv::Mat img = cv::imread("../goruntu.jpg", cv::IMREAD_GRAYSCALE);
    if (img.empty()) {
        std::cout << "Goruntu okunamadi!" << std::endl;
        return 1;
    }

    int R = img.rows;
    int parcaHist[4][256] = {0}; // her thread'in KENDİ histogramı (race condition olmasın)
    int anaHist[256] = {0};      // birleştirilmiş histogram

    // 1) 4 thread ile 4 parçanın histogramını hesapla
    std::thread t[4];
    for (int i = 0; i < 4; i++) {
        cv::Mat parca = img.rowRange(i * R / 4, (i + 1) * R / 4);
        t[i] = std::thread(histogramHesapla, parca, parcaHist[i]);
    }
    for (int i = 0; i < 4; i++) {
        t[i].join();
    }

    // 2) 4 histogramı topla -> ana histogram
    for (int i = 0; i < 4; i++) {
        for (int k = 0; k < 256; k++) {
            anaHist[k] += parcaHist[i][k];
        }
    }

    // 3) Kontrol: histogramdaki toplam = görüntüdeki piksel sayısı olmalı
    int toplam = 0;
    for (int k = 0; k < 256; k++) {
        toplam += anaHist[k];
    }
    std::cout << "Histogram toplami : " << toplam << std::endl;
    std::cout << "Piksel sayisi     : " << img.rows * img.cols << std::endl;

    // 4) Histogramı çiz: her k değeri için yüksekliği sayıyla orantılı bir çizgi
    int enBuyuk = 0;
    for (int k = 0; k < 256; k++) {
        if (anaHist[k] > enBuyuk) enBuyuk = anaHist[k];
    }
    cv::Mat grafik(300, 256, CV_8UC1, cv::Scalar(255));   // 256x300 beyaz tuval
    for (int k = 0; k < 256; k++) {
        int boy = anaHist[k] * 300 / enBuyuk;
        cv::line(grafik, cv::Point(k, 300), cv::Point(k, 300 - boy), cv::Scalar(0));
    }
    cv::imwrite("ana_histogram.png", grafik);
    std::cout << "Kaydedildi: ana_histogram.png" << std::endl;
    return 0;
}
