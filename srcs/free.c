#include "malloc.h"

void	free(void *ptr)
{
	t_zone	**list;
	t_zone *zone;
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
	if (zone->free == zone->size - sizeof(t_zone))
		free_zone(list, zone);
}
