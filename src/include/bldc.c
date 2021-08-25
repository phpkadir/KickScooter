// #INDEX# ======================================================================================================
// Title .........: Управление BLDC двигателем
// Arch ..........: GD32F130C8C6
// Author ........: GreenBytes ( https://vk.com/greenbytes ), Florian flo199213 ( https://github.com/flo199213/Hoverboard-Firmware-Hack-Gen2/tree/master/HoverBoardGigaDevice )
// Version .......: 1.0.0.0
// ==============================================================================================================

// Подключение библиотек
// Подключение основной библиотеки работы с микроконтроллером
#include "gd32f1x0.h"
// Подключение библиотеки работы с двигателем
#include "bldc.h"
// Подключение фреймворка
#include "../framework/framework.h"
// Подключение констант и макросов
#include "../define.h"

// Установка частоты ШИМ сигнала
const int16_t pwm_res = 72000000 / 2 / PWM_FREQ; // = 2000
// Установка скважности
int16_t bldc_inputFilterPwm = 0;
// Вкл/Откл мотора
FlagStatus bldc_enable = RESET;
// Датчики холла
// Датчки А
uint8_t hall_a;
// Датчик Б
uint8_t hall_b;
// Датчки С
uint8_t hall_c;
// Сумма холлов
uint8_t hall;
// Позиция двигателя
uint8_t pos;
// Предыдущия позиция двигателя
uint8_t lastPos;
// Конечный уровень ШИМ сигнала
int16_t bldc_outputFilterPwm = 0;
// Фильтр
int32_t filter_reg;
// Количество тактов за оборот двигателя
uint32_t speedcount = 0;
// Просчет тактов за оборот
uint32_t speedtick = 0;

// Таблица положения холлов
const uint8_t hall_to_pos[8] =
		{
			0, // Позиция холла [-] - Нет функции (доступ с 1-6) 
			3, // Позиция холла [1] (SA=1, SB=0, SC=0) -> ШИМ-положение 3
			5, // Позиция холла [2] (SA=0, SB=1, SC=0) -> ШИМ-положение 5
			4, // Позиция холла [3] (SA=1, SB=1, SC=0) -> ШИМ-положение 4
			1, // Позиция холла [4] (SA=0, SB=0, SC=1) -> ШИМ-положение 1
			2, // Позиция холла [5] (SA=1, SB=0, SC=1) -> ШИМ-положение 2
			6, // Позиция холла [6] (SA=0, SB=1, SC=1) -> ШИМ-положение 6
			0, // Позиция холла [-] - Нет функции (доступ с 1-6) 
		};

// #BLDC# ------------------------------------------------------
// Description....: Управление фазами
// Argument.......: 
//      int pwm                			= Уровень скважности от -1000 до 1000
//      uint32_t pwmPos    					= Положение двигателя
//      int *y       								= Адрес фазы Y
//      int *b       								= Адрес фазы B
//      int *g       	 							= Адрес фазы G
// ------------------------------------------------------------
__INLINE void enginePOV(int pwm, uint32_t pwmPos, int *y, int *b, int *g) {
  switch(pwmPos) {
    case 1:
      *y = 0;
      *b = pwm;
      *g = -pwm;
      break;
    case 2:
      *y = -pwm;
      *b = pwm;
      *g = 0;
      break;
    case 3:
      *y = -pwm;
      *b = 0;
      *g = pwm;
      break;
    case 4:
      *y = 0;
      *b = -pwm;
      *g = pwm;
      break;
    case 5:
      *y = pwm;
      *b = -pwm;
      *g = 0;
      break;
    case 6:
      *y = pwm;
      *b = 0;
      *g = -pwm;
      break;
    default:
      *y = 0;
      *b = 0;
      *g = 0;
  }
}

// #BLDC# ------------------------------------------------------
// Description....: Управление режимом двигателя
// Argument.......: 
//      uint8_t setMode							= Флаг режима управления двигателем (см. ф. bldc.h)
//					BLDC_OFF								= Двигатель отключен
//					BLDC_MEANDER						= Тип сигнала Меандр
//					BLDC_TRIANGULAR					= Треугольный тип сигнала 
//					BLDC_SAW								= Пилообразный тип сигнала 
//					BLDC_SINUS							= Синусный тип сигнала*
//
//		*Синусный тип сигнала достигается за счет изменением уровня скважности за цикл прохождения фазы
// ------------------------------------------------------------
void engineMode(FlagStatus setMode) {
	bldc_enable = setMode;
}

// #BLDC# ------------------------------------------------------
// Description....: Задание скажности двигателя
// Argument.......: 
//      int16_t setPwm							= Уровень скважности от -1000 до 1000
// ------------------------------------------------------------
void engineWrite(int16_t setPwm) {
	bldc_inputFilterPwm = CLAMP(setPwm, -1000, 1000);
}

// #BLDC# ------------------------------------------------------
// Description....: Расчет и установка ШИМ сигнала на фазы
// Argument.......: Нет
// ------------------------------------------------------------
void enginePWM(void) {
	int y = 0, b = 0, g = 0;
	if (bldc_enable == RESET) {
		timer_automatic_output_disable(TIMER_BLDC);
		timer_channel_output_pulse_value_config(TIMER_BLDC, TIMER_BLDC_G, 0);
		timer_channel_output_pulse_value_config(TIMER_BLDC, TIMER_BLDC_B, 0);
		timer_channel_output_pulse_value_config(TIMER_BLDC, TIMER_BLDC_Y, 0);
		return;
  } else {
		timer_automatic_output_enable(TIMER_BLDC);
  }
	hall_a = gpio_input_bit_get(getPort(PIN_HALL_A), getPin(PIN_HALL_A));
  hall_b = gpio_input_bit_get(getPort(PIN_HALL_B), getPin(PIN_HALL_B));
	hall_c = gpio_input_bit_get(getPort(PIN_HALL_C), getPin(PIN_HALL_C));
  hall = hall_a * 1 + hall_b * 2 + hall_c * 4;
  pos = hall_to_pos[hall];
	filter_reg = filter_reg - (filter_reg >> FILTER_SHIFT) + bldc_inputFilterPwm;
	bldc_outputFilterPwm = filter_reg >> FILTER_SHIFT;
  enginePOV(bldc_outputFilterPwm, pos, &y, &b, &g);
	timer_channel_output_pulse_value_config(TIMER_BLDC, TIMER_BLDC_G, CLAMP(g + pwm_res / 2, 10, pwm_res-10));
	timer_channel_output_pulse_value_config(TIMER_BLDC, TIMER_BLDC_B, CLAMP(b + pwm_res / 2, 10, pwm_res-10));
	timer_channel_output_pulse_value_config(TIMER_BLDC, TIMER_BLDC_Y, CLAMP(y + pwm_res / 2, 10, pwm_res-10));
	if (lastPos != 1 && pos == 1) {
		speedcount = speedtick;
		speedtick = 0;
	}
	lastPos = pos;
	speedtick++;
}
