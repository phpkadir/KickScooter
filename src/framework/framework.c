// #INDEX# ======================================================================================================
// Title .........: framework для упрощеной работы с командами.
// Arch ..........: GD32F130C8C6
// Author ........: GreenBytes ( https://vk.com/greenbytes )
// Version .......: 1.0.0.0
// ==============================================================================================================

// Подключение библиотек
// Подключение основной библиотеки работы с микроконтроллером
#include "gd32f1x0.h"
// Подключение фреймворка
#include "framework.h"
// Подключение констант и макросов
#include "../define.h"

// Структуры для инициализации таймеров
dma_parameter_struct dma_init_struct_adc;
timer_parameter_struct timerBldc_paramter_struct;	
timer_break_parameter_struct timerBldc_break_parameter_struct;
timer_oc_parameter_struct timerBldc_oc_parameter_struct;

// Подсчет времени в миллисекундах
uint32_t msTicks = 0;
// Заглушка для инициализации АЦП
uint16_t fw_adc;

// #FRAMEWORK# ------------------------------------------------
// Description....: Номер порта по виртулаьному пину
// Argument.......: 
//      uint8_t PIN									= Виртуальный номер пина МК
// ------------------------------------------------------------
uint32_t getPort(uint8_t PIN) {
	if (PIN >= PA0 && PIN <= PA15) return GPIOA;
	if (PIN >= PB0 && PIN <= PB15) return GPIOB;
	if (PIN >= PC0 && PIN <= PC15) return GPIOC;
	if (PIN >= PD0 && PIN <= PD15) return GPIOD;
	if (PIN >= PF0 && PIN <= PF15) return GPIOF;
	return 0;
}

// #FRAMEWORK# ------------------------------------------------
// Description....: Номер пина по виртуальному пину
// Argument.......: 
//      uint8_t PIN									= Виртуальный номер пина МК
// ------------------------------------------------------------
uint32_t getPin(uint8_t PIN) {
	if (PIN >= PB0 && PIN <= PB15) PIN = PIN - PB0;
	if (PIN >= PC0 && PIN <= PC15) PIN = PIN - PC0;
	if (PIN >= PD0 && PIN <= PD15) PIN = PIN - PD0;
	if (PIN >= PF0 && PIN <= PF15) PIN = PIN - PF0;
	return BIT(PIN);
}

// #FRAMEWORK# ------------------------------------------------
// Description....: Канал АЦП по виртуальному пину
// Argument.......: 
//      uint8_t PIN									= Виртуальный номер пина МК
// ------------------------------------------------------------
uint8_t getChannel(uint8_t PIN) {
	switch (PIN) {
		case PA0:
			return ADC_CHANNEL_0;
		case PA1:
			return ADC_CHANNEL_1;
		case PA2:
			return ADC_CHANNEL_2;
		case PA3:
			return ADC_CHANNEL_3;
		case PA4:
			return ADC_CHANNEL_4;
		case PA5:
			return ADC_CHANNEL_5;
		case PA6:
			return ADC_CHANNEL_6;
		case PA7:
			return ADC_CHANNEL_7;
		case PB0:
			return ADC_CHANNEL_8;
		case PB1:
			return ADC_CHANNEL_9;
	}
	return 0;
}

// #FRAMEWORK# ------------------------------------------------
// Description....: Установка типа порта
// Argument.......: 
//      uint8_t PIN									= Виртуальный номер пина МК
//			uint32_t TYPE								= Тип порта (см. ф. framefork.h)
//					INPUT										= Входной цифровой пин
//					OUTPUT									= Выходной цифровой пин
//					ANALOG									= Входной аналоговый пин
//			uint32_t MODE								= Режим выходного пина
// ------------------------------------------------------------
void pinMode(uint8_t PIN, uint32_t TYPE, uint32_t MODE) {
	if (TYPE == OUTPUT) {
		gpio_mode_set(getPort(PIN) , TYPE, GPIO_PUPD_NONE, getPin(PIN));	
		gpio_output_options_set(getPort(PIN), GPIO_OTYPE_PP, MODE, getPin(PIN));
		return;
	}
	gpio_mode_set(getPort(PIN) , TYPE, GPIO_PUPD_NONE, getPin(PIN));
}

