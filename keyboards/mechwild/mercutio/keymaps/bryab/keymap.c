/* Copyright 2021 Kyle McCreery
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include QMK_KEYBOARD_H
#include <stdio.h>

//layer names
#define _QWERTY 0
#define _COLEMAK 1
#define _NUMROW 2
#define _NUMPAD 3

//oled stuffs
#define MAX_DISPLAYS 4
#ifdef BONGOCAT
#include "bongotest.h"
#endif
#ifdef BAYMAX
#include "baymax.h"
#endif
#ifdef UCLA_SPLASH
#include "ucla.h"
#endif

//#define BAYMAX
//#define UCLA_SPLASH

bool gui_on = true;
char wpm_str[10];

uint8_t selected_layer = 0;
uint8_t oled_display = 0;
uint8_t current_oled_display = 0;

//TAPDANCE
typedef enum 
{ // tappy types
    TD_NONE,
    TD_UNKNOWN,
    TD_SINGLE_TAP,
    TD_SINGLE_HOLD,
    TD_DOUBLE_TAP,
    TD_DOUBLE_HOLD,
    TD_DOUBLE_SINGLE_TAP, // Send two single taps
    TD_TRIPLE_TAP,
    TD_TRIPLE_HOLD
} td_state_t;

typedef struct 
{
	bool is_press_action;
	td_state_t state;
} td_tap_t;

enum 
{ // dance enums
    PLAY_SKIP,
	Q_S,
	NC // momentary layer 1 on single hold, caps lock on double tap, momentary layer 1 + left shift on double hold
};

td_state_t cur_dance(qk_tap_dance_state_t *state);

void nc_finished(qk_tap_dance_state_t *state, void *user_data);
void nc_reset(qk_tap_dance_state_t *state, void *user_data);

// LAYOUT
const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = 
{
  [0] = LAYOUT_all(
                                                                                                                  TD(PLAY_SKIP),
      KC_TAB,           KC_Q,    KC_W,    KC_E,    KC_R,    KC_T,    KC_Y,    KC_U,    KC_I,    KC_O,    KC_P,    KC_BSPC,
      TD(NC),           KC_A,    KC_S,    KC_D,    KC_F,    KC_G,    KC_H,    KC_J,    KC_K,    KC_L,    TD(Q_S), KC_ENT,
      KC_LSFT, KC_TRNS, KC_Z,    KC_X,    KC_C,    KC_V,    KC_B,    KC_N,    KC_M,    KC_COMM, KC_DOT,           RSFT_T(KC_SLSH),
      KC_LCTL, KC_LGUI, KC_LALT,          KC_SPC,  KC_SPC,            KC_SPC,          KC_RALT, MO(3),            KC_RCTL ),

  [1] = LAYOUT_all(
                                                                                                                  TD(PLAY_SKIP),
      KC_TAB,           KC_Q,    KC_W,    KC_F,    KC_P,    KC_G,    KC_J,    KC_L,    KC_U,    KC_Y,    TD(Q_S), KC_BSPC,
      TD(NC),           KC_A,    KC_R,    KC_S,    KC_T,    KC_D,    KC_H,    KC_N,    KC_E,    KC_I,    KC_O,    KC_ENT,
      KC_LSFT, KC_TRNS, KC_Z,    KC_X,    KC_C,    KC_V,    KC_B,    KC_K,    KC_M,    KC_COMM, KC_DOT,           RSFT_T(KC_SLSH),
      KC_LCTL, KC_LGUI, KC_LALT,          KC_SPC,  KC_SPC,            KC_SPC,          KC_RALT, MO(3),            KC_RCTL ),

  [2] = LAYOUT_all(
                                                                                                                  KC_MUTE,
  	  KC_ESC,           KC_1,    KC_2,    KC_3,    KC_4,    KC_5,    KC_6,    KC_7,    KC_8,    KC_9,   KC_0,     KC_BSPC,
  	  KC_TRNS,          KC_GRV,  KC_BSLS, KC_TRNS, KC_TRNS, KC_TRNS, KC_LBRC, KC_RBRC, KC_MINS, KC_EQL, KC_SCLN,  KC_TRNS,
  	  KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_UP,            KC_TRNS,
  	  KC_TRNS, KC_TRNS, KC_TRNS,          KC_TRNS, KC_TRNS,          KC_TRNS,          KC_LEFT, KC_DOWN,          KC_RIGHT ),

  [3] = LAYOUT_all(
                                                                                                                  KC_TRNS,
  	  KC_ESC,           KC_F1,   KC_F2,   KC_F3,   KC_F4,   KC_F5,   KC_F6,   KC_F7,   KC_F8,   KC_F9,   KC_F10,  KC_DEL,
  	  KC_CAPS,          KC_F11,  KC_F12,  KC_F13,  KC_F14,  KC_F15,  KC_F16,  KC_F17,  KC_F18,  KC_F19,  KC_F20,  KC_PSCR,
  	  KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS,          KC_PGUP,
  	  KC_TRNS, KC_TRNS, KC_TRNS,          KC_TRNS, KC_TRNS,          KC_TRNS,          KC_RALT, KC_TRNS,          KC_PGDN ),

};

#ifdef ENCODER_ENABLE
bool encoder_update_user(uint8_t index, bool clockwise) {
    switch (index) {
		case 0:
			if ( clockwise ) 
			{
				if ( selected_layer  < 3 && keyboard_report->mods & MOD_BIT(KC_LSFT) ) 
				{ // If you are holding L shift, encoder changes layers
                        selected_layer ++;
                        layer_move(selected_layer);
				} else if (oled_display < MAX_DISPLAYS && keyboard_report->mods & MOD_BIT(KC_LCTL))
				{ // Holding L ctrl, encoder changes OLED displays
					oled_display ++;

				} else 
				{
						tap_code(KC_VOLU);                                                   // Otherwise it just changes volume
				}
            } else 
			{
				if ( selected_layer  > 0 && keyboard_report->mods & MOD_BIT(KC_LSFT) )
				{
                        selected_layer --;
                        layer_move(selected_layer);
				} else if (oled_display > 0 && keyboard_report->mods & MOD_BIT(KC_LCTL))
				{
					oled_display --;

				} else 
				{
						tap_code(KC_VOLD);                                                   
				}
			}			
				
        break;
    }
    return true;
}
#endif

#ifdef OLED_ENABLE

oled_rotation_t oled_init_user(oled_rotation_t rotation) 
{
    return OLED_ROTATION_180;  // flips the display 180 degrees
}

/* render mercutio splash
static void render_name(void) 
{     // Render Mercutio Script Text
	static const char PROGMEM mercutio_name[] = {
		0xB6, 0xB6, 0xB6, 0xB6, 0xB6, 0xB6, 0xB6, 0xB6, 0xB6, 0xB6, 0xB6, 0xB6, 0xB6, 0xB6, 0xB6, 0x95, 0xB5, 0x96, 0xD5, 0xB6, 0xB6,
		0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x8A, 0x8B, 0x8C, 0x8D, 0x8E, 0x8F, 0x90, 0x91, 0x92, 0x93, 0x94,
		0xA0, 0xA1, 0xA2, 0xA3, 0xA4, 0xA5, 0xA6, 0xA7, 0xA8, 0xA9, 0xAA, 0xAB, 0xAC, 0xAD, 0xAE, 0xAF, 0xB0, 0xB1, 0xB2, 0xB3, 0xB4,
		0xC0, 0xC1, 0xC2, 0xC3, 0xC4, 0xC5, 0xC6, 0xC7, 0xC8, 0xC9, 0xCA, 0xCB, 0xCC, 0xCD, 0xCE, 0xCF, 0xD0, 0xD1, 0xD2, 0xD3, 0xD4, 0x00
	};
	oled_write_P(mercutio_name, false);
}
*/

