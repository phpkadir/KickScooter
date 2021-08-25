// #INDEX# ======================================================================================================
// Title .........: Константы устройства
// Arch ..........: GD32F130C8C6
// Author ........: GreenBytes ( https://vk.com/greenbytes )
// Version .......: 1.0.0.0
// ==============================================================================================================

#ifndef DEFINE_H
	#define DEFINE_H

	// Список использования Пин'ов микроконтроллера
	// Пин курка газа (АЦП)
	# define PIN_DAMPER														PA0
	// ???
	# define PIN_MOSFET_OUT												PC13
	// Пин чтения напряжение питания (АЦП)
	# define PIN_VBAT															PA4
	// Пин чтения датчика тока (АЦП)
	# define PIN_CURRENT_DC												PA6
	// Пин включения устройства
	# define PIN_SELF_HOLD												PB2
	// Пин кнопки включения устройства
	# define PIN_POWER														PC15
	//Пин пищалки
	# define PIN_BUZZER														PB10
	//
	# define PIN_CHARGE_STATE											PF0
	//
	# define PIN_BLDC_EMERGENCY_SHUTDOWN					PB12


	// Управление бесщеточным двигателем (BLDC)
	// Канал [G]
	# define PIN_BLDC_GH													PA10
	# define PIN_BLDC_GL													PB15
	// Канал [B]
	# define PIN_BLDC_BH													PA9
	# define PIN_BLDC_BL													PB14
	// Канал [Y]
	# define PIN_BLDC_YH													PA8
	# define PIN_BLDC_YL													PB13
	// Датчики холла
	# define PIN_HALL_A														PB11
	# define PIN_HALL_B														PF1
	# define PIN_HALL_C														PC14


	// Список используемых таймеров
	// Управление бесщеточным двигателем (BLDC)
	# define TIMER_BLDC 													TIMER0
	// Канал [G]
	# define TIMER_BLDC_G 												TIMER_CH_2
	// Канал [B]
	# define TIMER_BLDC_B 												TIMER_CH_1
	// Канал [Y]
	# define TIMER_BLDC_Y 												TIMER_CH_0

	// Константы
	// Преобразование значений АЦП
	# define CONST_MOTOR_AMP 											0.201465201465
	# define CONST_BATTERY_VOLT      							0.024169921875

	# define PWM_FREQ															16000
	# define TIMEOUT_FREQ													1000
	# define DEAD_TIME														60
	# define FILTER_SHIFT													12
	# define DELAY_IN_MAIN_LOOP										5

	// 
	# define ARM_MATH_CM3

	// Useful math function defines
	# define ABS(a) (((a) < 0.0) ? -(a) : (a))
	# define CLAMP(x, low, high) (((x) > (high)) ? (high) : (((x) < (low)) ? (low) : (x)))
	# define MAX(x, high) (((x) > (high)) ? (high) : (x))
	# define MAP(x, xMin, xMax, yMin, yMax) ((x - xMin) * (yMax - yMin) / (xMax - xMin) + yMin)
#endif
