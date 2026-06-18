#include "malloc.h"

t_zone	*new_zone(size_t zone_size, size_t max_block_size)
{
	t_zone	*zone;
	t_block	*block;

	(void)max_block_size;
	zone = mmap(NULL, zone_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

	if (zone == MAP_FAILED)
		return (NULL);

	zone->prev = NULL;
	zone->next = NULL;
	zone->size = zone_size;
	zone->free = zone_size - sizeof(t_zone);

	block = (t_block *)(zone + 1);
	block->prev = NULL;
	block->next = NULL;
	block->size = zone_size - sizeof(t_zone) - sizeof(t_block);
	block->free = 1;

	return (zone);
}

void	free_zone(t_zone **list, t_zone *zone)
{
	if (zone->prev)
		zone->prev->next = zone->next;
	else
		*list = zone->next;

	if (zone->next)
		zone->next->prev = zone->prev;

	munmap(zone, zone->size);
}

t_zone	*find_zone(void *ptr, t_zone ***out_list)
{
	t_zone	**lists[3];
	t_zone	*zone;
	int		i;

	lists[0] = &g_heap.tiny;
	lists[1] = &g_heap.small;
	lists[2] = &g_heap.large;

	i = 0;
	while (i < 3)
	{
		zone = *lists[i];

		while (zone)
		{

			if ((char *)ptr > (char *)zone && (char *)ptr < (char *)zone + zone->size)
			{
				if (out_list)
					*out_list = lists[i];

				return (zone);
			}

			zone = zone->next;
		}
		i++;
	}

	return (NULL);
}
