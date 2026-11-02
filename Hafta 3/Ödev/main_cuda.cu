// Hafta 3 - Ödev (GPU, CUDA)
// Görev 1-5'in CUDA çekirdekleri (kernel) ile gerçeklenmesi. Her işlem için
// GPU sonucu, islemler.hpp'deki çoklu thread CPU sürümüyle karşılaştırılır
// (birebir aynı olmalı) ve süreler ölçülür.
//
//   1) Kontrast germe : paralel min/max indirgeme + tablo uygulama
//   2) Hist. eşitleme : paylaşımlı bellekte blok histogramları + CDF + tablo
//   3) CLAHE          : her karo için bir blok (histogram, kırpma, CDF)
//                       + piksel başına bilineer interpolasyon
//   4) Ortalama 3x3   : piksel başına bir thread
//   5) Medyan 5x5     : piksel başına bir thread, 25 elemanlı sıralama
//
// Süreler: "kernel" = sadece GPU hesaplaması, "toplam" = bellek kopyalama
// (CPU->GPU->CPU) dahil. Ölçüm 2048x2048 görüntüde yapılır.
//
// Derleme (Colab): nvcc -O2 -std=c++17 -fmad=false main_cuda.cu -o odev_cuda \
//                  $(pkg-config --cflags --libs opencv4)
// Kullanım: ./odev_cuda [goruntu_dizini]   (varsayılan: ../goruntuler)

#include "islemler.hpp"

#include <cuda_runtime.h>

#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <string>

#define CUDA_KONTROL(cagri)                                                           \
    do {                                                                              \
        cudaError_t hata = (cagri);                                                   \
        if (hata != cudaSuccess) {                                                    \
            fprintf(stderr, "CUDA hatasi %s:%d: %s\n", __FILE__, __LINE__,            \
                    cudaGetErrorString(hata));                                        \
            exit(1);                                                                  \
        }                                                                             \
    } while (0)

constexpr int BLOK = 256;
constexpr int BLOK_2B = 16;

__device__ __forceinline__ int sinirla(int v, int a, int b) { return v < a ? a : (v > b ? b : v); }
__device__ __forceinline__ unsigned char doyur(float v) {  // cv::saturate_cast<uchar>(float) ile aynı
    return static_cast<unsigned char>(sinirla(__float2int_rn(v), 0, 255));
}

// ---------------------------------------------------------------------------
// Ortak çekirdekler
// ---------------------------------------------------------------------------
__global__ void tabloUygulaK(const unsigned char* giris, unsigned char* cikis, int n,
                             const unsigned char* tablo) {
    __shared__ unsigned char t[256];
    for (int i = threadIdx.x; i < 256; i += blockDim.x) t[i] = tablo[i];
    __syncthreads();
    for (int i = blockIdx.x * blockDim.x + threadIdx.x; i < n; i += gridDim.x * blockDim.x)
        cikis[i] = t[giris[i]];
}

__global__ void histogramK(const unsigned char* giris, int n, unsigned int* hist) {
    __shared__ unsigned int yerel[256];
    for (int i = threadIdx.x; i < 256; i += blockDim.x) yerel[i] = 0;
    __syncthreads();
    for (int i = blockIdx.x * blockDim.x + threadIdx.x; i < n; i += gridDim.x * blockDim.x)
        atomicAdd(&yerel[giris[i]], 1u);
    __syncthreads();
    for (int i = threadIdx.x; i < 256; i += blockDim.x) atomicAdd(&hist[i], yerel[i]);
}

// ---------------------------------------------------------------------------
// Görev 1: kontrast germe
// ---------------------------------------------------------------------------
__global__ void minMaxK(const unsigned char* giris, int n, int* sonuc /* [0]=min, [1]=max */) {
    __shared__ int mn[BLOK], mx[BLOK];
    int yMin = 255, yMax = 0;
    for (int i = blockIdx.x * blockDim.x + threadIdx.x; i < n; i += gridDim.x * blockDim.x) {
        yMin = min(yMin, (int)giris[i]);
        yMax = max(yMax, (int)giris[i]);
    }
    mn[threadIdx.x] = yMin;
    mx[threadIdx.x] = yMax;
    __syncthreads();
    for (int s = blockDim.x / 2; s > 0; s >>= 1) {  // blok içi ağaç indirgeme
        if (threadIdx.x < s) {
            mn[threadIdx.x] = min(mn[threadIdx.x], mn[threadIdx.x + s]);
            mx[threadIdx.x] = max(mx[threadIdx.x], mx[threadIdx.x + s]);
        }
        __syncthreads();
    }
    if (threadIdx.x == 0) {
        atomicMin(&sonuc[0], mn[0]);
        atomicMax(&sonuc[1], mx[0]);
    }
}

