// Hafta 6 - Görev 3: Canny + medyan filtre + OpenCV HoughCircles
//   1) Canny ile kenarları bul
//   2) Kenar resmini medyan filtreden geçir
//   3) cv::HoughCircles ile çemberleri bul
//   4) Çemberleri orijinal resim üzerine farklı bir renkle çiz
//
// Not (Görev 1'deki gibi): 1 piksellik Canny kenarları 3x3 medyandan sağ
// çıkamadığı için medyandan önce kenarları 3x3 genişletiyoruz (dilate).

#include <opencv2/opencv.hpp>
#include <iostream>

int main() {
    cv::Mat img = cv::imread("../goruntuler/cemberler.png", cv::IMREAD_GRAYSCALE);
    if (img.empty()) {
        std::cout << "Goruntu okunamadi!" << std::endl;
        return 1;
    }

    // ================= 1) CANNY =================
    cv::Mat yumusak, kenar;
    cv::GaussianBlur(img, yumusak, cv::Size(5, 5), 1.5);
    cv::Canny(yumusak, kenar, 50, 150);

    // ================= 2) MEDYAN FİLTRE =================
    cv::Mat kalin, temizKenar;
    cv::dilate(kenar, kalin, cv::Mat());     // kenarları kalınlaştır
    cv::medianBlur(kalin, temizKenar, 3);    // gürültüyü temizle

    std::cout << "Canny kenar pikseli    : " << cv::countNonZero(kenar) << std::endl;
    std::cout << "Medyan sonrasi kenar   : " << cv::countNonZero(temizKenar) << std::endl;

    // ================= 3) HOUGH CIRCLES =================
    // HOUGH_GRADIENT yöntemi kenar noktalarının gradyan yönünde merkeze oy verir.
    //   dp = 1        : oylama matrisi resimle aynı çözünürlükte
    //   minDist = 40  : iki çember merkezi arasındaki en az uzaklık
    //   param1 = 100  : içerideki Canny'nin üst eşiği
    //   param2 = 30   : merkez için gereken oy eşiği (küçülürse daha çok çember bulur)
    //   yarıçap aralığı 20..100 piksel
    std::vector<cv::Vec3f> cemberler;
    cv::HoughCircles(temizKenar, cemberler, cv::HOUGH_GRADIENT, 1, 40, 100, 30, 20, 100);

    // ================= 4) ÇİZ =================
    cv::Mat sonuc;
    cv::cvtColor(img, sonuc, cv::COLOR_GRAY2BGR);
    std::cout << "\nBulunan cemberler:" << std::endl;
    for (size_t i = 0; i < cemberler.size(); i++) {
        cv::Point merkez(cvRound(cemberler[i][0]), cvRound(cemberler[i][1]));
        int r = cvRound(cemberler[i][2]);
        cv::circle(sonuc, merkez, r, cv::Scalar(0, 255, 0), 2);     // çember: YEŞİL
        cv::circle(sonuc, merkez, 3, cv::Scalar(0, 0, 255), -1);    // merkez: KIRMIZI
        std::cout << "  merkez = (" << merkez.x << ", " << merkez.y << "), yaricap = " << r << std::endl;
    }
    std::cout << "Toplam " << cemberler.size() << " cember" << std::endl;

    // ================= KAYDET =================
    cv::Mat kenarRenkli, yanYana;
    cv::cvtColor(temizKenar, kenarRenkli, cv::COLOR_GRAY2BGR);
    cv::hconcat(kenarRenkli, sonuc, yanYana);
    cv::imwrite("kenar_canny.png", kenar);
    cv::imwrite("kenar_medyan.png", temizKenar);
    cv::imwrite("cemberler_sonuc.png", sonuc);
    cv::imwrite("karsilastirma.png", yanYana);
    std::cout << "\nKaydedildi: kenar_canny.png, kenar_medyan.png, cemberler_sonuc.png, karsilastirma.png" << std::endl;
    return 0;
}