// #FRAMEWORK# ------------------------------------------------
// Description....: Управление цифровым виртуальным пином
// Argument.......: 
//      uint8_t PIN									= Виртуальный номер пина МК
//			FlagStatus fSTATUS					= Статус управления (см. ф. framefork.h)
//					LOW											= Низкий уровень
//					HIGH										= Высокий уровень
// ------------------------------------------------------------
void digitalWrite(uint8_t PIN, FlagStatus fSTATUS) {
	gpio_bit_write(getPort(PIN), getPin(PIN), fSTATUS);
}

// #FRAMEWORK# ------------------------------------------------
// Description....: Чтение цифрового виртуального пина
// Argument.......: 
//      uint8_t PIN									= Виртуальный номер пина МК
// ------------------------------------------------------------
FlagStatus digitalRead(uint8_t PIN) {
	return gpio_input_bit_get(getPort(PIN), getPin(PIN));
}

// #FRAMEWORK# ------------------------------------------------
// Description....: Чтение аналогового виртуального пина
// Argument.......: 
//      uint8_t PIN									= Виртуальный номер пина МК
// ------------------------------------------------------------
uint16_t analogRead(uint8_t PIN) {
    adc_regular_channel_config(0U, getChannel(PIN), ADC_SAMPLETIME_13POINT5);
    while(!adc_flag_get(ADC_FLAG_EOC));
    adc_flag_clear(ADC_FLAG_EOC);
    return (adc_regular_data_read() & 0xfff);    
}

// #FRAMEWORK# ------------------------------------------------
// Description....: Возвращает количество миллисекунд с момента начала выполнения текущей программы
// Argument.......: Нет
// ------------------------------------------------------------
uint32_t millis(void) {
	return msTicks;
}

// #FRAMEWORK# ------------------------------------------------
// Description....: Останавливает выполнение программы на заданное в параметре количество миллисекунд
// Argument.......: 
//      uint32_t dlyTicks						= Колличество миллисекунд паузы программы
// ------------------------------------------------------------
void delay(uint32_t dlyTicks) {
  uint32_t curTicks = msTicks;
	fwdgt_counter_reload();
  while ((msTicks - curTicks) < dlyTicks) {
		__NOP();
	}
}

