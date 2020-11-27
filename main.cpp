#include <chrono>
#include <utility>
#include <iostream>
#include <cstring>
#include <unistd.h>
#include <malloc.h>
#include <sys/mman.h>
#include <limits>

static std::string cSeparator = "|";
static long PAGE = 2048; // cat /proc/meminfo | grep page
static long PAGE_SIZE = sysconf (_SC_PAGESIZE); // PAGE * 1024;
static long COUNT_CHAR_PAGE = (sysconf (_SC_PAGESIZE) / sizeof(char)); // ((PAGE * 1024) / sizeof(char));

template<typename ToDuration, typename F, typename ...Args>
static typename ToDuration::rep measureTime(F &&func, Args &&...args) {
    using namespace std::chrono;
    auto start = high_resolution_clock::now();
    std::forward<decltype(func)>(func)(std::forward<Args>(args)...);
    return duration_cast<ToDuration>(high_resolution_clock::now() - start).count();
}

template<typename ToDuration, typename F, typename... Args>
static void reportTime(const std::string &unit,
        const std::string &funName, F &&func, Args &&...args) {
    auto duration = measureTime<ToDuration>(func, args...);
    std::cout << funName << cSeparator << duration << cSeparator << unit << "\n";
}

template<typename F, typename... Args>
static void reportTimeMs(const std::string &funName, F &&func, Args &&...args) {
    reportTime<std::chrono::milliseconds>("ms", funName, func, args...);
}

template<typename F, typename... Args>
static void reportTimeUs(const std::string &funName, F &&func, Args &&...args) {
    reportTime<std::chrono::microseconds>("us", funName, func, args...);
}

template<typename T>
static void set_loop(T *p, size_t count) {
    for (size_t i = 0; i < count; ++i) {
        p[i] = 0;
    }
}

template<unsigned char VAL = std::numeric_limits<unsigned char>::max()>
static void memset_page(char *p, size_t bytes) {
    const size_t count = bytes / sizeof(char);
    for (size_t  offset = 0; offset < count; offset += COUNT_CHAR_PAGE) {
        memset(p + offset, VAL, PAGE_SIZE);
    }
}

