#include "malloc.h"

static void	coalesce(t_block *block)
{
	if (block->next && block->next->free)
	{
		block->size += sizeof(t_block) + block->next->size;
		block->next = block->next->next;
		if (block->next)
			block->next->prev = block;
	}
	if (block->prev && block->prev->free)
	{
		block->prev->size += sizeof(t_block) + block->size;
		block->prev->next = block->next;
		if (block->next)
			block->next->prev = block->prev;
	}
}

void	free(void *ptr)
{
	t_zone	**list;
	t_zone	*zone;
	t_block	*block;

	if (!ptr)
		return ;
	zone = find_zone(ptr, &list);
	if (!zone)
		return ;
	block = (t_block *)ptr - 1;
	if (block->free)
		return ;
	block->free = 1;
	if (list == &g_heap.large)
	{
		free_zone(list, zone);
		return ;
	}
	zone->free += block->size + sizeof(t_block);
	coalesce(block);
	if (zone->free == zone->size - sizeof(t_zone))
		free_zone(list, zone);
}