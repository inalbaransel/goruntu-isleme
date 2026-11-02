// Hafta 3 - Ödev: görevlerin çoklu iş parçacıklı (multi-threaded) CPU sürümleri
//
// Her işlem, kullanılacak thread sayısını parametre olarak alır. Görüntü
// satır bantlarına bölünür ve her bandı ayrı bir std::thread işler.
// Thread'ler farklı satırlara yazdığı için kilit (mutex) gerekmez; ortak
// toplama gereken yerlerde (min/max, histogram) her thread kendi kısmi
// sonucunu üretir, thread'ler bitince bu sonuçlar birleştirilir.
//
// Bu dosya hem main_mt.cpp hem de main_cuda.cu tarafından (CPU referansı
// olarak) kullanılır.

#pragma once

#include <opencv2/opencv.hpp>

#include <algorithm>
#include <cmath>
#include <thread>
#include <vector>

namespace islem {

// [0, n) aralığını 'threadSayisi' parçaya böler; her parça için
// fonk(parcaNo, bas, bit) ayrı bir thread'de çalıştırılır.
template <typename F>
void paralel(int n, int threadSayisi, F fonk) {
    threadSayisi = std::max(1, std::min(threadSayisi, n));
    if (threadSayisi == 1) {
        fonk(0, 0, n);
        return;
    }
    std::vector<std::thread> threadler;
    for (int t = 0; t < threadSayisi; ++t) {
        int bas = static_cast<int>(static_cast<long long>(n) * t / threadSayisi);
        int bit = static_cast<int>(static_cast<long long>(n) * (t + 1) / threadSayisi);
        threadler.emplace_back(fonk, t, bas, bit);
    }
    for (auto& th : threadler) th.join();
}

// Tablo (LUT) uygulama: cikis(y,x) = tablo[giris(y,x)]
inline cv::Mat tabloUygula(const cv::Mat& gri, const uchar tablo[256], int threadSayisi) {
    cv::Mat cikis(gri.size(), CV_8UC1);
    paralel(gri.rows, threadSayisi, [&](int, int bas, int bit) {
        for (int y = bas; y < bit; ++y) {
            const uchar* g = gri.ptr<uchar>(y);
            uchar* c = cikis.ptr<uchar>(y);
            for (int x = 0; x < gri.cols; ++x) c[x] = tablo[g[x]];
        }
    });
    return cikis;
}

// ---------------------------------------------------------------------------
// Görev 1: min/max ile doğrusal kontrast germe
// ---------------------------------------------------------------------------
inline void kontrastTablosu(int enKucuk, int enBuyuk, uchar tablo[256]) {
    int aralik = std::max(enBuyuk - enKucuk, 1);
    for (int v = 0; v < 256; ++v) {
        int fark = std::clamp(v - enKucuk, 0, aralik);
        tablo[v] = static_cast<uchar>((fark * 255 * 2 + aralik) / (2 * aralik));
    }
}

inline cv::Mat kontrastGer(const cv::Mat& gri, int threadSayisi) {
    std::vector<int> mins(threadSayisi, 255), maxs(threadSayisi, 0);
    paralel(gri.rows, threadSayisi, [&](int t, int bas, int bit) {
        int mn = 255, mx = 0;
        for (int y = bas; y < bit; ++y) {
            const uchar* s = gri.ptr<uchar>(y);
            for (int x = 0; x < gri.cols; ++x) {
                mn = std::min<int>(mn, s[x]);
                mx = std::max<int>(mx, s[x]);
            }
        }
        mins[t] = mn;
        maxs[t] = mx;
    });
    int enKucuk = *std::min_element(mins.begin(), mins.end());
    int enBuyuk = *std::max_element(maxs.begin(), maxs.end());

    uchar tablo[256];
    kontrastTablosu(enKucuk, enBuyuk, tablo);
    return tabloUygula(gri, tablo, threadSayisi);
}

// ---------------------------------------------------------------------------
// Görev 2: histogram + CDF ile histogram eşitleme
// ---------------------------------------------------------------------------
inline void esitlemeTablosu(const long long hist[256], uchar tablo[256]) {
    long long cdf[256];
    cdf[0] = hist[0];
    for (int i = 1; i < 256; ++i) cdf[i] = cdf[i - 1] + hist[i];
    long long N = cdf[255], cdfMin = 0;
    for (int i = 0; i < 256; ++i)
        if (cdf[i] > 0) { cdfMin = cdf[i]; break; }
    for (int v = 0; v < 256; ++v) {
        double oran = N == cdfMin ? 0.0 : double(cdf[v] - cdfMin) / double(N - cdfMin);
        tablo[v] = static_cast<uchar>(std::floor(std::clamp(oran, 0.0, 1.0) * 255.0 + 0.5));
    }
}

inline cv::Mat histogramEsitle(const cv::Mat& gri, int threadSayisi) {
    std::vector<std::vector<long long>> kismi(threadSayisi, std::vector<long long>(256, 0));
    paralel(gri.rows, threadSayisi, [&](int t, int bas, int bit) {
        long long* h = kismi[t].data();
        for (int y = bas; y < bit; ++y) {
            const uchar* s = gri.ptr<uchar>(y);
            for (int x = 0; x < gri.cols; ++x) ++h[s[x]];
        }
    });
    long long hist[256] = {0};
    for (const auto& h : kismi)
        for (int i = 0; i < 256; ++i) hist[i] += h[i];

    uchar tablo[256];
    esitlemeTablosu(hist, tablo);
    return tabloUygula(gri, tablo, threadSayisi);
}

// ---------------------------------------------------------------------------
// Görev 3: CLAHE (OpenCV'nin CPU gerçeklemesiyle aynı sonucu verir)
// ---------------------------------------------------------------------------
struct ClaheBoyut {
    int karoW, karoH, limit;
    float lutOlcek;
};

inline int yansit(int i, int n) { return i < n ? i : 2 * (n - 1) - i; }

inline ClaheBoyut claheBoyut(cv::Size boyut, double kirpmaLimiti, int karoX, int karoY) {
    int genW = boyut.width, genH = boyut.height;
    if (genW % karoX != 0 || genH % karoY != 0) {
        genW += karoX - genW % karoX;
        genH += karoY - genH % karoY;
    }
    ClaheBoyut b;
    b.karoW = genW / karoX;
    b.karoH = genH / karoY;
    int alan = b.karoW * b.karoH;
    b.limit = std::max(static_cast<int>(kirpmaLimiti * alan / 256), 1);
    b.lutOlcek = 255.0f / alan;
    return b;
}

// Kırpılmış histogramdan karo tablosu
inline void claheKaroTablosu(int hist[256], int limit, float lutOlcek, uchar* tablo) {
    int kirpilan = 0;
    for (int i = 0; i < 256; ++i)
        if (hist[i] > limit) {
            kirpilan += hist[i] - limit;
            hist[i] = limit;
        }
    int herSeviyeye = kirpilan / 256, artan = kirpilan - herSeviyeye * 256;
    for (int i = 0; i < 256; ++i) hist[i] += herSeviyeye;
    if (artan > 0) {
        int adim = std::max(256 / artan, 1);
        for (int i = 0; i < 256 && artan > 0; i += adim, --artan) ++hist[i];
    }
    int toplam = 0;
    for (int i = 0; i < 256; ++i) {
        toplam += hist[i];
        tablo[i] = cv::saturate_cast<uchar>(toplam * lutOlcek);
    }
}

inline cv::Mat clahe(const cv::Mat& gri, double kirpmaLimiti, int karoX, int karoY, int threadSayisi) {
    const int W = gri.cols, H = gri.rows;
    ClaheBoyut b = claheBoyut(gri.size(), kirpmaLimiti, karoX, karoY);

    // Karo tabloları: karolar thread'lere paylaştırılır
    std::vector<uchar> tablolar(karoX * karoY * 256);
    paralel(karoX * karoY, threadSayisi, [&](int, int bas, int bit) {
        for (int k = bas; k < bit; ++k) {
            int ky = k / karoX, kx = k % karoX;
            int hist[256] = {0};
            for (int y = ky * b.karoH; y < (ky + 1) * b.karoH; ++y) {
                const uchar* s = gri.ptr<uchar>(yansit(y, H));
                for (int x = kx * b.karoW; x < (kx + 1) * b.karoW; ++x) ++hist[s[yansit(x, W)]];
            }
            claheKaroTablosu(hist, b.limit, b.lutOlcek, &tablolar[k * 256]);
        }
    });

    // Bilineer interpolasyon: satırlar thread'lere paylaştırılır
    cv::Mat cikis(gri.size(), CV_8UC1);
    const float tersW = 1.0f / b.karoW, tersH = 1.0f / b.karoH;
    paralel(H, threadSayisi, [&](int, int bas, int bit) {
        for (int y = bas; y < bit; ++y) {
            float kyf = y * tersH - 0.5f;
            int ky1 = static_cast<int>(std::floor(kyf)), ky2 = ky1 + 1;
            float ya = kyf - ky1, ya1 = 1.0f - ya;
            ky1 = std::max(ky1, 0);
            ky2 = std::min(ky2, karoY - 1);
            const uchar* g = gri.ptr<uchar>(y);
            uchar* c = cikis.ptr<uchar>(y);
            for (int x = 0; x < W; ++x) {
                float kxf = x * tersW - 0.5f;
                int kx1 = static_cast<int>(std::floor(kxf)), kx2 = kx1 + 1;
                float xa = kxf - kx1, xa1 = 1.0f - xa;
                kx1 = std::max(kx1, 0);
                kx2 = std::min(kx2, karoX - 1);
                int p = g[x];
                float ust = tablolar[(ky1 * karoX + kx1) * 256 + p] * xa1 +
                            tablolar[(ky1 * karoX + kx2) * 256 + p] * xa;
                float alt = tablolar[(ky2 * karoX + kx1) * 256 + p] * xa1 +
                            tablolar[(ky2 * karoX + kx2) * 256 + p] * xa;
                c[x] = cv::saturate_cast<uchar>(ust * ya1 + alt * ya);
            }
        }
    });
    return cikis;
}

// ---------------------------------------------------------------------------
// Görev 4: 3x3 ortalama filtre (konvolüsyon, çekirdek = 1/9 * birler matrisi)
// Tam sayı toplam + yuvarlama: round(toplam / 9) = (toplam + 4) / 9
// ---------------------------------------------------------------------------
inline cv::Mat ortalama3x3(const cv::Mat& gri, int threadSayisi) {
    cv::Mat cikis(gri.size(), CV_8UC1);
    paralel(gri.rows, threadSayisi, [&](int, int bas, int bit) {
        for (int y = bas; y < bit; ++y)
            for (int x = 0; x < gri.cols; ++x) {
                int toplam = 0;
                for (int i = -1; i <= 1; ++i) {
                    const uchar* s = gri.ptr<uchar>(std::clamp(y + i, 0, gri.rows - 1));
                    for (int j = -1; j <= 1; ++j) toplam += s[std::clamp(x + j, 0, gri.cols - 1)];
                }
                cikis.at<uchar>(y, x) = static_cast<uchar>((toplam + 4) / 9);
            }
    });
    return cikis;
}

// ---------------------------------------------------------------------------
// Görev 5: 5x5 medyan filtre
// ---------------------------------------------------------------------------
inline cv::Mat medyan5x5(const cv::Mat& gri, int threadSayisi) {
    cv::Mat cikis(gri.size(), CV_8UC1);
    paralel(gri.rows, threadSayisi, [&](int, int bas, int bit) {
        uchar p[25];
        for (int y = bas; y < bit; ++y)
            for (int x = 0; x < gri.cols; ++x) {
                int k = 0;
                for (int i = -2; i <= 2; ++i) {
                    const uchar* s = gri.ptr<uchar>(std::clamp(y + i, 0, gri.rows - 1));
                    for (int j = -2; j <= 2; ++j) p[k++] = s[std::clamp(x + j, 0, gri.cols - 1)];
                }
                for (int a = 1; a < 25; ++a) {  // eklemeli sıralama
                    uchar d = p[a];
                    int b = a - 1;
                    while (b >= 0 && p[b] > d) {
                        p[b + 1] = p[b];
                        --b;
                    }
                    p[b + 1] = d;
                }
                cikis.at<uchar>(y, x) = p[12];
            }
    });
    return cikis;
}

}  // namespace islem