int main(int argc, char *argv[]) {
    const int type = std::stoi(argv[1]);
    const auto bytes_str = argv[2];
    const auto use_madvise = (4 == argc) && (0 == (strcmp(argv[3], "madvise")));
    const std::string use_madvise_str = cSeparator + "madvise " + (use_madvise ? "ON" : "OFF");
    const size_t bytes = std::stoll(bytes_str);
    const size_t count_float = bytes / sizeof(float);
    const size_t count_char_page = bytes / sizeof(char) / PAGE_SIZE;
    const size_t count_double = bytes / sizeof(double);
    void *p = nullptr;
    char **pc = (char**)&p;
    float **pf = (float**)&p;
    double **pd = (double**)&p;
    switch(type) {
    // set memory - baselines
    case 0:
        p = malloc(bytes);
        memset(p, std::numeric_limits<unsigned char>::max(), bytes); // write there, so we know it's allocated
        reportTimeUs("memset" + use_madvise_str + cSeparator + bytes_str,
            [&]{memset(p, 0, bytes);});
        break;
    case 1:
        p = malloc(bytes);
        memset(p, std::numeric_limits<unsigned char>::max(), bytes); // write there, so we know it's allocated
        reportTimeUs("memset (page)" + use_madvise_str + cSeparator + bytes_str, 
            [&] { memset_page<0>(*pc, bytes);});
        break;
    case 2:
        p = malloc(bytes);
        memset(p, std::numeric_limits<unsigned char>::max(), bytes); // write there, so we know it's allocated
        reportTimeUs("for loop(float)" + use_madvise_str + cSeparator + bytes_str, 
            [&]{ set_loop(*pf, count_float);});
        break;
    case 3:
        p = malloc(bytes);
        memset(p, std::numeric_limits<unsigned char>::max(), bytes); // write there, so we know it's allocated
        reportTimeUs("for loop(double)" + use_madvise_str + cSeparator + bytes_str,
            [&]{set_loop(*pd, count_double);});
        break;
    // allocation without init
    case 4:
        reportTimeUs("new<float>[]" + use_madvise_str + cSeparator + bytes_str, [&]{
            p = new float[count_float];
            if (use_madvise) madvise(p, bytes, MADV_HUGEPAGE);
        });
        break;
    case 5:
        reportTimeUs("new<double>[]" + use_madvise_str + cSeparator + bytes_str, [&]{
                p = new double[count_double];
                if (use_madvise) madvise(p, bytes, MADV_HUGEPAGE);
            });
        break;
    case 6:
        reportTimeUs("page_aligned" + use_madvise_str + cSeparator + bytes_str, [&]{
            p = aligned_alloc(PAGE_SIZE, bytes);
            if (use_madvise) madvise(p, bytes, MADV_HUGEPAGE);
        });
        break;
    case 7:
        reportTimeUs("malloc" + use_madvise_str + cSeparator + bytes_str, [&]{
            p = malloc(bytes);
            if (use_madvise) madvise(p, bytes, MADV_HUGEPAGE);
        });
        break;
    case 8:
        reportTimeUs("operator new" + use_madvise_str + cSeparator + bytes_str, [&]{
            p = operator new(bytes);
            if (use_madvise) madvise(p, bytes, MADV_HUGEPAGE);
        });
        break;
    // allocation with init
    // new float
    case 9:
        reportTimeUs("new<float>[]()" + use_madvise_str + cSeparator + bytes_str, [&]{
            p = new float[count_float]();
            if (use_madvise) madvise(p, bytes, MADV_HUGEPAGE);
        });
        break;
    case 10:
        reportTimeUs("new<float>[]+memset" + use_madvise_str + cSeparator + bytes_str, [&]{
            p = new float[count_float]();
            if (use_madvise) madvise(p, bytes, MADV_HUGEPAGE);
            memset(p, std::numeric_limits<unsigned char>::max(), bytes);
        });
        break;
    case 11:
        reportTimeUs("new<float>[]+memset_page" + use_madvise_str + cSeparator + bytes_str, [&]{
            p = new float[count_float]();
            if (use_madvise) madvise(p, bytes, MADV_HUGEPAGE);
            memset_page<std::numeric_limits<unsigned char>::max()>(*pc, bytes);
        });
        break;
    case 12:
        reportTimeUs("new<float>[]+loop_set" + use_madvise_str + cSeparator + bytes_str, [&]{
            p = new float[count_float]();
            if (use_madvise) madvise(p, bytes, MADV_HUGEPAGE);
            set_loop(*pf, count_float);
        });
        break;
    // new double
    case 13:
        reportTimeUs("new<double>[]()" + use_madvise_str + cSeparator + bytes_str, [&]{
            p = new double[count_double]();
            if (use_madvise) madvise(p, bytes, MADV_HUGEPAGE);
        });
        break;
    case 14:
        reportTimeUs("new<double>[]+memset" + use_madvise_str + cSeparator + bytes_str, [&]{
            p = new double[count_double]();
            if (use_madvise) madvise(p, bytes, MADV_HUGEPAGE);
            memset(p, std::numeric_limits<unsigned char>::max(), bytes);
        });
        break;
    case 15:
        reportTimeUs("new<double>[]+memset_page" + use_madvise_str + cSeparator + bytes_str, [&]{
            p = new double[count_double]();
            if (use_madvise) madvise(p, bytes, MADV_HUGEPAGE);
            memset_page(*pc, bytes);
        });
        break;
    case 16:
        reportTimeUs("new<double>[]+loop_set" + use_madvise_str + cSeparator + bytes_str, [&]{
            p = new double[count_double]();
            if (use_madvise) madvise(p, bytes, MADV_HUGEPAGE);
            set_loop(*pd, count_double);
        });
        break;
    // aligned_alloc
    case 17:
        reportTimeUs("page_aligned+memset" + use_madvise_str + cSeparator + bytes_str, [&]{
            p = aligned_alloc(PAGE_SIZE, bytes);
            if (use_madvise) madvise(p, bytes, MADV_HUGEPAGE);
            memset(p, std::numeric_limits<unsigned char>::max(), bytes);
        });
        break;
    case 18:
        reportTimeUs("page_aligned+memset_page" + use_madvise_str + cSeparator + bytes_str, [&]{
            p = aligned_alloc(PAGE_SIZE, bytes);
            if (use_madvise) madvise(p, bytes, MADV_HUGEPAGE);
            memset_page(*pc, bytes);
        });
        break;
    case 19:
        reportTimeUs("page_aligned+loop_set(float)" + use_madvise_str + cSeparator + bytes_str, [&]{
            p = aligned_alloc(PAGE_SIZE, bytes);
            if (use_madvise) madvise(p, bytes, MADV_HUGEPAGE);
            set_loop(*pf, count_float);
        });
        break;
    case 20:
        reportTimeUs("page_aligned+loop_set(double)" + use_madvise_str + cSeparator + bytes_str, [&]{
            p = aligned_alloc(PAGE_SIZE, bytes);
            if (use_madvise) madvise(p, bytes, MADV_HUGEPAGE);
            set_loop(*pd, count_double);
        });
        break;
    // malloc
    case 21:
        reportTimeUs("malloc+memset" + use_madvise_str + cSeparator + bytes_str, [&]{
            p = malloc(bytes);
            if (use_madvise) madvise(p, bytes, MADV_HUGEPAGE);
            memset(p, std::numeric_limits<unsigned char>::max(), bytes);
        });
        break;
    case 22:
        reportTimeUs("malloc+memset_page" + use_madvise_str + cSeparator + bytes_str, [&]{
            p = malloc(bytes);
            if (use_madvise) madvise(p, bytes, MADV_HUGEPAGE);
            memset_page(*pc, bytes);
        });
        break;
    case 23:
        reportTimeUs("malloc+loop_set(float)" + use_madvise_str + cSeparator + bytes_str, [&]{
            p = malloc(bytes);
            if (use_madvise) madvise(p, bytes, MADV_HUGEPAGE);
            set_loop(*pf, count_float);
        });
        break;
    case 24:
        reportTimeUs("malloc+loop_set(double)" + use_madvise_str + cSeparator + bytes_str, [&]{
            p = malloc(bytes);
            if (use_madvise) madvise(p, bytes, MADV_HUGEPAGE);
            set_loop(*pd, count_double);
        });
        break;
    // malloc mallopt
    case 25:
        reportTimeUs("malloc+mallopt+memset" + use_madvise_str + cSeparator + bytes_str, [&]{
            mallopt (M_MMAP_MAX , 0);
            p = malloc(bytes);
            if (use_madvise) madvise(p, bytes, MADV_HUGEPAGE);
            memset(p, std::numeric_limits<unsigned char>::max(), bytes);
        });
        break;
    case 26:
        reportTimeUs("malloc+mallopt+memset_page" + use_madvise_str + cSeparator + bytes_str, [&]{
            mallopt (M_MMAP_MAX , 0);
            p = malloc(bytes);
            if (use_madvise) madvise(p, bytes, MADV_HUGEPAGE);
            memset_page(*pc, bytes);
        });
        break;
    case 27:
        reportTimeUs("malloc+mallopt+loop_set(float)" + use_madvise_str + cSeparator + bytes_str, [&]{
            mallopt (M_MMAP_MAX , 0);
            p = malloc(bytes);
            if (use_madvise) madvise(p, bytes, MADV_HUGEPAGE);
            set_loop(*pf, count_float);
        });
        break;
    case 28:
        reportTimeUs("malloc+mallopt+loop_set(double)" + use_madvise_str + cSeparator + bytes_str, [&]{
            mallopt (M_MMAP_MAX , 0);
            p = malloc(bytes);
            if (use_madvise) madvise(p, bytes, MADV_HUGEPAGE);
            set_loop(*pd, count_double);
        });
        break;
    // operator new
    case 29:
        reportTimeUs("operator new+memset" + use_madvise_str + cSeparator + bytes_str, [&]{
            p = operator new(bytes);
            if (use_madvise) madvise(p, bytes, MADV_HUGEPAGE);
            memset(p, std::numeric_limits<unsigned char>::max(), bytes);
        });
        break;
    case 30:
        reportTimeUs("operator new+memset_page" + use_madvise_str + cSeparator + bytes_str, [&]{
            p = operator new(bytes);
            if (use_madvise) madvise(p, bytes, MADV_HUGEPAGE);
            memset_page(*pc, bytes);
        });
        break;
    case 31:
        reportTimeUs("operator new+loop_set(float)" + use_madvise_str + cSeparator + bytes_str, [&]{
            p = operator new(bytes);
            if (use_madvise) madvise(p, bytes, MADV_HUGEPAGE);
            set_loop(*pf, count_float);
        });
        break;
    case 32:
        reportTimeUs("operator new+loop_set(double)" + use_madvise_str + cSeparator + bytes_str, [&]{
            p = operator new(bytes);
            if (use_madvise) madvise(p, bytes, MADV_HUGEPAGE);
            set_loop(*pd, count_double);
        });
        break;
    // calloc float
    case 33:
        reportTimeUs("calloc<float>" + use_madvise_str + cSeparator + bytes_str, [&]{
            p = calloc(count_float, sizeof(float));
            if (use_madvise) madvise(p, bytes, MADV_HUGEPAGE);
        });
        break;
    case 34:
        reportTimeUs("calloc<double>" + use_madvise_str + cSeparator + bytes_str, [&]{
            p = calloc(count_double, sizeof(double));
            if (use_madvise) madvise(p, bytes, MADV_HUGEPAGE);
        });
        break;
    // mmap
    case 35:
        reportTimeUs("mmap+memset" + use_madvise_str + cSeparator + bytes_str, [&]{
            p = mmap(0, bytes, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);
            if (use_madvise) madvise(p, bytes, MADV_HUGEPAGE);
            memset(p, std::numeric_limits<unsigned char>::max(), bytes);
        });
        break;
    case 36:
        reportTimeUs("mmap+memset_page" + use_madvise_str + cSeparator + bytes_str, [&]{
            p = mmap(0, bytes, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);
            if (use_madvise) madvise(p, bytes, MADV_HUGEPAGE);
            memset_page(*pc, bytes);
        });
        break;
    case 37:
        reportTimeUs("mmap+loop_set(float)" + use_madvise_str + cSeparator + bytes_str, [&]{
            p = mmap(0, bytes, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);
            if (use_madvise) madvise(p, bytes, MADV_HUGEPAGE);
            set_loop(*pf, count_float);
        });
        break;
    case 38:
        reportTimeUs("mmap+loop_set(double)" + use_madvise_str + cSeparator + bytes_str, [&]{
            p = mmap(0, bytes, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);
            if (use_madvise) madvise(p, bytes, MADV_HUGEPAGE);
            set_loop(*pd, count_double);
        });
        break;
    case 39:
        reportTimeUs("mmap" + use_madvise_str + cSeparator + bytes_str, [&]{
            p = mmap(0, bytes, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);
            if (use_madvise) madvise(p, bytes, MADV_HUGEPAGE);
        });
        break;
    default: std::cout << "Don't know what to do with " << type << '\n';
    }   
    std::cout << "p: " << p << '\n';
}