/* mechwild logo
static const char PROGMEM mwlogo_1[] = {0x97, 0x98, 0x99, 0x9A, 0x00};
static const char PROGMEM mwlogo_2[] = {0xB7, 0xB8, 0xB9, 0xBA, 0xBB, 0xBC, 0xBD, 0x00};
static const char PROGMEM mwlogo_3[] = {0xD7, 0xD8, 0xD9, 0xDA, 0xDB, 0xDC, 0xDD, 0xB6, 0x00};
static const char PROGMEM mwlogo_4[] = {0xB6, 0xB6, 0xB6, 0x9B, 0x9C, 0x9D, 0x9E, 0x00};

void draw_mwlogo(void) 
{
	oled_set_cursor(0,0);
	oled_write_P(mwlogo_1, 5);
	oled_set_cursor(0,1);
	oled_write_P(mwlogo_2, 8);
	oled_set_cursor(0,2);
	oled_write_P(mwlogo_3, 9);
	oled_set_cursor(0,3);
	oled_write_P(mwlogo_4, 8);
}
*/

void draw_default(void)
{
	//draw_mwlogo();
	oled_set_cursor(0,2);
	switch(selected_layer)
	{
		case 0:
			oled_write_P(PSTR("Lock Layer 0"), false);
			break;
		case 1:
			oled_write_P(PSTR("Lock Layer 1"), false);
			break;
		case 2:
			oled_write_P(PSTR("Lock Layer 2"), false);
			break;
		case 3:
			oled_write_P(PSTR("Lock Layer 3"), false);
			break;
		default:
			oled_write_P(PSTR("Lock Layer ?"), false);    // Should never display, here as a catchall
	}
	oled_set_cursor(0,3);
	if (get_highest_layer(layer_state) == selected_layer) 
	{
		oled_write_P(PSTR("            "), false);
	} else 
	{
		switch (get_highest_layer(layer_state)) 
		{
			case 0:
				oled_write_P(PSTR("Temp Layer 0"), false);
				break;
			case 1:
				oled_write_P(PSTR("Temp Layer 1"), false);
				break;
			case 2:
				oled_write_P(PSTR("Temp Layer 2"), false);
				break;
			case 3:
				oled_write_P(PSTR("Temp Layer 3"), false);
				break;
			default:
				oled_write_P(PSTR("Temp Layer ?"), false);    // Should never display, here as a catchall
		}
	}
	led_t led_state = host_keyboard_led_state();
	oled_set_cursor(0,0);
	oled_write_P(led_state.scroll_lock ? PSTR("SCRLK") : PSTR("     "), false);
	oled_set_cursor(0,1);
	oled_write_P(led_state.num_lock ? PSTR("NLCK ") : PSTR("     "), false);
	oled_write_P(led_state.caps_lock ? PSTR("CAPS ") : PSTR("     "), false);
}

