#include "malloc.h"
#include "libft.h"

static int	try_grow_inplace(t_block *block, size_t old_size, size_t aligned, t_zone *zone)
{
	size_t	next_total;
	size_t	remainder;
	t_block	*split;

	if (!block->next || !block->next->free)
		return (0);
	next_total = sizeof(t_block) + block->next->size;
	if (old_size + next_total < aligned)
		return (0);
	remainder = old_size + next_total - aligned;
	if (remainder > sizeof(t_block) + 16)
	{
		split = (t_block *)((char *)(block + 1) + aligned);
		split->size = remainder - sizeof(t_block);
		split->free = 1;
		split->prev = block;
		split->next = block->next->next;
		if (split->next)
			split->next->prev = split;
		block->next = split;
		block->size = aligned;
		zone->free -= aligned - old_size;
	}
	else
	{
		block->size = old_size + next_total;
		block->next = block->next->next;
		if (block->next)
			block->next->prev = block;
		zone->free -= next_total;
	}
	return (1);
}

void	*realloc(void *ptr, size_t size)
{
	t_zone	**list;
	t_zone	*zone;
	t_block	*block;
	size_t	old_size;
	size_t	aligned;
	void	*new_ptr;

	if (!ptr)
		return (malloc(size));
	if (size == 0)
	{
		free(ptr);
		return (NULL);
	}
	block = (t_block *)ptr - 1;
	old_size = block->size;
	aligned = ALIGN16(size);
	if (aligned == old_size)
		return (ptr);
	if (aligned < old_size)
	{
		if ((old_size <= TINY_MAX && aligned <= TINY_MAX)
			|| (old_size <= SMALL_MAX && aligned <= SMALL_MAX))
			return (ptr);
	}
	if (aligned > old_size && aligned <= SMALL_MAX)
	{
		zone = find_zone(ptr, &list);
		if (zone && list != &g_heap.large
			&& try_grow_inplace(block, old_size, aligned, zone))
			return (ptr);
	}
	new_ptr = malloc(size);
	if (!new_ptr)
		return (NULL);
	ft_memcpy(new_ptr, ptr, old_size < aligned ? old_size : aligned);
	free(ptr);
	return (new_ptr);
}