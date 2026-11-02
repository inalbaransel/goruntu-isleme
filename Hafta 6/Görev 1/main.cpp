// Hafta 6 - Görev 1: Canny + medyan filtre + OpenCV Hough doğru bulma
//   1) Canny ile kenarları bul
//   2) Kenar resmini medyan filtreden geçir (tek tük gürültü kenarlarını sil)
//   3) İki farklı OpenCV Hough fonksiyonu ile doğruları bul:
//        a) cv::HoughLines  : standart Hough, sonsuz doğru verir (rho, theta)
//        b) cv::HoughLinesP : olasılıksal Hough, doğru PARÇASI verir (x1,y1,x2,y2)
//   4) Doğruları orijinal resim üzerine farklı renklerle çiz
//
// Not: Canny kenarları 1 piksel kalınlığındadır. 3x3 medyan filtrede bir kenar
// pikselinin penceresindeki 9 pikselin sadece ~3'ü kenar olduğu için medyan 0
// çıkar ve kenarların çoğu silinir. Bu yüzden medyandan önce kenarları 3x3
// genişletiyoruz (dilate): çizgiler 3 piksel kalınlaşır ve medyandan sağ çıkar,
// tek başına duran gürültü noktaları ise yine silinir.

#include <opencv2/opencv.hpp>
#include <cmath>
#include <iostream>

int main() {
    cv::Mat img = cv::imread("../goruntuler/cizgiler.png", cv::IMREAD_GRAYSCALE);
    if (img.empty()) {
        std::cout << "Goruntu okunamadi!" << std::endl;
        return 1;
    }

    // ================= 1) CANNY =================
    cv::Mat yumusak, kenar;
    cv::GaussianBlur(img, yumusak, cv::Size(5, 5), 1.5);   // gürültüyü biraz azalt
    cv::Canny(yumusak, kenar, 50, 150);

    // ================= 2) MEDYAN FİLTRE =================
    cv::Mat sadeceMedyan;                                  // karşılaştırma için: direkt medyan
    cv::medianBlur(kenar, sadeceMedyan, 3);

    cv::Mat kalin, temizKenar;
    cv::dilate(kenar, kalin, cv::Mat());                   // kenarları 3 piksel kalınlaştır
    cv::medianBlur(kalin, temizKenar, 3);                  // gürültüyü temizle

    std::cout << "Canny kenar pikseli          : " << cv::countNonZero(kenar) << std::endl;
    std::cout << "Direkt medyan sonrasi        : " << cv::countNonZero(sadeceMedyan)
              << "  (kenarlarin cogu silindi)" << std::endl;
    std::cout << "Kalinlastirma + medyan sonrasi: " << cv::countNonZero(temizKenar) << std::endl;

    // ================= 3a) STANDART HOUGH (cv::HoughLines) =================
    // rho çözünürlüğü 1 piksel, theta çözünürlüğü 1 derece, en az 120 oy
    std::vector<cv::Vec2f> hepsi;
    cv::HoughLines(temizKenar, hepsi, 1, CV_PI / 180, 120);

    // Kalın kenar aynı doğru için birbirine çok yakın birçok (rho, theta) verir.
    // HoughLines sonuçları oy sayısına göre sıralı döner; daha önce seçilmiş bir
    // doğruya çok yakın olanları (rho farkı < 10 piksel, açı farkı < 5 derece) atla.
    std::vector<cv::Vec2f> dogrular;
    for (size_t i = 0; i < hepsi.size(); i++) {
        bool yakin = false;
        for (size_t j = 0; j < dogrular.size(); j++) {
            if (std::abs(hepsi[i][0] - dogrular[j][0]) < 10 &&
                std::abs(hepsi[i][1] - dogrular[j][1]) < 5 * CV_PI / 180) {
                yakin = true;
            }
        }
        if (!yakin) dogrular.push_back(hepsi[i]);
    }

    cv::Mat sonuc1;
    cv::cvtColor(img, sonuc1, cv::COLOR_GRAY2BGR);
    for (size_t i = 0; i < dogrular.size(); i++) {
        float rho = dogrular[i][0], theta = dogrular[i][1];
        // Doğru üzerindeki en yakın nokta (x0, y0), oradan iki yöne uzağa git
        double a = std::cos(theta), b = std::sin(theta);
        double x0 = a * rho, y0 = b * rho;
        cv::Point p1(cvRound(x0 - 1000 * b), cvRound(y0 + 1000 * a));
        cv::Point p2(cvRound(x0 + 1000 * b), cvRound(y0 - 1000 * a));
        cv::line(sonuc1, p1, p2, cv::Scalar(0, 0, 255), 1);          // KIRMIZI
    }
    std::cout << "\nHoughLines  : " << hepsi.size() << " dogru bulundu, yakinlar elenince "
              << dogrular.size() << " kaldi" << std::endl;

    // ================= 3b) OLASILIKSAL HOUGH (cv::HoughLinesP) =================
    // en az 60 oy, en az 50 piksel uzunluk, aradaki 10 piksellik boşluklar birleştirilir
    std::vector<cv::Vec4i> parcalar;
    cv::HoughLinesP(temizKenar, parcalar, 1, CV_PI / 180, 60, 50, 10);

    cv::Mat sonuc2;
    cv::cvtColor(img, sonuc2, cv::COLOR_GRAY2BGR);
    for (size_t i = 0; i < parcalar.size(); i++) {
        cv::Vec4i p = parcalar[i];
        cv::line(sonuc2, cv::Point(p[0], p[1]), cv::Point(p[2], p[3]), cv::Scalar(0, 255, 0), 2);  // YEŞİL
    }
    std::cout << "HoughLinesP : " << parcalar.size() << " dogru parcasi bulundu" << std::endl;

    // ================= KAYDET =================
    cv::Mat kenarRenkli, ikili, yanYana;
    cv::cvtColor(temizKenar, kenarRenkli, cv::COLOR_GRAY2BGR);
    cv::hconcat(kenarRenkli, sonuc1, ikili);
    cv::hconcat(ikili, sonuc2, yanYana);
    cv::imwrite("kenar_canny.png", kenar);
    cv::imwrite("kenar_medyan.png", temizKenar);
    cv::imwrite("houghlines.png", sonuc1);
    cv::imwrite("houghlinesp.png", sonuc2);
    cv::imwrite("karsilastirma.png", yanYana);
    std::cout << "\nKaydedildi: kenar_canny.png, kenar_medyan.png, houghlines.png, houghlinesp.png, karsilastirma.png"
              << std::endl;
    std::cout << "(karsilastirma.png: kenar | HoughLines (kirmizi) | HoughLinesP (yesil))" << std::endl;
    return 0;
}
