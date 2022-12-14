/* Copyright 2021 Terry Mathews
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

enum layer_names {
    _BL, 
	_NL,
    _FL, 
};

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
}   td_tap_t;

enum 
{ // dance enums
	Q_S,
	NC, // momentary number layer on single hold, caps lock on double tap, momentary number layer + left shift on double hold
	FD, // numpad dot on single tap, momentary function layer on single hold
};

td_state_t cur_dance(qk_tap_dance_state_t *state);

void nc_finished(qk_tap_dance_state_t *state, void *user_data);
void nc_reset(qk_tap_dance_state_t *state, void *user_data);


const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {
  /* Keymap _BL: Base Layer (Default Layer)
   */
[_BL] = LAYOUT(
	KC_TAB,		KC_Q,		KC_W,		KC_E,		KC_R,		KC_T,		KC_Y,		KC_U,		KC_I,		KC_O,		KC_P,			KC_TRNS,KC_BSPC,	KC_P7,		KC_P8,		KC_P9,				KC_PAST, \
	TD(NC),		KC_A,		KC_S,		KC_D,		KC_F,		KC_G,		KC_H,		KC_J,		KC_K,		KC_L,		TD(Q_S),		KC_ENT,				KC_P4,		KC_P5,		KC_P6,				KC_PMNS, \
	KC_LSFT,	KC_Z,		KC_X,		KC_C,		KC_V,		KC_B,		KC_N,		KC_M,		KC_COMM,	KC_DOT,		RSFT_T(KC_SLSH),KC_UP,				KC_P1,		KC_P2,		KC_P3,				KC_PPLS, \
	KC_LCTL,	KC_LGUI,	KC_LALT,	KC_TRNS,						KC_SPC,			KC_TRNS,	KC_TRNS,	KC_RALT,	KC_LEFT,		KC_DOWN,			KC_RGHT,	KC_P0,		LT(_FL, KC_PDOT),	KC_PENT),

  /* Keymap _NL: Number Layer
   */
[_NL] = LAYOUT(
	KC_ESC,		KC_1,		KC_2,		KC_3,		KC_4,		KC_5,		KC_6,		KC_7,		KC_8,		KC_9,		KC_0,			KC_TRNS,KC_DEL,		KC_TRNS,	KC_TRNS,	KC_TRNS,			KC_TRNS, \
	KC_TRNS,	KC_GRV, 	KC_BSLS, 	KC_TRNS,	KC_TRNS,	KC_TRNS, 	KC_LBRC, 	KC_RBRC, 	KC_MINS, 	KC_EQL, 	KC_SCLN,		KC_TRNS,			KC_TRNS,	KC_TRNS,	KC_TRNS,			KC_TRNS, \
	KC_TRNS,	KC_TRNS,	KC_TRNS,	KC_TRNS,	KC_TRNS,	KC_TRNS,	KC_TRNS,	KC_TRNS,	KC_TRNS,	KC_TRNS,	KC_TRNS,		KC_PGUP,			KC_TRNS,	KC_TRNS,	KC_TRNS,			KC_TRNS, \
	KC_TRNS,	KC_TRNS,	KC_TRNS,	KC_TRNS,						KC_TRNS,		KC_TRNS,	KC_TRNS,	MO(_FL),	KC_HOME,		KC_PGDN,			KC_END,		KC_INS,		MO(_FL),			KC_TRNS),
	
/* Keymap _FL: Function Layer
   */
[_FL] = LAYOUT(
	KC_TRNS,	KC_F1,		KC_F2,		KC_F3,		KC_F4,		KC_F5,		KC_F6,		KC_F7,		KC_F8,		KC_F9,		KC_F10,			KC_TRNS,KC_DEL,		KC_MRWD,	KC_MPLY,	KC_MFFD,			KC_DEL,  \
	KC_TRNS,	KC_F11, 	KC_F12, 	KC_F13,		KC_F14,		KC_F15, 	KC_F16, 	KC_F17, 	KC_F18, 	KC_F19, 	KC_F20,			KC_TRNS,			KC_VOLD,	KC_MUTE,	KC_VOLU,			KC_PSCR, \
	KC_TRNS,	KC_TRNS,	KC_TRNS,	KC_TRNS,	KC_TRNS,	KC_TRNS,	KC_TRNS,	KC_TRNS,	KC_TRNS,	KC_TRNS,	KC_TRNS,		KC_TRNS,			KC_TRNS,	KC_TRNS,	KC_TRNS,			KC_NUM, \
	KC_TRNS,	KC_TRNS,	KC_TRNS,	KC_TRNS,						KC_TRNS,		KC_TRNS,	KC_TRNS,	KC_TRNS,	KC_TRNS,		KC_TRNS,			KC_TRNS,	KC_TRNS,	KC_TRNS,			RESET),
};


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
	layer_on(_NL); 
}

void nc_finished(qk_tap_dance_state_t *state, void *user_data) 
{
	nc_tap_state.state = cur_dance(state);
	switch (nc_tap_state.state) 
	{
		case TD_SINGLE_HOLD: layer_on(_NL); break;
		case TD_DOUBLE_TAP: register_code(KC_CAPS); break;
		default: break;
	}
}

void nc_reset(qk_tap_dance_state_t *state, void *user_data) 
{
	switch (nc_tap_state.state) 
	{
		case TD_SINGLE_TAP: layer_off(_NL); break;
		case TD_SINGLE_HOLD: layer_off(_NL); break;
		case TD_DOUBLE_TAP: layer_off(_NL); unregister_code(KC_CAPS); break;
		default: break;
	}
	nc_tap_state.state = TD_NONE;
}

//TD_FD
static td_tap_t fd_tap_state = 
{
	.is_press_action = true,
	.state = TD_NONE
};

void fd_each_tap(qk_tap_dance_state_t *state, void *user_data)
{
	
}

void fd_finished(qk_tap_dance_state_t *state, void *user_data)
{
	fd_tap_state.state = cur_dance(state);
	switch (fd_tap_state.state)
	{
		case TD_SINGLE_TAP: register_code(KC_PDOT); break;
		case TD_SINGLE_HOLD: layer_on(_FL); break;
		default: break;
	}
}

void fd_reset(qk_tap_dance_state_t *state, void *user_data)
{
	switch (nc_tap_state.state)
	{
		case TD_SINGLE_TAP: unregister_code(KC_PDOT); break;
		case TD_SINGLE_HOLD: layer_off(_FL); break;
		default: break;
	}
	fd_tap_state.state = TD_NONE;
}
	

qk_tap_dance_action_t tap_dance_actions[] = 
{ // dancing definitions
	[Q_S]       = ACTION_TAP_DANCE_DOUBLE(KC_QUOT, KC_SCLN),
	[NC]        = ACTION_TAP_DANCE_FN_ADVANCED_TIME(nc_each_tap, nc_finished, nc_reset, 200),
	[FD]        = ACTION_TAP_DANCE_FN_ADVANCED_TIME(fd_each_tap, fd_finished, fd_reset, 200),
};

