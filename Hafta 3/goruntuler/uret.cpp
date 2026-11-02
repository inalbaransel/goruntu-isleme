// Hafta 3 test görüntülerini üretir.
// Verilen (normal) bir fotoğraftan:
//   temiz.png           : gri seviye orijinal
//   dusuk_kontrast.png  : 0.35 * p + 80  (değerler yaklaşık 80..170 arasına sıkışır)
//   gurultulu.png       : Gauss gürültüsü eklenmiş (sigma = 20)
//   tuz_biber.png       : %5 tuz (255) + %5 biber (0) gürültüsü
//
// Kullanım: ./uret <kaynak_fotograf>   (çıktılar çalışılan dizine yazılır)

#include <opencv2/opencv.hpp>

#include <iostream>

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "Kullanim: " << argv[0] << " <kaynak_fotograf>\n";
        return 1;
    }
    cv::Mat gri = cv::imread(argv[1], cv::IMREAD_GRAYSCALE);
    if (gri.empty()) {
        std::cerr << "Goruntu okunamadi: " << argv[1] << "\n";
        return 1;
    }
    cv::theRNG().state = 42;  // her seferinde aynı gürültü

    cv::Mat dusuk;
    gri.convertTo(dusuk, CV_8U, 0.35, 80);

    cv::Mat gurultu(gri.size(), CV_16S), gurultulu;
    cv::randn(gurultu, 0, 20);
    cv::Mat gri16;
    gri.convertTo(gri16, CV_16S);
    cv::Mat(gri16 + gurultu).convertTo(gurultulu, CV_8U);

    cv::Mat tuzBiber = gri.clone(), rastgele(gri.size(), CV_32F);
    cv::randu(rastgele, 0, 1);
    tuzBiber.setTo(0, rastgele < 0.05);
    tuzBiber.setTo(255, rastgele > 0.95);

    cv::imwrite("temiz.png", gri);
    cv::imwrite("dusuk_kontrast.png", dusuk);
    cv::imwrite("gurultulu.png", gurultulu);
    cv::imwrite("tuz_biber.png", tuzBiber);
    std::cout << "Uretildi (" << gri.cols << "x" << gri.rows
              << "): temiz.png, dusuk_kontrast.png, gurultulu.png, tuz_biber.png\n";
    return 0;
}