// #FRAMEWORK# ------------------------------------------------
// Description....: Инициализация framework'а и двигателя bldc
// Argument.......: Нет
// ------------------------------------------------------------
void fw_init(void) {
	if (wd_init() == ERROR) while(1);
	SystemCoreClockUpdate();
  SysTick_Config(SystemCoreClock / 100);
	nvic_priority_group_set(NVIC_PRIGROUP_PRE4_SUB0);
	rcu_periph_clock_enable(RCU_GPIOA);
	rcu_periph_clock_enable(RCU_GPIOB);
	rcu_periph_clock_enable(RCU_GPIOC);
	rcu_periph_clock_enable(RCU_GPIOF);
	gpio_mode_set(getPort(PIN_HALL_A) , GPIO_MODE_INPUT, GPIO_PUPD_NONE, getPin(PIN_HALL_A));
	gpio_mode_set(getPort(PIN_HALL_B) , GPIO_MODE_INPUT, GPIO_PUPD_NONE, getPin(PIN_HALL_B));
	gpio_mode_set(getPort(PIN_HALL_C) , GPIO_MODE_INPUT, GPIO_PUPD_NONE, getPin(PIN_HALL_C));	
	gpio_mode_set(getPort(PIN_BLDC_EMERGENCY_SHUTDOWN) , GPIO_MODE_AF, GPIO_PUPD_NONE, getPin(PIN_BLDC_EMERGENCY_SHUTDOWN));
	gpio_af_set(getPort(PIN_BLDC_EMERGENCY_SHUTDOWN), GPIO_AF_2, getPin(PIN_BLDC_EMERGENCY_SHUTDOWN));
	gpio_mode_set(getPort(PIN_BLDC_GH), GPIO_MODE_AF, GPIO_PUPD_NONE, getPin(PIN_BLDC_GH));
	gpio_mode_set(getPort(PIN_BLDC_BH), GPIO_MODE_AF, GPIO_PUPD_NONE, getPin(PIN_BLDC_BH));
	gpio_mode_set(getPort(PIN_BLDC_YH), GPIO_MODE_AF, GPIO_PUPD_NONE, getPin(PIN_BLDC_YH));
	gpio_mode_set(getPort(PIN_BLDC_GL), GPIO_MODE_AF, GPIO_PUPD_NONE, getPin(PIN_BLDC_GL));
	gpio_mode_set(getPort(PIN_BLDC_BL), GPIO_MODE_AF, GPIO_PUPD_NONE, getPin(PIN_BLDC_BL));
	gpio_mode_set(getPort(PIN_BLDC_YL), GPIO_MODE_AF, GPIO_PUPD_NONE, getPin(PIN_BLDC_YL));
	gpio_output_options_set(getPort(PIN_BLDC_GH), GPIO_OTYPE_PP, GPIO_OSPEED_2MHZ, getPin(PIN_BLDC_GH));
  gpio_output_options_set(getPort(PIN_BLDC_BH), GPIO_OTYPE_PP, GPIO_OSPEED_2MHZ, getPin(PIN_BLDC_BH));
  gpio_output_options_set(getPort(PIN_BLDC_YH), GPIO_OTYPE_PP, GPIO_OSPEED_2MHZ, getPin(PIN_BLDC_YH));
	gpio_output_options_set(getPort(PIN_BLDC_GL), GPIO_OTYPE_PP, GPIO_OSPEED_2MHZ, getPin(PIN_BLDC_GL));
  gpio_output_options_set(getPort(PIN_BLDC_BL), GPIO_OTYPE_PP, GPIO_OSPEED_2MHZ, getPin(PIN_BLDC_BL));
  gpio_output_options_set(getPort(PIN_BLDC_YL), GPIO_OTYPE_PP, GPIO_OSPEED_2MHZ, getPin(PIN_BLDC_YL));
	gpio_af_set(getPort(PIN_BLDC_GH), GPIO_AF_2, getPin(PIN_BLDC_GH));
  gpio_af_set(getPort(PIN_BLDC_BH), GPIO_AF_2, getPin(PIN_BLDC_BH));
	gpio_af_set(getPort(PIN_BLDC_YH), GPIO_AF_2, getPin(PIN_BLDC_YH));
	gpio_af_set(getPort(PIN_BLDC_GL), GPIO_AF_2, getPin(PIN_BLDC_GL));
  gpio_af_set(getPort(PIN_BLDC_BL), GPIO_AF_2, getPin(PIN_BLDC_BL));
	gpio_af_set(getPort(PIN_BLDC_YL), GPIO_AF_2, getPin(PIN_BLDC_YL));
	rcu_periph_clock_enable(RCU_DMA);
	
	rcu_periph_clock_enable(RCU_ADC);
	rcu_periph_clock_enable(RCU_DMA);
	rcu_adc_clock_config(RCU_ADCCK_APB2_DIV6);
	nvic_irq_enable(DMA_Channel0_IRQn, 1, 0);
	dma_deinit(DMA_CH0);
	dma_init_struct_adc.direction = DMA_PERIPHERAL_TO_MEMORY;
	dma_init_struct_adc.memory_addr = (uint32_t)&fw_adc;
	dma_init_struct_adc.memory_inc = DMA_MEMORY_INCREASE_ENABLE;
	dma_init_struct_adc.memory_width = DMA_MEMORY_WIDTH_16BIT;
	dma_init_struct_adc.number = 0;
	dma_init_struct_adc.periph_addr = (uint32_t)&ADC_RDATA;
	dma_init_struct_adc.periph_inc = DMA_PERIPH_INCREASE_DISABLE;
	dma_init_struct_adc.periph_width = DMA_PERIPHERAL_WIDTH_16BIT;
	dma_init_struct_adc.priority = DMA_PRIORITY_ULTRA_HIGH;
	dma_init(DMA_CH0, &dma_init_struct_adc);
	dma_circulation_enable(DMA_CH0);
	dma_memory_to_memory_disable(DMA_CH0);
	dma_interrupt_enable(DMA_CH0, DMA_CHXCTL_FTFIE);
	dma_transfer_number_config(DMA_CH0, 0);
	dma_channel_enable(DMA_CH0);
	adc_channel_length_config(ADC_REGULAR_CHANNEL, 0);
	adc_data_alignment_config(ADC_DATAALIGN_RIGHT);
	adc_external_trigger_config(ADC_REGULAR_CHANNEL, ENABLE);
	adc_external_trigger_source_config(ADC_REGULAR_CHANNEL, ADC_EXTTRIG_REGULAR_NONE);
	adc_tempsensor_vrefint_disable();
	adc_vbat_disable();
	adc_watchdog_disable();
	adc_enable();
	adc_calibration_enable();
	adc_dma_mode_enable();
	adc_special_function_config(ADC_SCAN_MODE, ENABLE);
	
	rcu_periph_clock_enable(RCU_TIMER0);
	timer_deinit(TIMER_BLDC);
	timerBldc_paramter_struct.counterdirection 	= TIMER_COUNTER_UP;
	timerBldc_paramter_struct.prescaler 				= 0;
	timerBldc_paramter_struct.alignedmode 			= TIMER_COUNTER_CENTER_DOWN;
	timerBldc_paramter_struct.period						= 72000000 / 2 / PWM_FREQ;
	timerBldc_paramter_struct.clockdivision 		= TIMER_CKDIV_DIV1;
	timerBldc_paramter_struct.repetitioncounter = 0;
	timer_auto_reload_shadow_disable(TIMER_BLDC);
	timer_init(TIMER_BLDC, &timerBldc_paramter_struct);
	timer_channel_output_fast_config(TIMER_BLDC, TIMER_BLDC_G, TIMER_OC_FAST_DISABLE);
	timer_channel_output_fast_config(TIMER_BLDC, TIMER_BLDC_B, TIMER_OC_FAST_DISABLE);
	timer_channel_output_fast_config(TIMER_BLDC, TIMER_BLDC_Y, TIMER_OC_FAST_DISABLE);
	timer_channel_output_shadow_config(TIMER_BLDC, TIMER_BLDC_G, TIMER_OC_SHADOW_DISABLE);
	timer_channel_output_shadow_config(TIMER_BLDC, TIMER_BLDC_B, TIMER_OC_SHADOW_DISABLE);
	timer_channel_output_shadow_config(TIMER_BLDC, TIMER_BLDC_Y, TIMER_OC_SHADOW_DISABLE);
	timer_channel_output_mode_config(TIMER_BLDC, TIMER_BLDC_G, TIMER_OC_MODE_PWM1);
	timer_channel_output_mode_config(TIMER_BLDC, TIMER_BLDC_B, TIMER_OC_MODE_PWM1);
	timer_channel_output_mode_config(TIMER_BLDC, TIMER_BLDC_Y, TIMER_OC_MODE_PWM1);
	timer_channel_output_pulse_value_config(TIMER_BLDC, TIMER_BLDC_G, 0);
	timer_channel_output_pulse_value_config(TIMER_BLDC, TIMER_BLDC_B, 0);
	timer_channel_output_pulse_value_config(TIMER_BLDC, TIMER_BLDC_Y, 0);
	timerBldc_oc_parameter_struct.ocpolarity 		= TIMER_OC_POLARITY_HIGH;
	timerBldc_oc_parameter_struct.ocnpolarity 	= TIMER_OCN_POLARITY_LOW;
	timerBldc_oc_parameter_struct.ocidlestate 	= TIMER_OC_IDLE_STATE_LOW;
	timerBldc_oc_parameter_struct.ocnidlestate 	= TIMER_OCN_IDLE_STATE_HIGH;
	timer_channel_output_config(TIMER_BLDC, TIMER_BLDC_G, &timerBldc_oc_parameter_struct);
  timer_channel_output_config(TIMER_BLDC, TIMER_BLDC_B, &timerBldc_oc_parameter_struct);
	timer_channel_output_config(TIMER_BLDC, TIMER_BLDC_Y, &timerBldc_oc_parameter_struct);
	timerBldc_break_parameter_struct.runoffstate			= TIMER_ROS_STATE_ENABLE;
	timerBldc_break_parameter_struct.ideloffstate 		= TIMER_IOS_STATE_DISABLE;
	timerBldc_break_parameter_struct.protectmode			= TIMER_CCHP_PROT_OFF;
	timerBldc_break_parameter_struct.deadtime 				= DEAD_TIME;
	timerBldc_break_parameter_struct.breakstate				= TIMER_BREAK_ENABLE;
	timerBldc_break_parameter_struct.breakpolarity		= TIMER_BREAK_POLARITY_LOW;
	timerBldc_break_parameter_struct.outputautostate 	= TIMER_OUTAUTO_ENABLE;
	timer_break_config(TIMER_BLDC, &timerBldc_break_parameter_struct);
	timer_disable(TIMER_BLDC);
	timer_channel_output_state_config(TIMER_BLDC, TIMER_BLDC_G, TIMER_CCX_ENABLE);
	timer_channel_output_state_config(TIMER_BLDC, TIMER_BLDC_B, TIMER_CCX_ENABLE);
	timer_channel_output_state_config(TIMER_BLDC, TIMER_BLDC_Y, TIMER_CCX_ENABLE);
	timer_channel_complementary_output_state_config(TIMER_BLDC, TIMER_BLDC_G, TIMER_CCXN_ENABLE);
	timer_channel_complementary_output_state_config(TIMER_BLDC, TIMER_BLDC_B, TIMER_CCXN_ENABLE);
	timer_channel_complementary_output_state_config(TIMER_BLDC, TIMER_BLDC_Y, TIMER_CCXN_ENABLE);
	nvic_irq_enable(TIMER0_BRK_UP_TRG_COM_IRQn, 0, 0);
	timer_interrupt_enable(TIMER_BLDC, TIMER_INT_UP);
	timer_enable(TIMER_BLDC);
}

// #FRAMEWORK# ------------------------------------------------
// Description....: Инициализация сторжевого таймера
// Argument.......: Нет
// ------------------------------------------------------------
ErrStatus wd_init(void) {
	if (RESET != rcu_flag_get(RCU_FLAG_FWDGTRST)) {
		rcu_all_reset_flag_clear();
	}
	if (fwdgt_config(0x0FFF, FWDGT_PSC_DIV16) != SUCCESS ||
		fwdgt_window_value_config(0x0FFF) != SUCCESS) {
		return ERROR;
	}
	fwdgt_enable();
	return SUCCESS;
}

// #FRAMEWORK# ------------------------------------------------
// Description....: Аппаратные прерывания
// Argument.......: Нет
// ------------------------------------------------------------
void SysTick_Handler(void) {
  msTicks++;
}

// #FRAMEWORK# ------------------------------------------------
// Description....: Таймер АЦП аппартного прерывания
// Argument.......: Нет
// ------------------------------------------------------------
void DMA_Channel0_IRQHandler(void) {
	if (dma_interrupt_flag_get(DMA_CH0, DMA_INT_FLAG_FTF)) {
		dma_interrupt_flag_clear(DMA_CH0, DMA_INT_FLAG_FTF);        
	}
}
