/*
 * Copyright (c) 2009 Vegard Nossum
 *
 * This file is released under the GPL version 2 with the following
 * clarification and special exception:
 *
 *     Linking this library statically or dynamically with other modules is
 *     making a combined work based on this library. Thus, the terms and
 *     conditions of the GNU General Public License cover the whole
 *     combination.
 *
 *     As a special exception, the copyright holders of this library give you
 *     permission to link this library with independent modules to produce an
 *     executable, regardless of the license terms of these independent
 *     modules, and to copy and distribute the resulting executable under terms
 *     of your choice, provided that you also meet, for each linked independent
 *     module, the terms and conditions of the license of that module. An
 *     independent module is a module which is not derived from or based on
 *     this library. If you modify this library, you may extend this exception
 *     to your version of the library, but you are not obligated to do so. If
 *     you do not wish to do so, delete this exception statement from your
 *     version.
 *
 * Please refer to the file LICENSE for details.
 */

#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include <vm/classloader.h>
#include <vm/class.h>
#include <vm/itable.h>
#include <vm/method.h>

bool opt_trace_itable;

struct itable_entry {
	struct vm_method *method;

	struct list_head node;
};

static uint32_t itable_hash_string(const char *str)
{
	/* Stolen shamelessly from
	 * http://en.wikipedia.org/wiki/Jenkins_hash_function */

	uint32_t hash = 0;
	for (unsigned int i = 0, n = strlen(str); i < n; ++i) {
		hash += str[i];
		hash += (hash << 10);
		hash ^= (hash >> 6);
	}

	hash += (hash << 3);
	hash ^= (hash >> 11);
	hash += (hash << 15);
	return hash;
}

static uint32_t itable_hash_combine(uint32_t a, uint32_t b)
{
	/* The itable can't be longer than 256 entries anyway, so we can
	 * compress those 32 bits from each argument into a combined 8 bit
	 * result. */
	return ((a >> 24) ^ (a >> 16) ^ (a >> 8) ^ a)
		^ ((b >> 24) ^ (b >> 16) ^ (b >> 8) ^ b);
}

unsigned int itable_hash(struct vm_method *vmm)
{
#if 0
	/* Very simple hash based on the middle bits of the method pointer.
	 * This is probably not sufficient because the result depends
	 * implicitly on the way the memory allocator works. */
	return ((unsigned long) vmm / 8) % VM_ITABLE_SIZE;
#endif

	return itable_hash_combine(itable_hash_string(vmm->name),
		itable_hash_string(vmm->type)) % VM_ITABLE_SIZE;
}

static int itable_add_entries(struct vm_class *vmc, struct list_head *itable)
{
	const struct cafebabe_class *class = vmc->class;

	/* Note about error handling: We don't actually clean up on error,
	 * but assume that the caller will free the entries that have been
	 * added to the itable so far. That really simplifies this whole
	 * function. */
	if (vm_class_is_interface(vmc)) {
		for (unsigned int i = 0; i < class->methods_count; ++i) {
			struct vm_method *vmm = &vmc->methods[i];
			unsigned int bucket = itable_hash(vmm);

			struct itable_entry *entry = malloc(sizeof *entry);
			if (!entry)
				return -1;

			entry->method = vmm;
			list_add(&entry->node, &itable[bucket]);
		}
	}

	/* XXX: Move this to class linking? */
	for (unsigned int i = 0; i < class->interfaces_count; ++i) {
		uint16_t idx = class->interfaces[i];

		const struct cafebabe_constant_info_class *interface;
		if (cafebabe_class_constant_get_class(class, idx, &interface))
			return -1;

		const struct cafebabe_constant_info_utf8 *name;
		if (cafebabe_class_constant_get_utf8(class,
			interface->name_index, &name))
			return -1;

		char *c_name = strndup((char *) name->bytes, name->length);
		if (!c_name)
			return -1;

		struct vm_class *vmi = classloader_load(c_name);
		free(c_name);
		if (!vmi)
			return -1;

		int ret = itable_add_entries(vmi, itable);
		if (ret)
			return ret;
	}

	/* Yay for tail recursion. */
	if (vmc->super)
		return itable_add_entries(vmc->super, itable);

	return 0;
}

static void itable_resolver_stub(void)
{
	NOT_IMPLEMENTED;
	abort();
}

static void itable_resolver_stub_error(void)
{
	printf("itable resolver stub error!\n");
	abort();
}

static void *itable_create_conflict_resolver(struct list_head *methods)
{
	/* No methods at this index -- return something that will choke the
	 * caller. */
	if (list_is_empty(methods))
		return &itable_resolver_stub_error;

	/* If it's not empty, and the first element is the same as the last,
	 * there can only be one element in the list. If so, we can put the
	 * trampoline of that method directly into the itable. */
	if (list_first(methods) == list_last(methods)) {
		struct itable_entry *entry = list_first_entry(methods,
			struct itable_entry, node);

		return vm_method_trampoline_ptr(entry->method);
	}

	return &itable_resolver_stub;
}

static void trace_itable(struct vm_class *vmc, struct list_head *itable)
{
	printf("trace itable: %s\n", vmc->name);

	for (unsigned int i = 0; i < VM_ITABLE_SIZE; ++i) {
		if (list_is_empty(&itable[i]))
			continue;

		printf(" %d: ", i);

		struct itable_entry *entry;
		list_for_each_entry(entry, &itable[i], node) {
			printf("%s.%s%s%s",
				entry->method->class->name,
				entry->method->name, entry->method->type,
				&entry->node == list_last(&itable[i])
					? "" : ", ");
		}

		printf("\n");
	}
}

int vm_itable_setup(struct vm_class *vmc)
{
	/* We need a temporary array of lists for storing multiple results.
	 * The final itable (the one that gets stored in the class struct
	 * itself) will only have one method per slot. */
	struct list_head *itable = malloc(sizeof(*itable) * VM_ITABLE_SIZE);
	if (!itable)
		return -ENOMEM;

	for (unsigned int i = 0; i < VM_ITABLE_SIZE; ++i)
		INIT_LIST_HEAD(&itable[i]);

	itable_add_entries(vmc, itable);

	for (unsigned int i = 0; i < VM_ITABLE_SIZE; ++i)
		vmc->itable[i] = itable_create_conflict_resolver(&itable[i]);

	if (opt_trace_itable)
		trace_itable(vmc, itable);

	/* Free the temporary itable */
	for (unsigned int i = 0; i < VM_ITABLE_SIZE; ++i) {
		struct itable_entry *entry, *tmp;

		list_for_each_entry_safe(entry, tmp, &itable[i], node)
			free(entry);
	}

	free(itable);
	return 0;
}
