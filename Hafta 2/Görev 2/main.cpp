// Hafta 2 - Görev 2
// Görüntüyü 4 yatay parçaya böl, her parçayı ayrı bir thread ile
// farklı bir parlaklık değerine ayarla.

#include <opencv2/opencv.hpp>
#include <iostream>
#include <thread>

// Bir parçadaki her piksele "deger" ekler
void parlaklik(cv::Mat parca, int deger) {
    for (int y = 0; y < parca.rows; y++) {
        for (int x = 0; x < parca.cols; x++) {
            int yeniDeger = parca.at<uchar>(y, x) + deger;
            // saturate_cast: 255'i geçerse 255, 0'ın altına inerse 0 yapar
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

    int degerler[4] = {-60, -20, 20, 60}; // her parçanın parlaklık değeri
    int R = img.rows;                     // toplam satır sayısı

    // 4 thread başlat: i. thread, i. parçayı işlesin
    // rowRange(bas, son) = resmin o satırları (kopya değil, aynı piksellere bakar)
    std::thread t[4];
    for (int i = 0; i < 4; i++) {
        cv::Mat parca = img.rowRange(i * R / 4, (i + 1) * R / 4);
        t[i] = std::thread(parlaklik, parca, degerler[i]);
    }

    // 4 thread'in de bitmesini bekle
    for (int i = 0; i < 4; i++) {
        t[i].join();
    }

    cv::imwrite("parlaklik_4_thread.png", img);
    std::cout << "Kaydedildi: parlaklik_4_thread.png" << std::endl;
    return 0;
}
