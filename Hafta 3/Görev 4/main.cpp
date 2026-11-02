// Hafta 3 - Görev 4: 3x3 ortalama filtresi (konvolüsyon)
// 3x3'lük çekirdeği görüntü üzerinde gezdirerek gürültüyü bastır.
//
//            | 1 1 1 |
//   K = 1/9  | 1 1 1 |     -> her piksel, kendisi + 8 komşusunun ortalaması olur
//            | 1 1 1 |

#include <opencv2/opencv.hpp>
#include <iostream>

int main() {
    cv::Mat img = cv::imread("../goruntuler/gurultulu.png", cv::IMREAD_GRAYSCALE);
    if (img.empty()) {
        std::cout << "Goruntu okunamadi!" << std::endl;
        return 1;
    }

    // Çekirdek (kernel): 9 hücre, her biri 1/9
    double K[3][3] = {
        {1.0 / 9, 1.0 / 9, 1.0 / 9},
        {1.0 / 9, 1.0 / 9, 1.0 / 9},
        {1.0 / 9, 1.0 / 9, 1.0 / 9},
    };

    cv::Mat yeni = img.clone();

    // Kenar pikselleri atlıyoruz (y=0, son satır vb.), çünkü komşuları görüntü dışında kalır
    for (int y = 1; y < img.rows - 1; y++) {
        for (int x = 1; x < img.cols - 1; x++) {

            // Çekirdeği (y, x) pikselinin üstüne koy: 9 komşuyu çekirdekle çarp ve topla
            double toplam = 0;
            for (int i = -1; i <= 1; i++) {          // üst, orta, alt satır
                for (int j = -1; j <= 1; j++) {      // sol, orta, sağ sütun
                    toplam += K[i + 1][j + 1] * img.at<uchar>(y + i, x + j);
                }
            }

            yeni.at<uchar>(y, x) = cv::saturate_cast<uchar>(toplam);
        }
    }

    // Kaydet: sol gürültülü, sağ filtrelenmiş
    cv::Mat yanYana;
    cv::hconcat(img, yeni, yanYana);
    cv::imwrite("ortalama_filtreli.png", yeni);
    cv::imwrite("karsilastirma.png", yanYana);
    std::cout << "Kaydedildi: ortalama_filtreli.png, karsilastirma.png" << std::endl;
    return 0;
}
