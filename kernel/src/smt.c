#include "smt.h"
#include "log.h"  // For kinfo()
#include "apic.h" // For get_lapic_id()

#define MAX_CPUS 256

PhysicalCpuInfo cpu_topology[MAX_CPUS];
uint32_t cpu_count = 0;

CpuTopologyInfo detect_cpu_topology(void)
{
    uint32_t eax, ebx, ecx, edx;
    CpuTopologyInfo info = {0};

    ecx = 0;
    __asm__ volatile("cpuid"
                     : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx)
                     : "a"(0x0B), "c"(ecx));
    info.smt_bits = eax & 0x1F;
    // ebx = number of logical processors at this level

    ecx = 1;
    __asm__ volatile("cpuid"
                     : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx)
                     : "a"(0x0B), "c"(ecx));
    info.core_bits = eax & 0x1F;
    info.logical_per_package = ebx;

    return info;
}

uint32_t get_smt_id(uint32_t apic_id, uint32_t smt_bits)
{
    return apic_id & ((1U << smt_bits) - 1);
}

uint32_t get_core_id(uint32_t apic_id, uint32_t smt_bits, uint32_t core_bits)
{
    return (apic_id >> smt_bits) & ((1U << (core_bits - smt_bits)) - 1);
}

uint32_t get_package_id(uint32_t apic_id, uint32_t core_bits)
{
    return apic_id >> core_bits;
}

void init_smt(uint32_t apic_id)
{
    CpuTopologyInfo topo = detect_cpu_topology();

    uint32_t smt_id = get_smt_id(apic_id, topo.smt_bits);
    uint32_t core_id = get_core_id(apic_id, topo.smt_bits, topo.core_bits);
    uint32_t pkg_id = get_package_id(apic_id, topo.core_bits);

    cpu_topology[apic_id] = (PhysicalCpuInfo){
        .apic_id = apic_id,
        .smt_id = smt_id,
        .core_id = core_id,
        .package_id = pkg_id,
    };

    kinfo("[CPU %u] SMT ID: %u, Core ID: %u, Package ID: %u",
          apic_id, smt_id, core_id, pkg_id);

    if (apic_id + 1 > cpu_count)
        cpu_count = apic_id + 1;
}
