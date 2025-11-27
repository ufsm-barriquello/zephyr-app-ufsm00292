#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>

int main(void)
{
    printk("App do Nicolas rodando no Zephyr!\n");

    while (1) {
        printk("Loop executando...\n");
        k_sleep(K_SECONDS(1));
    }
}

