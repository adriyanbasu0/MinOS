#ifndef SMT_H
#define SMT_H

#include <stdint.h>

typedef struct
{
    uint32_t smt_bits;            // Number of bits for SMT level (thread)
    uint32_t core_bits;           // Number of bits for Core level
    uint32_t logical_per_package; // Total logical processors per package
} CpuTopologyInfo;

typedef struct
{
    uint32_t apic_id;
    uint32_t smt_id;
    uint32_t core_id;
    uint32_t package_id;
} PhysicalCpuInfo;

// Detect topology once, from CPUID leaf 0xB
CpuTopologyInfo detect_cpu_topology(void);

// Extract SMT ID from APIC ID given smt_bits
uint32_t get_smt_id(uint32_t apic_id, uint32_t smt_bits);

// Extract Core ID from APIC ID given smt_bits and core_bits
uint32_t get_core_id(uint32_t apic_id, uint32_t smt_bits, uint32_t core_bits);

// Extract Package ID from APIC ID given core_bits
uint32_t get_package_id(uint32_t apic_id, uint32_t core_bits);

// Initialize SMT info for a CPU, call per CPU during startup
void init_smt(uint32_t apic_id);

// Global CPU topology info array, indexed by APIC ID
extern PhysicalCpuInfo cpu_topology[];

// Number of CPUs initialized
extern uint32_t cpu_count;

#endif // SMT_H
