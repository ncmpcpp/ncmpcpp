/* ncmpc
 * Copyright (C) 2008 Max Kellermann <max@duempel.org>
 * This project's homepage is: http://www.musicpd.org
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "str_pool.h"

#include <assert.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#define NUM_SLOTS 4096

struct slot {
	struct slot *next;
	unsigned char ref;
	char value[1];
} __attribute__((packed));

struct slot *slots[NUM_SLOTS];

static inline unsigned
calc_hash(const char *p)
{
	unsigned hash = 5381;

	assert(p != NULL);

	while (*p != 0)
		hash = (hash << 5) + hash + *p++;

	return hash;
}

static inline struct slot *
value_to_slot(char *value)
{
	return (struct slot*)(value - offsetof(struct slot, value));
}

static struct slot *slot_alloc(struct slot *next, const char *value)
{
	size_t length = strlen(value);
	struct slot *slot = malloc(sizeof(*slot) + length);
	if (slot == NULL)
		abort(); /* XXX */

	slot->next = next;
	slot->ref = 1;
	memcpy(slot->value, value, length + 1);
	return slot;
}

char *str_pool_get(const char *value)
{
	struct slot **slot_p, *slot;

	slot_p = &slots[calc_hash(value) % NUM_SLOTS];
	for (slot = *slot_p; slot != NULL; slot = slot->next) {
		if (strcmp(value, slot->value) == 0 && slot->ref < 0xff) {
			assert(slot->ref > 0);
			++slot->ref;
			return slot->value;
		}
	}

	slot = slot_alloc(*slot_p, value);
	*slot_p = slot;
	return slot->value;
}

char *str_pool_dup(char *value)
{
	struct slot *slot = value_to_slot(value);

	assert(slot->ref > 0);

	if (slot->ref < 0xff) {
		++slot->ref;
		return value;
	} else {
		/* the reference counter overflows above 0xff;
		   duplicate the value, and start with 1 */
		struct slot **slot_p =
			&slots[calc_hash(slot->value) % NUM_SLOTS];
		slot = slot_alloc(*slot_p, slot->value);
		*slot_p = slot;
		return slot->value;
	}
}

void str_pool_put(char *value)
{
	struct slot **slot_p, *slot;

	slot = value_to_slot(value);
	assert(slot->ref > 0);
	--slot->ref;

	if (slot->ref > 0)
		return;

	for (slot_p = &slots[calc_hash(value) % NUM_SLOTS];
	     *slot_p != slot;
	     slot_p = &(*slot_p)->next) {
		assert(*slot_p != NULL);
	}

	*slot_p = slot->next;
	free(slot);
}
