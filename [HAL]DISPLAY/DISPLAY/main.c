#include <atmel_start.h>
#include <hal_gpio.h>
#include <string.h>
#include "gfx_mono_main.h"
#include "gfx_mono_font_basic_6x7.h"


#define TEXT_X_POS   20
#define TEXT_Y_POS   12
#define TEXT_WIDTH   (UG2832HSWEG04_LCD_WIDTH - TEXT_X_POS) // Limpa até a borda direita
#define TEXT_HEIGHT  7


void clear_text_area(void) {
	gfx_mono_draw_filled_rect(&MONOCHROME_GRAPHICS_desc,
						 TEXT_X_POS,
						 TEXT_Y_POS,
						 TEXT_WIDTH,
						 TEXT_HEIGHT,
						 GFX_PIXEL_CLR // Cor para limpar (desliga todos os pixels da área)
						 );
}
int main(void)
{
	/* Initializes MCU, drivers and middleware */
	atmel_start_init();

	// Display Message
	uint8_t msg[32];
	strcpy((char *)msg, "Inicializando");
	
	// Enable SPI and OLED
	spi_m_sync_enable(&DISPLAY_SPI);
	gfx_mono_init();
	
	// Draw a Rectangle
	gfx_mono_draw_rect(&MONOCHROME_GRAPHICS_desc,
						 0, 
						 0,
						 UG2832HSWEG04_LCD_WIDTH,
						 UG2832HSWEG04_LCD_HEIGHT,
						 GFX_PIXEL_SET
						 );
	
	
	// Draw a Message
	gfx_mono_text_draw_string(&MONOCHROME_TEXT_desc, msg, TEXT_X_POS, TEXT_Y_POS, &basic_6x7);
	
	/* Replace with your application code */
	while (1) {
		// 1. LIGAR LEDs (setar LOW/false)
		gpio_set_pin_level(PA12, false);
		gpio_set_pin_level(PA13, true);
		gpio_set_pin_level(PA15, false);

		// 2. Atualizar Display para "Ligado"
		clear_text_area(); // **LIMPA O TEXTO ANTERIOR**
		strcpy((char *)msg, "LEDs Ligados");
		gfx_mono_text_draw_string(&MONOCHROME_TEXT_desc, msg, TEXT_X_POS, TEXT_Y_POS, &basic_6x7);
		delay_ms(500);

		// 3. DESLIGAR LEDs (setar HIGH/true)
		gpio_set_pin_level(PA12, true);
		gpio_set_pin_level(PA13, true);
		gpio_set_pin_level(PA15, true);

		// 4. Atualizar Display para "Desligado"
		clear_text_area(); // **LIMPA O TEXTO ANTERIOR**
		strcpy((char *)msg, "LEDs Desligados");
		gfx_mono_text_draw_string(&MONOCHROME_TEXT_desc, msg, TEXT_X_POS, TEXT_Y_POS, &basic_6x7);
		delay_ms(500);
	}
}
