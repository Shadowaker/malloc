#include "malloc.h"

t_heap	g_heap;

static void	split_block(t_block *block, size_t size)
{
	t_block	*remainder;

	if (block->size <= size + sizeof(t_block) + 16)
		return ;

	remainder = (t_block *)((char *)(block + 1) + size);
	remainder->size = block->size - size - sizeof(t_block);
	remainder->free = 1;
	remainder->prev = block;
	remainder->next = block->next;

	if (block->next)
		block->next->prev = remainder;

	block->next = remainder;
	block->size = size;
}

static void	*malloc_large(size_t size)
{
	size_t	zone_size;
	t_zone	*zone;
	t_block	*block;
	size_t	page_size;

	page_size = get_page_size();
	zone_size = sizeof(t_zone) + sizeof(t_block) + size;
	zone_size = (zone_size + page_size - 1) / page_size * page_size;
	zone = mmap(
			NULL, zone_size, PROT_READ | PROT_WRITE,
			MAP_PRIVATE | MAP_ANONYMOUS, -1, 0
		);

	if (zone == MAP_FAILED)
		return (NULL);

	zone->prev = NULL;
	zone->next = g_heap.large;
	zone->size = zone_size;
	zone->free = 0;

	if (g_heap.large)
		g_heap.large->prev = zone;

	g_heap.large = zone;
	block = (t_block *)(zone + 1);
	block->prev = NULL;
	block->next = NULL;
	block->size = size;
	block->free = 0;

	return ((void *)(block + 1));
}

static void	*malloc_zone(t_zone **list, size_t size, size_t max_size)
{
	t_zone	*zone;
	t_block	*block;

	zone = *list;

	while (zone)
	{
		block = (t_block *)(zone + 1);

		while (block)
		{
			if (block->free && block->size >= size)
			{
				split_block(block, size);
				block->free = 0;
				zone->free -= block->size + sizeof(t_block);

				return ((void *)(block + 1));
			}
			block = block->next;
		}
		zone = zone->next;
	}

	zone = new_zone(get_zone_size(max_size), 0);

	if (!zone)
		return (NULL);

	zone->next = *list;

	if (*list)
		(*list)->prev = zone;

	*list = zone;
	block = (t_block *)(zone + 1);
	split_block(block, size);

	block->free = 0;
	zone->free -= block->size + sizeof(t_block);

	return ((void *)(block + 1));
}

void	*malloc(size_t size)
{
	size_t	aligned;

	if (size == 0)
		return (NULL);

	aligned = ALIGN16(size);

	if (aligned <= TINY_MAX)
		return (malloc_zone(&g_heap.tiny, aligned, TINY_MAX));

	if (aligned <= SMALL_MAX)
		return (malloc_zone(&g_heap.small, aligned, SMALL_MAX));

	return (malloc_large(aligned));
}
