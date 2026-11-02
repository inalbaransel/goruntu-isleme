// Hafta 6 test görüntülerini üretir:
//   cizgiler.png    : doğru içeren şekiller (Görev 1, 2)
//   cemberler.png   : farklı boyutta çemberler / paralar (Görev 3, 4)
//   es_merkezli.png : iki grup eş merkezli çember + tek başına bir çember (Görev 5)
// Hepsine biraz gürültü eklenir ki gerçek resme benzesin.

#include <opencv2/opencv.hpp>
#include <iostream>

// Resme Gauss gürültüsü ve birkaç tuz-biber noktası ekle
void gurultuEkle(cv::Mat& img) {
    cv::Mat gurultu(img.size(), CV_8UC1);
    cv::randn(gurultu, 0, 12);
    img += gurultu;
    for (int i = 0; i < 400; i++) {
        int x = rand() % img.cols, y = rand() % img.rows;
        img.at<uchar>(y, x) = (i % 2 == 0) ? 255 : 0;
    }
}

int main() {
    srand(42);
    cv::theRNG().state = 42;

    // ---- 1) Doğrular: bir ev ve birkaç çubuk ----
    cv::Mat cizgi(400, 400, CV_8UC1, cv::Scalar(60));
    cv::rectangle(cizgi, cv::Point(80, 200), cv::Point(260, 340), cv::Scalar(200), cv::FILLED);  // ev gövdesi
    std::vector<cv::Point> cati = {{60, 200}, {170, 110}, {280, 200}};
    cv::fillConvexPoly(cizgi, cati, cv::Scalar(150));                                           // çatı
    cv::rectangle(cizgi, cv::Point(150, 260), cv::Point(190, 340), cv::Scalar(90), cv::FILLED);  // kapı
    cv::line(cizgi, cv::Point(300, 40), cv::Point(380, 360), cv::Scalar(230), 4);                // eğik çubuk
    cv::line(cizgi, cv::Point(20, 370), cv::Point(380, 370), cv::Scalar(230), 4);                // zemin
    gurultuEkle(cizgi);

    // ---- 2) Çemberler: farklı boyutta dolu daireler (paralar gibi) ----
    cv::Mat cember(400, 400, CV_8UC1, cv::Scalar(50));
    cv::circle(cember, cv::Point(100, 100), 60, cv::Scalar(200), cv::FILLED);
    cv::circle(cember, cv::Point(280, 120), 45, cv::Scalar(170), cv::FILLED);
    cv::circle(cember, cv::Point(150, 290), 75, cv::Scalar(220), cv::FILLED);
    cv::circle(cember, cv::Point(320, 300), 35, cv::Scalar(180), cv::FILLED);
    gurultuEkle(cember);

    // ---- 3) Eş merkezli çemberler: 2 hedef tahtası + 1 tek çember ----
    cv::Mat es(400, 400, CV_8UC1, cv::Scalar(40));
    for (int r : {80, 55, 30}) cv::circle(es, cv::Point(130, 140), r, cv::Scalar(220), 3);  // 1. grup (3 halka)
    for (int r : {60, 35}) cv::circle(es, cv::Point(290, 280), r, cv::Scalar(220), 3);      // 2. grup (2 halka)
    cv::circle(es, cv::Point(300, 90), 45, cv::Scalar(220), 3);                            // tek çember
    gurultuEkle(es);

    cv::imwrite("cizgiler.png", cizgi);
    cv::imwrite("cemberler.png", cember);
    cv::imwrite("es_merkezli.png", es);
    std::cout << "Uretildi: cizgiler.png, cemberler.png, es_merkezli.png" << std::endl;
    return 0;
}