__global__ void kontrastTablosuK(const int* minMax, unsigned char* tablo) {
    int v = threadIdx.x;
    int aralik = max(minMax[1] - minMax[0], 1);
    int fark = sinirla(v - minMax[0], 0, aralik);
    tablo[v] = static_cast<unsigned char>((fark * 255 * 2 + aralik) / (2 * aralik));
}

// ---------------------------------------------------------------------------
// Görev 2: histogram eşitleme
// ---------------------------------------------------------------------------
__global__ void esitlemeTablosuK(const unsigned int* hist, unsigned char* tablo) {
    __shared__ long long cdf[256];
    __shared__ long long cdfMin;
    int v = threadIdx.x;
    cdf[v] = hist[v];
    __syncthreads();
    for (int adim = 1; adim < 256; adim <<= 1) {  // Hillis-Steele paralel prefix toplamı
        long long ek = v >= adim ? cdf[v - adim] : 0;
        __syncthreads();
        cdf[v] += ek;
        __syncthreads();
    }
    if (v == 0) {
        cdfMin = 0;
        for (int i = 0; i < 256; ++i)
            if (cdf[i] > 0) { cdfMin = cdf[i]; break; }
    }
    __syncthreads();
    long long N = cdf[255];
    double oran = N == cdfMin ? 0.0 : double(cdf[v] - cdfMin) / double(N - cdfMin);
    oran = fmin(fmax(oran, 0.0), 1.0);
    tablo[v] = static_cast<unsigned char>(floor(oran * 255.0 + 0.5));
}

// ---------------------------------------------------------------------------
// Görev 3: CLAHE
// ---------------------------------------------------------------------------
__device__ __forceinline__ int yansitD(int i, int n) { return i < n ? i : 2 * (n - 1) - i; }

// Her blok bir karo: 256 thread histogramı, kırpmayı ve CDF'yi birlikte yapar
__global__ void claheKaroK(const unsigned char* giris, int W, int H, int karoX, int karoW, int karoH,
                           int limit, float lutOlcek, unsigned char* tablolar) {
    __shared__ int hist[256];
    __shared__ int kirpilan;
    int kx = blockIdx.x % karoX, ky = blockIdx.x / karoX;
    int v = threadIdx.x;
    hist[v] = 0;
    if (v == 0) kirpilan = 0;
    __syncthreads();

    for (int i = v; i < karoW * karoH; i += blockDim.x) {
        int y = yansitD(ky * karoH + i / karoW, H);
        int x = yansitD(kx * karoW + i % karoW, W);
        atomicAdd(&hist[giris[y * W + x]], 1);
    }
    __syncthreads();

    int h = hist[v];
    if (h > limit) {
        atomicAdd(&kirpilan, h - limit);
        h = limit;
    }
    __syncthreads();

    int herSeviyeye = kirpilan / 256, artan = kirpilan - herSeviyeye * 256;
    h += herSeviyeye;
    if (artan > 0) {  // CPU'daki "i += adim" döngüsünün paralel karşılığı
        int adim = max(256 / artan, 1);
        if (v % adim == 0 && v / adim < artan) ++h;
    }
    hist[v] = h;
    __syncthreads();

    if (v == 0) {  // 256 elemanlı CDF: tek thread yeterince hızlı
        int toplam = 0;
        unsigned char* tablo = tablolar + blockIdx.x * 256;
        for (int i = 0; i < 256; ++i) {
            toplam += hist[i];
            tablo[i] = doyur(toplam * lutOlcek);
        }
    }
}

