#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/display.h>
#include <zephyr/display/cfb.h>

/* LEDs do OLED1 - active LOW */
#define LED1_PIN 18  /* PA18 - Pin 7 EXT1 */
#define LED2_PIN 19  /* PA19 - Pin 8 EXT1 */
#define LED3_PIN 28  /* PA28 - Pin 6 EXT1 */

/* Botões do OLED1 - active LOW (pull-up interno) */
#define BTN1_PIN 22  /* PA22 - Pin 9 EXT1 */
#define BTN2_PIN 6   /* PA06 - Pin 3 EXT1 */
#define BTN3_PIN 7   /* PA07 - Pin 4 EXT1 */

int main(void)
{
	const struct device *gpio_dev;
	const struct device *display;
	int btn_state;
	
	k_sleep(K_MSEC(500));
	
	printk("\n\n");
	printk("=============================================\n");
	printk(" Teste Completo OLED1 Xplained Pro\n");
	printk("=============================================\n\n");

	/* Inicializa GPIO porta */
	gpio_dev = DEVICE_DT_GET(DT_NODELABEL(porta));
	
	if (!device_is_ready(gpio_dev)) {
		printk("ERRO: GPIO porta nao disponivel!\n");
		return -1;
	}
	
	printk("GPIO porta OK\n");
	
	/* Configura LEDs como saída (desligados inicialmente) */
	gpio_pin_configure(gpio_dev, LED1_PIN, GPIO_OUTPUT_HIGH);  /* HIGH = OFF (active LOW) */
	gpio_pin_configure(gpio_dev, LED2_PIN, GPIO_OUTPUT_HIGH);
	gpio_pin_configure(gpio_dev, LED3_PIN, GPIO_OUTPUT_HIGH);
	
	/* Configura Botões como entrada com pull-up */
	gpio_pin_configure(gpio_dev, BTN1_PIN, GPIO_INPUT | GPIO_PULL_UP);
	gpio_pin_configure(gpio_dev, BTN2_PIN, GPIO_INPUT | GPIO_PULL_UP);
	gpio_pin_configure(gpio_dev, BTN3_PIN, GPIO_INPUT | GPIO_PULL_UP);
	
	printk("LEDs e Botoes configurados\n\n");
	
	/* Teste sequencial dos LEDs */
	printk("=== Teste de LEDs ===\n");
	printk("LED1 ON...\n");
	gpio_pin_set(gpio_dev, LED1_PIN, 0); // Active LOW = ON
	k_sleep(K_MSEC(500));
	gpio_pin_set(gpio_dev, LED1_PIN, 1); // OFF
	
	printk("LED2 ON...\n");
	gpio_pin_set(gpio_dev, LED2_PIN, 0);
	k_sleep(K_MSEC(500));
	gpio_pin_set(gpio_dev, LED2_PIN, 1);
	
	printk("LED3 ON...\n");
	gpio_pin_set(gpio_dev, LED3_PIN, 0);
	k_sleep(K_MSEC(500));
	gpio_pin_set(gpio_dev, LED3_PIN, 1);
	
	printk("Todos LEDs ON...\n");
	gpio_pin_set(gpio_dev, LED1_PIN, 0);
	gpio_pin_set(gpio_dev, LED2_PIN, 0);
	gpio_pin_set(gpio_dev, LED3_PIN, 0);
	k_sleep(K_SECONDS(1));
	gpio_pin_set(gpio_dev, LED1_PIN, 1);
	gpio_pin_set(gpio_dev, LED2_PIN, 1);
	gpio_pin_set(gpio_dev, LED3_PIN, 1);
	
	printk("LEDs testados com sucesso!\n\n");
	
	printk("=== Teste de Display ===\n");
	
	display = DEVICE_DT_GET(DT_CHOSEN(zephyr_display));
	
	if (!device_is_ready(display)) {
		printk("ERRO: Display nao pronto!\n");
	} else {
		printk("Display OK: %s\n", display->name);
		
		if (cfb_framebuffer_init(display) == 0) {
			printk("CFB inicializado\n");
			
			cfb_framebuffer_clear(display, false);
			cfb_framebuffer_set_font(display, 0);
			
			cfb_print(display, "OLED1 TEST", 0, 0);
			cfb_print(display, "Press BTN", 0, 16);
			
			cfb_framebuffer_finalize(display);
			printk("Texto enviado\n");
		} else {
			printk("ERRO: CFB init falhou\n");
		}
	}
	
	printk("\n=== Modo Interativo ===\n");
	printk("Pressione os botoes do OLED1:\n");
	printk("  BTN1 (pin 9) -> LED1\n");
	printk("  BTN2 (pin 3) -> LED2\n");
	printk("  BTN3 (pin 4) -> LED3\n\n");
	
	int counter = 0;
	bool display_ok = device_is_ready(display);
	
	while (1) {
		/* Lê botões e controla LEDs */
		btn_state = gpio_pin_get(gpio_dev, BTN1_PIN);
		if (btn_state == 0) {
			printk("BTN1!\n");
			gpio_pin_set(gpio_dev, LED1_PIN, 0);
		} else {
			gpio_pin_set(gpio_dev, LED1_PIN, 1);
		}
		
		btn_state = gpio_pin_get(gpio_dev, BTN2_PIN);
		if (btn_state == 0) {
			printk("BTN2!\n");
			gpio_pin_set(gpio_dev, LED2_PIN, 0);
		} else {
			gpio_pin_set(gpio_dev, LED2_PIN, 1);
		}
		
		btn_state = gpio_pin_get(gpio_dev, BTN3_PIN);
		if (btn_state == 0) {
			printk("BTN3!\n");
			gpio_pin_set(gpio_dev, LED3_PIN, 0);
		} else {
			gpio_pin_set(gpio_dev, LED3_PIN, 1);
		}
		
		/* Atualiza display a cada 5 segundos */
		if (display_ok && (counter % 50 == 0)) {
			char buf[32];
			cfb_framebuffer_clear(display, false);
			cfb_print(display, "OLED1 TEST", 0, 0);
			snprintf(buf, sizeof(buf), "Cnt:%d", counter / 50);
			cfb_print(display, buf, 0, 16);
			cfb_framebuffer_finalize(display);
			printk("[%d] Display atualizado\n", counter / 50);
		}
		
		counter++;
		k_sleep(K_MSEC(100));
	}

	return 0;
}