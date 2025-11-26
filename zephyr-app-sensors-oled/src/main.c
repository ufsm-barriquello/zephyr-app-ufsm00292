#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/drivers/display.h>
#include <zephyr/display/cfb.h>
#include <stdio.h>

/* LEDs do OLED1 - active LOW */
#define LED1_PIN 18  /* PA18 */
#define LED2_PIN 19  /* PA19 */
#define LED3_PIN 28  /* PA28 */

/* Botões do OLED1 */
#define BTN1_PIN 22  /* PA22 */
#define BTN2_PIN 6   /* PA06 */
#define BTN3_PIN 7   /* PA07 */

/* Sensor de temperatura I2C */
#define TEMP_SENSOR_ADDR 0x4F
#define TEMP_REG 0x00

/* ADC para sensor de luz */
#define ADC_CHANNEL 6
#define ADC_RESOLUTION 12
#define ADC_GAIN ADC_GAIN_1
#define ADC_REFERENCE ADC_REF_INTERNAL
#define ADC_ACQUISITION_TIME ADC_ACQ_TIME_DEFAULT

/* Estrutura de dados dos sensores */
struct sensor_data {
	float temperature;
	uint16_t light_raw;
	float light_percent;
	bool temp_valid;
	bool light_valid;
};

/* Protótipos */
static int read_temperature(const struct device *i2c, float *temp);
static int read_light_sensor(const struct device *adc, uint16_t *raw, float *percent);
static void update_display(const struct device *display, struct sensor_data *data);
static void control_leds(const struct device *gpio, struct sensor_data *data);

/* Configuração do ADC */
static const struct adc_channel_cfg adc_cfg = {
	.gain = ADC_GAIN,
	.reference = ADC_REFERENCE,
	.acquisition_time = ADC_ACQUISITION_TIME,
	.channel_id = ADC_CHANNEL,
};

int main(void)
{
	const struct device *gpio_dev;
	const struct device *display;
	const struct device *i2c_dev;
	const struct device *adc_dev;
	struct sensor_data data = {0};
	int16_t adc_buffer;
	struct adc_sequence adc_seq = {
		.channels = BIT(ADC_CHANNEL),
		.buffer = &adc_buffer,
		.buffer_size = sizeof(adc_buffer),
		.resolution = ADC_RESOLUTION,
	};
	
	k_sleep(K_MSEC(500));
	
	printk("\n\n");
	printk("=============================================\n");
	printk(" SAMR21 + OLED1 + IO1 Xplained Pro\n");
	printk(" Monitoramento de Temperatura e Luz\n");
	printk("=============================================\n\n");

	/* ========== GPIO (OLED1 LEDs e Botões) ========== */
	gpio_dev = DEVICE_DT_GET(DT_NODELABEL(porta));
	if (!device_is_ready(gpio_dev)) {
		printk("[ERRO] GPIO nao disponivel\n");
		return -1;
	}
	
	gpio_pin_configure(gpio_dev, LED1_PIN, GPIO_OUTPUT_HIGH);
	gpio_pin_configure(gpio_dev, LED2_PIN, GPIO_OUTPUT_HIGH);
	gpio_pin_configure(gpio_dev, LED3_PIN, GPIO_OUTPUT_HIGH);
	gpio_pin_configure(gpio_dev, BTN1_PIN, GPIO_INPUT | GPIO_PULL_UP);
	gpio_pin_configure(gpio_dev, BTN2_PIN, GPIO_INPUT | GPIO_PULL_UP);
	gpio_pin_configure(gpio_dev, BTN3_PIN, GPIO_INPUT | GPIO_PULL_UP);
	
	printk("[OK] GPIO configurado\n");

	/* ========== Display (OLED1) ========== */
	display = DEVICE_DT_GET(DT_CHOSEN(zephyr_display));
	if (device_is_ready(display)) {
		printk("[OK] Display SSD1306\n");
		if (cfb_framebuffer_init(display) == 0) {
			cfb_framebuffer_clear(display, false);
			cfb_framebuffer_set_font(display, 0);
			cfb_print(display, "Inicializando", 0, 0);
			cfb_print(display, "Sensores...", 0, 16);
			cfb_framebuffer_finalize(display);
		}
	} else {
		printk("[WARN] Display nao disponivel\n");
	}

	/* ========== I2C - Sensor de Temperatura ========== */
	i2c_dev = DEVICE_DT_GET(DT_ALIAS(i2c_0));
	if (device_is_ready(i2c_dev)) {
		printk("[OK] I2C para AT30TSE758\n");
		data.temp_valid = true;
	} else {
		printk("[WARN] I2C nao disponivel\n");
		data.temp_valid = false;
	}

	/* ========== ADC - Sensor de Luz ========== */
	adc_dev = DEVICE_DT_GET(DT_NODELABEL(adc));
	if (device_is_ready(adc_dev)) {
		if (adc_channel_setup(adc_dev, &adc_cfg) == 0) {
			printk("[OK] ADC configurado (Canal %d)\n", ADC_CHANNEL);
			data.light_valid = true;
		} else {
			printk("[WARN] Falha ao configurar ADC\n");
			data.light_valid = false;
		}
	} else {
		printk("[WARN] ADC nao disponivel\n");
		data.light_valid = false;
	}

	printk("\n=============================================\n");
	printk(" Sistema Iniciado!\n");
	printk("=============================================\n\n");

	/* Teste LEDs */
	for (int i = 0; i < 3; i++) {
		gpio_pin_set(gpio_dev, LED1_PIN, 0);
		gpio_pin_set(gpio_dev, LED2_PIN, 0);
		gpio_pin_set(gpio_dev, LED3_PIN, 0);
		k_sleep(K_MSEC(100));
		gpio_pin_set(gpio_dev, LED1_PIN, 1);
		gpio_pin_set(gpio_dev, LED2_PIN, 1);
		gpio_pin_set(gpio_dev, LED3_PIN, 1);
		k_sleep(K_MSEC(100));
	}

	/* ========== Loop Principal ========== */
	int counter = 0;
	
	while (1) {
		printk("\n[%d] ========================================\n", counter);
		
		/* Lê temperatura via I2C */
		if (data.temp_valid) {
			if (read_temperature(i2c_dev, &data.temperature) == 0) {
				int temp_int = (int)data.temperature;
				int temp_frac = (int)((data.temperature - temp_int) * 100);
				printk("  Temperatura: %d.%02d C\n", temp_int, temp_frac);
			} else {
				printk("  Temperatura: Erro leitura\n");
			}
		}
		
		/* Lê luz via ADC */
		if (data.light_valid) {
			if (adc_read(adc_dev, &adc_seq) == 0) {
				data.light_raw = adc_buffer;
				data.light_percent = (data.light_raw * 100.0f) / 4095.0f;
				int light_pct = (int)data.light_percent;
				printk("  Luz: %d/4095 (%d%%)\n", data.light_raw, light_pct);
			} else {
				printk("  Luz: Erro leitura ADC\n");
			}
		}
		
		/* Atualiza display */
		if (device_is_ready(display)) {
			update_display(display, &data);
		}
		
		/* Controla LEDs baseado nos sensores */
		control_leds(gpio_dev, &data);
		
		/* Verifica botão para reset */
		if (gpio_pin_get(gpio_dev, BTN1_PIN) == 0) {
			printk("  [BTN1] Reset!\n");
			if (device_is_ready(display)) {
				cfb_framebuffer_clear(display, false);
				cfb_print(display, "RESET", 40, 12);
				cfb_framebuffer_finalize(display);
				k_sleep(K_MSEC(500));
			}
		}
		
		counter++;
		k_sleep(K_SECONDS(2));
	}

	return 0;
}