__global__ void claheInterpolasyonK(const unsigned char* giris, unsigned char* cikis, int W, int H,
                                    int karoX, int karoY, float tersW, float tersH,
                                    const unsigned char* tablolar) {
    int x = blockIdx.x * blockDim.x + threadIdx.x;
    int y = blockIdx.y * blockDim.y + threadIdx.y;
    if (x >= W || y >= H) return;

    float kyf = y * tersH - 0.5f;
    int ky1 = static_cast<int>(floorf(kyf)), ky2 = ky1 + 1;
    float ya = kyf - ky1, ya1 = 1.0f - ya;
    ky1 = max(ky1, 0);
    ky2 = min(ky2, karoY - 1);

    float kxf = x * tersW - 0.5f;
    int kx1 = static_cast<int>(floorf(kxf)), kx2 = kx1 + 1;
    float xa = kxf - kx1, xa1 = 1.0f - xa;
    kx1 = max(kx1, 0);
    kx2 = min(kx2, karoX - 1);

    int p = giris[y * W + x];
    float ust = tablolar[(ky1 * karoX + kx1) * 256 + p] * xa1 + tablolar[(ky1 * karoX + kx2) * 256 + p] * xa;
    float alt = tablolar[(ky2 * karoX + kx1) * 256 + p] * xa1 + tablolar[(ky2 * karoX + kx2) * 256 + p] * xa;
    cikis[y * W + x] = doyur(ust * ya1 + alt * ya);
}

// ---------------------------------------------------------------------------
// Görev 4 ve 5: ortalama ve medyan filtre
// ---------------------------------------------------------------------------
__global__ void ortalama3x3K(const unsigned char* giris, unsigned char* cikis, int W, int H) {
    int x = blockIdx.x * blockDim.x + threadIdx.x;
    int y = blockIdx.y * blockDim.y + threadIdx.y;
    if (x >= W || y >= H) return;
    int toplam = 0;
    for (int i = -1; i <= 1; ++i)
        for (int j = -1; j <= 1; ++j) toplam += giris[sinirla(y + i, 0, H - 1) * W + sinirla(x + j, 0, W - 1)];
    cikis[y * W + x] = static_cast<unsigned char>((toplam + 4) / 9);
}

__global__ void medyan5x5K(const unsigned char* giris, unsigned char* cikis, int W, int H) {
    int x = blockIdx.x * blockDim.x + threadIdx.x;
    int y = blockIdx.y * blockDim.y + threadIdx.y;
    if (x >= W || y >= H) return;
    unsigned char p[25];
    int k = 0;
    for (int i = -2; i <= 2; ++i)
        for (int j = -2; j <= 2; ++j) p[k++] = giris[sinirla(y + i, 0, H - 1) * W + sinirla(x + j, 0, W - 1)];
    for (int a = 1; a < 25; ++a) {
        unsigned char d = p[a];
        int b = a - 1;
        while (b >= 0 && p[b] > d) {
            p[b + 1] = p[b];
            --b;
        }
        p[b + 1] = d;
    }
    cikis[y * W + x] = p[12];
}

// ---------------------------------------------------------------------------
// Sunucu (host) tarafı
// ---------------------------------------------------------------------------
struct GpuTampon {
    unsigned char *giris = nullptr, *cikis = nullptr, *tablo = nullptr;
    unsigned int* hist = nullptr;
    int* minMax = nullptr;
    int W = 0, H = 0;

    GpuTampon(int w, int h) : W(w), H(h) {
        CUDA_KONTROL(cudaMalloc(&giris, size_t(W) * H));
        CUDA_KONTROL(cudaMalloc(&cikis, size_t(W) * H));
        CUDA_KONTROL(cudaMalloc(&tablo, 64 * 256));  // CLAHE için 8x8 karo tablosuna yeter
        CUDA_KONTROL(cudaMalloc(&hist, 256 * sizeof(unsigned int)));
        CUDA_KONTROL(cudaMalloc(&minMax, 2 * sizeof(int)));
    }
    ~GpuTampon() {
        cudaFree(giris);
        cudaFree(cikis);
        cudaFree(tablo);
        cudaFree(hist);
        cudaFree(minMax);
    }
};

int izgaraBoyutu(int n) { return std::min((n + BLOK - 1) / BLOK, 1024); }
dim3 izgara2B(int W, int H) { return dim3((W + BLOK_2B - 1) / BLOK_2B, (H + BLOK_2B - 1) / BLOK_2B); }

