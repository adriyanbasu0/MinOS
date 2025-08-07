// TODO: Refactor this out? Idk how necessary it is. I feel like this is just way simpler.
// Then again a preboot will just fix all of this anyway so I'm postponing :)
#include <limine.h>
#include "log.h"
static volatile struct limine_smp_request limine_smp_request = {
    .id = LIMINE_SMP_REQUEST,
    .revision = 0,
    .flags = 0,
};
extern void ap_init(struct limine_smp_info*);
#include "arch/x86_64/gdt.h"
#include "arch/x86_64/enable_arch_extra.h"
#include "apic.h"
#define AP_STACK_SIZE PAGE_SIZE
static Mutex tss_sync = { 0 };
void ap_main(struct limine_smp_info* info) {
    reload_idt();
    reload_gdt();
    kernel_reload_gdt_registers();
    mutex_lock(&tss_sync);
    reload_tss();
    mutex_unlock(&tss_sync);

    __asm__ volatile(
            "movq %0, %%cr3\n"
            :
            : "r" ((uintptr_t)kernel.pml4 & ~KERNEL_MEMORY_MASK)
        );
    // APIC divider of 16
    kinfo("Hello from logical processor %zu lapic_id %zu", info->lapic_id, get_lapic_id());
    enable_cpu_features();
    kernel.processors[info->lapic_id].initialised = true;
    lapic_timer_reload();
    irq_clear(kernel.task_switch_irq);
    enable_interrupts();
    asm volatile( "int $0x20" );
}

static void* ap_stack_pool = NULL;

void init_smp(void) {
    struct limine_smp_response *resp = limine_smp_request.response;
    if (!resp)
        return;

    size_t cpu_count = resp->cpu_count;
    kinfo("There are %zu CPUs", cpu_count);

    // Allocate one big stack pool for all APs (including BSP, but BSP doesn't use it)
    ap_stack_pool = kernel_malloc(AP_STACK_SIZE * cpu_count);
    assert(ap_stack_pool && "Not enough memory for AP stack pool");

    uint32_t bsp_lapic = resp->bsp_lapic_id;
    kernel.processors[bsp_lapic].initialised = true;

    for (size_t i = 0; i < cpu_count; ++i) {
        struct limine_smp_info *info = resp->cpus[i];
        uint32_t lapic_id = info->lapic_id;

        if (lapic_id == bsp_lapic)
            continue;

        if (lapic_id >= cpu_count) {
            kerror("CPU LAPIC ID %u out of range (cpu_count = %zu)", lapic_id, cpu_count);
            continue;
        }

        if (kernel.max_processor_id < lapic_id)
            kernel.max_processor_id = lapic_id;

        // Calculate AP's stack top inside the pool
        void* ap_stack = (void*)((uintptr_t)ap_stack_pool + lapic_id * AP_STACK_SIZE);

        // Pass the top of the stack to the AP (stack grows down)
        info->extra_argument = (uintptr_t)ap_stack + AP_STACK_SIZE;
        info->goto_address = (void*)&ap_init;
    }
}
