/**
 * stm.c - pequeño motor de maquina de estados donde los eventos son los
 *         del selector.c
 */
#include "include/stm.h"

#include <stdlib.h>
#include <assert.h>

#define N(x) (sizeof(x) / sizeof((x)[0]))

void stm_init(struct state_machine *stm) {
	// verificamos que los estados son correlativos, y que están bien asignados.
	for (unsigned i = 0; i <= stm->max_state; i++) {
		if (i != stm->states[i].state) {
			assert("The states must be ordered and consecutive" && 0);
		}
	}

	if (stm->initial < stm->max_state) {
		stm->current = NULL;
	} else {
		assert("The initial state must be less than the number of states" && 0);
	}
}

inline static void handle_first(struct state_machine *stm, struct selector_key *key) {
	if (stm->current == NULL) {
		stm->current = stm->states + stm->initial;
		if (NULL != stm->current->on_arrival) {
			stm->current->on_arrival(stm->current->state, key);
		}
	}
}

inline static void jump(struct state_machine *stm, unsigned next, struct selector_key *key) {
	if (next > stm->max_state) {
		assert("The state to jump to is invalid" && 0);
	}
	if (stm->current != stm->states + next) {
		if (stm->current != NULL && stm->current->on_departure != NULL) {
			stm->current->on_departure(stm->current->state, key);
		}
		stm->current = stm->states + next;

		if (NULL != stm->current->on_arrival) {
			stm->current->on_arrival(stm->current->state, key);
		}
	}
}

unsigned stm_handler_read(struct state_machine *stm, struct selector_key *key) {
	handle_first(stm, key);
	if (stm->current->on_read_ready == 0) {
		assert("The current state does not have a read handler" && 0);
	}
	const unsigned int ret = stm->current->on_read_ready(key);
	jump(stm, ret, key);

	return ret;
}

unsigned stm_handler_write(struct state_machine *stm, struct selector_key *key) {
	handle_first(stm, key);
	if (stm->current->on_write_ready == 0) {
		assert("The current state does not have a write handler" && 0);
	}
	const unsigned int ret = stm->current->on_write_ready(key);
	jump(stm, ret, key);

	return ret;
}

unsigned stm_handler_block(struct state_machine *stm, struct selector_key *key) {
	handle_first(stm, key);
	if (stm->current->on_block_ready == 0) {
		assert("The current state does not have a block handler" && 0);
	}
	const unsigned int ret = stm->current->on_block_ready(key);
	jump(stm, ret, key);

	return ret;
}

void stm_handler_close(struct state_machine *stm, struct selector_key *key) {
	if (stm->current != NULL && stm->current->on_departure != NULL) {
		stm->current->on_departure(stm->current->state, key);
	}
}

unsigned stm_state(struct state_machine *stm) {
	unsigned ret = stm->initial;
	if (stm->current != NULL) {
		ret = stm->current->state;
	}
	return ret;
}