void blank_oled(void) 
{
    int i, j;
    for (i = 0; i < 128 ; i++) {
        for (j = 0; j < 32; j++) {
            oled_write_pixel(i, j, false);
        }
    }
	oled_clear();
	oled_render();
}

bool oled_task_user(void) 
{
	if (oled_display == 0)
	{
		if (current_oled_display != oled_display)
		{
			current_oled_display = oled_display;
			blank_oled();
		}
		draw_default();
	} 
	
	#ifdef BONGOCAT
	if (oled_display == 1)
	{
		if (current_oled_display != oled_display)
		{
			current_oled_display = oled_display;
			blank_oled();
		}
		draw_bongocat();
	} 
	#endif
	
	#ifdef BAYMAX
	if (oled_display == 2)
	{
		if (current_oled_display != oled_display)
		{
			current_oled_display = oled_display;
			blank_oled();
		}
		draw_baymax();
	}
	#endif
	
	#ifdef UCLA_SPLASH
	if (oled_display == 3)
	{
		if (current_oled_display != oled_display)
		{
			current_oled_display = oled_display;
			blank_oled();
		}
		draw_ucla_logo();
	}
	#endif

	if (oled_display == MAX_DISPLAYS)
	{
		if (current_oled_display != oled_display)
		{
			current_oled_display = oled_display;
			blank_oled();
		}
	}
    return false;
}
#endif

td_state_t cur_dance(qk_tap_dance_state_t *state) 
{ //tapping states
    if (state->count == 1) 
	{
        if (state->interrupted || !state->pressed) return TD_SINGLE_TAP; 
        else return TD_SINGLE_HOLD;
    } else if (state->count == 2) 
	{
		if (state->interrupted) return TD_DOUBLE_SINGLE_TAP;
		else if (state->pressed) return TD_DOUBLE_HOLD;
		else return TD_DOUBLE_TAP;
    } else return TD_UNKNOWN;
}

// TD_NC
static td_tap_t nc_tap_state =
{
	.is_press_action = true,
	.state = TD_NONE
};

void nc_each_tap(qk_tap_dance_state_t *state, void *user_data)
{
	layer_on(_NUMROW); 
}

void nc_finished(qk_tap_dance_state_t *state, void *user_data) 
{
	nc_tap_state.state = cur_dance(state);
	switch (nc_tap_state.state) 
	{
		case TD_SINGLE_HOLD: layer_on(_NUMROW); break;
		case TD_DOUBLE_TAP: register_code(KC_CAPS); break;
		default: break;
	}
}

void nc_reset(qk_tap_dance_state_t *state, void *user_data) 
{
	switch (nc_tap_state.state) 
	{
		case TD_SINGLE_TAP: layer_off(_NUMROW); break;
		case TD_SINGLE_HOLD: layer_off(_NUMROW); break;
		case TD_DOUBLE_TAP: layer_off(_NUMROW); unregister_code(KC_CAPS); break;
		default: break;
	}
	nc_tap_state.state = TD_NONE;
}

qk_tap_dance_action_t tap_dance_actions[] = 
{ // dancing definitions
    [PLAY_SKIP] = ACTION_TAP_DANCE_DOUBLE(KC_MPLY, KC_MNXT),
	[Q_S]       = ACTION_TAP_DANCE_DOUBLE(KC_QUOT, KC_SCLN),
	[NC]        = ACTION_TAP_DANCE_FN_ADVANCED_TIME(nc_each_tap, nc_finished, nc_reset, 200),
};
