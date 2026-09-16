#ifndef ANALYSIS_H
#define ANALYSIS_H

#include <stddef.h>
#include <stdint.h>

struct SecurityMetrics {
    double sac;   // Strict Avalanche Criterion (%)
    double npcr;  // Number of Pixels/Bytes Change Rate (%)
    double uaci;  // Unified Average Changing Intensity (%)
};

enum AlgorithmType {
    ALGO_RUBIK4D = 0,
    ALGO_AES128  = 1,
    ALGO_SPECK128 = 2
};

SecurityMetrics AnalyzeCipherSecurity(AlgorithmType algo, int samples = 1000);

#endif // ANALYSIS_H