// #INDEX# ======================================================================================================
// Title .........: ���������� BLDC ����������
// Arch ..........: GD32F130C8C6
// Author ........: GreenBytes ( https://vk.com/greenbytes ), Florian flo199213 ( https://github.com/flo199213/Hoverboard-Firmware-Hack-Gen2/tree/master/HoverBoardGigaDevice )
// Version .......: 1.0.0.0
// ==============================================================================================================

// ����������� ���������
// ����������� �������� ���������� ������ � �����������������
#include "gd32f1x0.h"
// ����������� ���������� ������ � ����������
#include "bldc.h"
// ����������� ����������
#include "../framework/framework.h"
// ����������� �������� � ��������
#include "../define.h"

// ��������� ������� ��� �������
const int16_t pwm_res = 72000000 / 2 / PWM_FREQ; // = 2000
// ��������� ����������
int16_t bldc_inputFilterPwm = 0;
// ���/���� ������
FlagStatus bldc_enable = RESET;
// ������� �����
// ������ �
uint8_t hall_a;
// ������ �
uint8_t hall_b;
// ������ �
uint8_t hall_c;
// ����� ������
uint8_t hall;
// ������� ���������
uint8_t pos;
// ���������� ������� ���������
uint8_t lastPos;
// �������� ������� ��� �������
int16_t bldc_outputFilterPwm = 0;
// ������
int32_t filter_reg;
// ���������� ������ �� ������ ���������
uint32_t speedcount = 0;
// ������� ������ �� ������
uint32_t speedtick = 0;

// ������� ��������� ������
const uint8_t hall_to_pos[8] =
		{
			0, // ������� ����� [-] - ��� ������� (������ � 1-6) 
			3, // ������� ����� [1] (SA=1, SB=0, SC=0) -> ���-��������� 3
			5, // ������� ����� [2] (SA=0, SB=1, SC=0) -> ���-��������� 5
			4, // ������� ����� [3] (SA=1, SB=1, SC=0) -> ���-��������� 4
			1, // ������� ����� [4] (SA=0, SB=0, SC=1) -> ���-��������� 1
			2, // ������� ����� [5] (SA=1, SB=0, SC=1) -> ���-��������� 2
			6, // ������� ����� [6] (SA=0, SB=1, SC=1) -> ���-��������� 6
			0, // ������� ����� [-] - ��� ������� (������ � 1-6) 
		};

// #BLDC# ------------------------------------------------------
// Description....: ���������� ������
// Argument.......: 
//      int pwm                			= ������� ���������� �� -1000 �� 1000
//      uint32_t pwmPos    					= ��������� ���������
//      int *y       								= ����� ���� Y
//      int *b       								= ����� ���� B
//      int *g       	 							= ����� ���� G
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
// Description....: ���������� ������� ���������
// Argument.......: 
//      uint8_t setMode							= ���� ������ ���������� ���������� (��. �. bldc.h)
//					BLDC_OFF								= ��������� ��������
//					BLDC_MEANDER						= ��� ������� ������
//					BLDC_TRIANGULAR					= ����������� ��� ������� 
//					BLDC_SAW								= ������������ ��� ������� 
//					BLDC_SINUS							= �������� ��� �������*
//
//		*�������� ��� ������� ����������� �� ���� ���������� ������ ���������� �� ���� ����������� ����
// ------------------------------------------------------------
void engineMode(FlagStatus setMode) {
	bldc_enable = setMode;
}

// #BLDC# ------------------------------------------------------
// Description....: ������� ��������� ���������
// Argument.......: 
//      int16_t setPwm							= ������� ���������� �� -1000 �� 1000
// ------------------------------------------------------------
void engineWrite(int16_t setPwm) {
	bldc_inputFilterPwm = CLAMP(setPwm, -1000, 1000);
}

// #BLDC# ------------------------------------------------------
// Description....: ������ � ��������� ��� ������� �� ����
// Argument.......: ���
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
