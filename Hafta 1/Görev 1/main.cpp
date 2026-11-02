#include <filesystem>         // klasör aletleri
#include <iostream>           // ekrana yazma
#include <opencv2/opencv.hpp> // resim aletleri

namespace fs = std::filesystem; // kısa isim: fs

int main() {                      // program buradan başlar
  fs::create_directory("output"); // output klasörünü oluştur

  for (const auto &dosya :
       fs::directory_iterator("input")) { // input'taki HER dosya için:

    cv::Mat img = cv::imread(dosya.path().string()); // 1) resmi aç
    if (img.empty())
      continue; //    açılmadıysa sıradakine geç

    cv::Mat yeni;                               // 2) boş kutu
    cv::resize(img, yeni, cv::Size(1024, 768)); //    1024x768 yap, yeni'ye koy

    cv::imwrite("output/" + dosya.path().filename().string(),
                yeni); // 3) kaydet
    std::cout << dosya.path().filename() << " tamam"
              << std::endl; //    ekrana yaz
  }

  return 0; // bitti
}