/* ========== Funções Auxiliares ========== */

static int read_temperature(const struct device *i2c, float *temp)
{
	uint8_t reg = TEMP_REG;
	uint8_t data[2];
	
	if (i2c_write_read(i2c, TEMP_SENSOR_ADDR, &reg, 1, data, 2) != 0) {
		return -1;
	}
	
	int16_t raw = (data[0] << 8) | data[1];
	*temp = raw / 256.0f;  /* Resolução de 1/256 °C */
	
	return 0;
}

static void update_display(const struct device *display, struct sensor_data *data)
{
	char line1[32];
	char line2[32];
	
	cfb_framebuffer_clear(display, false);
	
	if (data->temp_valid) {
		int temp_int = (int)data->temperature;
		int temp_frac = (int)((data->temperature - temp_int) * 10);
		snprintf(line1, sizeof(line1), "T:%d.%dC", temp_int, temp_frac);
	} else {
		snprintf(line1, sizeof(line1), "T:N/A");
	}
	
	if (data->light_valid) {
		int light_pct = (int)data->light_percent;
		snprintf(line2, sizeof(line2), "L:%d%%", light_pct);
	} else {
		snprintf(line2, sizeof(line2), "L:N/A");
	}
	
	cfb_print(display, line1, 0, 0);
	cfb_print(display, line2, 0, 16);
	cfb_framebuffer_finalize(display);
}

static void control_leds(const struct device *gpio, struct sensor_data *data)
{
	/* LED1: Temperatura alta (>25°C) */
	if (data->temp_valid && data->temperature > 25.0f) {
		gpio_pin_set(gpio, LED1_PIN, 0);
	} else {
		gpio_pin_set(gpio, LED1_PIN, 1);
	}
	
	/* LED2: Pouca luz (<30%) */
	if (data->light_valid && data->light_percent < 30.0f) {
		gpio_pin_set(gpio, LED2_PIN, 0);
	} else {
		gpio_pin_set(gpio, LED2_PIN, 1);
	}
	
	/* LED3: Sistema ativo (pisca) */
	static bool led3_state = false;
	led3_state = !led3_state;
	gpio_pin_set(gpio, LED3_PIN, led3_state ? 0 : 1);
}