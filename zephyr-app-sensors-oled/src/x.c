#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/drivers/display.h>
#include <zephyr/display/cfb.h>

#define LED1_PIN 18
#define LED2_PIN 19  
#define LED3_PIN 28
#define BTN1_PIN 22

#define TEMP_SENSOR_ADDR 0x4F
#define TEMP_REG 0x00

struct sensor_data {
	float temperature;
	uint16_t light_raw;
	int light_percent;
	bool temp_valid;
	bool light_valid;
	int counter;
};

static int read_temperature(const struct device *i2c, float *temp)
{
	uint8_t reg = TEMP_REG;
	uint8_t data[2];
	
	if (i2c_write_read(i2c, TEMP_SENSOR_ADDR, &reg, 1, data, 2) != 0) {
		return -1;
	}
	
	int16_t raw = (data[0] << 8) | data[1];
	*temp = raw / 256.0f;
	return 0;
}

static int read_light(const struct device *adc, uint16_t *raw, int *percent)
{
	int16_t buffer;
	
	/* Tenta vários canais possíveis */
	int channels[] = {0, 1, 2, 6, 16};  /* Canais mais prováveis */
	
	for (int i = 0; i < 5; i++) {
		int ch = channels[i];
		
		struct adc_channel_cfg cfg = {
			.gain = ADC_GAIN_1_2,
			.reference = ADC_REF_VDD_1_2,
			.acquisition_time = ADC_ACQ_TIME_DEFAULT,
			.channel_id = ch,
		};
		
		struct adc_sequence seq = {
			.channels = BIT(ch),
			.buffer = &buffer,
			.buffer_size = sizeof(buffer),
			.resolution = 12,
		};
		
		if (adc_channel_setup(adc, &cfg) == 0) {
			if (adc_read(adc, &seq) == 0) {
				/* Se encontrar valor razoável (não saturado) */
				if (buffer < 4000 && buffer > 100) {
					*raw = buffer;
					*percent = (buffer * 100) / 4095;
					return ch;  /* Retorna o canal encontrado */
				}
			}
		}
	}
	
	/* Fallback: usa canal 1 */
	struct adc_channel_cfg cfg = {
		.gain = ADC_GAIN_1_2,
		.reference = ADC_REF_VDD_1_2,
		.acquisition_time = ADC_ACQ_TIME_DEFAULT,
		.channel_id = 1,
	};
	
	struct adc_sequence seq = {
		.channels = BIT(1),
		.buffer = &buffer,
		.buffer_size = sizeof(buffer),
		.resolution = 12,
	};
	
	adc_channel_setup(adc, &cfg);
	adc_read(adc, &seq);
	*raw = buffer;
	*percent = (buffer * 100) / 4095;
	return 1;
}

static void update_display(const struct device *display, struct sensor_data *data)
{
	char lines[3][32];
	
	cfb_framebuffer_clear(display, false);
	
	if (data->temp_valid) {
		int t_int = (int)data->temperature;
		int t_frac = (int)((data->temperature - t_int) * 10);
		snprintf(lines[0], 32, "T:%d.%dC", t_int, t_frac);
	} else {
		snprintf(lines[0], 32, "T:N/A");
	}
	
	if (data->light_valid) {
		snprintf(lines[1], 32, "L:%d%%", data->light_percent);
	} else {
		snprintf(lines[1], 32, "L:N/A");
	}
	
	snprintf(lines[2], 32, "#%d", data->counter);
	
	cfb_print(display, lines[0], 0, 0);
	cfb_print(display, lines[1], 0, 12);
	cfb_print(display, lines[2], 0, 24);
	cfb_framebuffer_finalize(display);
}

int main(void)
{
	const struct device *gpio_dev;
	const struct device *display;
	const struct device *i2c_dev;
	const struct device *adc_dev;
	struct sensor_data data = {0};
	int light_channel = -1;
	
	k_sleep(K_MSEC(500));
	
	printk("\n=============================================\n");
	printk(" SAMR21 + OLED1 + IO1\n");
	printk("=============================================\n\n");

	/* GPIO */
	gpio_dev = DEVICE_DT_GET(DT_NODELABEL(porta));
	if (device_is_ready(gpio_dev)) {
		gpio_pin_configure(gpio_dev, LED1_PIN, GPIO_OUTPUT_HIGH);
		gpio_pin_configure(gpio_dev, LED2_PIN, GPIO_OUTPUT_HIGH);
		gpio_pin_configure(gpio_dev, LED3_PIN, GPIO_OUTPUT_HIGH);
		gpio_pin_configure(gpio_dev, BTN1_PIN, GPIO_INPUT | GPIO_PULL_UP);
		printk("[OK] GPIO\n");
	}

	/* Display */
	display = DEVICE_DT_GET(DT_CHOSEN(zephyr_display));
	if (device_is_ready(display)) {
		cfb_framebuffer_init(display);
		cfb_framebuffer_clear(display, false);
		cfb_framebuffer_set_font(display, 0);
		cfb_print(display, "Iniciando...", 0, 12);
		cfb_framebuffer_finalize(display);
		printk("[OK] Display\n");
	}

	/* I2C - Temperatura */
	i2c_dev = DEVICE_DT_GET(DT_ALIAS(i2c_0));
	if (device_is_ready(i2c_dev)) {
		printk("[OK] I2C - AT30TSE758\n");
		data.temp_valid = true;
	} else {
		printk("[WARN] I2C indisponivel\n");
		data.temp_valid = false;
	}

	/* ADC - Luz */
	adc_dev = DEVICE_DT_GET(DT_NODELABEL(adc));
	if (device_is_ready(adc_dev)) {
		printk("[OK] ADC\n");
		data.light_valid = true;
	} else {
		printk("[WARN] ADC indisponivel\n");
		data.light_valid = false;
	}

	printk("\n=============================================\n\n");

	/* Loop principal */
	while (1) {
		printk("[%d] ", data.counter);
		
		/* Temperatura */
		if (data.temp_valid) {
			if (read_temperature(i2c_dev, &data.temperature) == 0) {
				int t_int = (int)data.temperature;
				int t_frac = (int)((data.temperature - t_int) * 100);
				printk("T:%d.%02dC ", t_int, t_frac);
				
				gpio_pin_set(gpio_dev, LED1_PIN, 
					data.temperature > 25.0f ? 0 : 1);
			} else {
				printk("T:Err ");
			}
		}
		
		/* Luz */
		if (data.light_valid) {
			light_channel = read_light(adc_dev, &data.light_raw, 
				&data.light_percent);
			printk("L:%d%% (CH%d) ", data.light_percent, light_channel);
			
			gpio_pin_set(gpio_dev, LED2_PIN, 
				data.light_percent < 30 ? 0 : 1);
		}
		
		printk("\n");
		
		/* Display */
		if (device_is_ready(display)) {
			update_display(display, &data);
		}
		
		/* LED3 pisca */
		gpio_pin_toggle(gpio_dev, LED3_PIN);
		
		/* Botão reset */
		if (gpio_pin_get(gpio_dev, BTN1_PIN) == 0) {
			data.counter = 0;
			printk("  [RESET]\n");
		}
		
		data.counter++;
		k_sleep(K_SECONDS(2));
	}

	return 0;
}