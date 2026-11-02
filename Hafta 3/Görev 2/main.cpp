// Hafta 3 - Görev 2: Histogram eşitleme
// Histogramı çıkar -> kümülatif dağılımı (CDF) hesapla -> CDF ile eşitle.
// (Hazır fonksiyon kullanılmadan)
//
//   yeni(v) = (CDF(v) - CDF_min) * 255 / (N - CDF_min)
//   N = toplam piksel sayısı, CDF_min = sıfır olmayan ilk CDF değeri

#include <opencv2/opencv.hpp>
#include <iostream>

// Histogramı çizer (sadece görmek için)
cv::Mat histogramCiz(int hist[256]) {
    int enBuyuk = 0;
    for (int k = 0; k < 256; k++) {
        if (hist[k] > enBuyuk) enBuyuk = hist[k];
    }
    cv::Mat grafik(300, 256, CV_8UC1, cv::Scalar(255));   // beyaz tuval
    for (int k = 0; k < 256; k++) {
        int boy = hist[k] * 300 / enBuyuk;
        cv::line(grafik, cv::Point(k, 300), cv::Point(k, 300 - boy), cv::Scalar(0));
    }
    return grafik;
}

int main() {
    cv::Mat img = cv::imread("../goruntuler/dusuk_kontrast.png", cv::IMREAD_GRAYSCALE);
    if (img.empty()) {
        std::cout << "Goruntu okunamadi!" << std::endl;
        return 1;
    }

    // 1) Histogram: hist[k] = değeri k olan piksel sayısı
    int hist[256] = {0};
    for (int y = 0; y < img.rows; y++) {
        for (int x = 0; x < img.cols; x++) {
            hist[img.at<uchar>(y, x)]++;
        }
    }

    // 2) CDF: cdf[k] = değeri k veya daha küçük olan piksel sayısı
    int cdf[256];
    cdf[0] = hist[0];
    for (int k = 1; k < 256; k++) {
        cdf[k] = cdf[k - 1] + hist[k];
    }

    int N = img.rows * img.cols;   // toplam piksel (= cdf[255])
    int cdfMin = 0;                // sıfır olmayan ilk CDF değeri
    for (int k = 0; k < 256; k++) {
        if (cdf[k] > 0) {
            cdfMin = cdf[k];
            break;                 // bulduk, döngüden çık
        }
    }

    // 3) Dönüşüm tablosu: eski değer -> yeni değer
    uchar tablo[256];
    for (int k = 0; k < 256; k++) {
        double oran = (double)(cdf[k] - cdfMin) / (N - cdfMin);
        tablo[k] = cv::saturate_cast<uchar>(oran * 255);   // yuvarla ve 0-255'te tut
    }

    // 4) Tabloyu her piksele uygula
    cv::Mat yeni = img.clone();
    for (int y = 0; y < img.rows; y++) {
        for (int x = 0; x < img.cols; x++) {
            yeni.at<uchar>(y, x) = tablo[img.at<uchar>(y, x)];
        }
    }

    // 5) Yeni görüntünün histogramı (karşılaştırma için)
    int yeniHist[256] = {0};
    for (int y = 0; y < yeni.rows; y++) {
        for (int x = 0; x < yeni.cols; x++) {
            yeniHist[yeni.at<uchar>(y, x)]++;
        }
    }

    std::cout << "N = " << N << ", CDF_min = " << cdfMin << std::endl;

    // 6) Kaydet
    cv::Mat yanYana, grafikler;
    cv::hconcat(img, yeni, yanYana);                                  // orijinal | eşitlenmiş
    cv::hconcat(histogramCiz(hist), histogramCiz(yeniHist), grafikler); // histogram önce | sonra
    cv::imwrite("esitlenmis.png", yeni);
    cv::imwrite("karsilastirma.png", yanYana);
    cv::imwrite("histogramlar.png", grafikler);
    std::cout << "Kaydedildi: esitlenmis.png, karsilastirma.png, histogramlar.png" << std::endl;
    return 0;
}