// Sadece GPU hesaplaması (veri zaten g.giris'te, sonuç g.cikis'e)
void kontrastGerGpu(GpuTampon& g) {
    int n = g.W * g.H, baslangic[2] = {255, 0};
    CUDA_KONTROL(cudaMemcpy(g.minMax, baslangic, sizeof(baslangic), cudaMemcpyHostToDevice));
    minMaxK<<<izgaraBoyutu(n), BLOK>>>(g.giris, n, g.minMax);
    kontrastTablosuK<<<1, 256>>>(g.minMax, g.tablo);
    tabloUygulaK<<<izgaraBoyutu(n), BLOK>>>(g.giris, g.cikis, n, g.tablo);
}

void histogramEsitleGpu(GpuTampon& g) {
    int n = g.W * g.H;
    CUDA_KONTROL(cudaMemset(g.hist, 0, 256 * sizeof(unsigned int)));
    histogramK<<<izgaraBoyutu(n), BLOK>>>(g.giris, n, g.hist);
    esitlemeTablosuK<<<1, 256>>>(g.hist, g.tablo);
    tabloUygulaK<<<izgaraBoyutu(n), BLOK>>>(g.giris, g.cikis, n, g.tablo);
}

void claheGpu(GpuTampon& g) {
    const int karoX = 8, karoY = 8;
    islem::ClaheBoyut b = islem::claheBoyut(cv::Size(g.W, g.H), 2.0, karoX, karoY);
    claheKaroK<<<karoX * karoY, 256>>>(g.giris, g.W, g.H, karoX, b.karoW, b.karoH, b.limit, b.lutOlcek, g.tablo);
    claheInterpolasyonK<<<izgara2B(g.W, g.H), dim3(BLOK_2B, BLOK_2B)>>>(
        g.giris, g.cikis, g.W, g.H, karoX, karoY, 1.0f / b.karoW, 1.0f / b.karoH, g.tablo);
}

void ortalamaGpu(GpuTampon& g) {
    ortalama3x3K<<<izgara2B(g.W, g.H), dim3(BLOK_2B, BLOK_2B)>>>(g.giris, g.cikis, g.W, g.H);
}

void medyanGpu(GpuTampon& g) {
    medyan5x5K<<<izgara2B(g.W, g.H), dim3(BLOK_2B, BLOK_2B)>>>(g.giris, g.cikis, g.W, g.H);
}

// GPU'da çalıştırır; kernel süresini ve kopyalama dahil toplam süreyi (ms) döndürür
cv::Mat gpudaCalistir(const std::function<void(GpuTampon&)>& islem, const cv::Mat& gri, int tekrar,
                      float& kernelMs, float& toplamMs) {
    cv::Mat giris = gri.isContinuous() ? gri : gri.clone();
    cv::Mat cikis(gri.size(), CV_8UC1);
    GpuTampon g(gri.cols, gri.rows);
    size_t boyut = gri.total();

    cudaEvent_t a, b, c, d;
    cudaEventCreate(&a); cudaEventCreate(&b); cudaEventCreate(&c); cudaEventCreate(&d);

    // Isınma
    CUDA_KONTROL(cudaMemcpy(g.giris, giris.data, boyut, cudaMemcpyHostToDevice));
    islem(g);
    CUDA_KONTROL(cudaGetLastError());
    CUDA_KONTROL(cudaDeviceSynchronize());

    kernelMs = toplamMs = 0;
    for (int i = 0; i < tekrar; ++i) {
        cudaEventRecord(a);
        CUDA_KONTROL(cudaMemcpy(g.giris, giris.data, boyut, cudaMemcpyHostToDevice));
        cudaEventRecord(b);
        islem(g);
        cudaEventRecord(c);
        CUDA_KONTROL(cudaMemcpy(cikis.data, g.cikis, boyut, cudaMemcpyDeviceToHost));
        cudaEventRecord(d);
        cudaEventSynchronize(d);
        float k, t;
        cudaEventElapsedTime(&k, b, c);
        cudaEventElapsedTime(&t, a, d);
        kernelMs += k;
        toplamMs += t;
    }
    CUDA_KONTROL(cudaGetLastError());
    kernelMs /= tekrar;
    toplamMs /= tekrar;
    cudaEventDestroy(a); cudaEventDestroy(b); cudaEventDestroy(c); cudaEventDestroy(d);
    return cikis;
}

double cpuOlcMs(const std::function<cv::Mat(const cv::Mat&, int)>& f, const cv::Mat& img, int thread, int tekrar) {
    f(img, thread);
    auto bas = std::chrono::steady_clock::now();
    for (int i = 0; i < tekrar; ++i) f(img, thread);
    return std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - bas).count() / tekrar;
}

int main(int argc, char** argv) {
    std::string dizin = argc >= 2 ? argv[1] : "../goruntuler";
    cv::Mat dusuk = cv::imread(dizin + "/dusuk_kontrast.png", cv::IMREAD_GRAYSCALE);
    cv::Mat gurultulu = cv::imread(dizin + "/gurultulu.png", cv::IMREAD_GRAYSCALE);
    cv::Mat tuzBiber = cv::imread(dizin + "/tuz_biber.png", cv::IMREAD_GRAYSCALE);
    if (dusuk.empty() || gurultulu.empty() || tuzBiber.empty()) {
        std::cerr << "Goruntuler okunamadi (" << dizin << "/dusuk_kontrast.png, gurultulu.png, tuz_biber.png)\n";
        return 1;
    }

    int cihazSayisi = 0;
    CUDA_KONTROL(cudaGetDeviceCount(&cihazSayisi));
    if (cihazSayisi == 0) {
        std::cerr << "CUDA destekli GPU bulunamadi.\n";
        return 1;
    }
    cudaDeviceProp ozellik;
    CUDA_KONTROL(cudaGetDeviceProperties(&ozellik, 0));
    const int cekirdek = std::max(1u, std::thread::hardware_concurrency());
    std::cout << "GPU: " << ozellik.name << " | CPU cekirdek: " << cekirdek
              << " | sure olcumu 2048x2048 goruntude\n\n";

    struct Gorev {
        std::string ad, dosya;
        cv::Mat giris;
        std::function<cv::Mat(const cv::Mat&, int)> cpu;
        std::function<void(GpuTampon&)> gpu;
    };
    std::vector<Gorev> gorevler = {
        {"1) Kontrast germe", "1_kontrast_gerilmis_gpu.png", dusuk, islem::kontrastGer, kontrastGerGpu},
        {"2) Histogram esitleme", "2_esitlenmis_gpu.png", dusuk, islem::histogramEsitle, histogramEsitleGpu},
        {"3) CLAHE", "3_clahe_gpu.png", dusuk,
         [](const cv::Mat& g, int t) { return islem::clahe(g, 2.0, 8, 8, t); }, claheGpu},
        {"4) Ortalama 3x3", "4_ortalama_gpu.png", gurultulu, islem::ortalama3x3, ortalamaGpu},
        {"5) Medyan 5x5", "5_medyan_gpu.png", tuzBiber, islem::medyan5x5, medyanGpu},
    };

    printf("%-22s | CPU 1 thr | CPU %2d thr | GPU kernel | GPU toplam | hizlanma* | CPU ile ayni\n",
           "Islem (ms)", cekirdek);
    for (auto& g : gorevler) {
        cv::Mat buyuk;
        cv::resize(g.giris, buyuk, cv::Size(2048, 2048));
        int tekrar = g.ad[0] == '5' ? 2 : 10;

        double cpu1 = cpuOlcMs(g.cpu, buyuk, 1, tekrar);
        double cpuN = cpuOlcMs(g.cpu, buyuk, cekirdek, tekrar);
        float kernelMs, toplamMs;
        cv::Mat gpuSonuc = gpudaCalistir(g.gpu, buyuk, 20, kernelMs, toplamMs);
        double fark = cv::norm(gpuSonuc, g.cpu(buyuk, cekirdek), cv::NORM_INF);

        // Orijinal boyuttaki sonucu kaydet
        float k2, t2;
        cv::imwrite(g.dosya, gpudaCalistir(g.gpu, g.giris, 1, k2, t2));

        printf("%-22s | %9.2f | %10.2f | %10.3f | %10.3f | %8.1fx | %s\n", g.ad.c_str(), cpu1, cpuN, kernelMs,
               toplamMs, cpu1 / kernelMs, fark == 0 ? "evet" : ("HAYIR (fark " + std::to_string(int(fark)) + ")").c_str());
    }
    std::cout << "\n* hizlanma = CPU 1 thread / GPU kernel suresi\n"
                 "GPU toplam = CPU->GPU kopyalama + kernel + GPU->CPU kopyalama\n"
                 "Cikti resimleri bu dizine kaydedildi (*_gpu.png)\n";
    return 0;
}
